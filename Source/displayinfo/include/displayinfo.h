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

#include <stdbool.h>
#include <stdint.h>

#ifdef _MSVC_LANG
#undef EXTERNAL
#ifdef DISPLAYINFO_EXPORTS
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

/* Where is this needed?
typedef enum displayinfo_error_type {
    DISPLAYINFO_ERROR_NONE = 0,
    DISPLAYINFO_ERROR_UNKNOWN = 1,
    DISPLAYINFO_ERROR_INVALID_INSTANCE = 2,
} displayinfo_error_t;
*/

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
 * @brief Get a @ref displayinfo_type instance that matches the a DisplayInfo implementation.
 * 
 * @return @ref displayinfo_type instance, NULL on error.
 **/
EXTERNAL struct displayinfo_type* displayinfo_instance();

/**
 * @brief Release the @ref displayinfo_type instance.
 * 
 * @param instance Instance of ref displayinfo_type.
 * 
 **/
EXTERNAL void displayinfo_release(struct displayinfo_type* instance);

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
EXTERNAL uint32_t displayinfo_register_operational_state_change_callback(struct displayinfo_type* instance,
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
EXTERNAL uint32_t displayinfo_unregister_operational_state_change_callback(struct displayinfo_type* instance,
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
EXTERNAL uint32_t displayinfo_register_display_output_change_callback(struct displayinfo_type* instance, displayinfo_display_output_change_cb callback, void* userdata);

/**
 * @brief Unregister for updates of the display output.
 * 
 * @param instance Instance of @ref displayinfo_type.
 * @param callback Callback that was used to @ref displayinfo_registet
 * @return ERROR_NONE on succes, 
 *         ERROR_NOT_EXIST if callback not registered
 *         ERROR_UNAVAILABLE if: instance or is NULL
 **/
EXTERNAL uint32_t displayinfo_unregister_display_output_change_callback(struct displayinfo_type* instance, displayinfo_display_output_change_cb callback);

/**
 * @brief Returns name of display output.
 *
 * @param instance Instance of @ref displayinfo_type.
 * @param buffer Buffer that will contain instance name.
 * @param length Size of @ref buffer.
 *
 **/
EXTERNAL void displayinfo_name(struct displayinfo_type* instance, char buffer[], const uint8_t length);

/**
 * @brief Checks if a audio passthrough is enabled.
 * 
 * @param instance instance of @ref displayinfo_type
 * @param is_passthrough true if audio passthrough is enabled, false otherwise
 * @return ERROR_NONE on succes,
 *         ERROR_UNAVAILABLE if instance or is_enabled param is NULL or invalid connection
 */
EXTERNAL uint32_t displayinfo_is_audio_passthrough(struct displayinfo_type* instance, bool* is_passthrough);

/**
 * @brief Checks if a display is connected to the display output.
 * 
 * @param instance Instance of @ref displayinfo_type
 * @param is_connected  true a dispplay is connected, false otherwise.
 * @return ERROR_NONE on succes,
 *         ERROR_UNAVAILABLE if instance or is_enabled param is NULL or invalid connection
 */
EXTERNAL uint32_t displayinfo_connected(struct displayinfo_type* instance, bool* is_connected);

/**
 * @brief Get the width of the connected display  
 * 
 * @param instance Instance of @ref displayinfo_type
 * @param width The current width in pixels
 * @return ERROR_NONE on succes,
 *         ERROR_UNAVAILABLE if instance or is_enabled param is NULL or invalid connection
 */
EXTERNAL uint32_t displayinfo_width(struct displayinfo_type* instance, uint32_t* width);

/**
 * @brief Get the height of the connected display  
 * 
 * @param instance Instance of @ref displayinfo_type
 * @param width The current height in pixels
 * @return ERROR_NONE on succes,
 *         ERROR_UNAVAILABLE if instance or is_enabled param is NULL or invalid connection
 */
EXTERNAL uint32_t displayinfo_height(struct displayinfo_type* instance, uint32_t* height);

/**
 * @brief Get the vertical refresh rate ("v-sync")  
 * 
 * @param instance Instance of @ref displayinfo_type
 * @param vertical_freq The vertical refresh rate
 * @return ERROR_NONE on succes,
 *         ERROR_UNAVAILABLE if instance or is_enabled param is NULL or invalid connection
 */
EXTERNAL uint32_t displayinfo_vertical_frequency(struct displayinfo_type* instance, uint32_t* vertical_freq);

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
EXTERNAL uint32_t displayinfo_hdr(struct displayinfo_type* instance, displayinfo_hdr_t* hdr);

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
EXTERNAL uint32_t displayinfo_hdcp_protection(struct displayinfo_type* instance, displayinfo_hdcp_protection_t* hdcp);

/**
 * @brief Get the total available GPU RAM space in bytes.
 * 
 * @param instance Instance of @ref displayinfo_type
 * @param total_ram The total amount of GPU RAM available on the device.
 * @return ERROR_NONE on succes,
 *         ERROR_UNAVAILABLE if instance or total_ram param is NULL or invalid connection  
 */
EXTERNAL uint32_t displayinfo_total_gpu_ram(struct displayinfo_type* instance, uint64_t* total_ram);

/**
 * @brief Get the free available GPU RAM space in bytes.
 * 
 * @param instance Instance of @ref displayinfo_type
 * @param free_ram The total amount of GPU RAM available on the device.
 * @return ERROR_NONE on succes,
 *         ERROR_UNAVAILABLE if instance or free_ram param is NULL or invalid connection
 */
EXTERNAL uint32_t displayinfo_free_gpu_ram(struct displayinfo_type* instance, uint64_t* free_ram);

/**
 * @brief Returns EDID data of a connected display.
 * 
 * @param instance Instance of @ref displayinfo_type
 * @param buffer Buffer that will contain the data.
 * @param length Size of @ref buffer. On success it'll be set to the length of the actuall data in @ref buffer.
 * @return  ERROR_NONE on succes,
 *         ERROR_UNAVAILABLE if instance or free_ram param is NULL or invalid connection
 */
EXTERNAL uint32_t displayinfo_edid(struct displayinfo_type* instance, uint8_t buffer[], uint16_t* length);

/**
 * @brief Get the width of the connected display in centimaters
 * 
 * @param instance Instance of @ref displayinfo_type
 * @param width The current width in centimeters
 * @return ERROR_NONE on succes,
 *         ERROR_UNAVAILABLE if instance or width param is NULL or invalid connection 
 */
EXTERNAL uint32_t displayinfo_width_in_centimeters(struct displayinfo_type* instance, uint8_t* width);

/**
 * @brief Get the heigth of the connected display in centimaters
 * 
 * @param instance Instance of @ref displayinfo_type
 * @param height The current height in centimeters
 * @return ERROR_NONE on succes,
 *         ERROR_UNAVAILABLE if instance or height param is NULL or invalid connection 
 */
EXTERNAL uint32_t displayinfo_height_in_centimeters(struct displayinfo_type* instance, uint8_t* height);

/**
 * @brief Checks if Dolby ATMOS is enabled.
 * 
 * @param instance Instance of @ref displayinfo_type.
 * @return true if Dolby ATMOS is enabled, false otherwise.
 */
EXTERNAL bool displayinfo_is_atmos_supported(struct displayinfo_type* instance);

#ifdef __cplusplus
} // extern "C"
#endif
