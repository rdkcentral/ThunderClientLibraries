#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <drm/drm_fourcc.h>
#ifdef __cplusplus
}
#endif

#include <cstdint>
#include <vector>
#include <string>
#include <limits>
 
class RenderDevice {
    public :

        RenderDevice() = delete;
        ~RenderDevice() = delete;

        RenderDevice (RenderDevice const &) = delete;

    public :

        static constexpr uint32_t SupportedBufferType () {
            static_assert (sizeof(uint32_t) >= sizeof (DRM_FORMAT_XRGB8888));
            static_assert (std::numeric_limits < decltype (DRM_FORMAT_XRGB8888) >::min () >= std::numeric_limits < uint32_t >::min ());
            static_assert (std::numeric_limits < decltype (DRM_FORMAT_XRGB8888) >::max () <= std::numeric_limits < uint32_t >::max ());

            // DRM_FORMAT_ARGB8888 and DRM_FORMAT_XRGB888 should be considered equivalent / interchangeable
            return static_cast < uint32_t > (DRM_FORMAT_XRGB8888);
        }

        static constexpr uint8_t BPP () {
            // See SupportedBufferType(), total number of bits representing all channels
            return 32;
        }

        static constexpr uint8_t ColorDepth () {
            // See SupportedBufferType(), total number of bits representing the R, G, B channels
            return 24;
        }

        static void GetNodes (uint32_t type, std::vector < std::string > & list);
};
