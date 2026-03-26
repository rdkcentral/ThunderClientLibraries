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

#pragma once

#include "Module.h"
#include "IModel.h"
#include "Fonts/Font.h"

#include <core/core.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

using namespace Thunder;
namespace Thunder {
namespace Compositor {
    class TextRender {
    public:
        class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;
            Config()
                : Core::JSON::Container()
                , FontAtlas()
                , Scale(1.0f)
                , Red(1.0f)
                , Green(1.0f)
                , Blue(1.0f)
                , Alpha(1.0f)
            {
                Add(_T("fontatlas"), &FontAtlas);
                Add(_T("scale"), &Scale);
                Add(_T("red"), &Red);
                Add(_T("green"), &Green);
                Add(_T("blue"), &Blue);
                Add(_T("alpha"), &Alpha);
            }
            ~Config() = default;

        public:
            Core::JSON::String FontAtlas;
            Core::JSON::DecUInt32 X;
            Core::JSON::DecUInt32 Y;
            Core::JSON::Float Scale;
            Core::JSON::Float Red;
            Core::JSON::Float Green;
            Core::JSON::Float Blue;
            Core::JSON::Float Alpha;
        };

        TextRender(const Font* font);

        TextRender() = delete;
        TextRender(const TextRender&) = delete;
        TextRender(TextRender&&) = delete;
        TextRender& operator=(const TextRender&) = delete;
        TextRender& operator=(TextRender&&) = delete;
        ~TextRender();

        bool Initialize(const uint16_t width, const uint16_t height, const std::string& config);
        void Draw(const std::string& text, float x, float y);
        void SetColor(float r, float g, float b, float a);
        void SetScale(float scale);

    private:
        bool HasExtension(const char* extension) const;
        bool LoadFontAtlas(const std::string& path);
        bool CreateShaderProgram();
        bool CheckShaderCompile(GLuint shader, const char* label);
        void CreateQuadBuffer();
        const Character* FindCharacter(int codePoint) const;
        void Cleanup();

    private:
        GLuint _program;
        GLuint _fontTexture;
        GLuint _vbo;

        int _canvasWidth;
        int _canvasHeight;

        float _scale;
        float _colorR;
        float _colorG;
        float _colorB;
        float _colorA;

        const Font* _font;
    };

} // namespace Compositor
} // namespace Thunder