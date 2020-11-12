#include <ctype.h>
#include <deviceidentification.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define Trace(fmt, ...)                                 \
    do {                                                \
        fprintf(stdout, "<< " fmt "\n", ##__VA_ARGS__); \
        fflush(stdout);                                 \
    } while (0)
#define BUFFER_LENGTH 50


void ShowMenu()
{
    printf("Enter\n"
           "\tL : Get instance.\n"
           "\tI : Get ID.\n"
           "\tC : Get chipset\n"
           "\tF : Get firmware version\n"
           );
}

void ResetBuffer(char buffer[]){
    memset(buffer,0, BUFFER_LENGTH);
}

int main(int argc, char* argv[])
{
    struct deviceidentification_type* device = NULL;
    char buffer[BUFFER_LENGTH];

    ShowMenu();

    int character;
    do {
        character = toupper(getc(stdin));

        switch (character) {
        case 'L': {
            device = deviceidentification_instance("DeviceIdentification");
            
            if (device == NULL) {
                Trace("Exiting: getting interface failed.");
                character = 'Q';
            }

            break;
        }
        case 'I': {
            deviceidentification_id(device, buffer, BUFFER_LENGTH);
            Trace("ID: %s", buffer);
            ResetBuffer(buffer);
            break;
            
        }
        case 'C': {
            deviceidentification_chipset(device, buffer, BUFFER_LENGTH);
            Trace("Chipset: %s", buffer);
            ResetBuffer(buffer);
            break;
        }
        case 'F': {
            deviceidentification_firmware_version(device, buffer, BUFFER_LENGTH);
            Trace("Firmware Version: %s", buffer);
            ResetBuffer(buffer);
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

    if(device != NULL){
        deviceidentification_release(device);
    }

    Trace("Done");

    return 0;
}