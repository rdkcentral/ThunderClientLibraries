/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
#include <ctype.h>
#include <displayinfo.h>
#include <stdint.h>
#include <stdio.h>

#define Trace(fmt, ...)                                 \
    do {                                                \
        fprintf(stdout, "<< " fmt "\n", ##__VA_ARGS__); \
        fflush(stdout);                                 \
    } while (0)

static uint32_t updatedCount = 0;

static void displayinfo_updated(struct displayinfo_type* session, void* data)
{
    updatedCount++;
    fprintf(stdout, "## %d display updated event%s received.\n", updatedCount, (updatedCount == 1) ? "" : "s");
}

void ShowMenu()
{
    printf("Enter\n"
           "\tI : Get instance.\n"
           "\tA : Is audio passthru.\n"
           "\tC : Check display connection.\n"
           "\tD : Get current display resolution.\n"
           "\tV : Get vertical frequency.\n"
           "\tH : Get current HDR standard\n"
           "\tP : Get current HDCP protection\n"
           "\tE : Get EDID\n"
           "\tT : Get total gpu ram\n"
           "\tF : Get free gpu ram\n"
           "\tX : Get display dimensions in centimeters \n"
           // "\tR : Enable display info events\n"
           //"\tU : Disable display info events\n"
           "\t? : Help\n"
           "\tQ : Quit\n");
}

int main(int argc, char* argv[])
{
    struct displayinfo_type* display = NULL;

    ShowMenu();

    int character;
    do {
        character = toupper(getc(stdin));

        switch (character) {
        case 'I': {
            display = displayinfo_instance();

            if (display == NULL) {
                Trace("Exiting: getting interface failed.");
                character = 'Q';
            } else {
                Trace("Created instance");
            }

            break;
        }
        case 'A': {
            bool is_passthru = false;
            if (displayinfo_is_audio_passthrough(display, &is_passthru) == 0) {
                Trace("Audio %s passthrough", is_passthru ? "is" : "is not");
            } else {
                Trace("Instance or is_passthru param is NULL, or invalid connection");
            }

            break;
        }

        case 'C': {
            bool is_connected = false;
            if (displayinfo_is_audio_passthrough(display, &is_connected) == 0) {
                Trace("Display %s connected", is_connected ? "is" : "is not");
            } else {
                Trace("Instance or is_connected param is NULL, or invalid connection");
            }

            break;
        }
        case 'D': {
            uint32_t width = 0, height = 0;
            if (displayinfo_width(display, &width) == 0 && displayinfo_height(display, &height) == 0) {
                Trace("Display resolution %dhx%dw", width, height);
            } else {
                Trace("Instance or width/height param is NULL, or invalid connection");
            }

            break;
        }
        case 'V': {
            uint32_t vertical_frequency = 0;
            if (displayinfo_vertical_frequency(display, &vertical_frequency) == 0) {
                Trace("Vertical frequency %d", vertical_frequency);
            } else {
                Trace("Instance or vertical_frequency is NULL, or invalid connection");
            }

            break;
        }

        case 'H': {
            displayinfo_hdr_t hdr;

            if (displayinfo_hdr(display, &hdr) != 0) {
                Trace("Instance or hdr param is null or invalid connection");
            } else {

                switch (hdr) {
                case DISPLAYINFO_HDR_OFF: {
                    Trace("HDR: OFF");
                    break;
                }
                case DISPLAYINFO_HDR_10: {
                    Trace("HDR: HDR10");
                    break;
                }
                case DISPLAYINFO_HDR_10PLUS: {
                    Trace("HDR: HDR10 Plus");
                    break;
                }
                case DISPLAYINFO_HDR_DOLBYVISION: {
                    Trace("HDR: DolbyVision");
                    break;
                }
                case DISPLAYINFO_HDR_TECHNICOLOR: {
                    Trace("HDR: Technicolor");
                    break;
                }
                default: {
                    Trace("HDR: Unknown");
                    break;
                }
                }
            }
            break;
        }
        case 'P': {
            displayinfo_hdcp_protection_t hdcp;

            if (displayinfo_hdcp_protection(display, &hdcp) != 0) {
                Trace("Instance or hdcp param is null or invalid connection");
            } else {
                switch (hdcp) {
                case DISPLAYINFO_HDCP_UNENCRYPTED: {
                    Trace("HDCP: Unencrypted");
                    break;
                }
                case DISPLAYINFO_HDCP_1X: {
                    Trace("HDCP: 1.x Enabled");
                    break;
                }
                case DISPLAYINFO_HDCP_2X: {
                    Trace("HDCP: 2.x Enabled");
                    break;
                }
                default: {
                    Trace("HDCP: Unknown");
                    break;
                }
                }
            }
            break;
        }
        case 'T': {
            uint64_t total_ram = 0;
            if (displayinfo_total_gpu_ram(display, &total_ram) == 0) {
                Trace("Total ram: %lld", total_ram);
            } else {
                Trace("Instance or total_ram is NULL, or invalid connection");
            }
            break;
        }
        case 'E': {
            //just for testing purposed

            uint8_t buffer[1000];
            uint16_t length = 1000;
            if (displayinfo_edid(display, buffer, &length) == 0) {
                Trace("Length: %d", length);
                Trace("EDID: %s", buffer);
            } else {
                Trace("Instance or buffer or length is NULL, or invalid connection");
            }
            break;
        }

        case 'F': {
            uint64_t free_ram = 0;
            if (displayinfo_free_gpu_ram(display, &free_ram) == 0) {
                Trace("Free ram: %lld", free_ram);
            } else {
                Trace("Instance or free_ram is NULL, or invalid connection");
            }
            break;
        }
        case 'X': {
            uint8_t width = 0, height = 0;
            if (displayinfo_width_in_centimeters(display, &width) == 0 && displayinfo_height_in_centimeters(display, &height) == 0) {
                Trace("Display dimension in cm: %dhx%dw", width, height);
            } else {
                Trace("Instance or width/height param is NULL, or invalid connection");
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

    displayinfo_release(display);

    Trace("Done");

    return 0;
}