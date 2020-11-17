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
#define BUFFER_LENGTH 150


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
    int16_t result = 0;

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
            else
            {
                Trace("Created instance");
            }
            

            break;
        }
        case 'I': {
            if(result = deviceidentification_id(device, buffer, BUFFER_LENGTH) > 0){
                Trace("ID: %s", buffer);
                ResetBuffer(buffer);
            }
            else if(result == 0)
            {
                Trace("No ID available for this device, or instance or buffer is null");
            }
            
            else
            {
                Trace("Buffer too small, or invalid parameters");
            }
            
            
            break;
            
        }
        case 'C': {
            if(result = deviceidentification_chipset(device, buffer, BUFFER_LENGTH) > 0){
                Trace("Chipset: %s", buffer);
                ResetBuffer(buffer);
            }
            else if(result == 0){
                Trace("Instance or buffer is null");
            }
            else
            {
                Trace("Buffer too small");
            }
            
            break;
        }
        case 'F': {
            if(result = deviceidentification_firmware_version(device, buffer, BUFFER_LENGTH) > 0){
                Trace("Firmware Version: %s", buffer);
                ResetBuffer(buffer);
            }
            else if(result == 0){
                Trace("Instance or buffer is null");
            }
            else
            {
                Trace("Buffer too small");
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

    deviceidentification_release(device);
    

    Trace("Done");

    return 0;
}