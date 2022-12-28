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

#ifndef PLAYERINFO_H
#define PLAYERINFO_H

#include <stdbool.h>
#include <stdint.h>

#undef EXTERNAL
#ifdef _MSVC_LANG
#ifdef PLAYERINFO_EXPORTS
#define EXTERNAL __declspec(dllexport)
#else
#define EXTERNAL __declspec(dllimport)
#pragma comment(lib, "playerinfo.lib")
#endif
#else
#define EXTERNAL __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct playerinfo_type;

typedef enum playerinfo_audiocodec_type {
    PLAYERINFO_AUDIO_UNDEFINED,
    PLAYERINFO_AUDIO_AAC,
    PLAYERINFO_AUDIO_AC3,
    PLAYERINFO_AUDIO_AC3_PLUS,
    PLAYERINFO_AUDIO_DTS,
    PLAYERINFO_AUDIO_MPEG1,
    PLAYERINFO_AUDIO_MPEG2,
    PLAYERINFO_AUDIO_MPEG3,
    PLAYERINFO_AUDIO_MPEG4,
    PLAYERINFO_AUDIO_OPUS,
    PLAYERINFO_AUDIO_VORBIS_OGG,
    PLAYERINFO_AUDIO_WAV
} playerinfo_audiocodec_t;

typedef enum playerinfo_videocodec_type {
    PLAYERINFO_VIDEO_UNDEFINED,
    PLAYERINFO_VIDEO_H263,
    PLAYERINFO_VIDEO_H264,
    PLAYERINFO_VIDEO_H265,
    PLAYERINFO_VIDEO_H265_10,
    PLAYERINFO_VIDEO_MPEG,
    PLAYERINFO_VIDEO_VP8,
    PLAYERINFO_VIDEO_VP9,
    PLAYERINFO_VIDEO_VP10
} playerinfo_videocodec_t;

typedef enum playerinfo_playback_resolution_type {
    PLAYERINFO_RESOLUTION_UNKNOWN,
    PLAYERINFO_RESOLUTION_480I,
    PLAYERINFO_RESOLUTION_480P,
    PLAYERINFO_RESOLUTION_576I,
    PLAYERINFO_RESOLUTION_576P,
    PLAYERINFO_RESOLUTION_720P,
    PLAYERINFO_RESOLUTION_1080I,
    PLAYERINFO_RESOLUTION_1080P,
    PLAYERINFO_RESOLUTION_2160P30,
    PLAYERINFO_RESOLUTION_2160P60
} playerinfo_playback_resolution_t;

typedef enum playerinfo_dolby_mode_type {
    PLAYERINFO_DOLBY_MODE_DIGITAL_PCM,
    PLAYERINFO_DOLBY_MODE_DIGITAL_PLUS,
    PLAYERINFO_DOLBY_MODE_DIGITAL_AC3,
    PLAYERINFO_DOLBY_MODE_AUTO,
    PLAYERINFO_DOLBY_MODE_MS12
} playerinfo_dolby_mode_t;

typedef enum playerinfo_dolby_sound_mode_type {
    PLAYERINFO_DOLBY_SOUND_UNKNOWN,
    PLAYERINFO_DOLBY_SOUND_MONO,
    PLAYERINFO_DOLBY_SOUND_STEREO,
    PLAYERINFO_DOLBY_SOUND_SURROUND,
    PLAYERINFO_DOLBY_SOUND_PASSTHRU
} playerinfo_dolby_sound_mode_t;

/**
* @brief Will be called if there are changes regarding operational state of the
*        instance - if it is not operational that means any function calls using it 
*        will not succeed (will return Core::ERROR_UNAVAILABLE). Not operational state 
*        can occur if the plugin inside WPEFramework has been deactivated.
*
*
* @param userData Pointer passed along when @ref playerinfo_register was issued.
* @param is_operational If instance is operational or not
*/
typedef void (*playerinfo_operational_state_change_cb)(bool is_operational, void* userdata);

