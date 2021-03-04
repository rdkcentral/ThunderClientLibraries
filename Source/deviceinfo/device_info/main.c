#include <ctype.h>
#include <deviceinfo.h>
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
           "\tR : Get maximum supported resolution\n"
           );
}

void ResetBuffer(char buffer[]){
    memset(buffer,0, BUFFER_LENGTH);
}

int main(int argc, char* argv[])
{
    struct deviceinfo_type* device = NULL;
    char buffer[BUFFER_LENGTH];
    int16_t result = 0;

    ShowMenu();

    int character;
    do {
        character = toupper(getc(stdin));

        switch (character) {
        case 'L': {
            device = deviceinfo_instance("DeviceInfo");
            
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
            result = deviceinfo_id(device, buffer, BUFFER_LENGTH);
            if(result > 0){
                Trace("ID: %s", buffer);
                ResetBuffer(buffer);
            }
            else if(result == 0)
            {
                Trace("No ID available for this device, or instance or buffer is null");
            }
            
            else
            {
                Trace("Buffer too small (or invlid parameters), should be at least of size %d ", -result);
            }
            
            
            break;
            
        }
        case 'C': {
            result = deviceinfo_chipset(device, buffer, BUFFER_LENGTH);
            if(result > 0){
                Trace("Chipset: %s", buffer);
                ResetBuffer(buffer);
            }
            else if(result == 0){
                Trace("Instance or buffer is null");
            }
            else
            {
                Trace("Buffer too small, should be at least of size %d ", -result);
            }
            
            break;
        }
        case 'F': {
            result = deviceinfo_firmware_version(device, buffer, BUFFER_LENGTH);
            if( result > 0){
                Trace("Firmware Version: %s", buffer);
                ResetBuffer(buffer);
            }
            else if(result == 0){
                Trace("Instance or buffer is null");
            }
            else
            {
                Trace("Buffer too small, should be at least of size %d ", -result);
            }
            break;

        }
        case 'R': {
            deviceinfo_output_resolution_t res = deviceinfo_output_resolution(device);
            Trace("Output Resolution: %d", res);
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

    deviceinfo_release(device);
    

    Trace("Done");

    return 0;
}