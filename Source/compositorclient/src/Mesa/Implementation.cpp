/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological B.V.
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

#include "../Module.h"

extern "C" {
#include <drm_fourcc.h>

#include <gbm.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
}

#include <com/com.h>

#include <interfaces/IComposition.h>
#include <virtualinput/virtualinput.h>
#include <privilegedrequest/PrivilegedRequest.h>

#include <interfaces/ICompositionBuffer.h>
#include <compositorbuffer/CompositorBufferType.h>

#include <compositor/Client.h>

#include "RenderAPI.h"

// Copied from previous macros
#define XSTRINGIFY(X) STRINGIFY(X)
#define STRINGIFY(X) #X

#ifndef EGL_VERSION_1_5
#define _KHRFIX(left, right) left##right
#define KHRFIX(name) _KHRFIX(name, KHR)
#define EGL_SYNC_FENCE EGL_SYNC_FENCE_KHR
#define EGL_NO_SYNC EGL_NO_SYNC_KHR
#define EGL_FOREVER EGL_FOREVER_KHR
#define EGL_NO_IMAGE EGL_NO_IMAGE_KHR
#define EGL_CONDITION_SATISFIED EGL_CONDITION_SATISFIED_KHR
#define EGL_SYNC_FLUSH_COMMANDS_BIT EGL_SYNC_FLUSH_COMMANDS_BIT_KHR
#define EGL_SIGNALED EGL_SIGNALED_KHR
#define EGL_SYNC_STATUS EGL_SYNC_STATUS_KHR
using EGLAttrib = EGLint;
using EGLImage = EGLImageKHR;
#else
#define KHRFIX(name) name
#endif

namespace WPEFramework {
namespace Linux {
    namespace {
        const string ClientBridge()
        {
            string connector;
            if ((Core::SystemInfo::GetEnvironment(_T("COMPOSITOR_CLIENTBRIDGE"), connector) == false) || (connector.empty() == true)) {
                connector = _T("/tmp/clientbridge");
            }
            return connector;
        }

        const string CompositorConnector()
        {
            string connector;
            if ((Core::SystemInfo::GetEnvironment(_T("COMPOSITOR_COMMUNICATOR"), connector) == false) || (connector.empty() == true)) {
                connector = _T("/tmp/communicator");
            }
            return connector;
        }

        const string InputConnector()
        {
            string connector;
            if ((Core::SystemInfo::GetEnvironment(_T("VIRTUAL_INPUT"), connector) == false) || (connector.empty() == true)) {
                connector = _T("/tmp/keyhandler");
            }
            return connector;
        }

        // uint32_t WidthFromResolution(Exchange::IComposition::ScreenResolution const resolution) const
        // {
        //     // Assume an invalid width equals 0
        //     uint32_t width = 0;

        //     switch (resolution) {
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_480p: // 720x480
        //         width = 720;
        //         break;
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_576p50Hz: // 1024x576 progressive
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_576i: // 1024x576
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_576p: // 1024x576 progressive
        //         width = 1024;
        //         break;
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_720p: // 1280x720 progressive
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_720p50Hz: // 1280x720 @ 50 Hz
        //         width = 1280;
        //         break;
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p: // 1280x720 progressive
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_1080i: // 1280x720 progressive
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p24Hz: // 1920x1080 progressive @ 24 Hz
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_1080i25Hz: // 1920x1080 interlaced @ 25 Hz
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p25Hz: // 1920x1080 progressive @ 25 Hz
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_1080i50Hz: // 1920x1080 interlaced  @ 50 Hz
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p50Hz: // 1920x1080 progressive @ 50 Hz
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p60Hz: // 1920x1080 progressive @ 60 Hz
        //         width = 1920;
        //         break;
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_2160p50Hz: // 4K, 3840x2160 progressive @ 50 Hz
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_2160p60Hz: // 4K, 3840x2160 progressive @ 60 Hz
        //         width = 3840;
        //         break;
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_4320p30Hz: // 8K, 7680x4320 progressive @ 30 Hz
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_4320p60Hz: // 8K, 7680x4320 progressive @ 60 Hz
        //         width = 7680;
        //         break;
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_480i: // Unknown according to the standards (?)
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_Unknown:
        //     default:
        //         width = 0;
        //     }

        //     return width;
        // }

        // uint32_t HeightFromResolution(Exchange::IComposition::ScreenResolution const resolution)
        // {
        //     // Assume an invalid height equals 0
        //     uint32_t height = 0;

        //     switch (resolution) {
        //     // For descriptions see WidthFromResolution
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_480i: // 720x480
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_480p: // 720x480 progressive
        //         height = 480;
        //         break;
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_576p50Hz: // 1024x576 progressive
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_576i: // 1024x576
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_576p: // 1024x576 progressive
        //         height = 576;
        //         break;
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_720p: // 1280x720 progressive
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_720p50Hz: // 1280x720 progressive @ 50 Hz
        //         height = 720;
        //         break;
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p: // 1280x720 progressive
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_1080i: // 1280x720 progressive
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p24Hz: // 1920x1080 progressive @ 24 Hz
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_1080i25Hz: // 1920x1080 interlaced @ 25 Hz
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p25Hz: // 1920x1080 progressive @ 25 Hz
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_1080i50Hz: // 1920x1080 interlaced @ 50 Hz
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p50Hz: // 1920x1080 progressive @ 50 Hz
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p60Hz: // 1920x1080 progressive @ 60 Hz
        //         height = 1080;
        //         break;
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_2160p50Hz: // 4K, 3840x2160 progressive @ 50 Hz
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_2160p60Hz: // 4K, 3840x2160 progressive @ 60 Hz
        //         height = 2160;
        //         break;
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_4320p30Hz: // 8K, 7680x4320 progressive @ 30 Hz
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_4320p60Hz: // 8K, 7680x4320 progressive @ 60 Hz
        //         height = 4320;
        //         break;
        //     case Exchange::IComposition::ScreenResolution::ScreenResolution_Unknown:
        //     default:
        //         height = 0;
        //     }