/**
* @brief Will be called if there are changes regarding Dolby Audio Output, you need to query 
*        yourself what exactly is changed
*
* @param userData Pointer passed along when @ref playerinfo_register was issued.
*/
typedef void (*playerinfo_dolby_audio_updated_cb)(void* userdata);

/**
 * @brief Get a @ref playerinfo_type instance that matches the PlayerInfo implementation
 * 
 * @return EXTERNAL struct* @ref instance, NULL if error.
 */
EXTERNAL struct playerinfo_type* playerinfo_instance();

/**
 * @brief Release the @ref instance
 * 
 * @param instance  Instance of @ref playerinfo_type
 */
EXTERNAL void playerinfo_release();

/**
 * @brief Register for the operational state change notification of the instance
 * 
 * @param instance Instance of playerinfo_type
 * @param callback Function to be called on update
 * @param userdata Data passed to callback function
 * @return ERROR_NONE on succes, 
 *          ERROR_GENERAL if callback already registered 
 *          ERROR_UNAVAILABLE if: instance or is NULL
 */
EXTERNAL uint32_t playerinfo_register_operational_state_change_callback(playerinfo_operational_state_change_cb callback,
    void* userdata);
/**
 * @brief Unregister from the operational state change notification of the instance
 * 
 * @param instance Instance of playerinfo_type
 * @param callback Function to be unregistered from callbacks
 * @return ERROR_NONE on succes, 
 *         ERROR_NOT_EXIST if callback not registered
 *         ERROR_UNAVAILABLE if: instance or is NULL
 */
EXTERNAL uint32_t playerinfo_unregister_operational_state_change_callback(playerinfo_operational_state_change_cb callback);
/**
 * @brief Register for the updates of the Dolby Audio Mode changes
 * 
 * @param instance Instance of @ref playerinfo_type.
 * @param callback Function to be called on update
 * @param userdata Data passed to callback funcion
 * @return  ERROR_NONE on succes, 
 *          ERROR_GENERAL if callback already registered 
 *          ERROR_UNAVAILABLE if: instance or is NULL
 */
EXTERNAL uint32_t playerinfo_register_dolby_sound_mode_updated_callback(playerinfo_dolby_audio_updated_cb callback, void* userdata);

/**
 * @brief Unregister from the updates of the Dolby Audio Mode changes
 * 
 * @param instance Instance of @ref playerinfo_type.
 * @param callback Callback function unregister
 * @return ERROR_NONE on succes, 
 *         ERROR_NOT_EXIST if callback not registered
 *         ERROR_UNAVAILABLE if: instance or is NULL */
EXTERNAL uint32_t playerinfo_unregister_dolby_sound_mode_updated_callback(playerinfo_dolby_audio_updated_cb callback);

/**
 * @brief Get the instance name (callsign)
 * 
 * @param instance Instance of @ref playerinfo_type
 * @param buffer Buffer that will contain instance name.
 * @param length Size of @ref buffer.
 */
EXTERNAL void playerinfo_name(char buffer[], const uint8_t length);

/**
 * @brief Get current video playback resolution
 * @param instance instance Instance of @ref playerinfo_type.
 * @param resolution The current resolution, PLAYER_INFO_RESOLUTION_UNKNOWN MIGHT occur if ThunderInterfaces contains new resolution 
 *        not defined in this library,
 * @return ERROR_NONE on succes, 
 *         ERROR_UNKNOWN_KEY if: ThunderInterfaces contains new resolution not defined in this library 
 *         ERROR_UNAVAILABLE if: instance or resolution param is NULL or invalid connection
 */
EXTERNAL uint32_t playerinfo_playback_resolution(playerinfo_playback_resolution_t* resolution);

/**
 * @brief Checks Loudness Equivalence in platform
 * @param instance Instance of @ref playerinfo_type.
 * @param loudness true if enabled, false if disabled
 * @return ERROR_NONE on succes,
 *         ERROR_UNAVAILABLE if instance or is_enabled param is NULL or invalid connection
 */
