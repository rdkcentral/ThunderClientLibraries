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

#ifdef __cplusplus
extern "C" {
#endif
struct displayinfo_type;

typedef enum displayinfo_hdr_type {
    DISPLAYINFO_HDR_OFF,
    DISPLAYINFO_HDR_10,
    DISPLAYINFO_HDR_10PLUS,
    DISPLAYINFO_HDR_DOLBYVISION,
    DISPLAYINFO_HDR_TECHNICOLOR,
    DISPLAYINFO_HDR_UNKNOWN
} displayinfo_hdr_t;

typedef enum displayinfo_hdcp_protection_type {
    DISPLAYINFO_HDCP_UNENCRYPTED,
    DISPLAYINFO_HDCP_1X,
    DISPLAYINFO_HDCP_2X,
    DISPLAYINFO_HDCP_UNKNOWN
} displayinfo_hdcp_protection_t;


typedef enum displayinfo_edid_video_interface {
    DISPLAYINFO_EDID_VIDEO_INTERFACE_UNDEFINED = 0,
    DISPLAYINFO_EDID_VIDEO_INTERFACE_HDMI_A = 2,
    DISPLAYINFO_EDID_VIDEO_INTERFACE_HDMI_B = 3,
    DISPLAYINFO_EDID_VIDEO_INTERFACE_MDDI = 4,
    DISPLAYINFO_EDID_VIDEO_INTERFACE_DISPLAYPORT = 5
} displayinfo_edid_video_interface_t;

typedef enum displayinfo_edid_colordepthtype  {
    DISPLAYINFO_EDID_BPC_UNDEFINED = (0 << 0),
    DISPLAYINFO_EDID_BPC_6 = (1 << 0),
    DISPLAYINFO_EDID_BPC_8 = (1 << 1),
    DISPLAYINFO_EDID_BPC_10 = (1 << 2),
    DISPLAYINFO_EDID_BPC_12 = (1 << 3),
    DISPLAYINFO_EDID_BPC_14 = (1 << 4),
    DISPLAYINFO_EDID_BPC_16 = (1 << 5),
} displayinfo_edid_colordepthtype_t;

typedef enum displayinfo_edid_colorformattype {
    DISPLAYINFO_EDID_COLOR_FORMAT_UNDEFINED = (0 << 0),
    DISPLAYINFO_EDID_COLOR_FORMAT_RGB = (1 << 0),
    DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR_4_2_2 = (1 << 1),
    DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR_4_4_4 = (1 << 2),
    DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR_4_2_0 = (1 << 3)
} displayinfo_edid_colorformattype_t;

typedef enum displayinfo_edid_colorspacetype {
    DISPLAYINFO_EDID_COLOR_SPACE_SRGB = (1 << 0),
    DISPLAYINFO_EDID_COLOR_SPACE_XVYCC_601 = (1 << 1), // ITU-R BT.601
    DISPLAYINFO_EDID_COLOR_SPACE_XVYCC_709 = (1 << 2), // ITU-R BT.709
    DISPLAYINFO_EDID_COLOR_SPACE_SYCC_601 = (1 << 3),
    DISPLAYINFO_EDID_COLOR_SPACE_OP_YCC_601 = (1 << 4), // or ADOBE_YCC_601
    DISPLAYINFO_EDID_COLOR_SPACE_OP_RGB = (1 << 5), // or ADOBE_RGB
    DISPLAYINFO_EDID_COLOR_SPACE_ITUR_BT_2020_CYCC = (1 << 6), // ITU-R BT.2020 Yc Cbc Crc
    DISPLAYINFO_EDID_COLOR_SPACE_ITUR_BT_2020_YCC = (1 << 7), // ITU-R BT.2020 RGB or YCbCr
    DISPLAYINFO_EDID_COLOR_SPACE_ITUR_BT_2020_RGB = (1 << 8),
    DISPLAYINFO_EDID_COLOR_SPACE_DCI_P3 = (1 << 9),
} displayinfo_edid_colorspacetype_t;

 typedef enum displayinfo_edid_audioformattype {
    DISPLAYINFO_EDID_AUDIOFORMAT_LPCM = (1 << 0),
    DISPLAYINFO_EDID_AUDIOFORMAT_AC3 = (1 << 1),
    DISPLAYINFO_EDID_AUDIOFORMAT_MPEG1 = (1 << 2),
    DISPLAYINFO_EDID_AUDIOFORMAT_MP3 = (1 << 3),
    DISPLAYINFO_EDID_AUDIOFORMAT_MPEG2 = (1 << 4),
    DISPLAYINFO_EDID_AUDIOFORMAT_AAC_LC = (1 << 5),
    DISPLAYINFO_EDID_AUDIOFORMAT_DTS = (1 << 6),
    DISPLAYINFO_EDID_AUDIOFORMAT_ATRAC = (1 << 7),
    DISPLAYINFO_EDID_AUDIOFORMAT_SUPER_AUDIO_CD = (1 << 8),
    DISPLAYINFO_EDID_AUDIOFORMAT_EAC3 = (1 << 9),
    DISPLAYINFO_EDID_AUDIOFORMAT_DTSHD = (1 << 10),
    DISPLAYINFO_EDID_AUDIOFORMAT_DOLBY_TRUEHD = (1 << 11),
    DISPLAYINFO_EDID_AUDIOFORMAT_DST_AUDIO = (1 << 12),
    DISPLAYINFO_EDID_AUDIOFORMAT_MS_WMA_PRO = (1 << 13),
    DISPLAYINFO_EDID_AUDIOFORMAT_MPEG4_HEAAC = (1 << 14),
    DISPLAYINFO_EDID_AUDIOFORMAT_MPEG4_HEAAC_V2 = (1 << 15),
    DISPLAYINFO_EDID_AUDIOFORMAT_MPEG4_ACC_LC = (1 << 16),
    DISPLAYINFO_EDID_AUDIOFORMAT_DRA = (1 << 17),
    DISPLAYINFO_EDID_AUDIOFORMAT_MPEG4_HEAAC_MPEG_SURROUND = (1 << 18),
    DISPLAYINFO_EDID_AUDIOFORMAT_MPEG4_HEAAC_LC_MPEG_SURROUND = (1 << 19),
    DISPLAYINFO_EDID_AUDIOFORMAT_MPEGH_3DAUDIO = (1 << 20),
    DISPLAYINFO_EDID_AUDIOFORMAT_LPCM_3DAUDIO = (1 << 21),
    DISPLAYINFO_EDID_AUDIOFORMAT_AC4 = (1 << 22),
    DISPLAYINFO_EDID_AUDIOFORMAT_DOLBY_ATMOS = (1 << 23),
} displayinfo_edid_audioformattype_t;

typedef enum displayinfo_edid_displayaspectratiotype {
    DISPLAYINFO_EDID_DAR_4_TO_3,
    DISPLAYINFO_EDID_DAR_16_TO_9,
    DISPLAYINFO_EDID_DAR_64_TO_27,
    DISPLAYINFO_EDID_DAR_256_TO_135,
} displayinfo_edid_displayaspectratiotype_t;

typedef enum displayinfo_edid_timingshortnametype {
    DISPLAYINFO_EDID_TSN_1080I = 0,
    DISPLAYINFO_EDID_TSN_1080I25,
    DISPLAYINFO_EDID_TSN_1080I50,
    DISPLAYINFO_EDID_TSN_1080I60,
    DISPLAYINFO_EDID_TSN_1080P,
    DISPLAYINFO_EDID_TSN_1080P100,
    DISPLAYINFO_EDID_TSN_1080P120,
    DISPLAYINFO_EDID_TSN_1080P24,
    DISPLAYINFO_EDID_TSN_1080P25,
    DISPLAYINFO_EDID_TSN_1080P2X,
    DISPLAYINFO_EDID_TSN_1080P2X100,
    DISPLAYINFO_EDID_TSN_1080P2X120,
    DISPLAYINFO_EDID_TSN_1080P2X24,
    DISPLAYINFO_EDID_TSN_1080P2X25,
    DISPLAYINFO_EDID_TSN_1080P2X30,
    DISPLAYINFO_EDID_TSN_1080P2X48,
    DISPLAYINFO_EDID_TSN_1080P2X50,
    DISPLAYINFO_EDID_TSN_1080P30,
    DISPLAYINFO_EDID_TSN_1080P48,
    DISPLAYINFO_EDID_TSN_1080P50,
    DISPLAYINFO_EDID_TSN_2160P,
    DISPLAYINFO_EDID_TSN_2160P100,
    DISPLAYINFO_EDID_TSN_2160P120,
    DISPLAYINFO_EDID_TSN_2160P24,
    DISPLAYINFO_EDID_TSN_2160P25,
    DISPLAYINFO_EDID_TSN_2160P2X,
    DISPLAYINFO_EDID_TSN_2160P2X100,
    DISPLAYINFO_EDID_TSN_2160P2X120,
    DISPLAYINFO_EDID_TSN_2160P2X24,
    DISPLAYINFO_EDID_TSN_2160P2X25,
    DISPLAYINFO_EDID_TSN_2160P2X30,
    DISPLAYINFO_EDID_TSN_2160P2X48,
    DISPLAYINFO_EDID_TSN_2160P2X50,
    DISPLAYINFO_EDID_TSN_2160P30,
    DISPLAYINFO_EDID_TSN_2160P48,
    DISPLAYINFO_EDID_TSN_2160P50,
    DISPLAYINFO_EDID_TSN_2160P60,
    DISPLAYINFO_EDID_TSN_240P,
    DISPLAYINFO_EDID_TSN_240P4X,
    DISPLAYINFO_EDID_TSN_240P4XH,
    DISPLAYINFO_EDID_TSN_240PH,
    DISPLAYINFO_EDID_TSN_288P,
    DISPLAYINFO_EDID_TSN_288P4X,
    DISPLAYINFO_EDID_TSN_288P4XH,
    DISPLAYINFO_EDID_TSN_288PH,
    DISPLAYINFO_EDID_TSN_4320P,
    DISPLAYINFO_EDID_TSN_4320P100,
    DISPLAYINFO_EDID_TSN_4320P120,
    DISPLAYINFO_EDID_TSN_4320P24,
    DISPLAYINFO_EDID_TSN_4320P25,
    DISPLAYINFO_EDID_TSN_4320P2X,
    DISPLAYINFO_EDID_TSN_4320P2X100,
    DISPLAYINFO_EDID_TSN_4320P2X120,
    DISPLAYINFO_EDID_TSN_4320P2X24,
    DISPLAYINFO_EDID_TSN_4320P2X25,
    DISPLAYINFO_EDID_TSN_4320P2X30,
    DISPLAYINFO_EDID_TSN_4320P2X48,
    DISPLAYINFO_EDID_TSN_4320P2X50,
    DISPLAYINFO_EDID_TSN_4320P30,
    DISPLAYINFO_EDID_TSN_4320P48,
    DISPLAYINFO_EDID_TSN_4320P50,
    DISPLAYINFO_EDID_TSN_480I,
    DISPLAYINFO_EDID_TSN_480I119,
    DISPLAYINFO_EDID_TSN_480I119H,
    DISPLAYINFO_EDID_TSN_480I4X,
    DISPLAYINFO_EDID_TSN_480I4XH,
    DISPLAYINFO_EDID_TSN_480I59,
    DISPLAYINFO_EDID_TSN_480I59H,
    DISPLAYINFO_EDID_TSN_480IH,
    DISPLAYINFO_EDID_TSN_480P,
    DISPLAYINFO_EDID_TSN_480P119,
    DISPLAYINFO_EDID_TSN_480P119H,
    DISPLAYINFO_EDID_TSN_480P239,
    DISPLAYINFO_EDID_TSN_480P239H,
    DISPLAYINFO_EDID_TSN_480P2X,
    DISPLAYINFO_EDID_TSN_480P2XH,
    DISPLAYINFO_EDID_TSN_480P4X,
    DISPLAYINFO_EDID_TSN_480P4XH,
    DISPLAYINFO_EDID_TSN_480PH,
    DISPLAYINFO_EDID_TSN_576I,
    DISPLAYINFO_EDID_TSN_576I100,
    DISPLAYINFO_EDID_TSN_576I100H,
    DISPLAYINFO_EDID_TSN_576I4X,
    DISPLAYINFO_EDID_TSN_576I4XH,
    DISPLAYINFO_EDID_TSN_576I50,
    DISPLAYINFO_EDID_TSN_576I50H,
    DISPLAYINFO_EDID_TSN_576IH,
    DISPLAYINFO_EDID_TSN_576P,
    DISPLAYINFO_EDID_TSN_576P100,
    DISPLAYINFO_EDID_TSN_576P100H,
    DISPLAYINFO_EDID_TSN_576P200,
    DISPLAYINFO_EDID_TSN_576P200H,
    DISPLAYINFO_EDID_TSN_576P2X,
    DISPLAYINFO_EDID_TSN_576P2XH,
    DISPLAYINFO_EDID_TSN_576P4X,
    DISPLAYINFO_EDID_TSN_576P4XH,
    DISPLAYINFO_EDID_TSN_576PH,
    DISPLAYINFO_EDID_TSN_720P,
    DISPLAYINFO_EDID_TSN_720P100,
    DISPLAYINFO_EDID_TSN_720P120,
    DISPLAYINFO_EDID_TSN_720P24,
    DISPLAYINFO_EDID_TSN_720P25,
    DISPLAYINFO_EDID_TSN_720P2X,
    DISPLAYINFO_EDID_TSN_720P2X100,
    DISPLAYINFO_EDID_TSN_720P2X120,
    DISPLAYINFO_EDID_TSN_720P2X24,
    DISPLAYINFO_EDID_TSN_720P2X25,
    DISPLAYINFO_EDID_TSN_720P2X30,
    DISPLAYINFO_EDID_TSN_720P2X48,
    DISPLAYINFO_EDID_TSN_720P2X50,
    DISPLAYINFO_EDID_TSN_720P30,
    DISPLAYINFO_EDID_TSN_720P48,
    DISPLAYINFO_EDID_TSN_720P50,
    DISPLAYINFO_EDID_TSN_DMT0659,
} displayinfo_edid_timingshortnametype_t;

typedef enum displayinfo_edid_verticalfrequencytype {
    // in milliHertzs
    DISPLAYINFO_EDID_VF_23980_OR_24000 = (1 << 0),
    DISPLAYINFO_EDID_VF_25000 = (1 << 1),
    DISPLAYINFO_EDID_VF_29970_OR_30000 = (1 << 2),
    DISPLAYINFO_EDID_VF_47960_OR_48000 = (1 << 3),
    DISPLAYINFO_EDID_VF_50000 = (1 << 4),
    DISPLAYINFO_EDID_VF_59826 = (1 << 5),
    DISPLAYINFO_EDID_VF_59940 = (1 << 6),
    DISPLAYINFO_EDID_VF_60000 = (1 << 7),
    DISPLAYINFO_EDID_VF_100000 = (1 << 8),
    DISPLAYINFO_EDID_VF_119880_OR_120000 = (1 << 9),
    DISPLAYINFO_EDID_VF_200000 = (1 << 10),
    DISPLAYINFO_EDID_VF_239760 = (1 << 11),
} displayinfo_edid_verticalfrequencytype_t;

typedef struct displayinfo_edid_base_info {

    /* e.g, SAM - Samsung Electric Company*/
    char manufacturer_id[3];

    uint16_t product_code;
    uint32_t serial_number;

    /*Week of manufacture; or FF model year flag*/
    uint8_t manufacture_week;

    /*Year of manufacture, or year of model, if model year flag is set. Year = datavalue + 1990.*/
    uint16_t manufacture_year;

    /*EDID version, usually 01 (for 1.3 and 1.4)*/
    uint8_t version;

    /*	EDID revision, usually 03 (for 1.3) or 04 (for 1.4)*/
    uint8_t revision;

    /* Digital input flag*/
    bool digital;

     /* If Digital input flag is set, then bits_per_color and video_interface apply*/
    uint8_t bits_per_color;
    displayinfo_edid_video_interface_t video_interface;

    /*Bitmap containing all supported values from displayinfo_edid_colorformattype*/
    uint8_t supported_digital_display_types;

    uint8_t width_in_centimeters;
    uint8_t height_in_centimeters;

    // EDID v1.3 - https://glenwing.github.io/docs/VESA-EEDID-A1.pdf
    // EDID v1.4 - https://glenwing.github.io/docs/VESA-EEDID-A2.pdf
    // According the VESA standard:
    //
    // 3.10.1 First Detailed Timing Descriptor Block
    // The first Detailed Timing (at addresses 36h→47h) shall only be used to indicate the mode
    // that the monitor vendor has determined will give an optimal image. For LCD monitors,
    // this will in most cases be the panel "native timing" and “native resolution”.
    // Use of the EDID Preferred Timing bit shall be used to indicate that the timing indeed
    // conforms to this definition.
    uint16_t preferred_width_in_pixels;
    uint16_t preferred_height_in_pixels;

} displayinfo_edid_base_info_t;

typedef struct displayinfo_edid_standardtiming {

    uint8_t vic; // Video Idendification Code

    displayinfo_edid_timingshortnametype_t short_name;

    displayinfo_edid_displayaspectratiotype_t dar;

    displayinfo_edid_verticalfrequencytype_t vertical_frequency;

    uint16_t active_height;

    uint16_t active_width;
} displayinfo_edid_standardtiming_t;


typedef struct displayinfo_edid_cea_extension {

    uint8_t version;

    /*Bitmap containing all supported values from displayinfo_edid_colordepthtype_t*/
    uint8_t supported_color_depths;

    displayinfo_edid_colorformattype_t supported_color_format;

    /*Bitmap containing all supported values from displayinfo_edid_colorformattype*/
    uint8_t supported_color_formats;

    /*Bitmap containing all supported values from displayinfo_edid_colorspacetype_t*/
    uint8_t supported_color_spaces;

    /*Bitmap containing all supported values from displayinfo_edid_audioformattype_t*/
    uint32_t supported_audio_formats;

    /* Number of relevant VICs in supported_timings*/
    uint8_t number_of_supported_timings;

    uint8_t supported_timings[256];

} displayinfo_edid_cea_extension_t;


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
EXTERNAL uint32_t displayinfo_parse_edid(const uint8_t buffer[], uint16_t length, displayinfo_edid_base_info_t* edid_info);

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
EXTERNAL uint32_t displayinfo_edid_cea_extension_info(const uint8_t buffer[], uint16_t length, displayinfo_edid_cea_extension_t *cea_info);

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
EXTERNAL uint32_t displayinfo_edid_vic_to_standard_timing(uint8_t vic, displayinfo_edid_standardtiming_t* result);

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
