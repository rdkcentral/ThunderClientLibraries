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
#if defined(WIN32) || defined(_WINDOWS) || defined (__CYGWIN__) || defined(_WIN64)
#ifdef DEVICEINFO_EXPORTS
#define EXTERNAL __declspec(dllexport)
#else
#define EXTERNAL __declspec(dllimport)
#pragma comment(lib, "deviceinfo.lib")
#endif
#else
#define EXTERNAL __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum deviceinfo_hdcp_type {
    DEVICEINFO_HDCP_UNAVAILABLE = 0,
    DEVICEINFO_HDCP_14,
    DEVICEINFO_HDCP_20,
    DEVICEINFO_HDCP_21,
    DEVICEINFO_HDCP_22,
    DEVICEINFO_HDCP_LENGTH
} deviceinfo_hdcp_t;

typedef enum deviceinfo_output_resolution_type {
    DEVICEINFO_RESOLUTION_UNKNOWN = 0,
    DEVICEINFO_RESOLUTION_480I,
    DEVICEINFO_RESOLUTION_480P,
    DEVICEINFO_RESOLUTION_720P,
    DEVICEINFO_RESOLUTION_720P50HZ,
    DEVICEINFO_RESOLUTION_1080P24HZ,
    DEVICEINFO_RESOLUTION_1080I50HZ,
    DEVICEINFO_RESOLUTION_1080P50HZ,
    DEVICEINFO_RESOLUTION_1080P60HZ,
    DEVICEINFO_RESOLUTION_2160P50HZ,
    DEVICEINFO_RESOLUTION_2160P60HZ,
    DEVICEINFO_RESOLUTION_LENGTH
} deviceinfo_output_resolution_t;

typedef enum deviceinfo_audio_output_type {
    DEVICEINFO_AUDIO_OTHER = 0,
    DEVICEINFO_AUDIO_RF_MODULATOR,
    DEVICEINFO_AUDIO_ANALOG,
    DEVICEINFO_AUDIO_SPDIF, 
    DEVICEINFO_AUDIO_HDMI,
    DEVICEINFO_AUDIO_DISPLAYPORT,
    DEVICEINFO_AUDIO_LENGTH
} deviceinfo_audio_output_t;

typedef enum deviceinfo_audio_capability_type {
    DEVICEINFO_AUDIO_CAPABILITY_NONE,
    DEVICEINFO_AUDIO_CAPABILITY_ATMOS,
    DEVICEINFO_AUDIO_CAPABILITY_DD,
    DEVICEINFO_AUDIO_CAPABILITY_DDPLUS,
    DEVICEINFO_AUDIO_CAPABILITY_DAD,
    DEVICEINFO_AUDIO_CAPABILITY_DAPV2,
    DEVICEINFO_AUDIO_CAPABILITY_MS12
} deviceinfo_audio_capability_t;

typedef enum deviceinfo_audio_ms12_capability_type { 
    DEVICEINFO_AUDIO_MS12_CAPABILITY_NONE,
    DEVICEINFO_AUDIO_MS12_CAPABILITY_DOLBYVOLUME,
    DEVICEINFO_AUDIO_MS12_CAPABILITY_INTELIGENTEQUALIZER,
    DEVICEINFO_AUDIO_MS12_CAPABILITY_DIALOGUEENHANCER
} deviceinfo_audio_ms12_capability_t;

typedef enum deviceinfo_audio_ms12_profile_type {
    DEVICEINFO_AUDIO_MS12_PROFILE_NONE,
    DEVICEINFO_AUDIO_MS12_PROFILE_MUSIC,
    DEVICEINFO_AUDIO_MS12_PROFILE_MOVIE,
    DEVICEINFO_AUDIO_MS12_PROFILE_VOICE,
} deviceinfo_audio_ms12_profile_t;

