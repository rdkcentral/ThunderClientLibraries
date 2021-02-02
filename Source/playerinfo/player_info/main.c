#include <ctype.h>
#include <playerinfo.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define Trace(fmt, ...)                                 \
    do {                                                \
        fprintf(stdout, "<< " fmt "\n", ##__VA_ARGS__); \
        fflush(stdout);                                 \
    } while (0)

#define BUFFER_LENGTH 15

void show_menu()
{
    printf("Enter\n"
           "\tI : Get instance.\n"
           "\tS : Register for an event.\n"
           "\tU : Unregister from an event.\n"
           "\tL : Check Loudness Equivalence in platform .\n"
           "\tR : Check playback resolution.\n"
           "\tA : Check audio codecs.\n"
           "\tV : Check video codecs.\n"
           "\tM : Is Atmos metadata supported.\n"
           "\tB : Get Dolby soundmode .\n"
           "\tE : Enable Atmos output.\n"
           "\tD : Disable Atmos output.\n"
           "\tO : Set Dolby Mode (to PLAYERINFO_DOLBY_MODE_DIGITAL_PCM).\n"
           "\tP : Get Dolby Mode.\n"
           "\t? : Help\n"
           "\tQ : Quit\n");
}

void on_dolby_event(void* data)
{
    Trace("Dolby event triggered");
}

void on_operational_state_change(bool is_operational, void* data)
{
    Trace("Operational state of the instance %s operational", is_operational ? "is" : "not");
}

int main(int argc, char* argv[])
{
    struct playerinfo_type* player = NULL;
    playerinfo_videocodec_t videoCodecs[BUFFER_LENGTH];
    playerinfo_audiocodec_t audioCodecs[BUFFER_LENGTH];

    show_menu();

    int character;
    do {
        character = toupper(getc(stdin));

        switch (character) {
        case 'I': {
            player = playerinfo_instance();

            if (player == NULL) {
                Trace("Exiting: getting interface failed.");
                character = 'Q';
            } else {
                Trace("Created instance");
                playerinfo_register_operational_state_change_callback(player, on_operational_state_change, NULL);
                Trace("Registered for operational state changes.");
            }

            break;
        }
        case 'S': {
            playerinfo_register_dolby_sound_mode_updated_callback(player, on_dolby_event, NULL);
            Trace("Registered for an dolby sound mode update.");
            break;
        }

        case 'U': {
            playerinfo_unregister_dolby_sound_mode_updated_callback(player, on_dolby_event);
            Trace("Registered for an dolby sound mode update.");
            break;
        }

        case 'L': {
            bool is_enabled = false;
            if (playerinfo_is_audio_equivalence_enabled(player, &is_enabled) == 0) {
                Trace("Loudnes %s enabled", is_enabled ? "is" : "not");
            } else {
                Trace("Instance or is_enabled param is NULL");
            }

            break;
        }

        case 'R': {
            playerinfo_playback_resolution_t resolution;
            if (playerinfo_playback_resolution(player, &resolution) != 0) {
                Trace("Instance or resolution param is null");
            } else {
                switch (resolution) {
                case PLAYERINFO_RESOLUTION_UNKNOWN: {
                    Trace("PLAYERINFO_RESOLUTION_UNKNOWN");
                    break;
                }
                case PLAYERINFO_RESOLUTION_480I: {
                    Trace("PLAYERINFO_RESOLUTION_480I");
                    break;
                }
                case PLAYERINFO_RESOLUTION_480P: {
                    Trace("PLAYERINFO_RESOLUTION_480P");
                    break;
                }
                case PLAYERINFO_RESOLUTION_576I: {
                    Trace("PLAYERINFO_RESOLUTION_576I");
                    break;
                }
                case PLAYERINFO_RESOLUTION_576P: {
                    Trace("PLAYERINFO_RESOLUTION_576P");
                    break;
                }
                case PLAYERINFO_RESOLUTION_720P: {
                    Trace("PLAYERINFO_RESOLUTION_720P");
                    break;
                }
                case PLAYERINFO_RESOLUTION_1080I: {
                    Trace("PLAYERINFO_RESOLUTION_1080I");
                    break;
                }
                case PLAYERINFO_RESOLUTION_1080P: {
                    Trace("PLAYERINFO_RESOLUTION_1080P");
                    break;
                }
                case PLAYERINFO_RESOLUTION_2160P30: {
                    Trace("PLAYERINFO_RESOLUTION_2160P30");
                    break;
                }
                case PLAYERINFO_RESOLUTION_2160P60: {
                    Trace("PLAYERINFO_RESOLUTION_2160P60");
                    break;
                }
                default: {
                    Trace("Resolution not specified in client switch-case.");
                    break;
                }
                }
            }
            break;
        }
        case 'A': {
            int8_t numberOfCodecs = playerinfo_audio_codecs(player, audioCodecs, BUFFER_LENGTH);
            if (numberOfCodecs < 0) {
                Trace("Buffer too small, need at least %d length", -numberOfCodecs);

            } else if (numberOfCodecs == 0) {
                Trace("Instance or array NULL, or unable to set up connection.");

            } else {
                for (uint8_t i = 0; i < numberOfCodecs; ++i) {
                    switch (audioCodecs[i]) {
                    case PLAYERINFO_AUDIO_UNDEFINED:
                        Trace("PLAYERINFO_AUDIO_UNDEFINED");
                        break;
                    case PLAYERINFO_AUDIO_AAC:
                        Trace("PLAYERINFO_AUDIO_AAC");
                        break;
                    case PLAYERINFO_AUDIO_AC3:
                        Trace("PLAYERINFO_AUDIO_AC3");
                        break;
                    case PLAYERINFO_AUDIO_AC3_PLUS:
                        Trace("PLAYERINFO_AUDIO_AC3_PLUS");
                        break;
                    case PLAYERINFO_AUDIO_DTS:
                        Trace("PLAYERINFO_AUDIO_DTS");
                        break;
                    case PLAYERINFO_AUDIO_MPEG1:
                        Trace("PLAYERINFO_AUDIO_MPEG1");
                        break;
                    case PLAYERINFO_AUDIO_MPEG2:
                        Trace("PLAYERINFO_AUDIO_MPEG2");
                        break;
                    case PLAYERINFO_AUDIO_MPEG3:
                        Trace("PLAYERINFO_AUDIO_MPEG3");
                        break;
                    case PLAYERINFO_AUDIO_MPEG4:
                        Trace("PLAYERINFO_AUDIO_MPEG4");
                        break;
                    case PLAYERINFO_AUDIO_OPUS:
                        Trace("PLAYERINFO_AUDIO_OPUS");
                        break;
                    case PLAYERINFO_AUDIO_VORBIS_OGG:
                        Trace("PLAYERINFO_AUDIO_VORBIS_OGG");
                        break;
                    case PLAYERINFO_AUDIO_WAV:
                        Trace("PLAYERINFO_AUDIO_WAV");
                        break;
                    default:
                        Trace("Audio codec not specified in client switch-case.");
                        break;
                    }
                }
                //reset buffer
                memset(audioCodecs, 0, BUFFER_LENGTH * sizeof(playerinfo_audiocodec_t));
            }
            break;
        }

        case 'V': {
            int8_t numberOfCodecs = playerinfo_video_codecs(player, videoCodecs, BUFFER_LENGTH);
            if (numberOfCodecs < 0) {
                Trace("Buffer too small, need at least %d length", -numberOfCodecs);

            } else if (numberOfCodecs == 0) {
                Trace("Instance or array NULL, or unable to set up connection.");

            } else {
                for (uint8_t i = 0; i < numberOfCodecs; ++i) {
                    switch (videoCodecs[i]) {
                    case PLAYERINFO_VIDEO_UNDEFINED:
                        Trace("PLAYERINFO_VIDEO_UNDEFINED");
                        break;
                    case PLAYERINFO_VIDEO_H263:
                        Trace("PLAYERINFO_VIDEO_H263");
                        break;
                    case PLAYERINFO_VIDEO_H264:
                        Trace("PLAYERINFO_VIDEO_H264");
                        break;
                    case PLAYERINFO_VIDEO_H265:
                        Trace("PLAYERINFO_VIDEO_H265");
                        break;
                    case PLAYERINFO_VIDEO_H265_10:
                        Trace("PLAYERINFO_VIDEO_H265_10");
                        break;
                    case PLAYERINFO_VIDEO_MPEG:
                        Trace("PLAYERINFO_VIDEO_MPEG");
                        break;
                    case PLAYERINFO_VIDEO_VP8:
                        Trace("PLAYERINFO_VIDEO_VP8");
                        break;
                    case PLAYERINFO_VIDEO_VP9:
                        Trace("PLAYERINFO_VIDEO_VP9");
                        break;
                    case PLAYERINFO_VIDEO_VP10:
                        Trace("PLAYERINFO_VIDEO_VP10");
                        break;
                    default:
                        Trace("Audio codec not specified in client switch-case.");
                        break;
                    }
                }
                memset(videoCodecs, 0, BUFFER_LENGTH * sizeof(playerinfo_videocodec_t));
            }
            break;
        }

        case 'M': {
            bool is_supported = false;
            is_supported = playerinfo_is_dolby_atmos_supported(player);
            Trace("Dolby Atmos %s supported", is_supported ? "is" : "not");
            break;
        }

        case 'B': {
            playerinfo_dolby_sound_mode_t sound_mode;
            if (playerinfo_set_dolby_sound_mode(player, &sound_mode) != 0) {
                Trace("Instance or sound_mode param is null");
            } else {
                switch (sound_mode) {
                case PLAYERINFO_DOLBY_SOUND_UNKNOWN:
                    Trace("PLAYERINFO_DOLBY_SOUND_UNKNOWN");
                    break;
                case PLAYERINFO_DOLBY_SOUND_MONO:
                    Trace("PLAYERINFO_DOLBY_SOUND_MONO");
                    break;
                case PLAYERINFO_DOLBY_SOUND_STEREO:
                    Trace("PLAYERINFO_DOLBY_SOUND_STEREO");
                    break;
                case PLAYERINFO_DOLBY_SOUND_SURROUND:
                    Trace("PLAYERINFO_DOLBY_SOUND_SURROUND");
                    break;
                case PLAYERINFO_DOLBY_SOUND_PASSTHRU:
                    Trace("PLAYERINFO_DOLBY_SOUND_PASSTHRU");
                    break;
                default:
                    Trace("Sound mode not specified in client switch-case");
                    break;
                }
            }
            break;
        }
        case 'E': {
            if (playerinfo_enable_atmos_output(player, true) == 0) {
                Trace("Enabled Atmos output");

            } else {
                Trace("Error enabling atmos output");
            }
            break;
        }

        case 'D': {
            if (playerinfo_enable_atmos_output(player, false) == 0) {
                Trace("Disable Atmos output");

            } else {
                Trace("Error disabling atmos output");
            }
            break;
        }

        case 'O': {
            if (playerinfo_set_dolby_mode(player, PLAYERINFO_DOLBY_MODE_DIGITAL_PCM) == 0) {
                Trace("Setting succeded");

            } else {
                Trace("Setting failed");
            }
            break;
        }

        case 'P': {
            playerinfo_dolby_mode_t mode;
            if (playerinfo_get_dolby_mode(player, &mode) == 0) {
                switch (mode) {

                case PLAYERINFO_DOLBY_MODE_DIGITAL_PCM:
                    Trace("PLAYERINFO_DOLBY_MODE_DIGITAL_PCM");
                    break;
                case PLAYERINFO_DOLBY_MODE_DIGITAL_PLUS:
                    Trace("PLAYERINFO_DOLBY_MODE_DIGITAL_PLUS");
                    break;
                case PLAYERINFO_DOLBY_MODE_DIGITAL_AC3:
                    Trace("PLAYERINFO_DOLBY_MODE_DIGITAL_AC3");
                    break;
                case PLAYERINFO_DOLBY_MODE_AUTO:
                    Trace("PLAYERINFO_DOLBY_MODE_AUTO");
                    break;
                case PLAYERINFO_DOLBY_MODE_MS12:
                    Trace("PLAYERINFO_DOLBY_MODE_MS12");
                    break;
                default:
                    Trace("Dolby mode not specified in client switch-case");
                    break;
                }
            }

            break;
        }

        case '?': {
            show_menu();
            break;
        }

        default:
            break;
        }
    } while (character != 'Q');

    playerinfo_unregister_operational_state_change_callback(player, on_operational_state_change);
    Trace("Unregistered operational state changed callback.");
    playerinfo_release(player);

    Trace("Released instance.");

    return 0;
}