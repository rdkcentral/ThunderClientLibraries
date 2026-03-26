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
#include "TextRender.h"
#include <core/core.h>
#include <compositor/Client.h>
#include <EGL/egl.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <random>

namespace Thunder {
namespace Compositor {
    class Render : public Thunder::Compositor::IDisplay::ISurface::ICallback {
    public:
        static constexpr uint16_t DefaultWidth = 1920;
        static constexpr uint16_t DefaultHeight = 1080;

        Render(const Render&) = delete;
        Render& operator=(const Render&) = delete;
        Render& operator=(Render&&) = delete;

        Render();

        virtual ~Render() override
        {
            Stop();

            for (auto* model : _models) {
                Unregister(model);
            }

            CleanupEGL();

            if (_surface) {
                _surface->Release();
                _surface = nullptr;
            }
            if (_display) {
                _display->Release();
                _display = nullptr;
            }
        }

        EGLDisplay GetEGLDisplay() const { return _eglDisplay; }
        EGLContext GetEGLContext() const { return _eglContext; }
        EGLSurface GetEGLSurface() const { return _eglSurface; }

        bool Configure(const uint16_t width = DefaultWidth, const uint16_t height = DefaultHeight);

        void Start()
        {
            if (_models.empty()) {
                TRACE(Trace::Warning, ("No models registered, cannot start rendering"));
                return;
            }

            bool expected = false;

            std::uniform_int_distribution<uint8_t> modelDist(0, static_cast<uint8_t>(_models.size() - 1));

            if (_running.compare_exchange_strong(expected, true)) {
                TRACE(Trace::Information, ("Starting Render"));
                _selectedModel = modelDist(_rng);
                _render = std::thread(&Render::Draw, this);
            }
        }
        void Stop()
        {
            _running.store(false);
            _renderSync.notify_all();

            if (_render.joinable()) {
                TRACE(Trace::Information, ("Stopping Render thread"));
                _render.join();
                TRACE(Trace::Information, ("Render thread stopped"));
            }

            _selectedModel = ~0;
        }
        bool IsRunning() const
        {
            return _running.load();
        }
        void RequestExit()
        {
            TRACE(Trace::Information, ("Exit requested via output termination"));
            std::lock_guard<std::mutex> lock(_exitMutex);
            _exitRequested = true;
            _exitSignal.notify_one();
        }
        bool ShouldExit() const
        {
            return _exitRequested.load();
        }
        bool ToggleFPS()
        {
            _showFps = !_showFps;
            return _showFps;
        }

        bool ToggleRequestRender()
        {
            _skipRender = !_skipRender;
            return _skipRender;
        }

        bool ToggleModelRender()
        {
            _skipModel = !_skipModel;
            return _skipModel;
        }

        void TriggerRender()
        {
            _surface->RequestRender();

            if (WaitForRendered(1000) == Core::ERROR_TIMEDOUT) {
                TRACE(Trace::Warning, ("Timed out waiting for rendered callback"));
            }
        }

        // ICallback
        void Rendered(Thunder::Compositor::IDisplay::ISurface*) override;
        void Published(Thunder::Compositor::IDisplay::ISurface*) override;

        bool Register(IModel* model, const std::string& config);
        void Unregister(IModel* model);

    private:
        bool InitializeModel(IModel* model, const std::string& config);
        bool InitializeEGL();
        void CleanupEGL();
        void Draw();
        uint32_t WaitForRendered(uint32_t timeoutMs);

    private:
        Thunder::Compositor::IDisplay* _display;
        Thunder::Compositor::IDisplay::ISurface* _surface;

        std::string _displayName;
        uint16_t _canvasWidth;
        uint16_t _canvasHeight;

        EGLDisplay _eglDisplay;
        EGLContext _eglContext;
        EGLSurface _eglSurface;

        std::thread _render;

        std::mutex _exitMutex;
        std::condition_variable _exitSignal;
        std::atomic<bool> _exitRequested;

        std::atomic<bool> _running;
        std::mutex _rendering;
        std::condition_variable _renderSync;

        bool _showFps;
        bool _skipRender;
        bool _skipModel;

        std::vector<IModel*> _models;
        std::atomic<uint8_t> _selectedModel;

        std::mt19937 _rng;

        TextRender _textRender;
        uint64_t _lastFPSUpdate;
        uint32_t _frameCount;

        float _currentFPS;
    };
} // namespace Compositor
} // namespace Thunder