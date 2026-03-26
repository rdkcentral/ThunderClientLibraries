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

#include "TextRender.h"
#include "TextureLoader.h"
#include <cstdio>
#include <cstring>

namespace Thunder {
namespace Compositor {
    static const char Namespace[] = EXPAND_AND_QUOTE(NAMESPACE);

    TextRender::TextRender(const Font* font)
        : _program(0)
        , _fontTexture(0)
        , _vbo(0)
        , _canvasWidth(0)
        , _canvasHeight(0)
        , _scale(1.0f)
        , _colorR(1.0f)
        , _colorG(1.0f)
        , _colorB(1.0f)
        , _colorA(1.0f)
        , _font(font)
    {
    }

    TextRender::~TextRender()
    {
        Cleanup();
    }

    bool TextRender::Initialize(const uint16_t width, const uint16_t height, const std::string& config)
    {
        Config cfg;
        Core::OptionalType<Core::JSON::Error> error;
        cfg.FromString(config);

        if (error.IsSet()) {
            TRACE(Trace::Error, ("Failed to parse TextRender config: %s", error.Value().Message().c_str()));
            return false;
        }

        _canvasWidth = width;
        _canvasHeight = height;

        if (!CreateShaderProgram()) {
            return false;
        }

        std::string fontPath = cfg.FontAtlas.Value();
        if (fontPath.empty()) {
            fontPath = "/usr/share/" + std::string(Namespace) + "/ClientCompositorRender/Arial.png";
        }

        if (!LoadFontAtlas(fontPath)) {
            return false;
        }

        CreateQuadBuffer();

        _scale = cfg.Scale.Value();
        _colorR = cfg.Red.Value();
        _colorG = cfg.Green.Value();
        _colorB = cfg.Blue.Value();
        _colorA = cfg.Alpha.Value();

        return true;
    }

    bool TextRender::LoadFontAtlas(const std::string& path)
    {
        auto pixelData = Thunder::Compositor::Texture::LoadPNG(path);
        if (pixelData.data.empty()) {
            fprintf(stderr, "Failed to load font atlas: %s\n", path.c_str());
            return false;
        }

        glGenTextures(1, &_fontTexture);
        glBindTexture(GL_TEXTURE_2D, _fontTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pixelData.width, pixelData.height, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, pixelData.data.data());
        glBindTexture(GL_TEXTURE_2D, 0);

        return true;
    }

    bool TextRender::CreateShaderProgram()
    {
        const char* vsrc = R"(
        attribute vec2 aPos;
        attribute vec2 aTex;
        varying vec2 vTex;
        uniform vec2 uResolution;
        void main() {
            vec2 clipSpace = (aPos / uResolution) * 2.0 - 1.0;
            gl_Position = vec4(clipSpace * vec2(1, -1), 0.0, 1.0);
            vTex = aTex;
        }
    )";

        const char* fsrc = nullptr;

