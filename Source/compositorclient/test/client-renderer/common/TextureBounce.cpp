/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "TextureBounce.h"

#include <cmath>
#include <cstdio>
#include <random>

namespace Thunder {
namespace Compositor {
    TextureBounce::TextureBounce()
        : _program(0)
        , _textureId(0)
        , _vbo(0)
        , _canvasHeight(0)
        , _canvasWidth(0)
        , _textureWidth(0)
        , _textureHeight(0)
        , _sprites()
        , _scale(1.0f)
        , _lastFrameTime(Core::Time::Now().Ticks())
    {
    }

    TextureBounce::~TextureBounce()
    {
        Cleanup();
    }

    bool TextureBounce::Initialize(const uint16_t width, const uint16_t height, const std::string& config)
    {
        Config cfg;
        Core::OptionalType<Core::JSON::Error> error;

        cfg.FromString(config);

        if (error.IsSet() == true) {
            TRACE(Trace::Error, ("Failed to parse config: %s", error.Value().Message().c_str()));
            return false;
        }

        _canvasWidth = width;
        _canvasHeight = height;

        if (!CreateShaderProgram()) {
            return false;
        }
        if (!LoadTexture(cfg.Image.Value())) {
            return false;
        }
        CreateVertexBuffer();

        // Initialize sprites
        std::mt19937 rng(static_cast<unsigned int>(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::uniform_real_distribution<float> xDist(0.0f, DefaultWidth - _textureWidth * _scale);
        std::uniform_real_distribution<float> yDist(0.0f, DefaultHeight - _textureHeight * _scale);
        std::uniform_real_distribution<float> vDist(-300.0f, 300.0f);
        std::uniform_real_distribution<float> scaleDist(0.3f, 1.0f);

        _sprites.clear();
        for (int i = 0; i < MaxSprites; ++i) {
            Sprite s;
            s.x = xDist(rng);
            s.y = yDist(rng);
            s.vx = vDist(rng);
            s.vy = vDist(rng);
            float spriteScale = _scale * scaleDist(rng);
            s.width = _textureWidth * spriteScale;
            s.height = _textureHeight * spriteScale;
            s.mass = s.width * s.height; // massa proportioneel aan oppervlakte
            _sprites.push_back(s);
        }

        return true;
    }

    bool TextureBounce::Draw()
    {
        uint64_t now = Core::Time::Now().Ticks();
        float dt = (now - _lastFrameTime) / 1000000.0f;
        _lastFrameTime = now;

        UpdateSprites(dt);
        return RenderFrame();
    }

    bool TextureBounce::LoadTexture(const std::string& path)
    {
        auto pixelData = Thunder::Compositor::Texture::LoadPNG(path);
        if (pixelData.data.empty()) {
            fprintf(stderr, "Failed to load texture: %s\n", path.c_str());
            return false;
        }

        glGenTextures(1, &_textureId);
        glBindTexture(GL_TEXTURE_2D, _textureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pixelData.width, pixelData.height, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, pixelData.data.data());
        glBindTexture(GL_TEXTURE_2D, 0);

        _textureWidth = pixelData.width;
        _textureHeight = pixelData.height;
        return true;
    }

    bool TextureBounce::CreateShaderProgram()
    {
        const char* vsrc = R"(
        attribute vec2 aPos;
        attribute vec2 aTex;
        varying vec2 vTex;
        uniform vec2 uResolution;
        uniform vec2 uPos;
        uniform vec2 uSize;
        void main() {
            vec2 pos = aPos * uSize + uPos;
            vec2 zeroToOne = pos / uResolution;
            vec2 clipSpace = zeroToOne * 2.0 - 1.0;
            gl_Position = vec4(clipSpace * vec2(1, -1), 0.0, 1.0);
            vTex = aTex;
        }
    )";

        const char* fsrc = R"(
        precision mediump float;
        varying vec2 vTex;
        uniform sampler2D uTexture;
        void main() {
            gl_FragColor = texture2D(uTexture, vTex);
        }
    )";

        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vsrc, nullptr);
        glCompileShader(vs);
        if (!CheckShaderCompile(vs, "vertex"))
            return false;

        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fsrc, nullptr);
        glCompileShader(fs);
        if (!CheckShaderCompile(fs, "fragment"))
            return false;

        _program = glCreateProgram();
        glAttachShader(_program, vs);
        glAttachShader(_program, fs);
        glLinkProgram(_program);

        GLint linked = 0;
        glGetProgramiv(_program, GL_LINK_STATUS, &linked);
        if (!linked) {
            char log[512];
            glGetProgramInfoLog(_program, sizeof(log), nullptr, log);
            fprintf(stderr, "Shader link error: %s\n", log);
            return false;
        }

