/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#ifdef __GNUC__
#define VARIABLE_IS_NOT_USED __attribute__((unused))
#elif defined(_MSC_VER)
#define VARIABLE_IS_NOT_USED
#else
#define VARIABLE_IS_NOT_USED
#endif

static uint8_t edid_lg[] = {
        /* Base block */
	0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,
	0x1E,0x6D,0x06,0x77,0x0A,0xAC,0x0A,0x00,
	0x07,0x1E,0x01,0x03,0x80,0x3C,0x22,0x78,
	0xEA,0x3E,0x31,0xAE,0x50,0x47,0xAC,0x27,
	0x0C,0x50,0x54,0x21,0x08,0x00,0x71,0x40,
	0x81,0x80,0x81,0xC0,0xA9,0xC0,0xD1,0xC0,
	0x81,0x00,0x01,0x01,0x01,0x01,0x08,0xE8,
	0x00,0x30,0xF2,0x70,0x5A,0x80,0xB0,0x58,
	0x8A,0x00,0x58,0x54,0x21,0x00,0x00,0x1E,
	0x04,0x74,0x00,0x30,0xF2,0x70,0x5A,0x80,
	0xB0,0x58,0x8A,0x00,0x58,0x54,0x21,0x00,
	0x00,0x1A,0x00,0x00,0x00,0xFD,0x00,0x28,
	0x3D,0x1E,0x87,0x3C,0x00,0x0A,0x20,0x20,
	0x20,0x20,0x20,0x20,0x00,0x00,0x00,0xFC,
	0x00,0x4C,0x47,0x20,0x48,0x44,0x52,0x20,
	0x34,0x4B,0x0A,0x20,0x20,0x20,0x01,0x29,
	/* CEA Extension block */
	0x02,0x03,0x44,0x71,0x4D,0x90,0x22,0x20,
	0x1F,0x12,0x03,0x04,0x01,0x61,0x60,0x5D,
	0x5E,0x5F,0x23,0x09,0x07,0x07,0x6D,0x03,
	0x0C,0x00,0x10,0x00,0xB8,0x3C,0x20,0x00,
	0x60,0x01,0x02,0x03,0x67,0xD8,0x5D,0xC4,
	0x01,0x78,0x80,0x03,0xE3,0x0F,0x00,0x03,
	0x68,0x1A,0x00,0x00,0x01,0x01,0x28,0x3D,
	0x00,0xE3,0x05,0xC0,0x00,0xE6,0x06,0x05,
	0x01,0x52,0x48,0x5D,0x02,0x3A,0x80,0x18,
	0x71,0x38,0x2D,0x40,0x58,0x2C,0x45,0x00,
	0x58,0x54,0x21,0x00,0x00,0x1E,0x56,0x5E,
	0x00,0xA0,0xA0,0xA0,0x29,0x50,0x30,0x20,
	0x35,0x00,0x58,0x54,0x21,0x00,0x00,0x1A,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF1
};

static uint8_t edid_dell[] = {
        /* Base block */
	0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,
	0x10,0xAC,0xBC,0xA0,0x55,0x52,0x31,0x32,
	0x1C,0x1D,0x01,0x03,0x80,0x34,0x20,0x78,
	0xEA,0x04,0x95,0xA9,0x55,0x4D,0x9D,0x26,
	0x10,0x50,0x54,0xA5,0x4B,0x00,0x71,0x4F,
	0x81,0x80,0xA9,0x40,0xD1,0xC0,0xD1,0x00,
	0x01,0x01,0x01,0x01,0x01,0x01,0x28,0x3C,
	0x80,0xA0,0x70,0xB0,0x23,0x40,0x30,0x20,
	0x36,0x00,0x06,0x44,0x21,0x00,0x00,0x1E,
	0x00,0x00,0x00,0xFF,0x00,0x56,0x57,0x36,
	0x31,0x31,0x39,0x37,0x38,0x32,0x31,0x52,
	0x55,0x0A,0x00,0x00,0x00,0xFC,0x00,0x44,
	0x45,0x4C,0x4C,0x20,0x55,0x32,0x34,0x31,
	0x35,0x0A,0x20,0x20,0x00,0x00,0x00,0xFD,
	0x00,0x31,0x3D,0x1E,0x53,0x11,0x00,0x0A,
	0x20,0x20,0x20,0x20,0x20,0x20,0x01,0x9E,
        /* CEA Extension block */
	0x02,0x03,0x22,0xF1,0x4F,0x90,0x05,0x04,
	0x03,0x02,0x07,0x16,0x01,0x14,0x1F,0x12,
	0x13,0x20,0x21,0x22,0x23,0x09,0x07,0x07,
	0x65,0x03,0x0C,0x00,0x20,0x00,0x83,0x01,
	0x00,0x00,0x02,0x3A,0x80,0x18,0x71,0x38,
	0x2D,0x40,0x58,0x2C,0x45,0x00,0x06,0x44,
	0x21,0x00,0x00,0x1E,0x01,0x1D,0x80,0x18,
	0x71,0x1C,0x16,0x20,0x58,0x2C,0x25,0x00,
	0x06,0x44,0x21,0x00,0x00,0x9E,0x01,0x1D,
	0x00,0x72,0x51,0xD0,0x1E,0x20,0x6E,0x28,
	0x55,0x00,0x06,0x44,0x21,0x00,0x00,0x1E,
	0x8C,0x0A,0xD0,0x8A,0x20,0xE0,0x2D,0x10,
	0x10,0x3E,0x96,0x00,0x06,0x44,0x21,0x00,
	0x00,0x18,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x72
};