typedef enum deviceinfo_video_output_type {
    DEVICEINFO_VIDEO_OTHER = 0,
    DEVICEINFO_VIDEO_RF_MODULATOR,
    DEVICEINFO_VIDEO_COMPOSITE,
    DEVICEINFO_VIDEO_SVIDEO,
    DEVICEINFO_VIDEO_COMPONENT,
    DEVICEINFO_VIDEO_SCART_RGB,
    DEVICEINFO_VIDEO_HDMI,
    DEVICEINFO_VIDEO_DISPLAYPORT,
    DEVICEINFO_VIDEO_LENGTH
} deviceinfo_video_output_t;


/**
 * @brief Get the device architectue string 
 *
 * @param buffer Buffer that will contain the architecture string (including the null characters at the end)
 * @param length Size of the @ref buffer
 *               if the buffer is not big enough, Core::ERROR_INVALID_INPUT_LENGTH will be returned and LENGTH will be set to the needed size(including the null character at the end)
 *               When the return value is Core::ERROR_NONE, LENGTH will contain the size of the device architecture(including the null character at the end)
 *               if instance or buffer is null, returns 0
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 *         Core::ERROR_INVALID_INPUT_LENGTH if the buffer is not big enough(including the null characetr at the end)
 *         apropriate error otherwise.
 */
EXTERNAL uint32_t deviceinfo_architecture(char buffer[], uint8_t* length);

/**
 * @brief Get the device chipset name 
 *
 * @param buffer Buffer that will contain the device chipset string (including the null characters at the end)
 * @param length Size of the @ref buffer
 *               if the buffer is not big enough, Core::ERROR_INVALID_INPUT_LENGTH will be returned and LENGTH will be set to the needed size(including the null character at the end)
 *               When the return value is Core::ERROR_NONE, LENGTH will contain the size of the device chipset(including the null character at the end)
 *               if instance or buffer is null, returns 0
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 *         Core::ERROR_INVALID_INPUT_LENGTH if the buffer is not big enough(including the null characetr at the end)
 *         apropriate error otherwise.
 */
EXTERNAL uint32_t deviceinfo_chipset(char buffer[], uint8_t* length);

/**
 * @brief Get the device serial number
 *
 * @param buffer Buffer that will contain the serial number string (including the null characters at the end)
 * @param length Size of the @ref buffer
 *               if the buffer is not big enough, Core::ERROR_INVALID_INPUT_LENGTH will be returned and LENGTH will be set to the needed size(including the null character at the end)
 *               When the return value is Core::ERROR_NONE, LENGTH will contain the size of the serial number(including the null character at the end)
 *               if instance or buffer is null, returns 0
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 *         Core::ERROR_INVALID_INPUT_LENGTH if the buffer is not big enough(including the null characetr at the end)
 *         apropriate error otherwise.
 */
EXTERNAL uint32_t deviceinfo_serial_number(char buffer[], uint8_t* length);

/**
 * @brief Get the device sku
 *
 * @param buffer Buffer that will contain the sku string (including the null characters at the end)
 * @param length Size of the @ref buffer
 *               if the buffer is not big enough, Core::ERROR_INVALID_INPUT_LENGTH will be returned and LENGTH will be set to the needed size(including the null character at the end)
 *               When the return value is Core::ERROR_NONE, LENGTH will contain the size of the sku(including the null character at the end)
 *               if instance or buffer is null, returns 0
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 *         Core::ERROR_INVALID_INPUT_LENGTH if the buffer is not big enough(including the null characetr at the end)
 *         apropriate error otherwise.
 */
EXTERNAL uint32_t deviceinfo_sku(char buffer[], uint8_t* length);

