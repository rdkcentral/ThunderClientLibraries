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

#include <core/core.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <random>
#include "TextureLoader.h"

namespace Thunder {
namespace Compositor {
    class TextureBounce : public IModel {
    public:
        class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;
            Config()
                : Core::JSON::Container()
                , Image()
                , ImageCount(1)
            {
                Add(_T("image"), &Image);
                Add(_T("imagecount"), &ImageCount);
            }
            ~Config() = default;

        public:
            Core::JSON::String Image;
            Core::JSON::DecUInt32 ImageCount;
        }; // class Config

        struct Sprite {
            float x, y;
            float vx, vy;
            float width, height;
            float mass;
        };

        static constexpr uint16_t DefaultWidth = 1920;
        static constexpr uint16_t DefaultHeight = 1080;
        static constexpr uint8_t MaxSprites = 40;

        TextureBounce(); //  = delete;
        TextureBounce(const TextureBounce&) = delete;
        TextureBounce(TextureBounce&&) = delete;
        TextureBounce& operator=(const TextureBounce&) = delete;
        TextureBounce& operator=(TextureBounce&&) = delete;

        // TextureBounce(const uint16_t width = DefaultWidth, const uint16_t height = DefaultHeight);
        ~TextureBounce();

        bool Initialize(const uint16_t width, const uint16_t height, const std::string& config) override;
        bool Draw() override;

    private:
        bool LoadTexture(const std::string& path);
        bool CreateShaderProgram();
        bool CheckShaderCompile(GLuint shader, const char* label);
        void CreateVertexBuffer();
        bool RenderFrame();
        void UpdateSprites(float dt);
        void HandleCollisions();
        void Cleanup();

        template <typename T>
        inline const T& Clamp(const T& v, const T& lo, const T& hi)
        {
            return (v < lo) ? lo : (v > hi) ? hi
                                            : v;
        }

    private:
        // GL
        GLuint _program;
        GLuint _textureId;
        GLuint _vbo;

        int _canvasHeight;
        int _canvasWidth;

        uint32_t _textureWidth;
        uint32_t _textureHeight;

        // Sprites
        std::vector<Sprite> _sprites;
        float _scale;

        // Timing
        uint64_t _lastFrameTime;
    };
} // namespace Compositor
} // namespace Thunder