static void toHexString(
    const uint8_t data[], const uint32_t dataLength,
    char buf[], uint32_t* bufLength)
{
    uint32_t totalLength = (6 * dataLength) + (dataLength / 8);

    if (*bufLength >= totalLength) {
        uint32_t length = 0;

        for (uint32_t i = 0; i < dataLength; i++) {
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

void displayinfo_display_updated(VARIABLE_IS_NOT_USED void* data)
{
    Trace("Display updated callback!\n");
}

void on_operational_state_change(bool is_operational, VARIABLE_IS_NOT_USED void* data)
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
           "\tM : Parse Canned EDID\n"
           "\tT : Get total gpu ram\n"
           "\tF : Get free gpu ram\n"
           "\tX : Get display dimensions in centimeters \n"
           "\t? : Help\n"
           "\tQ : Quit\n",
        __TIMESTAMP__);
}

void binary_print(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;

    for (i = size-1; i >= 0; i--) {
        for (j = 7; j >= 0; j--) {
            byte = (b[i] >> j) & 1;
            fprintf(stdout,"%u", byte);
        }
    }
    fprintf(stdout,"\n");
    fflush(stdout);
}

void print_edid_info(displayinfo_edid_base_info_t* edid_info) {
    puts("");
    Trace("EDID Base Info : ");
    Trace("Manufacturer ID: %.*s", sizeof(edid_info->manufacturer_id), edid_info->manufacturer_id);
    Trace("Serial Number: %u" , edid_info->serial_number);
    Trace("Product Code : %u" , edid_info->product_code);
    Trace("Week of manufacture : %u" , edid_info->manufacture_week);
    Trace("Year of manufacture : %u" , edid_info->manufacture_year);
    Trace("Version : %02X" , edid_info->version);
    Trace("Revision : %02X" , edid_info->revision);
    Trace("Width in cm : %u" , edid_info->width_in_centimeters);
    Trace("Height in cm : %u" , edid_info->height_in_centimeters);
    Trace("Preferred width in pixels : %u" , edid_info->preferred_width_in_pixels);
    Trace("Preferred height in pixels : %u" , edid_info->preferred_height_in_pixels);
    Trace("Digital : %s" , (edid_info->digital == true) ? "Yes" : "No");
    if (edid_info->digital == true) {
        if (edid_info->bits_per_color != 0) {
            Trace("Bits per color : %u" , edid_info->bits_per_color);
        }
        fprintf(stdout, "<< Display type : ");
        binary_print(sizeof(edid_info->display_type), &(edid_info->display_type));
        if (edid_info->video_interface != 0) {
            fprintf(stdout, "<< Video interface : ");
            binary_print(sizeof(edid_info->video_interface), &(edid_info->video_interface));
        }
    }
}

void print_cea_info(displayinfo_edid_cea_extension_info_t* cea_info) {
    puts("");
    Trace("EDID CEA Info : ");
    Trace("Version : %02X" , cea_info->version);
    fprintf(stdout, "<< Supported color formats : ");
    binary_print(sizeof(cea_info->color_formats), &(cea_info->color_formats));
    fprintf(stdout, "<< Supported color depths on RGB 4:4:4 : ");
    binary_print(sizeof(cea_info->color_depths[DISPLAYINFO_EDID_COLOR_DEPTH_INDEX_RGB]), &cea_info->color_depths[DISPLAYINFO_EDID_COLOR_DEPTH_INDEX_RGB]);
    if(cea_info->color_formats & DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR444) {
        fprintf(stdout, "<< Supported color depths on YCbCr 4:4:4 : ");
        binary_print(sizeof(cea_info->color_depths[DISPLAYINFO_EDID_COLOR_DEPTH_INDEX_YCBCR444]), &cea_info->color_depths[DISPLAYINFO_EDID_COLOR_DEPTH_INDEX_YCBCR444]);
    }
    if(cea_info->color_formats & DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR422) {
        fprintf(stdout, "<< Supported color depths on YCbCr 4:2:2 : ");
        binary_print(sizeof(cea_info->color_depths[DISPLAYINFO_EDID_COLOR_DEPTH_INDEX_YCBCR422]), &cea_info->color_depths[DISPLAYINFO_EDID_COLOR_DEPTH_INDEX_YCBCR422]);
    }
    if(cea_info->color_formats & DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR420) {
        fprintf(stdout, "<< Supported color depths on YCbCr 4:2:0 : ");
        binary_print(sizeof(cea_info->color_depths[DISPLAYINFO_EDID_COLOR_DEPTH_INDEX_YCBCR420]), &cea_info->color_depths[DISPLAYINFO_EDID_COLOR_DEPTH_INDEX_YCBCR420]);
    }
    fprintf(stdout, "<< Supported color spaces : ");
    binary_print(sizeof(cea_info->color_spaces), &(cea_info->color_spaces));
    fprintf(stdout, "<< Supported audio formats : ");
    binary_print(sizeof(cea_info->audio_formats), &(cea_info->audio_formats));

    Trace("Supported timings (%u) : ", cea_info->number_of_timings);
    displayinfo_edid_standard_timing_t ts;
    fprintf(stdout, "<< VIC\tActive Width\tActive Height\tVF\tDAR\n");
    for (uint8_t i = 0; i < cea_info->number_of_timings; ++i) {
        if(displayinfo_edid_vic_to_standard_timing(cea_info->timings[i], &ts) == 0) {
            fprintf(stdout, "<< %u\t%u\t\t%u\t\t%u\t%u\n", ts.vic, ts.active_width, ts.active_height, ts.vertical_frequency, ts.display_aspect_ratio);
        }
    }
}

int main()
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
            if (displayinfo_connected(&is_connected) == 0) {
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
                displayinfo_edid_base_info_t edid_info;
                if (displayinfo_parse_edid(edid_dell, sizeof(edid_dell), &edid_info) == 0) {
                    print_edid_info(&edid_info);
                    displayinfo_edid_cea_extension_info_t cea_info;
                    if(displayinfo_edid_cea_extension_info(edid_dell, sizeof(edid_dell), &cea_info) == 0) {
                        print_cea_info(&cea_info);
                    } else {
                        Trace("No CEA Block Available");
                    }

                }

            } else {
                Trace("Instance or buffer or length is NULL, or invalid connection");
            }
            break;
        }

        case 'M': {
            displayinfo_edid_base_info_t edid_info;
            if (displayinfo_parse_edid(edid_dell, sizeof(edid_dell), &edid_info) == 0) {
                print_edid_info(&edid_info);
                displayinfo_edid_cea_extension_info_t cea_info;
                if(displayinfo_edid_cea_extension_info(edid_dell, sizeof(edid_dell), &cea_info) == 0) {
                    print_cea_info(&cea_info);
                } else {
                    Trace("No CEA Block Available");
                }
            } else {
                Trace("buffer or length is NULL, or invalid EDID");
            }

            if (displayinfo_parse_edid(edid_lg, sizeof(edid_lg), &edid_info) == 0) {
                Trace("");
                print_edid_info(&edid_info);
                displayinfo_edid_cea_extension_info_t cea_info;
                if(displayinfo_edid_cea_extension_info(edid_lg, sizeof(edid_lg), &cea_info) == 0) {
                    print_cea_info(&cea_info);
                } else {
                    Trace("No CEA Block Available");
                }
            } else {
                Trace("buffer or length is NULL, or invalid EDID");
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
    displayinfo_dispose();

    return 0;
}