/**
 * @brief Get the device make
 *
 * @param buffer Buffer that will contain the make string (including the null characters at the end)
 * @param length Size of the @ref buffer
 *               if the buffer is not big enough, Core::ERROR_INVALID_INPUT_LENGTH will be returned and LENGTH will be set to the needed size(including the null character at the end)
 *               When the return value is Core::ERROR_NONE, LENGTH will contain the size of the make (including the null character at the end)
 *               if instance or buffer is null, returns 0
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 *         Core::ERROR_INVALID_INPUT_LENGTH if the buffer is not big enough(including the null characetr at the end)
 *         apropriate error otherwise.
 */
EXTERNAL uint32_t deviceinfo_make(char buffer[], uint8_t* length);

/**
 * @brief Get the device type
 *
 * @param buffer Buffer that will contain the device type string (including the null characters at the end)
 * @param length Size of the @ref buffer
 *               if the buffer is not big enough, Core::ERROR_INVALID_INPUT_LENGTH will be returned and LENGTH will be set to the needed size(including the null character at the end)
 *               When the return value is Core::ERROR_NONE, LENGTH will contain the size of the device type (including the null character at the end)
 *               if instance or buffer is null, returns 0
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 *         Core::ERROR_INVALID_INPUT_LENGTH if the buffer is not big enough(including the null characetr at the end)
 *         apropriate error otherwise.
 */
EXTERNAL uint32_t deviceinfo_device_type(char buffer[], uint8_t* length);

/**
 * @brief Get the device model name string
 *
 * @param buffer Buffer that will contain the model name string (including the null characters at the end)
 * @param length Size of the @ref buffer
 *               if the buffer is not big enough, Core::ERROR_INVALID_INPUT_LENGTH will be returned and LENGTH will be set to the needed size(including the null character at the end)
 *               When the return value is Core::ERROR_NONE, LENGTH will contain the size of the model Name(including the null character at the end)
 *               if instance or buffer is null, returns 0
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 *         Core::ERROR_INVALID_INPUT_LENGTH if the buffer is not big enough(including the null characetr at the end)
 *         apropriate error otherwise.
 */
EXTERNAL uint32_t deviceinfo_model_name(char buffer[], uint8_t* length);

/**
 * @brief Get the device model year string
 *
 * @param buffer Buffer that will contain the model year string (including the null characters at the end)
 * @param length Size of the @ref buffer
 *               if the buffer is not big enough, Core::ERROR_INVALID_INPUT_LENGTH will be returned and LENGTH will be set to the needed size(including the null character at the end)
 *               When the return value is Core::ERROR_NONE, LENGTH will contain the size of the model year(including the null character at the end)
 *               if instance or buffer is null, returns 0
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 *         Core::ERROR_INVALID_INPUT_LENGTH if the buffer is not big enough(including the null characetr at the end)
 *         apropriate error otherwise.
 */
EXTERNAL uint32_t deviceinfo_model_year(char buffer[], uint8_t* length);

/**
 * @brief Get the system integrator name string
 *
 * @param buffer Buffer that will contain the integrator name string (including the null characters at the end)
 * @param length Size of the @ref buffer
 *               if the buffer is not big enough, Core::ERROR_INVALID_INPUT_LENGTH will be returned and LENGTH will be set to the needed size(including the null character at the end)
 *               When the return value is Core::ERROR_NONE, LENGTH will contain the size of the model integrator name(including the null character at the end)
 *               if instance or buffer is null, returns 0
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 *         Core::ERROR_INVALID_INPUT_LENGTH if the buffer is not big enough(including the null characetr at the end)
 *         apropriate error otherwise.
 */
EXTERNAL uint32_t deviceinfo_system_integrator_name(char buffer[], uint8_t* length);

/**
 * @brief Get the device friendly name string
 *
 * @param buffer Buffer that will contain the device friendly name string (including the null characters at the end)
 * @param length Size of the @ref buffer
 *               if the buffer is not big enough, Core::ERROR_INVALID_INPUT_LENGTH will be returned and LENGTH will be set to the needed size(including the null character at the end)
 *               When the return value is Core::ERROR_NONE, LENGTH will contain the size of the device friendly name(including the null character at the end)
 *               if instance or buffer is null, returns 0
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 *         Core::ERROR_INVALID_INPUT_LENGTH if the buffer is not big enough(including the null characetr at the end)
 *         apropriate error otherwise.
 */
