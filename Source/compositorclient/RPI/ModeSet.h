#pragma once

#include <cstdint>
#include <limits>
#include <ctime>
#include <drm/drm_fourcc.h>
 
class ModeSet
{
    public:
        struct ICallback {
            virtual ~ICallback() = default;
            virtual void PageFlip(unsigned int frame, unsigned int sec, unsigned int usec) = 0;
            virtual void VBlank(unsigned int frame, unsigned int sec, unsigned int usec) = 0;
        };
    using gbm_bo_t      = struct gbm_bo*;
    using gbm_surface_t = struct gbm_surface*;
    using gbm_device_t  = struct gbm_device*;
 
    public:
        ModeSet(const ModeSet&) = delete;
        ModeSet& operator= (const ModeSet&) = delete;

        ModeSet();
        ~ModeSet();

    public:
        const struct gbm_device* UnderlyingHandle() const
        {
            return _device;
        }
        static constexpr uint32_t SupportedBufferType()
        {
            static_assert(sizeof(uint32_t) >= sizeof(DRM_FORMAT_XRGB8888));
            static_assert(std::numeric_limits<decltype(DRM_FORMAT_XRGB8888)>::min() >= std::numeric_limits<uint32_t>::min());
            static_assert(std::numeric_limits<decltype(DRM_FORMAT_XRGB8888)>::max() <= std::numeric_limits<uint32_t>::max());

            // DRM_FORMAT_ARGB8888 and DRM_FORMAT_XRGB888 should be considered equivalent / interchangeable
            return static_cast<uint32_t>(DRM_FORMAT_XRGB8888);
        }
        static constexpr uint8_t BPP()
        {
            // See SupportedBufferType(), total number of bits representing all channels
            return 32;
        }
        static constexpr uint8_t ColorDepth()
        {
            // See SupportedBufferType(), total number of bits representing the R, G, B channels
            return 24;
        }

        static constexpr gbm_device_t gbm_device_t_DEFAULT () {
            return nullptr;
        }

        static constexpr gbm_surface_t gbm_surface_t_DEFAULT () {
            return nullptr;
        }

        static constexpr gbm_bo_t gbm_bo_t_DEFAULT () {
            return nullptr;
        }

        static constexpr uint32_t MinimumBufferCount () {
            return 1;
        }

        // TODO match with the window system, numbber of buffers
        static constexpr uint32_t MaximumBufferCount () {
            return 2;
        }

        // Seconds
        static constexpr time_t FrameDuration () {
            return 1;
        }

        int Descriptor () const {
            return (_fd);
        }

        uint32_t Width() const;
        uint32_t Height() const;
        struct gbm_surface* CreateRenderTarget(const uint32_t width, const uint32_t height);
        void DestroyRenderTarget(struct gbm_surface* surface);
        bool ScanOutRenderTarget (struct gbm_surface* surface);

    private:
        void Create();
        void Destruct();

    private:
        uint32_t _crtc;
        uint32_t _encoder;
        uint32_t _connector;

        uint32_t _fb;
        uint32_t _mode;

        struct gbm_device* _device;
        struct gbm_bo* _buffer;
        int _fd;
};
