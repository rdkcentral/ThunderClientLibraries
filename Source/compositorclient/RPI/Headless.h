/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#ifdef __cplusplus
extern "C" {
#endif

#include <gbm.h>
#include <drm/drm_fourcc.h>
#include <xf86drmMode.h>

#ifdef __cplusplus
}
#endif

#include <cstdint>
#include <vector>
#include <string>
#include <limits>
#include <list>

#include "include/traits.h"

class RenderDevice {
public :

    class DRM {
    public:

        DRM() = delete;
        DRM(const DRM&) = delete;
        DRM& operator=(const DRM &) = delete;

        static constexpr int InvalidFiledescriptor() { return -1; }

        static constexpr uint32_t InvalidFramebuffer() { return  0; }
        static constexpr uint32_t InvalidScreen() { return 0; }
        static constexpr uint32_t InvalidEncoder() { return 0; }
        static constexpr uint32_t InvalidConnector() { return 0; }

        static constexpr uint32_t InvalidWidth() { return 0; }
        static constexpr uint32_t InvalidHeight() { return 0; }

        static_assert(   !narrowing<decltype(DRM_FORMAT_INVALID), uint32_t, true>::value
                      || (   DRM_FORMAT_INVALID >= 0
                          && in_unsigned_range<uint32_t, DRM_FORMAT_INVALID>::value
                         )
                     );
        static constexpr uint32_t InvalidFormat() { return static_cast<uint32_t>(DRM_FORMAT_INVALID); }

        static_assert(   !narrowing<decltype(DRM_FORMAT_MOD_INVALID), uint64_t, true>::value
                      || (   DRM_FORMAT_MOD_INVALID >= 0
                          && in_unsigned_range<uint64_t, DRM_FORMAT_MOD_INVALID>::value
                         )
                     );
        static constexpr uint64_t InvalidModifier() { return static_cast<uint64_t>(DRM_FORMAT_MOD_INVALID); }
    };

    class GBM {
    public :

        GBM() = delete;
        GBM(const GBM&) = delete;
        GBM& operator=(const GBM&) = delete;

        static constexpr struct gbm_device* InvalidDevice() { return nullptr; }
        static constexpr struct gbm_surface* InvalidSurface() { return  nullptr; }
        static constexpr struct gbm_bo* InvalidBuffer() { return nullptr; }

        static constexpr int InvalidFiledescriptor() { return -1; }

        static constexpr uint32_t InvalidWidth() { return 0; }
        static constexpr uint32_t InvalidHeight() { return 0; }
        static constexpr uint32_t InvalidStride() { return 0; }

        static constexpr uint32_t InvalidFormat() { return DRM::InvalidFormat(); }
        static constexpr uint64_t InvalidModifier() { return DRM::InvalidModifier(); }
    };

    RenderDevice() = delete;
    ~RenderDevice() = delete;

    RenderDevice(const RenderDevice&) = delete;
    RenderDevice& operator=(const RenderDevice&) = delete;

    static_assert(   !narrowing<decltype(DRM_FORMAT_ARGB8888), uint32_t, true>::value
                  || (    DRM_FORMAT_ARGB8888 >= 0
                      && in_unsigned_range<uint32_t, DRM_FORMAT_ARGB8888>::value
                     )
                 );
    static constexpr uint32_t SupportedBufferType() { return static_cast<uint32_t>(DRM_FORMAT_ARGB8888); }

    static_assert(   !narrowing<decltype(DRM_FORMAT_MOD_LINEAR), uint64_t, true>::value
                  || (   DRM_FORMAT_MOD_LINEAR >= 0
                      && in_unsigned_range<uint64_t, DRM_FORMAT_MOD_LINEAR>::value
                     )
                 );
    static constexpr uint64_t FormatModifier() { return static_cast<uint64_t>(DRM_FORMAT_MOD_LINEAR); }

    static void GetNodes(uint32_t, std::vector<std::string>&);
};