        //     return height;
        // }

        uint8_t RefreshRateFromResolution(const Exchange::IComposition::ScreenResolution resolution)
        {
            // Assume 'unknown' rate equals 60 Hz
            uint8_t rate = 60;

            switch (resolution) {
            case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p24Hz: // 1920x1080 progressive @ 24 Hz
                rate = 24;
                break;
            case Exchange::IComposition::ScreenResolution::ScreenResolution_1080i25Hz: // 1920x1080 interlaced @ 25 Hz
            case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p25Hz: // 1920x1080 progressive @ 25 Hz
                rate = 25;
                break;
            case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p30Hz: // 1920x1080 progressive @ 30 Hz
            case Exchange::IComposition::ScreenResolution::ScreenResolution_2160p30Hz: // 4K, 3840x2160 progressive @ 30 Hz
            case Exchange::IComposition::ScreenResolution::ScreenResolution_4320p30Hz: // 8K, 7680x4320 progressive @ 30 Hz
                rate = 30;
                break;
            case Exchange::IComposition::ScreenResolution::ScreenResolution_576p50Hz: // 1024x576 progressive
            case Exchange::IComposition::ScreenResolution::ScreenResolution_720p50Hz: // 1280x720 progressive @ 50 Hz
            case Exchange::IComposition::ScreenResolution::ScreenResolution_1080i50Hz: // 1920x1080 interlaced @ 50 Hz
            case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p50Hz: // 1920x1080 progressive @ 50 Hz
            case Exchange::IComposition::ScreenResolution::ScreenResolution_2160p50Hz: // 4K, 3840x2160 progressive @ 50 Hz
                rate = 50;
                break;
            case Exchange::IComposition::ScreenResolution::ScreenResolution_480i: // 720x480
            case Exchange::IComposition::ScreenResolution::ScreenResolution_480p: // 720x480 progressive
            case Exchange::IComposition::ScreenResolution::ScreenResolution_576p: // 1024x576 progressive
            case Exchange::IComposition::ScreenResolution::ScreenResolution_576i: // 1024x576 progressive
            case Exchange::IComposition::ScreenResolution::ScreenResolution_720p: // 1280x720 progressive
            case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p: // 1280x720 progressive
            case Exchange::IComposition::ScreenResolution::ScreenResolution_1080i: // 1280x720 progressive
            case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p60Hz: // 1920x1080 progressive @ 60 Hz
            case Exchange::IComposition::ScreenResolution::ScreenResolution_2160p60Hz: // 4K, 3840x2160 progressive @ 60 Hz
            case Exchange::IComposition::ScreenResolution::ScreenResolution_4320p60Hz: // 8K, 7680x4320 progressive @ 60 Hz
            case Exchange::IComposition::ScreenResolution::ScreenResolution_Unknown:
                rate = 60;
            }

            return rate;
        };

        void GetDRMNodes(const uint32_t type, std::vector<std::string>& list)
        {
            // Just an arbitrary choice
            /* static */ constexpr const uint8_t DrmMaxDevices = 16;

            drmDevicePtr devices[DrmMaxDevices];

            int device_count = drmGetDevices2(0 /* flags */, &devices[0], static_cast<int>(DrmMaxDevices));

            if (device_count > 0) {
                for (int i = 0; i < device_count; i++) {
                    switch (type) {
                    case DRM_NODE_PRIMARY: // card<num>, always created, KMS, privileged
                    case DRM_NODE_CONTROL: // ControlD<num>, currently unused
                    case DRM_NODE_RENDER: // Solely for render clients, unprivileged
                    {
                        if ((1 << type) == (devices[i]->available_nodes & (1 << type))) {
                            list.push_back(std::string(devices[i]->nodes[type]));
                        }
                        break;
                    }
                    case DRM_NODE_MAX:
                    default: // Unknown (new) node type
                        break;
                    }
                }

                drmFreeDevices(&devices[0], device_count);
            }
        }
    }

    class Display : public Compositor::IDisplay {

    public:
        Display() = delete;
        Display(const Display&) = delete;
        Display& operator=(const Display&) = delete;

    private:
        Display(const std::string& displayName);

        class SurfaceImplementation;

        using InputFunction = std::function<void(SurfaceImplementation*)>;

        static void Publish(InputFunction& action);

        static void VirtualKeyboardCallback(keyactiontype, const unsigned int);
        static void VirtualMouseCallback(mouseactiontype, const unsigned short, const signed short, const signed short);
        static void VirtualTouchScreenCallback(touchactiontype, const unsigned short, const unsigned short, const unsigned short);