        if (HasExtension("GL_OES_standard_derivatives")) {
            fsrc = R"(
            #extension GL_OES_standard_derivatives : enable
            
            precision highp float;
            uniform sampler2D uTexture;
            uniform vec4 uColor;
            varying vec2 vTex;
            
            void main() {
                float sample = texture2D(uTexture, vTex).r;
                float scale = 1.0 / fwidth(sample);
                float signedDistance = (sample - 0.5) * scale;
                float alpha = clamp(signedDistance + 0.5, 0.0, 1.0);
                gl_FragColor = vec4(uColor.rgb, alpha * uColor.a);
            }
        )";
        } else {
            fsrc = R"(
            precision highp float;
            uniform sampler2D uTexture;
            uniform vec4 uColor;
            varying vec2 vTex;
            
            void main() {
                float sample = texture2D(uTexture, vTex).r;
                float signedDistance = (sample - 0.5) * 5.0;
                float alpha = clamp(signedDistance + 0.5, 0.0, 1.0);
                gl_FragColor = vec4(uColor.rgb, alpha * uColor.a);
            }
        )";
        }

        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vsrc, nullptr);
        glCompileShader(vs);
        if (!CheckShaderCompile(vs, "TextRender vertex")) {
            return false;
        }

        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fsrc, nullptr);
        glCompileShader(fs);
        if (!CheckShaderCompile(fs, "TextRender fragment")) {
            glDeleteShader(vs);
            return false;
        }

        _program = glCreateProgram();
        glAttachShader(_program, vs);
        glAttachShader(_program, fs);
        glLinkProgram(_program);

        GLint linked = 0;
        glGetProgramiv(_program, GL_LINK_STATUS, &linked);
        if (!linked) {
            char log[512];
            glGetProgramInfoLog(_program, sizeof(log), nullptr, log);
            fprintf(stderr, "TextRender shader link error: %s\n", log);
            glDeleteShader(vs);
            glDeleteShader(fs);
            return false;
        }

        glDeleteShader(vs);
        glDeleteShader(fs);
        return true;
    }

    bool TextRender::CheckShaderCompile(GLuint shader, const char* label)
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

    bool TextRender::HasExtension(const char* extension) const
    {
        const char* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
        if (extensions == nullptr) {
            return false;
        }
        return strstr(extensions, extension) != nullptr;
    }

    void TextRender::CreateQuadBuffer()
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

    const Character* TextRender::FindCharacter(int codePoint) const
    {
        if (_font == nullptr) {
            return nullptr;
        }

        for (int i = 0; i < _font->characterCount; ++i) {
            if (_font->characters[i].codePoint == codePoint) {
                return &_font->characters[i];
            }
        }
        return nullptr;
    }

    void TextRender::Draw(const std::string& text, float x, float y)
    {
        if (_font == nullptr || text.empty()) {
            return;
        }

        glUseProgram(_program);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _fontTexture);

        GLint uResolution = glGetUniformLocation(_program, "uResolution");
        GLint uTexture = glGetUniformLocation(_program, "uTexture");
        GLint uColor = glGetUniformLocation(_program, "uColor");
        GLint aPos = glGetAttribLocation(_program, "aPos");
        GLint aTex = glGetAttribLocation(_program, "aTex");

        glUniform2f(uResolution, static_cast<float>(_canvasWidth), static_cast<float>(_canvasHeight));
        glUniform1i(uTexture, 0);
        glUniform4f(uColor, _colorR, _colorG, _colorB, _colorA);

        glBindBuffer(GL_ARRAY_BUFFER, _vbo);
        glEnableVertexAttribArray(aPos);
        glVertexAttribPointer(aPos, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray(aTex);
        glVertexAttribPointer(aTex, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

        float cursorX = x;
        float cursorY = _canvasHeight - y;

        const float atlasWidth = static_cast<float>(_font->width);
        const float atlasHeight = static_cast<float>(_font->height);

        for (char c : text) {
            const Character* ch = FindCharacter(static_cast<int>(c));
            if (ch == nullptr) {
                continue;
            }

            float xpos = cursorX + ch->originX * _scale;
            float ypos = cursorY - ch->originY * _scale;
            float w = ch->width * _scale;
            float h = ch->height * _scale;

            float u0 = ch->x / atlasWidth;
            float v0 = ch->y / atlasHeight;
            float u1 = (ch->x + ch->width) / atlasWidth;
            float v1 = (ch->y + ch->height) / atlasHeight;

            GLfloat vertices[] = {
                xpos, ypos, u0, v0,
                xpos + w, ypos, u1, v0,
                xpos, ypos + h, u0, v1,
                xpos + w, ypos + h, u1, v1
            };

            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            cursorX += (ch->width + 1) * _scale;
        }

        glDisableVertexAttribArray(aPos);
        glDisableVertexAttribArray(aTex);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_BLEND);
    }

    void TextRender::SetColor(float r, float g, float b, float a)
    {
        _colorR = r;
        _colorG = g;
        _colorB = b;
        _colorA = a;
    }

    void TextRender::SetScale(float scale)
    {
        _scale = scale;
    }

    void TextRender::Cleanup()
    {
        if (_fontTexture) {
            glDeleteTextures(1, &_fontTexture);
            _fontTexture = 0;
        }
        if (_vbo) {
            glDeleteBuffers(1, &_vbo);
            _vbo = 0;
        }
        if (_program) {
            glDeleteProgram(_program);
            _program = 0;
        }
    }

} // namespace Compositor
} // namespace Thunder