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

class RenderDevice {
    template <class T>
    struct remove_pointer {
        typedef T type;
    };

    template <class T>
    struct remove_pointer <T*> {
        typedef T type;
    };

    template <class FROM, class TO, bool ENABLE>
    struct _narrowing {
        static_assert (( std::is_arithmetic < FROM > :: value && std::is_arithmetic < TO > :: value ) != false);

        // Not complete, assume zero is minimum for unsigned
        // Common type of signed and unsigned typically is unsigned
        using common_t = typename std::common_type < FROM, TO > :: type;
        static constexpr bool value =   ENABLE
                                        && (
                                            ( std::is_signed < FROM > :: value && std::is_unsigned < TO > :: value )
                                            || static_cast < common_t > ( std::numeric_limits < FROM >::max () ) >= static_cast < common_t > ( std::numeric_limits < TO >::max () )
                                        )
                                        ;
    };

    public :

        class DRM {
            public:
                using fd_t = int;

                using fb_id_t = remove_pointer < decltype (drmModeFB::fb_id) >::type;
                using crtc_id_t = remove_pointer < decltype (drmModeCrtc::crtc_id) >::type;
                using enc_id_t = remove_pointer < decltype (drmModeEncoder::encoder_id) >::type;
                using conn_id_t = remove_pointer < decltype (drmModeConnector::connector_id) >::type;

                using width_t = remove_pointer < decltype (drmModeFB2::width) >::type;
                using height_t = remove_pointer < decltype (drmModeFB2::height) >::type;
                using frmt_t = remove_pointer < decltype (drmModeFB2::pixel_format) >::type;
                using modifier_t = remove_pointer < decltype (drmModeFB2::modifier) >::type;

                using handle_t = remove_pointer < std::decay < decltype (drmModeFB2::handles) >::type >::type;
                using pitch_t = remove_pointer < std::decay < decltype (drmModeFB2::pitches) >::type >::type;
                using offset_t = remove_pointer < std::decay < decltype (drmModeFB2::offsets) >::type >::type;

                using x_t = remove_pointer < decltype (drmModeCrtc::x) >::type;
                using y_t = remove_pointer < decltype (drmModeCrtc::y) >::type;

                using duration_t = remove_pointer < decltype (timespec::tv_sec) >::type;

                static constexpr fd_t InvalidFd () { return -1; }
                static constexpr fb_id_t  InvalidFb () { return  0; }
                static constexpr crtc_id_t InvalidCrtc () { return 0; }
                static constexpr enc_id_t InvalidEncoder () { return 0; }
                static constexpr conn_id_t InvalidConnector () { return 0; }

                static constexpr width_t InvalidWidth () { return 0; }
                static constexpr height_t InvalidHeight () { return 0; }
                static constexpr frmt_t InvalidFrmt () { return DRM_FORMAT_INVALID; }
                static constexpr modifier_t InvalidModifier () { return DRM_FORMAT_MOD_INVALID; }
        };

        class GBM {
            public :
                using width_t = remove_pointer < decltype (gbm_import_fd_modifier_data::width) >::type;
                using height_t = remove_pointer < decltype (gbm_import_fd_modifier_data::height) >::type;
                using frmt_t = remove_pointer < decltype (gbm_import_fd_modifier_data::format) >::type;

                // See xf86drmMode.h for magic constant
                static_assert (GBM_MAX_PLANES == 4);

                using fd_t = remove_pointer < std::decay < decltype (gbm_import_fd_modifier_data::fds) > :: type >::type;
                using stride_t = remove_pointer < std::decay < decltype (gbm_import_fd_modifier_data::strides) > :: type >::type;
                using offset_t = remove_pointer < std::decay < decltype (gbm_import_fd_modifier_data::offsets) > :: type >::type;
                using modifier_t = remove_pointer < decltype (gbm_import_fd_modifier_data::modifier) >::type;

                using dev_t = struct gbm_device *;
                using surf_t = struct  gbm_surface *;
                using buf_t = struct gbm_bo *;

                using handle_t = decltype (gbm_bo_handle::u32);

                static constexpr dev_t InvalidDev () { return nullptr; }
                static constexpr surf_t InvalidSurf () { return nullptr; }
                static constexpr buf_t InvalidBuf () { return nullptr; }
                static constexpr fd_t InvalidFd () { return -1; }

                static constexpr width_t InvalidWidth () { return 0; }
                static constexpr height_t InvalidHeight () { return 0; }
                static constexpr stride_t InvalidStride () { return 0; }
                static constexpr frmt_t InvalidFrmt () { return DRM::InvalidFrmt (); }
                static constexpr modifier_t InvalidModifier () { return DRM::InvalidModifier (); }
        };

        RenderDevice() = delete;
        ~RenderDevice() = delete;

        RenderDevice (RenderDevice const &) = delete;

    public :

        static_assert (_narrowing < decltype (DRM_FORMAT_ARGB8888), GBM::frmt_t, true > :: value != false);
        static constexpr GBM::frmt_t SupportedBufferType() { return static_cast <GBM::frmt_t> (DRM_FORMAT_ARGB8888); }

        static_assert (_narrowing < decltype (DRM_FORMAT_MOD_LINEAR), GBM::modifier_t, true > :: value != false);
        static constexpr GBM::modifier_t FormatModifier () { return static_cast <GBM::modifier_t> (DRM_FORMAT_MOD_LINEAR); }

        static void GetNodes (uint32_t type, std::vector < std::string > & list);
};