        class RemoteBuffer : public WPEFramework::Compositor::CompositorBufferType<4> {
        private:
            using BaseClass = WPEFramework::Compositor::CompositorBufferType<4>;

        protected:
            RemoteBuffer(const uint32_t id, Core::PrivilegedRequest::Container& descriptors)
                : BaseClass(id, descriptors)
            {
            }

        public:
            RemoteBuffer() = delete;
            RemoteBuffer(RemoteBuffer&&) = delete;
            RemoteBuffer(const RemoteBuffer&) = delete;
            RemoteBuffer& operator=(const RemoteBuffer&) = delete;

            static Exchange::ICompositionBuffer* Create(const uint32_t id, Core::PrivilegedRequest::Container& descriptors)
            {
                Core::ProxyType<RemoteBuffer> element(Core::ProxyType<RemoteBuffer>::Create(id, descriptors));
                Exchange::ICompositionBuffer* result = &(*element);
                result->AddRef();
                return (result);
            }

        public:
            void Render() override
            {
                ASSERT(false); // This should never be called, we are a remote buffer
            }
        };

        class SurfaceImplementation : public Compositor::IDisplay::ISurface {
        private:
            void Fence(const EGLDisplay dpy)
            {
                ASSERT(dpy != EGL_NO_DISPLAY);
                EGLSync fence = _egl.eglCreateSync(dpy, EGL_SYNC_FENCE, NULL);

                glFlush(); // Mandatory

                _egl.eglClientWaitSync(dpy, fence, EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, EGL_FOREVER_KHR);
                _egl.eglDestroySync(dpy, fence);
            }

            bool CreateImage(EGLDisplay dpy)
            {
                ASSERT(_remoteClient != nullptr);
                ASSERT(_remoteBuffer != nullptr);

                Exchange::ICompositionBuffer::IIterator* planes = _remoteBuffer->Planes(10);
                ASSERT(planes != nullptr);

                planes->Next();
                ASSERT(planes->IsValid() == true);

                Exchange::ICompositionBuffer::IPlane* plane = planes->Plane();
                ASSERT(plane != nullptr); // we should atleast have 1 plane....

                Compositor::API::Attributes<EGLAttrib> imageAttributes;

                imageAttributes.Append(EGL_WIDTH, _remoteBuffer->Width());
                imageAttributes.Append(EGL_HEIGHT, _remoteBuffer->Height());
                imageAttributes.Append(EGL_LINUX_DRM_FOURCC_EXT, _remoteBuffer->Format());

                imageAttributes.Append(EGL_DMA_BUF_PLANE0_FD_EXT, plane->Accessor());
                imageAttributes.Append(EGL_DMA_BUF_PLANE0_OFFSET_EXT, plane->Offset());
                imageAttributes.Append(EGL_DMA_BUF_PLANE0_PITCH_EXT, plane->Stride());
                imageAttributes.Append(EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, (_remoteBuffer->Modifier() & 0xFFFFFFFF));
                imageAttributes.Append(EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, (_remoteBuffer->Modifier() >> 32));

                TRACE(Trace::Information, (_T("Add Plane 0 fd=%d, offset=%d, stride=%d, modifier=%" PRIu64), plane->Accessor(), plane->Offset(), plane->Stride(), _remoteBuffer->Modifier()));

                if (planes->Next() == true) {
                    plane = planes->Plane();

                    ASSERT(plane != nullptr);

                    imageAttributes.Append(EGL_DMA_BUF_PLANE1_FD_EXT, plane->Accessor());
                    imageAttributes.Append(EGL_DMA_BUF_PLANE1_OFFSET_EXT, plane->Offset());
                    imageAttributes.Append(EGL_DMA_BUF_PLANE1_PITCH_EXT, plane->Stride());
                    imageAttributes.Append(EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT, (_remoteBuffer->Modifier() & 0xFFFFFFFF));
                    imageAttributes.Append(EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT, (_remoteBuffer->Modifier() >> 32));

                    TRACE(Trace::Information, (_T("Add Plane 1 fd=%d, offset=%d, stride=%d, modifier=%" PRIu64), plane->Accessor(), plane->Offset(), plane->Stride(), _remoteBuffer->Modifier()));
                }

                if (planes->Next() == true) {
                    plane = planes->Plane();

                    ASSERT(plane != nullptr);

                    imageAttributes.Append(EGL_DMA_BUF_PLANE2_FD_EXT, plane->Accessor());
                    imageAttributes.Append(EGL_DMA_BUF_PLANE2_OFFSET_EXT, plane->Offset());
                    imageAttributes.Append(EGL_DMA_BUF_PLANE2_PITCH_EXT, plane->Stride());
                    imageAttributes.Append(EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT, (_remoteBuffer->Modifier() & 0xFFFFFFFF));
                    imageAttributes.Append(EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT, (_remoteBuffer->Modifier() >> 32));

                    TRACE(Trace::Information, (_T("Add Plane 2 fd=%d, offset=%d, stride=%d, modifier=%" PRIu64), plane->Accessor(), plane->Offset(), plane->Stride(), _remoteBuffer->Modifier()));
                }

                if (planes->Next() == true) {
                    plane = planes->Plane();

                    ASSERT(plane != nullptr);

                    imageAttributes.Append(EGL_DMA_BUF_PLANE3_FD_EXT, plane->Accessor());
                    imageAttributes.Append(EGL_DMA_BUF_PLANE3_OFFSET_EXT, plane->Offset());
                    imageAttributes.Append(EGL_DMA_BUF_PLANE3_PITCH_EXT, plane->Stride());
                    imageAttributes.Append(EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT, (_remoteBuffer->Modifier() & 0xFFFFFFFF));
                    imageAttributes.Append(EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT, (_remoteBuffer->Modifier() >> 32));

                    TRACE(Trace::Information, (_T("Add Plane 3 fd=%d, offset=%d, stride=%d, modifier=%" PRIu64), plane->Accessor(), plane->Offset(), plane->Stride(), _remoteBuffer->Modifier()));
                }

                imageAttributes.Append(EGL_IMAGE_PRESERVED_KHR, EGL_TRUE);

                _eglImage = _egl.eglCreateImage(dpy, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, imageAttributes);
                ASSERT(_eglImage != EGL_NO_IMAGE);

                glFlush(); // Mandatory

                TRACE(Trace::Information, (_T("Created image %dx%d(hxb) on buffer id=%d, _eglImage=%p, format=0x%04X"), _remoteBuffer->Height(), _remoteBuffer->Width(), _remoteBuffer->Identifier(), _eglImage, _remoteBuffer->Format()));

                return (_eglImage != EGL_NO_IMAGE);
            }

