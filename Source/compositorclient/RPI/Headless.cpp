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

#include "Headless.h"

#include <vector>
#include <list>
#include <string>
#include <cassert>
#include <limits>
#include <mutex>

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <drm/drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <EGL/egl.h>

#ifdef __cplusplus
}
#endif


// User space, kernel space, and other places do not always agree

static_assert(!narrowing<decltype(drmModeFB::fb_id), uint32_t, true>::value);

static_assert(!narrowing<decltype(drmModeCrtc::crtc_id), uint32_t, true>::value);
static_assert(!narrowing<decltype(drmModeCrtc::x), uint32_t, true>::value);
static_assert(!narrowing<decltype(drmModeCrtc::y), uint32_t, true>::value);

static_assert(!narrowing<decltype(drmModeEncoder::encoder_id), uint32_t, true>::value);

static_assert(!narrowing<decltype(drmModeConnector::connector_id), uint32_t, true>::value);

static_assert(!narrowing<decltype(drmModeFB2::fb_id), uint32_t, true>::value);
static_assert(!narrowing<decltype(drmModeFB2::width), uint32_t, true>::value);
static_assert(!narrowing<decltype(drmModeFB2::height), uint32_t, true>::value);
static_assert(!narrowing<decltype(drmModeFB2::pixel_format), uint32_t, true>::value);
static_assert(!narrowing<decltype(drmModeFB2::modifier), uint64_t, true>::value);
static_assert(!narrowing<remove_pointer<std::decay<decltype(drmModeFB2::handles)>::type>::type, uint32_t, true>::value);
static_assert(!narrowing<remove_pointer<std::decay<decltype(drmModeFB2::pitches)>::type>::type, uint32_t, true>::value);
static_assert(!narrowing<remove_pointer<std::decay<decltype(drmModeFB2::offsets)>::type>::type, uint32_t, true>::value);

static_assert(!narrowing<decltype(drmModeFB2::width), uint32_t, true>::value);

static_assert(!narrowing<decltype(gbm_import_fd_modifier_data::width), uint32_t, true>::value);
static_assert(!narrowing<decltype(gbm_import_fd_modifier_data::height), uint32_t, true>::value);
static_assert(!narrowing<decltype(gbm_import_fd_modifier_data::format), uint32_t, true>::value);

static_assert(!narrowing<remove_pointer<std::decay<decltype(gbm_import_fd_modifier_data::fds)>::type>::type, int, true>::value);
static_assert(!narrowing<remove_pointer<std::decay<decltype(gbm_import_fd_modifier_data::strides)>::type>::type, int, true>::value);
static_assert(!narrowing<remove_pointer<std::decay<decltype(gbm_import_fd_modifier_data::offsets)>::type>::type, int, true>::value);
static_assert(!narrowing<decltype(gbm_import_fd_modifier_data::modifier), uint64_t, true>::value);

static_assert(   std::is_integral<decltype(GBM_MAX_PLANES)>::value
              && GBM_MAX_PLANES == 4
             );

/* static */ void RenderDevice::GetNodes(uint32_t type, std::vector<std::string>& list)
{
    // Just an arbitrary choice
    constexpr int8_t DrmMaxDevices = 16;

    drmDevicePtr devices[DrmMaxDevices];

    int count = drmGetDevices2(0 /* flags */, &devices[0] , static_cast<int>(DrmMaxDevices));

    if (count > 0) {
        for (int i = 0; i < count; ++i) {
            static_assert(   !narrowing<decltype(DRM_NODE_PRIMARY), uint32_t, true>::value
                          || (   DRM_NODE_PRIMARY >= 0
                              && in_unsigned_range <uint32_t, DRM_NODE_PRIMARY>::value
                             )
                         );
            static_assert(   !narrowing<decltype(DRM_NODE_CONTROL), uint32_t, true>::value
                          || (   DRM_NODE_CONTROL >= 0
                              && in_unsigned_range <uint32_t, DRM_NODE_CONTROL>::value
                             )
                         );
            static_assert(   !narrowing<decltype(DRM_NODE_RENDER), uint32_t, true>::value
                          || (   DRM_NODE_RENDER >= 0
                              && in_unsigned_range <uint32_t, DRM_NODE_RENDER>::value
                             )
                         );

            switch (type) {
                case DRM_NODE_PRIMARY   :   // card<num>, always created, KMS, privileged
                                            __attribute__((fallthrough));
                case DRM_NODE_CONTROL   :   // ControlD<num>, currently unused
                                            __attribute__((fallthrough));
                case DRM_NODE_RENDER    :   // Solely for render clients, unprivileged
                                            {
                                                if ((1 << type) == (devices[i]->available_nodes & (1 << type))) {
                                                    list.push_back(std::string(devices[i]->nodes[type]));
                                                }
                                                break;
                                            }
                default                 :   // Unknown (new) node type
                                        ;
            }

        }

        drmFreeDevices(&devices[0], count);
    }
}
