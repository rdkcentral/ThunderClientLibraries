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

void ShowMenu()
{
    printf("Enter\n"
           "\tI : Get instance.\n"
           "\tL : Check Loudness Equivalence in platform .\n"
           "\tR : Check playback resolution.\n"
           "\t? : Help\n"
           "\tQ : Quit\n");
           
}

int main(int argc, char* argv[])
{
    struct playerinfo_type* player = NULL;
    
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
            }
            else
            {
                Trace("Created instance");
            }
            
            break;
        }
        case 'L': {
            Trace("Loudnes %s enabled", playerinfo_is_audio_equivalence_enabled(player) ? "is" : "not");
            break;
            
        }

        case 'R': {
            switch (playerinfo_playback_resolution(player)) {
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
            case PLAYERINFO_RESOLUTION_1080I : {
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
                Trace("PLAYERINFO_RESOLUTION_UNKNOWN");
                break;
            }
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