            bool RenderImage()
            {
                constexpr const GLuint target = GL_TEXTURE_2D;
                constexpr const GLuint filter = GL_LINEAR;
                constexpr const GLuint wrap = GL_CLAMP_TO_EDGE;

                bool ret(false);

                // Just an arbitrary selected unit
                glActiveTexture(GL_TEXTURE0);

                if (_textureId != 0) {
                    glDeleteTextures(1, &_textureId);
                    _textureId = 0;
                }

                // GLES (extension: GL_OES_EGL_image_external): Create GL texture from EGL image
                glGenTextures(1, &_textureId);
                glBindTexture(target, _textureId);
                glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter);
                glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter);
                glTexParameteri(target, GL_TEXTURE_WRAP_S, wrap);
                glTexParameteri(target, GL_TEXTURE_WRAP_T, wrap);

                _gl.glEGLImageTargetTexture2DOES(target, _eglImage);

                if (_textureId != 0) {
                    glDeleteFramebuffers(1, &_frameBuffer);
                    _frameBuffer = 0;
                }

                glGenFramebuffers(1, &_frameBuffer);
                glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);

                // Bind the created texture as one of the buffers of the frame buffer object
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, _textureId, 0 /* level */);

                ret = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

                if (ret != true) {
                    glDeleteFramebuffers(1, &_frameBuffer);
                    glDeleteTextures(1, &_textureId);

                    _frameBuffer = 0;
                    _textureId = 0;
                }