EXTERNAL uint32_t deviceinfo_friendly_name(char buffer[], uint8_t* length);

/**
 * @brief Get the platform name string
 *
 * @param buffer Buffer that will contain the platform name string (including the null characters at the end)
 * @param length Size of the @ref buffer
 *               if the buffer is not big enough, Core::ERROR_INVALID_INPUT_LENGTH will be returned and LENGTH will be set to the needed size(including the null character at the end)
 *               When the return value is Core::ERROR_NONE, LENGTH will contain the size of the platform name(including the null character at the end)
 *               if instance or buffer is null, returns 0
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 *         Core::ERROR_INVALID_INPUT_LENGTH if the buffer is not big enough(including the null characetr at the end)
 *         apropriate error otherwise.
 */
EXTERNAL uint32_t deviceinfo_platform_name(char buffer[], uint8_t* length);

/**
 * @brief Get the device firmware version string
 *
 * @param buffer Buffer that will contain the firmware version string (including the null characters at the end)
 * @param length Size of the @ref buffer
 *               if the buffer is not big enough, Core::ERROR_INVALID_INPUT_LENGTH will be returned and LENGTH will be set to the needed size(including the null character at the end)
 *               When the return value is Core::ERROR_NONE, LENGTH will contain the size of the firmware version(including the null character at the end)
 *               if instance or buffer is null, returns 0
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 *         Core::ERROR_INVALID_INPUT_LENGTH if the buffer is not big enough(including the null characetr at the end)
 *         apropriate error otherwise.
 */
EXTERNAL uint32_t deviceinfo_firmware_version(char buffer[], uint8_t* length);

/**
 * @brief Get the binary device ID  
 *
 * @param buffer Buffer that will contain the binary device ID
 * @param length Size of the @ref buffer
 *               returns 0 if ID is not available for the device, or instance is null.
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 * 
 */
EXTERNAL uint32_t deviceinfo_id(uint8_t buffer[], uint8_t* length);

/**
 * @brief Get the device ID as a string 
 *
 * @param buffer Buffer that will contain the device ID string (including the null characters at the end)
 * @param length Size of the @ref buffer
 *               if the buffer is not big enough, Core::ERROR_INVALID_INPUT_LENGTH will be returned and LENGTH will be set to the needed size(including the null character at the end)
 *               When the return value is Core::ERROR_NONE, LENGTH will contain the size of the device ID string(including the null character at the end)
 *               if instance or buffer is null, returns 0
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 *         Core::ERROR_INVALID_INPUT_LENGTH if the buffer is not big enough(including the null characetr at the end)
 *         apropriate error otherwise.
 * 
 */
EXTERNAL uint32_t deviceinfo_id_str(char buffer[], uint8_t* length);

/**
 * @brief Get the device all supported audio outputs
 *
 * @param value Buffer that will contain the audio outputs
 * @param length Size of the @ref buffer
 *               if instance is null, returns 0
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 */
EXTERNAL uint32_t deviceinfo_audio_outputs(deviceinfo_audio_output_t value[], uint8_t* length);

/**
 * @brief Get the device all supported audio capabilities against given port
 *
 * @param audioOutput Name of audio port
 * @param value Buffer that will contain the audio outputs
 * @param length Size of the @ref buffer
 *               if instance is null, returns 0
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 */
EXTERNAL uint32_t deviceinfo_audio_capabilities(const deviceinfo_audio_output_t audioOutput, deviceinfo_audio_capability_t value[], uint8_t* length);

