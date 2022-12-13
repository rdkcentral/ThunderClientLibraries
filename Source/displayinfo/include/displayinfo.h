/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#include <stdbool.h>
#include <stdint.h>

#undef EXTERNAL
#ifdef _MSVC_LANG
#ifdef CLIENTLIBRARIES_DISPLAYINFO_EXPORTS
#define EXTERNAL __declspec(dllexport)
#else
#define EXTERNAL __declspec(dllimport)
#pragma comment(lib, "displayinfo.lib")
#endif
#else
#define EXTERNAL __attribute__((visibility("default")))
#endif

#define DISPLAYINFO_VERSION 2

#ifdef __cplusplus
extern "C" {
#endif
struct displayinfo_type;

typedef enum displayinfo_edid_eot_type {
    DISPLAYINFO_EDID_EOT_UNDEFINED = 0,
    DISPLAYINFO_EDID_EOT_TRADITIONAL_GAMMA_SDR = (1 << 0),
    DISPLAYINFO_EDID_EOT_TRADITIONAL_GAMMA_HDR = (1 << 1),
    DISPLAYINFO_EDID_EOT_SMPTE_ST_2084 = (1 << 2),
    DISPLAYINFO_EDID_EOT_HYBRID_LOG_GAMMA_ITU_R_BT_2100 = (1 << 3)
} displayinfo_edid_eot_t;

typedef enum displayinfo_edid_hdr_static_metatype_type {
    DISPLAYINFO_EDID_HDR_STATIC_METATTYPE_UNDEFINED = 0,
    DISPLAYINFO_EDID_HDR_STATIC_METATAYPE_TYPE0 = (1 << 0)
} displayinfo_edid_hdr_static_metatype_t;

typedef uint8_t displayinfo_edid_eot_map_t;
typedef uint8_t displayinfo_edid_hdr_static_metatype_map_t;

typedef struct displayinfo_edid_hdr_static_metadata_type {
    displayinfo_edid_eot_map_t eot;
    displayinfo_edid_hdr_static_metatype_map_t type;
    struct luminance {
        uint8_t max_cv;
        uint8_t average_cv;
        uint8_t min_cv;
    } luminance;
} displayinfo_edid_hdr_static_metadata_t;

typedef enum displayinfo_edid_hdr_dynamic_flag_type {
    DISPLAYINFO_EDID_HDR_DYNAMIC_FLAG_TYPE_UNDEFINED = 0,
    DISPLAYINFO_EDID_HDR_DYNAMIC_FLAG_TYPE_1_HDR_METADATA_VERSION = 1,
    DISPLAYINFO_EDID_HDR_DYNAMIC_FLAG_TYPE_TS_103__433_SPEC_VERSION = 2,
    DISPLAYINFO_EDID_HDR_DYNAMIC_FLAG_TYPE_3_HDR_METADATA_VERSION = 3,
    DISPLAYINFO_EDID_HDR_DYNAMIC_FLAG_TYPE_4_HDR_METADATA_VERSION = 4
} displayinfo_edid_hdr_dynamic_flag_t;

typedef struct displayinfo_edid_hdr_dynamic_metatype_type {
    displayinfo_edid_hdr_dynamic_flag_t type;
    union {
        uint8_t* type_1_hdr_metadata_version;
        uint8_t* ts_103_433_spec_version;
        uint8_t* type_4_hdr_metadata_version;
    };
    union {
        uint8_t** fields;
    };
} displayinfo_edid_hdr_dynamic_metatype_t;

typedef struct displayinfo_edid_hdr_dynamic_metadata_type {
    displayinfo_edid_hdr_dynamic_metatype_t * type;
    uint8_t count;
} displayinfo_edid_hdr_dynamic_metadata_t;

typedef enum displayinfo_edid_hdr_licensor {
    DISPLAYINFO_EDID_HDR_LICENSOR_NONE = 0,
    DISPLAYINFO_EDID_HDR_LICENSOR_UNKNOWN = (1 << 0),
    DISPLAYINFO_EDID_HDR_LICENSOR_HDMI_LICENSING_LLC = (1 << 1),
    DISPLAYINFO_EDID_HDR_LICENSOR_HDMI_FORUM = (1 << 2),
    DISPLAYINFO_EDID_HDR_LICENSOR_HDR10PLUS_LLC = (1 << 3),
    DISPLAYINFO_EDID_HDR_LICENSOR_DOLBY_LABORATORIES_INC = (1 << 4),
    DISPLAYINFO_EDID_HDR_LICENSOR_TECHNICOLOR = (1 << 5)
} displayinfo_edid_hdr_licensor_t;

typedef uint8_t displayinfo_edid_hdr_licensor_map_t;

typedef enum displayinfo_hdr_type {
    DISPLAYINFO_HDR_OFF = 0,
    DISPLAYINFO_HDR_UNKNOWN = (1 << 0),
    DISPLAYINFO_HDR_10 = (1 << 1),
    DISPLAYINFO_HDR_10PLUS = (1 << 2),
    DISPLAYINFO_HDR_DOLBYVISION = (1 << 3),
    DISPLAYINFO_HDR_TECHNICOLOR = (1 << 4),
    DISPLAYINFO_HDR_HLG = (1 << 5),
    // pseudo standards
    DISPLAYINFO_HDR_400 = (1 << 8),
    DISPLAYINFO_HDR_500 = (1 << 9),
    DISPLAYINFO_HDR_600 = (1 << 10),
    DISPLAYINFO_HDR_1000 = (1 << 11),
    DISPLAYINFO_HDR_1400 = (1 << 12),
    DISPLAYINFO_HDR_TB_400 = (1 << 13),
    DISPLAYINFO_HDR_TB_500 = (1 << 14),
    DISPLAYINFO_HDR_TB_600 = (1 << 15),
    // VESA standards
    DISPLAYINFO_DISPLAYHDR_400 = (1 << 17),
    // Multitypes
    // Flag indicating displayinfo_hdr_vesa_t available in displayinfo_edid_hdr_multitype
    DISPLAYINFO_HDR_VESATYPE = (1 << 28),
    // Flag indicating displayinfo_hdr_pseudo_t available in displayinfo_edid_hdr_multitype
    DISPLAYINFO_HDR_PSEUDOTYPE = (1 << 29),
    // Flag indicating displayinfo_hdr_common_t available in displayinfo_edid_hdr_multitype
    DISPLAYINFO_HDR_COMMONTYPE = (1 << 30),
    // Flag indicating displayinfo_hdr_t compatibility mode in displayfino_edid_hdr_multitype
    DISPLAYINFO_HDR_MULTITYPE = (1 << 31)
} displayinfo_hdr_t;

typedef enum displayinfo_hdr_common_type {
    DISPLAYINFO_HDR_COMMON_OFF = 0,
    DISPLAYINFO_HDR_COMMON_UNKNOWN = (1 << 0),
    DISPLAYINFO_HDR_COMMON_10 = (1 << 1),
    DISPLAYINFO_HDR_COMMON_10PLUS = (1 << 2),
    DISPLAYINFO_HDR_COMMON_DOLBYVISION = (1 << 3),
    DISPLAYINFO_HDR_COMMON_TECHNICOLOR = (1 << 4),
    DISPLAYINFO_HDR_COMMON_HLG = (1 << 5),
//    DISPLAYINFO_HDR_COMMON_MAX = (1 << 6)
} displayinfo_hdr_common_t;

typedef enum displayinfo_hdr_pseudo_type {
    DISPLAYINFO_HDR_PSEUDO_OFF = 0,
    DISPLAYINFO_HDR_PSEUDO_400 = (1 << 0),
    DISPLAYINFO_HDR_PSEUDO_500 = (1 << 1),
    DISPLAYINFO_HDR_PSEUDO_600 = (1 << 2),
    DISPLAYINFO_HDR_PSEUDO_1000 = (1 << 3),
    DISPLAYINFO_HDR_PSEUDO_1400 = (1 << 4),
    DISPLAYINFO_HDR_PSEUDO_TB_400 = (1 << 5),
    DISPLAYINFO_HDR_PSEUDO_TB_500 = (1 << 6),
    DISPLAYINFO_HDR_PSEUDO_TB_600 = (1 << 7),
//    DISPLAYINFO_HDR_PSEUDO_PSEUDO_MAX = (1 << 8)
} displayinfo_hdr_pseudo_t;

typedef enum display_hdr_vesa_type {
    DISPLAYINFO_HDR_VESA_OFF = 0,
    DISPLAYINFO_HDR_VESA_DISPLAYHDR_400 = (1 << 0),
//    DISPLAYINFO_HDR_VESA_MAX = (1 << 1)
} displayinfo_hdr_vesa_t;

// At least capable of holding a pointer: 32 bits on 32 bit systems, 64 bits on 64 bits systems
typedef uintptr_t displayinfo_edid_hdr_type_map_t;

typedef union {
    // Legacy mode, eg, without multitype flags set
    displayinfo_edid_hdr_type_map_t legacy;
    struct {
        // Multitype flag/flags set, then lower 8 bits for version
        displayinfo_edid_hdr_type_map_t version;
        // Number of bytes to represent enum bitmasks
        uint8_t count;
        // Version 1.0
        uint8_t* common;
        uint8_t* pseudo;
        uint8_t* vesa;
    };
} displayinfo_edid_hdr_multitype_map_t;

typedef displayinfo_edid_hdr_multitype_map_t* displayinfo_edid_hdr_multitype_map_ptr_t;

typedef enum displayinfo_hdcp_protection_type {
    DISPLAYINFO_HDCP_UNENCRYPTED,
    DISPLAYINFO_HDCP_1X,
    DISPLAYINFO_HDCP_2X,
    DISPLAYINFO_HDCP_UNKNOWN
} displayinfo_hdcp_protection_t;

typedef enum displayinfo_edid_video_interface_type {
    DISPLAYINFO_EDID_VIDEO_INTERFACE_UNDEFINED = 0,
    DISPLAYINFO_EDID_VIDEO_INTERFACE_HDMI_A = 2,
    DISPLAYINFO_EDID_VIDEO_INTERFACE_HDMI_B = 3,
    DISPLAYINFO_EDID_VIDEO_INTERFACE_MDDI = 4,
    DISPLAYINFO_EDID_VIDEO_INTERFACE_DISPLAYPORT = 5
} displayinfo_edid_video_interface_t;

typedef uint8_t displayinfo_edid_color_depth_map_t;
typedef enum displayinfo_edid_color_depth_type  {
    DISPLAYINFO_EDID_COLOR_DEPTH_UNDEFINED = 0,
    DISPLAYINFO_EDID_COLOR_DEPTH_6_BPC = (1 << 0),
    DISPLAYINFO_EDID_COLOR_DEPTH_8_BPC = (1 << 1),
    DISPLAYINFO_EDID_COLOR_DEPTH_10_BPC = (1 << 2),
    DISPLAYINFO_EDID_COLOR_DEPTH_12_BPC = (1 << 3),
    DISPLAYINFO_EDID_COLOR_DEPTH_14_BPC = (1 << 4),
    DISPLAYINFO_EDID_COLOR_DEPTH_16_BPC = (1 << 5),
} displayinfo_edid_color_depth_t;

typedef enum displayinfo_edid_color_depth_index_type {
    DISPLAYINFO_EDID_COLOR_DEPTH_INDEX_RGB,
    DISPLAYINFO_EDID_COLOR_DEPTH_INDEX_YCBCR444,
    DISPLAYINFO_EDID_COLOR_DEPTH_INDEX_YCBCR422,
    DISPLAYINFO_EDID_COLOR_DEPTH_INDEX_YCBCR420,
    DISPLAYINFO_EDID_COLOR_DEPTH_INDEX_LAST
} displayinfo_edid_color_depth_index_t;

typedef uint8_t displayinfo_edid_color_format_map_t;
typedef enum displayinfo_edid_color_format_type {
    DISPLAYINFO_EDID_COLOR_FORMAT_UNDEFINED = 0,
    DISPLAYINFO_EDID_COLOR_FORMAT_RGB = (1 << 0), // RGB444
    DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR444 = (1 << 1),
    DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR422 = (1 << 2),
    DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR420 = (1 << 3),

    DISPLAYINFO_EDID_DISPLAY_UNDEFINED = DISPLAYINFO_EDID_COLOR_FORMAT_UNDEFINED,
    DISPLAYINFO_EDID_DISPLAY_RGB = DISPLAYINFO_EDID_COLOR_FORMAT_RGB,
    DISPLAYINFO_EDID_DISPLAY_MONOCHROME_GRAYSCALE = (1 << 4),
    DISPLAYINFO_EDID_DISPLAY_NONE_RGB = (1 << 5)
} displayinfo_edid_color_format_t;

typedef struct displayinfo_edid_rgb_colorspace_coordinates {
    uint16_t Rx;
    uint16_t Ry;
    uint16_t Gx;
    uint16_t Gy;
    uint16_t Bx;
    uint16_t By;
    uint16_t Wx;
    uint16_t Wy;
} displayinfo_edid_rb_colorspace_coordinates_t;

typedef uint16_t displayinfo_edid_color_space_map_t;
typedef enum displayinfo_edid_color_space_type {
    DISPLAYINFO_EDID_COLOR_SPACE_UNDEFINED = 0,
    DISPLAYINFO_EDID_COLOR_SPACE_SRGB = (1 << 0),
    DISPLAYINFO_EDID_COLOR_SPACE_XVYCC_601 = (1 << 1),
    DISPLAYINFO_EDID_COLOR_SPACE_XVYCC_709 = (1 << 2),
    DISPLAYINFO_EDID_COLOR_SPACE_SYCC_601 = (1 << 3),
    DISPLAYINFO_EDID_COLOR_SPACE_OP_YCC_601 = (1 << 4), /* or ADOBE_YCC_601 */
    DISPLAYINFO_EDID_COLOR_SPACE_OP_RGB = (1 << 5), /* or ADOBE_RGB */
    DISPLAYINFO_EDID_COLOR_SPACE_ITUR_BT_2020_CYCC = (1 << 6), /* ITU-R BT.2020 Yc Cbc Crc */
    DISPLAYINFO_EDID_COLOR_SPACE_ITUR_BT_2020_YCC = (1 << 7), /* ITU-R BT.2020 RGB or YCbCr */
    DISPLAYINFO_EDID_COLOR_SPACE_ITUR_BT_2020_RGB = (1 << 8),
    DISPLAYINFO_EDID_COLOR_SPACE_DCI_P3 = (1 << 9) /* Theater */,
    DISPLAYINFO_EDID_COLOR_SPACE_D65_P3 = (1 << 10) /* Display */
} displayinfo_edid_color_space_t;

typedef uint32_t displayinfo_edid_audio_format_map_t;
typedef enum displayinfo_edid_audio_format_type {
    DISPLAYINFO_EDID_AUDIO_FORMAT_UNDEFINED = 0,
    DISPLAYINFO_EDID_AUDIO_FORMAT_LPCM = (1 << 0),
    DISPLAYINFO_EDID_AUDIO_FORMAT_AC3 = (1 << 1),
    DISPLAYINFO_EDID_AUDIO_FORMAT_MPEG1 = (1 << 2),
    DISPLAYINFO_EDID_AUDIO_FORMAT_MP3 = (1 << 3),
    DISPLAYINFO_EDID_AUDIO_FORMAT_MPEG2 = (1 << 4),
    DISPLAYINFO_EDID_AUDIO_FORMAT_AAC_LC = (1 << 5),
    DISPLAYINFO_EDID_AUDIO_FORMAT_DTS = (1 << 6),
    DISPLAYINFO_EDID_AUDIO_FORMAT_ATRAC = (1 << 7),
    DISPLAYINFO_EDID_AUDIO_FORMAT_SUPER_AUDIO_CD = (1 << 8),
    DISPLAYINFO_EDID_AUDIO_FORMAT_EAC3 = (1 << 9),
    DISPLAYINFO_EDID_AUDIO_FORMAT_DTSHD = (1 << 10),
    DISPLAYINFO_EDID_AUDIO_FORMAT_DOLBY_TRUEHD = (1 << 11),
    DISPLAYINFO_EDID_AUDIO_FORMAT_DST_AUDIO = (1 << 12),
    DISPLAYINFO_EDID_AUDIO_FORMAT_MS_WMA_PRO = (1 << 13),
    DISPLAYINFO_EDID_AUDIO_FORMAT_MPEG4_HEAAC = (1 << 14),
    DISPLAYINFO_EDID_AUDIO_FORMAT_MPEG4_HEAAC_V2 = (1 << 15),
    DISPLAYINFO_EDID_AUDIO_FORMAT_MPEG4_ACC_LC = (1 << 16),
    DISPLAYINFO_EDID_AUDIO_FORMAT_DRA = (1 << 17),
    DISPLAYINFO_EDID_AUDIO_FORMAT_MPEG4_HEAAC_MPEG_SURROUND = (1 << 18),
    DISPLAYINFO_EDID_AUDIO_FORMAT_MPEG4_HEAAC_LC_MPEG_SURROUND = (1 << 19),
    DISPLAYINFO_EDID_AUDIO_FORMAT_MPEGH_3DAUDIO = (1 << 20),
    DISPLAYINFO_EDID_AUDIO_FORMAT_LPCM_3DAUDIO = (1 << 21),
    DISPLAYINFO_EDID_AUDIO_FORMAT_AC4 = (1 << 22),
    DISPLAYINFO_EDID_AUDIO_FORMAT_DOLBY_ATMOS = (1 << 23),
} displayinfo_edid_audio_format_t;

typedef enum displayinfo_edid_aspect_ratio_type {
    DISPLAYINFO_EDID_ASPECT_RATIO_UNDEFINED = 0,
    DISPLAYINFO_EDID_ASPECT_RATIO_4_TO_3,
    DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9,
    DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27,
    DISPLAYINFO_EDID_ASPECT_RATIO_256_TO_135,
} displayinfo_edid_aspect_ratio_t;

typedef enum displayinfo_edid_frequency_type {
    DISPLAYINFO_EDID_FREQUENCY_UNDEFINED = 0,
    DISPLAYINFO_EDID_FREQUENCY_23980_OR_24000 = (1 << 0),
    DISPLAYINFO_EDID_FREQUENCY_25000 = (1 << 1),
    DISPLAYINFO_EDID_FREQUENCY_29970_OR_30000 = (1 << 2),
    DISPLAYINFO_EDID_FREQUENCY_47960_OR_48000 = (1 << 3),
    DISPLAYINFO_EDID_FREQUENCY_50000 = (1 << 4),
    DISPLAYINFO_EDID_FREQUENCY_59826 = (1 << 5),
    DISPLAYINFO_EDID_FREQUENCY_59940 = (1 << 6),
    DISPLAYINFO_EDID_FREQUENCY_60000 = (1 << 7),
    DISPLAYINFO_EDID_FREQUENCY_100000 = (1 << 8),
    DISPLAYINFO_EDID_FREQUENCY_119880_OR_120000 = (1 << 9),
    DISPLAYINFO_EDID_FREQUENCY_200000 = (1 << 10),
    DISPLAYINFO_EDID_FREQUENCY_239760 = (1 << 11),
} displayinfo_edid_frequency_t;

typedef struct displayinfo_edid_base_info {
    uint8_t version;
    uint8_t revision;

    char manufacturer_id[3];
    uint16_t product_code;
    uint32_t serial_number;

    /* Week of manufacture or 0xFF = model year flag */
    uint8_t manufacture_week;

    /* Year of manufacture or year of model  */
    uint16_t manufacture_year;

    /* Digital input flag */
    bool digital;

     /* If Digital input flag is set then bits_per_color, video_interface and display_type apply */
    uint8_t bits_per_color;
    displayinfo_edid_video_interface_t video_interface;
    displayinfo_edid_color_format_map_t display_type;

    uint8_t width_in_centimeters;
    uint8_t height_in_centimeters;

    uint16_t preferred_width_in_pixels;
    uint16_t preferred_height_in_pixels;

} displayinfo_edid_base_info_t;

typedef uint8_t displayinfo_edid_vic_t;

typedef struct displayinfo_edid_standard_timing_type {
    displayinfo_edid_vic_t vic;
    displayinfo_edid_aspect_ratio_t display_aspect_ratio;
    displayinfo_edid_frequency_t vertical_frequency;
    uint16_t active_width;
    uint16_t active_height;
} displayinfo_edid_standard_timing_t;

typedef struct displayinfo_edid_cea_extension_info_type {
    uint8_t version;
    displayinfo_edid_audio_format_map_t audio_formats;
    displayinfo_edid_color_space_map_t color_spaces;
    displayinfo_edid_color_format_map_t color_formats;
    displayinfo_edid_color_depth_map_t color_depths[DISPLAYINFO_EDID_COLOR_DEPTH_INDEX_LAST];
    uint8_t number_of_timings;
    displayinfo_edid_vic_t timings[255-64];
} displayinfo_edid_cea_extension_info_t;


/**
* @brief Will be called if there are changes regarding operational state of the
*        instance - if it is not operational that means any function calls using it
*        will not succeed (will return Core::ERROR_UNAVAILABLE). Not operational state
*        can occur if the plugin inside WPEFramework has been deactivated.
*
*
* @param userData Pointer passed along when @ref displayinfo_register was issued.
* @param is_operational If instance is operational or not
*/
typedef void (*displayinfo_operational_state_change_cb)(bool is_operational, void* userdata);

/**
* @brief Will be called if there are changes regaring the display output, you need to query
*        yourself what exacally is changed
*
* @param userData Pointer passed along when \ref displayinfo_register was issued.
*/
typedef void (*displayinfo_display_output_change_cb)(void* userdata);

/**
 * @brief Register for the operational state change notification of the instance
 *
 * @param instance Instance of displayinfo_type
 * @param callback Function to be called on update
 * @param userdata Data passed to callback function
 * @return ERROR_NONE on succes,
 *         ERROR_GENERAL if callback already registered
 *         ERROR_UNAVAILABLE if: instance or is NULL
 */
EXTERNAL uint32_t displayinfo_register_operational_state_change_callback(
    displayinfo_operational_state_change_cb callback,
    void* userdata);
/**
 * @brief Unregister from the operational state change notification of the instance
 *
 * @param instance Instance of displayinfo_type
 * @param callback Function to be unregistered from callbacks
 * @return ERROR_NONE on succes,
 *         ERROR_NOT_EXIST if callback not registered
 *         ERROR_UNAVAILABLE if: instance or is NULL
 */
EXTERNAL uint32_t displayinfo_unregister_operational_state_change_callback(
    displayinfo_operational_state_change_cb callback);

/**
 * @brief Register for updates of the display output.
 *
 * @param instance Instance of @ref displayinfo_type.
 * @param callback Callback that needs to be called if a chaged is deteced.
 * @param userdata The user data to be passed back to the @ref displayinfo_updated_cb callback.
 * @return ERROR_NONE on succes,
 *         ERROR_GENERAL if callback already registered
 *         ERROR_UNAVAILABLE if: instance or is NULL
 **/
EXTERNAL uint32_t displayinfo_register_display_output_change_callback( displayinfo_display_output_change_cb callback, void* userdata);

/**
 * @brief Unregister for updates of the display output.
 *
 * @param instance Instance of @ref displayinfo_type.
 * @param callback Callback that was used to @ref displayinfo_registet
 * @return ERROR_NONE on succes,
 *         ERROR_NOT_EXIST if callback not registered
 *         ERROR_UNAVAILABLE if: instance or is NULL
 **/
EXTERNAL uint32_t displayinfo_unregister_display_output_change_callback( displayinfo_display_output_change_cb callback);

/**
 * @brief Returns name of display output.
 *
 * @param instance Instance of @ref displayinfo_type.
 * @param buffer Buffer that will contain instance name.
 * @param length Size of @ref buffer.
 *
 **/
EXTERNAL void displayinfo_name( char buffer[], const uint8_t length);

/**
 * @brief Checks if a audio passthrough is enabled.
 *
 * @param instance instance of @ref displayinfo_type
 * @param is_passthrough true if audio passthrough is enabled, false otherwise
 * @return ERROR_NONE on succes,
 *         ERROR_UNAVAILABLE if instance or is_enabled param is NULL or invalid connection
 */
EXTERNAL uint32_t displayinfo_is_audio_passthrough( bool* is_passthrough);

/**
 * @brief Checks if a display is connected to the display output.
 *
 * @param instance Instance of @ref displayinfo_type
 * @param is_connected  true a dispplay is connected, false otherwise.
 * @return ERROR_NONE on succes,
 *         ERROR_UNAVAILABLE if instance or is_enabled param is NULL or invalid connection
 */
EXTERNAL uint32_t displayinfo_connected( bool* is_connected);

/**
 * @brief Get the width of the connected display
 *
 * @param instance Instance of @ref displayinfo_type
 * @param width The current width in pixels
 * @return ERROR_NONE on succes,
 *         ERROR_UNAVAILABLE if instance or is_enabled param is NULL or invalid connection
 */
EXTERNAL uint32_t displayinfo_width( uint32_t* width);

/**
 * @brief Get the height of the connected display
 *
 * @param instance Instance of @ref displayinfo_type
 * @param width The current height in pixels
 * @return ERROR_NONE on succes,
 *         ERROR_UNAVAILABLE if instance or is_enabled param is NULL or invalid connection
 */
EXTERNAL uint32_t displayinfo_height( uint32_t* height);

/**
 * @brief Get the vertical refresh rate ("v-sync")
 *
 * @param instance Instance of @ref displayinfo_type
 * @param vertical_freq The vertical refresh rate
 * @return ERROR_NONE on succes,
 *         ERROR_UNAVAILABLE if instance or is_enabled param is NULL or invalid connection
 */
EXTERNAL uint32_t displayinfo_vertical_frequency( uint32_t* vertical_freq);

/**
 * @brief Get the current HDR system of the connected display
 *
 * @param instance Instance of @ref displayinfo_type
 * @param hdr The current enabled HDR system, DISPLAYINFO_HDR_UNKNOWN might occur if ThunderInterfaces contains new
 *            entry not defined in this client library
 * @return ERROR_NONE on succes,
 *         ERROR_UNKNOWN_KEY if: ThunderInterfaces contains new hdr type not defined in this library
 *         ERROR_UNAVAILABLE if: instance or hdr param is NULL or invalid connection
 */
EXTERNAL uint32_t displayinfo_hdr( displayinfo_hdr_t* hdr);

/**
 * @brief Get the current HDCP protection level of the connected display
 *
 * @param instance Instance of @ref displayinfo_type
 * @param hdcp The current enabled HDCP level, DISPLAYINFO_HDCP_UNKNOWN might occur if Thunder interfaces contains new
 *              entry not defined in client library
 * @return ERROR_NONE on succes,
 *         ERROR_UNKNOWN_KEY if: ThunderInterfaces contains new hdcp protection type not defined in this library
 *         ERROR_UNAVAILABLE if: instance or hdr param is NULL or invalid connection
 */
EXTERNAL uint32_t displayinfo_hdcp_protection( displayinfo_hdcp_protection_t* hdcp);

/**
 * @brief Get the total available GPU RAM space in bytes.
 *
 * @param instance Instance of @ref displayinfo_type
 * @param total_ram The total amount of GPU RAM available on the device.
 * @return ERROR_NONE on succes,
 *         ERROR_UNAVAILABLE if instance or total_ram param is NULL or invalid connection
 */
EXTERNAL uint32_t displayinfo_total_gpu_ram( uint64_t* total_ram);

/**
 * @brief Get the free available GPU RAM space in bytes.
 *
 * @param instance Instance of @ref displayinfo_type
 * @param free_ram The total amount of GPU RAM available on the device.
 * @return ERROR_NONE on succes,
 *         ERROR_UNAVAILABLE if instance or free_ram param is NULL or invalid connection
 */
EXTERNAL uint32_t displayinfo_free_gpu_ram( uint64_t* free_ram);

/**
 * @brief Returns EDID data of a connected display.
 *
 * @param instance Instance of @ref displayinfo_type
 * @param buffer Buffer that will contain the raw EDID data.
 * @param length Size of @ref buffer. On success it'll be set to the length of the actuall data in @ref buffer.
 * @return  ERROR_NONE on succes,
 *         ERROR_UNAVAILABLE if instance or free_ram param is NULL or invalid connection
 */
EXTERNAL uint32_t displayinfo_edid( uint8_t buffer[], uint16_t* length);

/**
 * @brief Parses EDID data.
 *
 * @param instance Instance of @ref displayinfo_type
 * @param buffer Buffer that will contain the raw EDID data.
 * @param length Size of @ref buffer.
 * @param edid_info Pointer to @ref displayinfo_edid_info_t containing parsed information. Caller passes memory.
 * @return  ERROR_NONE on success,
 *          ERROR_GENERAL on parsing error or invalid params
 *
 */
EXTERNAL uint32_t displayinfo_parse_edid(const uint8_t buffer[], const uint16_t length, displayinfo_edid_base_info_t* edid_info);

/**
 * @brief Returns info from CEA Extension Block if available.
 *
 * @param instance Instance of @ref displayinfo_type
 * @param buffer Buffer that will contain the raw EDID data.
 * @param cea_info  Returns the parsed CEA Information in @ref displayinfo_edid_cea_extension_t. Caller passes the memory.
 * @return  ERROR_NONE on success,
 *          ERROR_UNAVAILABLE if CEA extension block is not available in edid_info
 *          ERROR_GENERAL on parsing error
 *
 */
EXTERNAL uint32_t displayinfo_edid_cea_extension_info(const uint8_t buffer[], const uint16_t length, displayinfo_edid_cea_extension_info_t *cea_info);

/**
 * @brief Returns the corresponding displayinfo_edid_standardtiming_t for a given Video Idendification Code (VIC).
 *
 * @param instance Instance of @ref displayinfo_type
 * @param vic Video Information Code.
 * @param result  Returns standard timing information in @ref displayinfo_edid_standardtiming_t. Caller passes the memory.
 * @return  ERROR_NONE on success,
 *          ERROR_UNAVAILABLE if no mapping is available for VIC
 *
 */
EXTERNAL uint32_t displayinfo_edid_vic_to_standard_timing(const displayinfo_edid_vic_t vic, displayinfo_edid_standard_timing_t* result);

/**
 * @brief Get the width of the connected display in centimaters
 *
 * @param instance Instance of @ref displayinfo_type
 * @param width The current width in centimeters
 * @return ERROR_NONE on succes,
 *         ERROR_UNAVAILABLE if instance or width param is NULL or invalid connection
 */
EXTERNAL uint32_t displayinfo_width_in_centimeters( uint8_t* width);

/**
 * @brief Get the heigth of the connected display in centimaters
 *
 * @param instance Instance of @ref displayinfo_type
 * @param height The current height in centimeters
 * @return ERROR_NONE on succes,
 *         ERROR_UNAVAILABLE if instance or height param is NULL or invalid connection
 */
EXTERNAL uint32_t displayinfo_height_in_centimeters( uint8_t* height);

/**
 * @brief Checks if Dolby ATMOS is enabled.
 *
 * @param instance Instance of @ref displayinfo_type.
 * @return true if Dolby ATMOS is enabled, false otherwise.
 */
EXTERNAL bool displayinfo_is_atmos_supported();

/**
 * @brief Close the cached open connection if it exists.
 *
 */
EXTERNAL void displayinfo_dispose();

#ifdef __cplusplus
} // extern "C"
#endif
