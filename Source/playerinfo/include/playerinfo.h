#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef _MSVC_LANG
#undef EXTERNAL
#ifdef DISPLAYINFO_EXPORTS
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
    PLAYERINFO_AUDIO_OTHER_DEFINED_CODEC,
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
    PLAYERINFO_VIDEO_OTHER_DEFINED_CODEC,
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
    PLAYERINFO_OTHER_DEFINED_RESOLUTION,
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

/**
 * @brief Get a @ref playerinfo_type instance that matches the PlayerInfo implementation
 * 
 * @param name Name of the implementation
 * @return EXTERNAL struct* @ref instance, NULL if error.
 */
EXTERNAL struct playerinfo_type* playerinfo_instance(const char name[]);

/**
 * @brief Release the @ref instance
 * 
 * @param instance  Instance of @ref playerinfo_type
 */
EXTERNAL void playerinfo_release(struct playerinfo_type* instance);

/**
 * @brief Get current video playback resolution
 * @param instance Instance of @ref playerinfo_type.
 * @return The current resolution, PLAYERINFO_RESOLUTION_UNKNOWN on: unknown resolution, error, instance == NULL, or invalid connection,
 *         PLAYERINFO_OTHER_DEFINED_RESOLUTION if ThunderInterfaces contains new resolution not defined in this library,
 *          
 **/
EXTERNAL playerinfo_playback_resolution_t playerinfo_playback_resolution(struct playerinfo_type* instance);

/**
 * @brief Checks Loudness Equivalence in platform
 * @param instance Instance of @ref displayinfo_type.
 * @return true if enabled, false if: disabled, instance == NULL, or invalid connection.
 * 
 **/
EXTERNAL bool playerinfo_is_audio_equivalence_enabled(struct playerinfo_type* instance);

/**
 * @brief Gets Player audio codecs
 * 
 * @param instance Instance of @ref displayinfo_type.
 * @param array array which will contain audio codecs used by the player,
 *              if element == PLAYERINFO_AUDIO_OTHER_DEFINED_CODEC - ThunderInterfaces
 *              contains new codec not defined in this library. 
 * @param length length of given array
 * @return NUMBER OF CODECS used, or if array is not large enough return -NUMBER OF CODECS, 
 *         0 if: buffer or instance is NULL, or invalid connection.
 */
EXTERNAL int8_t playerinfo_audio_codecs(struct playerinfo_type* instance, playerinfo_audiocodec_t array[], const uint8_t length);

/**
 * @brief Gets Player video codecs
 * 
 * @param instance Instance of @ref displayinfo_type.
 * @param array array which will contain video codecs used by the player,
 *              if element == PLAYERINFO_VIDEO_OTHER_DEFINED_CODEC - ThunderInterfaces
 *              contains new codec not defined in this library. 
 * @param length length of given array
 * @return NUMBER OF CODECS used, ot if array is not large enough return -NUMBER OF CODECS,
 *         0 if: buffer or instance is NULL, or invalid connection.
 */
EXTERNAL int8_t playerinfo_video_codecs(struct playerinfo_type* instance, playerinfo_videocodec_t array[], const uint8_t length);

#ifdef __cplusplus
} // extern "C"
#endif