                return ret;
            }

        public:
            SurfaceImplementation() = delete;
            SurfaceImplementation(const SurfaceImplementation&) = delete;
            SurfaceImplementation& operator=(const SurfaceImplementation&) = delete;

            SurfaceImplementation(Display& display, const std::string& name, const uint32_t width, const uint32_t height)
                : _adminLock()
                , _display(display)
                , _keyboard(nullptr)
                , _wheel(nullptr)
                , _pointer(nullptr)
                , _touchpanel(nullptr)
                , _remoteClient(nullptr)
                , _surface(nullptr)
                , _eglImage(EGL_NO_IMAGE)
                , _textureId(0)
                , _frameBuffer(0)
                , _remoteBuffer(nullptr)
                , _egl()
                , _gl()
            {

                TRACE(Trace::Information, (_T("Construct surface %s  %dx%d (hxb)"), name.c_str(), height, width));

                _display.AddRef();

                // maybe we should use the SmartInterfaceType for _remoteClient
                _remoteClient = _display.CreateRemoteSurface(name, width, height);

                if (_remoteClient != nullptr) {
                    TRACE(Trace::Information, (_T("Created remote surface %s  %dx%d"), name.c_str(), width, height));

                    Core::PrivilegedRequest::Container descriptors;
                    Core::PrivilegedRequest request;

                    if (request.Request(1000, ClientBridge(), _remoteClient->Native(), descriptors) == Core::ERROR_NONE) {
                        _remoteBuffer = RemoteBuffer::Create(_remoteClient->Native(), descriptors);
                    }

                    if (_remoteBuffer != nullptr) {
                        TRACE(Trace::Information, (_T("Remote buffer %p ready %dx%d format=0x%04X"), _remoteBuffer, _remoteBuffer->Width(), _remoteBuffer->Height(), _remoteBuffer->Format()));

                        _surface = gbm_surface_create(
                            static_cast<gbm_device*>(_display.Native()),
                            _remoteBuffer->Width(), _remoteBuffer->Height(), _remoteBuffer->Format(),
                            GBM_BO_USE_RENDERING /* used for rendering */);

                        TRACE(Trace::Information, (_T("GBM surface %p ready %dx%d(hxb) format=0x%04X"), _surface, _remoteBuffer->Width(), _remoteBuffer->Height(), _remoteBuffer->Format()));
                    }

                    ASSERT(_surface != nullptr);
                } else {
                    TRACE(Trace::Error, (_T("Could not create remote surface for surface %s." ), name.c_str()));
                }

                _display.Register(this);
            }

            ~SurfaceImplementation() override
            {
                _display.Unregister(this);

                if (_keyboard != nullptr) {
                    _keyboard->Release();
                }

                if (_wheel != nullptr) {
                    _wheel->Release();
                }

                if (_pointer != nullptr) {
                    _pointer->Release();
                }

                if (_touchpanel != nullptr) {
                    _touchpanel->Release();
                }

                if (_frameBuffer != 0) {
                    glDeleteFramebuffers(1, &_frameBuffer);
                }

                if (_textureId != 0) {
                    glDeleteTextures(1, &_textureId);
                }

                EGLDisplay dpy = eglGetCurrentDisplay();

                if (_eglImage != EGL_NO_IMAGE) {
                    _egl.eglDestroyImage(dpy, _eglImage);
                }

                if (_surface != nullptr) {
                    gbm_surface_destroy(_surface);
                }

                if (_remoteBuffer != nullptr) {
                    _remoteBuffer->Release();
                }

                if (_remoteClient != nullptr) {
                    _remoteClient->Release();
                }

                _display.Release();
            }

            EGLNativeWindowType Native() const override
            {
                return static_cast<EGLNativeWindowType>(_surface);
            }

            std::string Name() const override
            {
                ASSERT(_remoteClient != nullptr);
                return _remoteClient->Name();
            }

            void Keyboard(Compositor::IDisplay::IKeyboard* keyboard) override
            {
                assert((_keyboard == nullptr) ^ (keyboard == nullptr));
                _keyboard = keyboard;
                _keyboard->AddRef();
            }
            void Pointer(Compositor::IDisplay::IPointer* pointer) override
            {
                assert((_pointer == nullptr) ^ (pointer == nullptr));
                _pointer = pointer;
                _pointer->AddRef();
            }
            void Wheel(Compositor::IDisplay::IWheel* wheel) override
            {
                assert((_wheel == nullptr) ^ (wheel == nullptr));
                _wheel = wheel;
                _wheel->AddRef();
            }
            void TouchPanel(Compositor::IDisplay::ITouchPanel* touchpanel) override
            {
                assert((_touchpanel == nullptr) ^ (touchpanel == nullptr));
                _touchpanel = touchpanel;
                _touchpanel->AddRef();
            }

            int32_t Width() const override
            {
                ASSERT(_remoteClient != nullptr);
                return _remoteClient->Geometry().width;
            }
            int32_t Height() const override
            {
                ASSERT(_remoteClient != nullptr);
                return _remoteClient->Geometry().height;
            }

            inline void SendKey(const uint32_t key, const IKeyboard::state action, const uint32_t timestamp VARIABLE_IS_NOT_USED)
            {
                if (_keyboard != nullptr) {
                    _keyboard->Direct(key, action);
                }
            }
            inline void SendWheelMotion(const int16_t x, const int16_t y, const uint32_t timestamp VARIABLE_IS_NOT_USED)
            {
                if (_wheel != nullptr) {
                    _wheel->Direct(x, y);
                }
            }
            inline void SendPointerButton(const uint8_t button, const IPointer::state state, const uint32_t timestamp VARIABLE_IS_NOT_USED)
            {
                if (_pointer != nullptr) {
                    _pointer->Direct(button, state);
                }
            }
            inline void SendPointerPosition(const int16_t x, const int16_t y, const uint32_t timestamp VARIABLE_IS_NOT_USED)
            {
                if (_pointer != nullptr) {
                    _pointer->Direct(x, y);
                }
            }
            inline void SendTouch(const uint8_t index, const ITouchPanel::state state, const uint16_t x, const uint16_t y, const uint32_t timestamp VARIABLE_IS_NOT_USED)
            {
                if (_touchpanel != nullptr) {
                    _touchpanel->Direct(index, state, x, y);
                }
            }

            uint32_t Process()
            {
                // TRACE(Trace::Information, (_T("Processing surface %p"), this));

                // Changes of currents cannot be reliably be monitored
                EGLDisplay dpy = eglGetCurrentDisplay();
                EGLContext ctx = eglGetCurrentContext();
                // EGLSurface surf = eglGetCurrentSurface(EGL_DRAW);

                bool status = (dpy != EGL_NO_DISPLAY && ctx != EGL_NO_CONTEXT /*&& surf != EGL_NO_SURFACE*/);

                ASSERT(status == true); // Process need to be called in a active GL context

                // A remote ClientSurface has been created and the IRender interface is supported so the compositor is able to support scan out for this client
                if ((status == true) && (_remoteBuffer != nullptr) && (_remoteClient != nullptr)) {
                    EGLSync fence = _egl.eglCreateSync(dpy, EGL_SYNC_FENCE, NULL);

                    if (_eglImage == EGL_NO_IMAGE) {
                        CreateImage(dpy);
                    }

                    // Lock the buffer
                    _remoteBuffer->Planes(10);

                    RenderImage();

                    // Wait for all EGL actions to be completed
                    _egl.eglClientWaitSync(dpy, fence, EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, EGL_FOREVER_KHR);

                    // Signal the other side to render the buffer
                    _remoteBuffer->Completed(true);
                    _egl.eglDestroySync(dpy, fence);
                } else {
                    TRACE(Trace::Error, (_T ( "Remote scan out is not (yet) supported. Has a remote surface been created? Is the IRender interface available?" )));
                }

                return Core::ERROR_NONE;
            }

        private:
            mutable Core::CriticalSection _adminLock;

            Display& _display;

            IKeyboard* _keyboard;
            IWheel* _wheel;
            IPointer* _pointer;
            ITouchPanel* _touchpanel;

            Exchange::IComposition::IClient* _remoteClient;

            struct gbm_surface* _surface;

            EGLImage _eglImage;
            GLuint _textureId;
            GLuint _frameBuffer;

            Exchange::ICompositionBuffer* _remoteBuffer;

            Compositor::API::EGL _egl;
            Compositor::API::GL _gl;
        };

    public:
        typedef std::map<const string, Display*> DisplayMap;

        ~Display() override;

        static Display& Instance(const string& displayName)
        {
            Display* result(nullptr);

            _displaysMapLock.Lock();

            DisplayMap::iterator index(_displays.find(displayName));

            if (index == _displays.end()) {
                result = new Display(displayName);
                _displays.insert(std::pair<const std::string, Display*>(displayName, result));
            } else {
                result = index->second;
            }
            result->AddRef();
            _displaysMapLock.Unlock();

            ASSERT(result != nullptr);

            return (*result);
        }

        uint32_t AddRef() const override
        {
            if (Core::InterlockedIncrement(_refCount) == 1) {
                const_cast<Display*>(this)->Initialize();
            }
            return Core::ERROR_NONE;
        }
        uint32_t Release() const override
        {
            if (Core::InterlockedDecrement(_refCount) == 0) {
                _displaysMapLock.Lock();

                DisplayMap::iterator display = _displays.find(_displayName);

                if (display != _displays.end()) {
                    _displays.erase(display);
                }

                _displaysMapLock.Unlock();

                const_cast<Display*>(this)->Deinitialize();

                return (Core::ERROR_DESTRUCTION_SUCCEEDED);
            }
            return (Core::ERROR_NONE);
        }

        EGLNativeDisplayType Native() const override
        {
            TRACE(Trace::Information, (_T("Get native display")));
            return static_cast<EGLNativeDisplayType>(_nativeDisplay.device);
        }

        const std::string& Name() const final override
        {
            return _displayName;
        }

        ISurface* Create(const std::string& name, const uint32_t width, const uint32_t height) override;

        int Process(const uint32_t data) override;

        int FileDescriptor() const override;

        ISurface* SurfaceByName(const std::string& name) override
        {
            IDisplay::ISurface* result = nullptr;

            _adminLock.Lock();

            std::list<SurfaceImplementation*>::iterator index(_surfaces.begin());
            while ((index != _surfaces.end()) && ((*index)->Name() != name)) {
                index++;
            }

            if (index != _surfaces.end()) {
                result = *index;
                result->AddRef();
            }

            _adminLock.Unlock();

            return result;
        }

        const Exchange::IComposition::IDisplay* RemoteDisplay() const
        {
            return _remoteDisplay;
        }
        Exchange::IComposition::IDisplay* RemoteDisplay()
        {
            return _remoteDisplay;
        }

    private:
        void Initialize()
        {
            TRACE(Trace::Information, (_T("PID: %d: Compositor connector: %s"), getpid(), CompositorConnector().c_str()));
            TRACE(Trace::Information, (_T("PID: %d: Client connector: %s"), getpid(), ClientBridge().c_str()));
            TRACE(Trace::Information, (_T("PID: %d: Input connector: %s"), getpid(), InputConnector().c_str()));

            _adminLock.Lock();

            if (WPEFramework::Core::WorkerPool::IsAvailable() == true) {
                // If we are in the same process space as where a WorkerPool is registered (Main Process or
                // hosting process) use, it!
                Core::ProxyType<RPC::InvokeServer> engine = Core::ProxyType<RPC::InvokeServer>::Create(&Core::WorkerPool::Instance());

                _compositorServerRPCConnection = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId(CompositorConnector().c_str()), Core::ProxyType<Core::IIPCServer>(engine));
            } else {
                // Seems we are not in a process space initiated from the Main framework process or its hosting process.
                // Nothing more to do than to create a workerpool for RPC our selves !
                Core::ProxyType<RPC::InvokeServerType<2, 0, 8>> engine = Core::ProxyType<RPC::InvokeServerType<2, 0, 8>>::Create();

                _compositorServerRPCConnection = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId(CompositorConnector().c_str()), Core::ProxyType<Core::IIPCServer>(engine));
            }

            // if (display != nullptr) {
            //     _remoteDisplay = display;
            //     _remoteDisplay->AddRef();
            // } else {
            uint32_t result = _compositorServerRPCConnection->Open(RPC::CommunicationTimeOut);

            if (result == Core::ERROR_NONE) {
                _remoteDisplay = _compositorServerRPCConnection->Acquire<Exchange::IComposition::IDisplay>(2000, _displayName, ~0);

                if (_remoteDisplay == nullptr) {
                    TRACE(Trace::Error, (_T ( "Could not create remote display for Display %s!" ), Name().c_str()));
                }
            } else {
                TRACE(Trace::Error, (_T("Could not open connection to Compositor with node %s. Error: %s"), _compositorServerRPCConnection->Source().RemoteId().c_str(), Core::NumberType<uint32_t>(result).Text().c_str()));
                _compositorServerRPCConnection.Release();
            }
            // }

            _virtualinput = virtualinput_open(_displayName.c_str(), InputConnector().c_str(), VirtualKeyboardCallback, VirtualMouseCallback, VirtualTouchScreenCallback);

            if (_virtualinput == nullptr) {
                TRACE(Trace::Error, (_T("Initialization of virtual input failed for Display %s!"), Name().c_str()));
            }

            _adminLock.Unlock();
        }

        void Deinitialize()
        {
            _adminLock.Lock();

            if (_virtualinput != nullptr) {
                virtualinput_close(_virtualinput);
                _virtualinput = nullptr;
            }

            std::list<SurfaceImplementation*>::iterator index(_surfaces.begin());
            while (index != _surfaces.end()) {
                string name = (*index)->Name();

                if ((*index)->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) { // note, need cast to prevent ambiguous call
                    TRACE(Trace::Error, (_T("Compositor Surface [%s] is not properly destructed" ), name.c_str()));
                }

                index = _surfaces.erase(index);
            }

            if (_remoteDisplay != nullptr) {
                _remoteDisplay->Release();
                _remoteDisplay = nullptr;
            }

            if (_compositorServerRPCConnection.IsValid() == true) {
                _compositorServerRPCConnection->Close(RPC::CommunicationTimeOut);
                _compositorServerRPCConnection.Release();
            }

            _adminLock.Unlock();
        }

        void Register(SurfaceImplementation* surface)
        {
            ASSERT(surface != nullptr);

            _adminLock.Lock();

            std::list<SurfaceImplementation*>::iterator index(std::find(_surfaces.begin(), _surfaces.end(), surface));

            if (index == _surfaces.end()) {
                _surfaces.push_back(surface);
            }

            _adminLock.Unlock();
        }

        void Unregister(SurfaceImplementation* surface)
        {
            ASSERT(surface != nullptr);

            _adminLock.Lock();

            auto index(std::find(_surfaces.begin(), _surfaces.end(), surface));

            ASSERT(index != _surfaces.end());

            if (index != _surfaces.end()) {
                _surfaces.erase(index);
            }

            _adminLock.Unlock();
        }

        WPEFramework::Exchange::IComposition::IClient* CreateRemoteSurface(const std::string& name, const uint32_t width, const uint32_t height)
        {
            return (_remoteDisplay != nullptr ? _remoteDisplay->CreateClient(name, width, height) : nullptr);
        }

    private:
        static DisplayMap _displays;
        static Core::CriticalSection _displaysMapLock;

    private:
        const std::string _displayName;
        mutable Core::CriticalSection _adminLock;
        mutable uint32_t _refCount;
        void* _virtualinput;
        std::list<SurfaceImplementation*> _surfaces;
        Core::ProxyType<RPC::CommunicatorClient> _compositorServerRPCConnection;
        Exchange::IComposition::IDisplay* _remoteDisplay;

        struct {
            struct gbm_device* device;
            int fd;
        } _nativeDisplay;
    }; // class Display

    Display::DisplayMap Display::_displays;
    Core::CriticalSection Display::_displaysMapLock;

    Display::Display(const string& name)
        : _displayName(name)
        , _adminLock()
        , _refCount(0)
        , _virtualinput(nullptr)
        , _surfaces()
        , _compositorServerRPCConnection()
        , _remoteDisplay(nullptr)
        , _nativeDisplay()
    {
        TRACE(Trace::Information, (_T("Constructing Display build @ %s"), __TIMESTAMP__));

        std::vector<std::string> nodes;

        GetDRMNodes(DRM_NODE_RENDER, nodes); // /dev/dri/Renderer128

        for (const auto& node : nodes) {
            TRACE(Trace::Information, (_T("Found render node %s"), node.c_str()));
        }

        int& fd = _nativeDisplay.fd;
        struct gbm_device*& device = _nativeDisplay.device;

        fd = 0;
        device = nullptr;

        for (const auto& node : nodes) {
            fd = open(node.c_str(), O_RDWR);

            if (fd) {
                device = gbm_create_device(fd);

                if (device) {
                    TRACE(Trace::Information, (_T ( "Opened RenderDevice: %s"), node.c_str()));
                    break;
                } else {
                    TRACE(Trace::Information, (_T ( "Failed to create GBM device using node %s" ), node.c_str()));
                    close(fd);
                    fd = 0;
                }
            }
        }

        ASSERT((device != nullptr) && (fd != 0));
    }

    Display::~Display()
    {
        int& fd = _nativeDisplay.fd;
        struct gbm_device*& device = _nativeDisplay.device;

        if (device != nullptr) {
            gbm_device_destroy(device);
        }

        if (fd != 0) {
            close(fd);
        }

        device = nullptr;
        fd = 0;

        Deinitialize();
    }

    int Display::Process(const uint32_t data VARIABLE_IS_NOT_USED)
    {
        static uint8_t rate = RefreshRateFromResolution((_remoteDisplay != nullptr) ? _remoteDisplay->Resolution() : Exchange::IComposition::ScreenResolution::ScreenResolution_Unknown);

        Core::Time next_frame = Core::Time::Now().Add(Core::Time::MilliSecondsPerSecond / rate);

        for (auto begin = _surfaces.begin(), it = begin, end = _surfaces.end(); it != end; it++) {
            (*it)->Process(); // render
        }

        // uint32_t delay((next_frame - Core::Time::Now()).MilliSeconds());

        return Core::ERROR_NONE;
    }

    int Display::FileDescriptor() const
    {
        TRACE(Trace::Error, (_T("TODO!!!")));
        return 0;
    }

    Compositor::IDisplay::ISurface* Display::Create(const std::string& name, const uint32_t width, const uint32_t height)
    {
        uint32_t realHeight = height;
        uint32_t realWidth = width;

        if (_remoteDisplay != nullptr) {
            // Exchange::IComposition::ScreenResolution resolution = _remoteDisplay->Resolution();

            // Let the compositor choose if we go full screen or not.

            // realHeight = HeightFromResolution(resolution);
            // realWidth = WidthFromResolution(resolution);

            // if (realWidth != width || realHeight != height) {
            //     TRACE(Trace::Information, (_T ( "Requested surface dimensions (%d x %d) differ from true (real) display dimensions (%d x %d). Continuing with the latter!" ), width, height, realWidth, realHeight));
            // }
        } else {
            TRACE(Trace::Information, (_T ( "No remote display exist. Unable to query its dimensions. Expect the unexpected!")));
        }

        Core::ProxyType<SurfaceImplementation> retval = (Core::ProxyType<SurfaceImplementation>::Create(*this, name, realWidth, realHeight));

        Compositor::IDisplay::ISurface* result = &(*retval);
        result->AddRef();

        return result;
    }

    /* static */ void Display::Publish(InputFunction& action)
    {
        if (action != nullptr) {
            _displaysMapLock.Lock();

            for (std::pair<const string, Display*>& entry : _displays) {
                entry.second->_adminLock.Lock();

                std::for_each(begin(entry.second->_surfaces), end(entry.second->_surfaces), action);

                entry.second->_adminLock.Unlock();
            }

            _displaysMapLock.Unlock();
        }
    }

    /* static */ void Display::VirtualKeyboardCallback(keyactiontype type, unsigned int code)
    {
        if (type != KEY_COMPLETED) {
            time_t timestamp = time(nullptr);
            const IDisplay::IKeyboard::state state = ((type == KEY_RELEASED) ? IDisplay::IKeyboard::released
                                                                             : ((type == KEY_REPEAT) ? IDisplay::IKeyboard::repeated
                                                                                                     : IDisplay::IKeyboard::pressed));

            InputFunction action = [=](SurfaceImplementation* s) {
                s->SendKey(code, state, timestamp);
            };

            Publish(action);
        }
    }

    /* static */ void Display::VirtualMouseCallback(mouseactiontype type, unsigned short button, signed short horizontal, signed short vertical)
    {
        static int32_t pointer_x = 0;
        static int32_t pointer_y = 0;

        time_t timestamp = time(nullptr);
        InputFunction action;
        pointer_x = pointer_x + horizontal;
        pointer_y = pointer_y + vertical;

        switch (type) {
        case MOUSE_MOTION:
            action = [=](SurfaceImplementation* s) {
                int32_t X = std::min(std::max(0, pointer_x), s->Width());
                int32_t Y = std::min(std::max(0, pointer_y), s->Height());
                s->SendPointerPosition(X, Y, timestamp);
            };
            break;
        case MOUSE_SCROLL:
            action = [=](SurfaceImplementation* s) {
                s->SendWheelMotion(horizontal, vertical, timestamp);
            };
            break;
        case MOUSE_RELEASED:
        case MOUSE_PRESSED:
            action = [=](SurfaceImplementation* s) {
                s->SendPointerButton(button, type == MOUSE_RELEASED ? IDisplay::IPointer::released : IDisplay::IPointer::pressed, timestamp);
            };
            break;
        default:
            assert(false);
        }

        Publish(action);
    }

    /* static */ void Display::VirtualTouchScreenCallback(touchactiontype type, unsigned short index, unsigned short x, unsigned short y)
    {
        static uint16_t touch_x = ~0;
        static uint16_t touch_y = ~0;
        static touchactiontype last_type = TOUCH_RELEASED;

        // Get touch position in pixels
        // Reduce IPC traffic. The physical touch coordinates might be different, but when scaled to screen position, they may be same as previous.
        if ((x != touch_x)
            || (y != touch_y)
            || (type != last_type)) {

            last_type = type;
            touch_x = x;
            touch_y = y;

            time_t timestamp = time(nullptr);
            const IDisplay::ITouchPanel::state state = ((type == TOUCH_RELEASED) ? ITouchPanel::released
                                                                                 : ((type == TOUCH_PRESSED) ? ITouchPanel::pressed
                                                                                                            : ITouchPanel::motion));

            InputFunction action = [=](SurfaceImplementation* s) {
                const uint16_t mapped_x = (s->Width() * x) >> 16;
                const uint16_t mapped_y = (s->Height() * y) >> 16;
                s->SendTouch(index, state, mapped_x, mapped_y, timestamp);
            };

            Publish(action);
        }
    }

} // namespace Linux

Compositor::IDisplay* Compositor::IDisplay::Instance(const string& displayName)
{
    return (&(Linux::Display::Instance(displayName)));
}
} // namespace WPEFramework