EXTERNAL uint32_t playerinfo_is_audio_equivalence_enabled(bool* is_enabled);

/**
 * @brief Gets Player audio codecs
 * 
 * @param instance Instance of @ref playerinfo_type.
 * @param array array which will contain audio codecs used by the player,
 *              if element == PLAYERINFO_AUDIO_UNDEFINED - MIGHT be a case when
 *              ThunderInterfaces contains new codec not defined in this library. 
 * @param length length of given array
 * @return NUMBER OF CODECS used
 *         -NUMBER OF CODECS if array is not large enough, 
 *         0 if: buffer or instance is NULL, or invalid connection.
 */
EXTERNAL int8_t playerinfo_audio_codecs(playerinfo_audiocodec_t array[], const uint8_t length);

/**
 * @brief Gets Player video codecs
 * 
 * @param instance Instance of @ref playerinfo_type.
 * @param array array which will contain video codecs used by the player,
 *              if element == PLAYERINFO_VIDEO_UNDEFINED - MIGHT be a case when
 *              ThunderInterfaces contains new codec not defined in this library. 
 * @param length length of given array
 * @return NUMBER OF CODECS used, 
 *         -NUMBER OF CODECS if: array is not large enough return 
 *         0 if: buffer or instance is NULL, or invalid connection.
 */
EXTERNAL int8_t playerinfo_video_codecs(playerinfo_videocodec_t array[], const uint8_t length);

/**
 * @brief Atmos capabilities of Sink
 * 
 * @param instance Instance of @ref playerinfo_type.
 * @return true if atmos is supported, 
 *         false otherwise
 */
EXTERNAL bool playerinfo_is_dolby_atmos_supported();

/**
 * @brief Get Sound Mode - Mono/Stereo/Surround etc.
 * 
 * @param instance Instance of @ref playerinfo_type
 * @param sound_mode Current sound mode, PLAYERINFO_DOLBY_SOUND_UNKNOWN MIGHT occur if ThunderInterfaces contains new resolution not defined in this library
 * @return ERROR_NONE on succes, 
 *         ERROR_UNAVAILABLE if instance or sound_mode param is NULL or invalid connection
 */
EXTERNAL uint32_t playerinfo_set_dolby_sound_mode(playerinfo_dolby_sound_mode_t* sound_mode);

/**
 * @brief Enable Atmos Audio Output
 * 
 * @param instance Instance of @ref playerinfo_type
 * @param is_enabled Enable or disable Atmos Audio Output
 * @return ERROR_NONE on succes
 *         ERROR_UNAVAILABLE if instance == NULL or invalid connection
 */
EXTERNAL uint32_t playerinfo_enable_atmos_output(const bool is_enabled);

/**
 * @brief Set the dolby mode 
 * 
 * @param instance Instance of @ref playerinfo_type
 * @param mode dolby mode to be set
 * @return ERROR_NONE on succes,
 *         ERROR_UNAVAILABLE on instance == NULL or invalid connection
 *         ERROR_UNKNOWN_KEY on unknown mode
 */
EXTERNAL uint32_t playerinfo_set_dolby_mode(const playerinfo_dolby_mode_t mode);

/**
 * @brief Get the dolby mode object
 * 
 * @param instance Instance of @ref playerinfo_type
 * @param mode Current dolby mode, PLAYERINFO_DOLBY_MODE_AUTO MIGHT occur if ThunderInterfaces contains new resolution not defined in this library
 * @return ERROR_NONE on succes,
 *         ERROR_UNAVAILABLE on instance == NULL or invalid connection
 *         ERROR_UNKNOWN_KEY on unknown mode
 */
EXTERNAL uint32_t playerinfo_get_dolby_mode(playerinfo_dolby_mode_t* mode);

/**
 * @brief Close the cached open connection if it exists.
 *
 */
EXTERNAL void playerinfo_dispose();

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PLAYERINFO_H