        glDeleteShader(vs);
        glDeleteShader(fs);
        return true;
    }

    bool TextureBounce::CheckShaderCompile(GLuint shader, const char* label)
    {
        GLint compiled;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            char log[512];
            glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
            fprintf(stderr, "%s shader compile error: %s\n", label, log);
            return false;
        }
        return true;
    }

    void TextureBounce::CreateVertexBuffer()
    {
        const GLfloat vertices[] = {
            0.0f, 0.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f
        };
        glGenBuffers(1, &_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void TextureBounce::UpdateSprites(float dt)
    {
        static constexpr float MaxSpeed = 600.0f;
        static constexpr float Friction = 0.9998f;
        static constexpr float BounceFactor = 1.3f;
        static constexpr float BaseMinSpeed = 10.0f; // voor een "standaard" sprite van massa 1

        for (auto& s : _sprites) {
            s.x += s.vx * dt;
            s.y += s.vy * dt;

            // Randbotsing
            if (s.x <= 0) {
                s.x = 0;
                s.vx = -s.vx * BounceFactor;
            } else if (s.x + s.width >= _canvasWidth) {
                s.x = _canvasWidth - s.width;
                s.vx = -s.vx * BounceFactor;
            }

            if (s.y <= 0) {
                s.y = 0;
                s.vy = -s.vy * BounceFactor;
            } else if (s.y + s.height >= _canvasHeight) {
                s.y = _canvasHeight - s.height;
                s.vy = -s.vy * BounceFactor;
            }

            // Lichte damping
            s.vx *= pow(Friction, dt);
            s.vy *= pow(Friction, dt);

            // Massa-afhankelijke minimale snelheid
            float minSpeed = BaseMinSpeed / std::sqrt(s.mass); // grotere massa → lagere minSpeed

            if (std::abs(s.vx) < minSpeed)
                s.vx = (s.vx >= 0) ? minSpeed : -minSpeed;
            if (std::abs(s.vy) < minSpeed)
                s.vy = (s.vy >= 0) ? minSpeed : -minSpeed;

            // Max snelheid
            s.vx = Clamp(s.vx, -MaxSpeed, MaxSpeed);
            s.vy = Clamp(s.vy, -MaxSpeed, MaxSpeed);
        }

        HandleCollisions();
    }

    void TextureBounce::HandleCollisions()
    {
        for (size_t i = 0; i < _sprites.size(); ++i) {
            for (size_t j = i + 1; j < _sprites.size(); ++j) {
                auto& a = _sprites[i];
                auto& b = _sprites[j];

                // Check overlap
                if (a.x < b.x + b.width && a.x + a.width > b.x && a.y < b.y + b.height && a.y + a.height > b.y) {
                    // Normaal vector
                    float nx = (b.x + b.width / 2.0f) - (a.x + a.width / 2.0f);
                    float ny = (b.y + b.height / 2.0f) - (a.y + a.height / 2.0f);
                    float dist = std::sqrt(nx * nx + ny * ny);
                    if (dist == 0) {
                        nx = 0;
                        ny = 1;
                        dist = 1;
                    }
                    nx /= dist;
                    ny /= dist;

                    // Tangent
                    float tx = -ny, ty = nx;

                    // Projecteer snelheden
                    float va_n = a.vx * nx + a.vy * ny;
                    float vb_n = b.vx * nx + b.vy * ny;
                    float va_t = a.vx * tx + a.vy * ty;
                    float vb_t = b.vx * tx + b.vy * ty;

                    // Elastische botsing (massa-gecorrigeerd)
                    float va_n_new = (va_n * (a.mass - b.mass) + 2.0f * b.mass * vb_n) / (a.mass + b.mass);
                    float vb_n_new = (vb_n * (b.mass - a.mass) + 2.0f * a.mass * va_n) / (a.mass + b.mass);

                    // Terug naar xy
                    a.vx = va_n_new * nx + va_t * tx;
                    a.vy = va_n_new * ny + va_t * ty;
                    b.vx = vb_n_new * nx + vb_t * tx;
                    b.vy = vb_n_new * ny + vb_t * ty;

                    // Position correction met massa-verdeling
                    float overlapX = (a.width + b.width) / 2.0f - std::abs((a.x + a.width / 2.0f) - (b.x + b.width / 2.0f));
                    float overlapY = (a.height + b.height) / 2.0f - std::abs((a.y + a.height / 2.0f) - (b.y + b.height / 2.0f));
                    float totalMass = a.mass + b.mass;

                    a.x -= nx * overlapX * (b.mass / totalMass);
                    a.y -= ny * overlapY * (b.mass / totalMass);
                    b.x += nx * overlapX * (a.mass / totalMass);
                    b.y += ny * overlapY * (a.mass / totalMass);
                }
            }
        }
    }

    bool TextureBounce::RenderFrame()
    {
        glViewport(0, 0, _canvasWidth, _canvasHeight);
        glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glUseProgram(_program);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo);

        GLint aPos = glGetAttribLocation(_program, "aPos");
        GLint aTex = glGetAttribLocation(_program, "aTex");
        GLint uRes = glGetUniformLocation(_program, "uResolution");
        GLint uPos = glGetUniformLocation(_program, "uPos");
        GLint uSize = glGetUniformLocation(_program, "uSize");

        glEnableVertexAttribArray(aPos);
        glVertexAttribPointer(aPos, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray(aTex);
        glVertexAttribPointer(aTex, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

        glUniform2f(uRes, (float)_canvasWidth, (float)_canvasHeight);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _textureId);
        glUniform1i(glGetUniformLocation(_program, "uTexture"), 0);

        for (auto& s : _sprites) {
            glUniform2f(uPos, s.x, s.y);
            glUniform2f(uSize, s.width, s.height);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        glDisableVertexAttribArray(aPos);
        glDisableVertexAttribArray(aTex);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        return true;
    }

    void TextureBounce::Cleanup()
    {

        if (_textureId)
            glDeleteTextures(1, &_textureId);
        if (_vbo)
            glDeleteBuffers(1, &_vbo);
        if (_program)
            glDeleteProgram(_program);
    }
} // namespace Compositor
} // namespace Thunder