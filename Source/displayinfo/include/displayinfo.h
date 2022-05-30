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

#define DISPLAYINFO_EDID_BLOCK_SIZE 128

typedef struct displayinfo_edid_extension{
    uint8_t tag;
    uint8_t data[DISPLAYINFO_EDID_BLOCK_SIZE];
} displayinfo_edid_extension_t;

typedef struct displayinfo_edid_info {

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

    uint8_t width_in_centimeters;
    uint8_t height_in_centimeters;

    uint8_t extension_block_count;
    displayinfo_edid_extension_t* edid_extensions;

} displayinfo_edid_info_t;

typedef struct displayinfo_edid_cea_extn {

    uint8_t version;

} displayinfo_edid_cea_extn_t;


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
 * @param buffer Buffer that will contain the data.
 * @param length Size of @ref buffer. On success it'll be set to the length of the actuall data in @ref buffer.
 * @return  ERROR_NONE on succes,
 *         ERROR_UNAVAILABLE if instance or free_ram param is NULL or invalid connection
 */
EXTERNAL uint32_t displayinfo_edid( uint8_t buffer[], uint16_t* length);

/**
 * @brief Parses EDID data.
 *
 * @param instance Instance of @ref displayinfo_type
 * @param buffer Buffer that will contain the data.
 * @param length Size of @ref buffer.
 * @param edid_info Pointer to @ref displayinfo_edid_info_t containing parsed information. Memory must be freed using
 *                  @ref displayinfo_edid_info_free()
 * @return  ERROR_NONE on success,
 *          ERROR_GENERAL on parsing error
 *
 */
EXTERNAL uint32_t displayinfo_parse_edid( uint8_t buffer[], uint16_t length, displayinfo_edid_info_t* edid_info);

/**
 * @brief Returns info from CEA Extension Block if available.
 *
 * @param instance Instance of @ref displayinfo_type
 * @param edid_info Pointer to @ref displayinfo_edid_info_t containing parsed information.
 * @param cea_info  Returns the parsed CEA Information in @ref displayinfo_edid_cea_extn_t
 * @return  ERROR_NONE on success,
 *          ERROR_UNAVAILABLE if CEA extension block is not available in edid_info
 *          ERROR_GENERAL on parsing error
 *
 */
EXTERNAL uint32_t displayinfo_edid_cea_extn_info(displayinfo_edid_info_t* edid_info, displayinfo_edid_cea_extn_t *cea_info);

/**
 * @brief Frees EDID info memory.
 *
 * @param instance Instance of @ref displayinfo_type
 * @param edid_info Pointer to @ref displayinfo_edid_info_t
 * @return  ERROR_NONE on success.
 *
 */
EXTERNAL uint32_t displayinfo_edid_info_free(displayinfo_edid_info_t* edid_info);

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
