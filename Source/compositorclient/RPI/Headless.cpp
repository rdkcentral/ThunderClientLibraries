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


/* static */ void RenderDevice::GetNodes (uint32_t type, std::vector <std::string > & list) {
    // Just an arbitrary choice
    /* static */ constexpr uint8_t DrmMaxDevices = 16;

    drmDevicePtr devices [ DrmMaxDevices ];

    static_assert ( sizeof ( DrmMaxDevices ) <= sizeof ( int ) );
    static_assert ( std::numeric_limits < decltype ( DrmMaxDevices ) > :: max () <= std::numeric_limits < int > :: max () );

    int device_count = drmGetDevices2 ( 0 /* flags */ , & devices [ 0 ] , static_cast < int > ( DrmMaxDevices ) );

    if ( device_count > 0 ) {
        for ( decltype ( device_count ) i = 0 ; i < device_count ; i++ ) {
            switch ( type ) {
                case DRM_NODE_PRIMARY   :   // card<num>, always created, KMS, privileged
                case DRM_NODE_CONTROL   :   // ControlD<num>, currently unused
                case DRM_NODE_RENDER    :   // Solely for render clients, unprivileged
                                            {
                                                if ( ( 1 << type ) == ( devices [ i ] -> available_nodes & ( 1 << type ) ) ) {
                                                    list.push_back ( std::string ( devices [ i ] -> nodes [ type ] ) );
                                                }
                                                break;
                                            }
                case DRM_NODE_MAX       :
                default                 :   // Unknown (new) node type
                                        ;
            }

        }

        drmFreeDevices ( & devices [ 0 ] , device_count );
    }
}