/**
 * @brief Get the device all supported audio ms12 capabilities against given port
 *
 * @param audioOutput Name of audio port
 * @param value Buffer that will contain the audio outputs
 * @param length Size of the @ref buffer
 *               if instance is null, returns 0
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 */
EXTERNAL uint32_t deviceinfo_audio_ms12_capabilities(const deviceinfo_audio_output_t audioOutput, deviceinfo_audio_ms12_capability_t value[], uint8_t* length);

/**
 * @brief Get the device all supported audio ms12 profiles against given port
 *
 * @param audioOutput Name of audio port
 * @param value Buffer that will contain the audio profiles
 * @param length Size of the @ref buffer
 *               if instance is null, returns 0
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 */
EXTERNAL uint32_t deviceinfo_audio_ms12_audio_profiles(const deviceinfo_audio_output_t audioOutput, deviceinfo_audio_ms12_profile_t value[], uint8_t* length);

/**
 * @brief Get the device all supported video outputs
 *
 * @param value Buffer that will contain the video output
 * @param length Size of the @ref value
 *               if instance  is null, returns 0
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 */
EXTERNAL uint32_t deviceinfo_video_outputs(deviceinfo_video_output_t value[], uint8_t* length);

/**
 * @brief Get the device all supported output resolutions
 *
 * @param videoOutput Name of video port
 * @param value Buffer that will contain resolutions
 * @param length Size of the @ref value buffer
 *               if instance or buffer is null, returns 0
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 */
EXTERNAL uint32_t deviceinfo_output_resolutions(const deviceinfo_video_output_t videoOutput, deviceinfo_output_resolution_t value[], uint8_t* length);

/**
 * @brief Get the device default output resolution against given port
 *
 * @param videoOutput Name of video port
 * @param value  Value of the output resolution
 *               Only read it if the returned value is Core::ERROR_NONE
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 */
EXTERNAL uint32_t deviceinfo_default_output_resolution(const deviceinfo_video_output_t videoOutput, deviceinfo_output_resolution_t* value);

/**
 * @brief Get the device maximum supported output resolution against given port
 *
 * @param videoOutput Name of video port
 * @param value  Value of the output resolution
 *               Only read it if the returned value is Core::ERROR_NONE
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 */
EXTERNAL uint32_t deviceinfo_maximum_output_resolution(const deviceinfo_video_output_t videoOutput, deviceinfo_output_resolution_t* value);

/**
 * @brief Get the device host edid
 *
 * @param buffer Buffer that will contain the host edid from device
 * @param length Size of the @ref buffer
 *               Only read it if the returned value is Core::ERROR_NONE
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 */
EXTERNAL uint32_t deviceinfo_host_edid(char buffer[], uint8_t* length);

/**
 * @brief Checks if HDR is is supported by the system..
 *
 * @param supportsHDR true if HDR is supported, false otherwise.
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 */
EXTERNAL uint32_t deviceinfo_hdr(bool* supportsHDR);

/**
 * @brief Checks if Dolby ATMOS is supported by the system.
 *
 * @param supportsAtmos true if Dolby ATMOS is supported, false otherwise.
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 */
EXTERNAL uint32_t deviceinfo_atmos(bool* supportsAtmos);

/**
 * @brief Checks if CEC supported by the system.
 *
 * @param supportsAtmos true if CEC is supported, false otherwise.
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 */
EXTERNAL uint32_t deviceinfo_cec(bool* supportsCEC);

/**
 * @brief provides the highest HDCP supported by the system against given port
 *
 * @param videoOutput Name of video port
 * @param hdcp type of @ref deviceinfo_hdcp_t
 * @return Core::ERROR_NONE if success, appropriate error otherwise.
 */
EXTERNAL uint32_t deviceinfo_hdcp(const deviceinfo_video_output_t videoOutput, deviceinfo_hdcp_t* hdcp);

/**
 * @brief Close the cached open connection if it exists.
 *
 */
EXTERNAL void deviceinfo_dispose();

#ifdef __cplusplus
} // extern "C"
#endif
