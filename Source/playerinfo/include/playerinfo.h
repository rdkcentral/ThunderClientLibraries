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
    AUDIO_UNDEFINED,
    AUDIO_AAC,
    AUDIO_AC3,
    AUDIO_AC3_PLUS,
    AUDIO_DTS,
    AUDIO_MPEG1,
    AUDIO_MPEG2,
    AUDIO_MPEG3,
    AUDIO_MPEG4,
    AUDIO_OPUS,
    AUDIO_VORBIS_OGG,
    AUDIO_WAV
} playerinfo_audiocodec_t;

typedef enum playerinfo_videocodec_type {
    VIDEO_UNDEFINED,
    VIDEO_H263,
    VIDEO_H264,
    VIDEO_H265,
    VIDEO_H265_10,
    VIDEO_MPEG,
    VIDEO_VP8,
    VIDEO_VP9,
    VIDEO_VP10
} playerinfo_videocodec_t;

typedef enum playerinfo_playback_resolution_type {
    RESOLUTION_UNKNOWN,
    RESOLUTION_480I,
    RESOLUTION_480P,
    RESOLUTION_576I,
    RESOLUTION_576P,
    RESOLUTION_720P,
    RESOLUTION_1080I,
    RESOLUTION_1080P,
    RESOLUTION_2160P30,
    RESOLUTION_2160P60
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
 * \brief Get current video playback resolution
 * 
 * \param instance Instance of \ref playerinfo_type.
 * 
 * \return The current resolution, RESOLUTION_UNKNOWN on error or invalid connection
 * 
 **/
EXTERNAL playerinfo_playback_resolution_t playerinfo_playback_resolution(struct playerinfo_type* instance);

/**
 * \brief Checks Loudness Equivalence in platform
 * 
 * \param instance Instance of \ref displayinfo_type.
 * 
 * \return true if enabled, false otherwise.
 **/
EXTERNAL bool playerinfo_is_audio_equivalence_enabled(struct playerinfo_type* instance);





#ifdef __cplusplus
} // extern "C"
#endif
