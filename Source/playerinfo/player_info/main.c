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

void ShowMenu()
{
    printf("Enter\n"
           "\tI : Get instance.\n"
           "\tL : Check Loudness Equivalence in platform .\n"
           "\tR : Check playback resolution.\n"
           "\tA : Check audio codecs.\n"
           "\tV : Check video codecs.\n"
           "\t? : Help\n"
           "\tQ : Quit\n");
}

void ResetBuffer(char buffer[])
{
    memset(buffer, 0, BUFFER_LENGTH);
}

int main(int argc, char* argv[])
{
    struct playerinfo_type* player = NULL;
    playerinfo_videocodec_t videoCodecs[BUFFER_LENGTH];
    playerinfo_audiocodec_t audioCodecs[BUFFER_LENGTH];

    ShowMenu();

    int character;
    do {
        character = toupper(getc(stdin));

        switch (character) {
        case 'I': {
            player = playerinfo_instance("PlayerInfo");

            if (player == NULL) {
                Trace("Exiting: getting interface failed.");
                character = 'Q';
            } else {
                Trace("Created instance");
            }

            break;
        }
        case 'L': {
            bool is_enabled = false;
            if (playerinfo_is_audio_equivalence_enabled(player, &is_enabled) == 1) {
                Trace("Loudnes %s enabled", is_enabled ? "is" : "not");
            } else {
                Trace("Instance is null, or invalid connection");
            }

            break;
        }

        case 'R': {
            playerinfo_playback_resolution_t resolution;
            if (playerinfo_playback_resolution(player, &resolution) == 0) {
                Trace("Instance is null, or invalid connection");
            } else {
                switch (resolution) {
                case PLAYERINFO_RESOLUTION_UNKNOWN: {
                    Trace("PLAYERINFO_RESOLUTION_UNKNOWN");
                    break;
                }
                case PLAYERINFO_OTHER_DEFINED_RESOLUTION: {
                    Trace("PLAYERINFO_OTHER_DEFINED_RESOLUTION");
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
                    case PLAYERINFO_AUDIO_OTHER_DEFINED_CODEC:
                        Trace("PLAYERINFO_AUDIO_OTHER_DEFINED_CODEC");
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
                    case PLAYERINFO_VIDEO_OTHER_DEFINED_CODEC:
                        Trace("PLAYERINFO_VIDEO_OTHER_DEFINED_CODEC");
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

        case '?': {
            ShowMenu();
            break;
        }

        default:
            break;
        }
    } while (character != 'Q');

    playerinfo_release(player);

    Trace("Done");

    return 0;
}