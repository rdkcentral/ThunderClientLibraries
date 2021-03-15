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
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

static void toHexString(
    const uint8_t data[], const uint32_t dataLength,
    char buf[], uint32_t* bufLength)
{
    uint32_t totalLength = (6 * dataLength) + (dataLength / 8);

    if (*bufLength >= totalLength) {
        uint32_t length = 0;

        for (int i = 0; i < dataLength; i++) {
            length += snprintf(&buf[length], (*bufLength - length), "%s0x%02X%s",
                (i % 8 == 0) ? "\n" : "",
                data[i],
                (i == dataLength - 1) ? "" : ", ");
        }

        buf[length] = '\0';

        *bufLength = length;

    } else {
        printf("ERROR: bufLength %d is too small for %d chars\n", *bufLength, totalLength);
        *bufLength = 0;
    }
}

#define Trace(fmt, ...)                                 \
    do {                                                \
        fprintf(stdout, "<< " fmt "\n", ##__VA_ARGS__); \
        fflush(stdout);                                 \
    } while (0)

void displayinfo_display_updated(void* data)
{
    Trace("Display updated callback!\n");
}

void on_operational_state_change(bool is_operational, void* data)
{
    Trace("Operational state of the instance %s operational", is_operational ? "is" : "not");
}

void ShowMenu()
{
    printf("Version %s\n"
           "Enter\n"
           "\tI : Register for events.\n"
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
           "\t? : Help\n"
           "\tQ : Quit\n",
        __TIMESTAMP__);
}

int main(int argc, char* argv[])
{
    ShowMenu();

    int character;
    do {
        character = toupper(getc(stdin));

        switch (character) {
        case 'I': {
            if (displayinfo_register_display_output_change_callback(displayinfo_display_updated, NULL) == 0) {
                Trace("Registered for display update events");
            }
            if (displayinfo_register_operational_state_change_callback(on_operational_state_change, NULL) == 0) {
                Trace("Registered for operational state changes of the instance");
            }

            break;
        }
        case 'A': {
            bool is_passthru = false;
            if (displayinfo_is_audio_passthrough(&is_passthru) == 0) {
                Trace("Audio %s passthrough", is_passthru ? "is" : "is not");
            } else {
                Trace("Instance or is_passthru param is NULL, or invalid connection");
            }

            break;
        }

        case 'C': {
            bool is_connected = false;
            if (displayinfo_is_audio_passthrough(&is_connected) == 0) {
                Trace("Display %s connected", is_connected ? "is" : "is not");
            } else {
                Trace("Instance or is_connected param is NULL, or invalid connection");
            }

            break;
        }
        case 'D': {
            uint32_t width = 0, height = 0;
            if (displayinfo_width(&width) == 0 && displayinfo_height(&height) == 0) {
                Trace("Display resolution %dhx%dw", width, height);
            } else {
                Trace("Instance or width/height param is NULL, or invalid connection");
            }

            break;
        }
        case 'V': {
            uint32_t vertical_frequency = 0;
            if (displayinfo_vertical_frequency(&vertical_frequency) == 0) {
                Trace("Vertical frequency %d", vertical_frequency);
            } else {
                Trace("Instance or vertical_frequency is NULL, or invalid connection");
            }

            break;
        }

        case 'H': {
            displayinfo_hdr_t hdr;

            if (displayinfo_hdr(&hdr) != 0) {
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

            if (displayinfo_hdcp_protection(&hdcp) != 0) {
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
            if (displayinfo_total_gpu_ram(&total_ram) == 0) {
                Trace("Total ram: %" PRIu64, total_ram);
            } else {
                Trace("Instance or total_ram is NULL, or invalid connection");
            }
            break;
        }
        case 'E': {
            //just for testing purposes

            uint8_t buffer[1000];
            uint16_t length = sizeof(buffer);
            if (displayinfo_edid(buffer, &length) == 0) {
                Trace("Length: %d", length);
                char edid[3000];
                uint32_t edidSize = sizeof(edid);
                toHexString(buffer, length, edid, &edidSize);
                Trace("EDID: %s", edid);
            } else {
                Trace("Instance or buffer or length is NULL, or invalid connection");
            }
            break;
        }

        case 'F': {
            uint64_t free_ram = 0;
            if (displayinfo_free_gpu_ram(&free_ram) == 0) {
                Trace("Free ram: %" PRIu64, free_ram);
            } else {
                Trace("Instance or free_ram is NULL, or invalid connection");
            }
            break;
        }
        case 'X': {
            uint8_t width = 0, height = 0;
            if (displayinfo_width_in_centimeters(&width) == 0 && displayinfo_height_in_centimeters(&height) == 0) {
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

    displayinfo_unregister_operational_state_change_callback(on_operational_state_change);
    displayinfo_unregister_display_output_change_callback(displayinfo_display_updated);

    Trace("Done");

    return 0;
}