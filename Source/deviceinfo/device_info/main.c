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
#include <deviceinfo.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void toHexString(
    const uint8_t data[], const uint32_t dataLength,
    char buf[], uint32_t* bufLength)
{
    uint32_t totalLength = (6 * dataLength) + (dataLength / 8);

    if (*bufLength >= totalLength) {
        uint32_t length = 0;

        for (unsigned int i = 0; i < dataLength; i++) {
            length += snprintf(&buf[length], (*bufLength - length), "%s0x%02X%s",
                ((i % 8 == 0) && (i != 0)) ? "\n" : "",
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

void ShowMenu()
{
    printf("Enter\n"
           "\tI : Get ID as a string.\n"
           "\tA : Get architecture\n"
           "\tB : Get binary ID.\n"
           "\tC : Get chipset\n"
           "\tD : Get friendly name\n"
           "\tE : Get make \n"
           "\tF : Get firmware version\n"
           "\tG : Get device type\n"
           "\tN : Get serial number\n"
           "\tK : Get sku \n"
           "\tM : Get model name\n"
           "\tY : Get model year\n"
           "\tX : Get distributor Id\n"
           "\tP : Get platform name\n"
           "\tS : Get summary of available audio outputs\n"
           "\tV : Get summary of available video outputs\n"
           );
}

int main()
{
    int16_t result = 0;

    ShowMenu();

    int character;
    do {
        character = toupper(getc(stdin));

        switch (character) {
        case 'B': {
            uint8_t buffer[150];
            uint8_t bufferLength = sizeof(buffer);
            memset(buffer, 0, bufferLength);

            result = deviceinfo_id(buffer, &bufferLength);
            if (result==0 && bufferLength > 0) {
                char id[125];
                uint32_t idSize = sizeof(id);
                toHexString(buffer, bufferLength, id, &idSize);

                Trace("ID[%d]: %s", bufferLength, id);
            } else  {
                Trace("No ID available for this device, or instance or buffer is null");
            }

            break;
        }
        case 'I': {
            char bufferstr[150];
            uint8_t bufferLength = sizeof(bufferstr);
            memset(bufferstr, 0, bufferLength);
            result = deviceinfo_id_str(bufferstr, &bufferLength);

            if (result==0) {
                Trace("ID[%d]: %s", bufferLength, bufferstr);
            } else if (result == 16) {
                Trace("Buffer too small, should be at least of size %d ",bufferLength);
            } else {
                Trace("Instance or buffer is null.Error code = %d ", result);
            }

            break;
        }
        case 'C': {
            char buffer[150];
            uint8_t bufferLength = sizeof(buffer);
            memset(buffer, 0, bufferLength);

            result = deviceinfo_chipset(buffer, &bufferLength);
            if (result==0) {
                Trace("Chipset: %s", buffer);
                memset(buffer, 0, bufferLength);
            } else if (result == 16) {
                Trace("Buffer too small, should be at least of size %d ",bufferLength);
            } else {
                Trace("Instance or buffer is null.Error code = %d ", result);
            }

            break;
        }
        case 'F': {
            char buffer[150];
            uint8_t bufferLength = sizeof(buffer);
            memset(buffer, 0, bufferLength);

            result = deviceinfo_firmware_version(buffer, &bufferLength);
            if (result==0) {
                Trace("Firmware Version: %s", buffer);
                memset(buffer, 0, bufferLength);
            } else if (result == 16) {
                Trace("Buffer too small, should be at least of size %d ",bufferLength);
            } else {
                Trace("Instance or buffer is null.Error code = %d ", result);
            }
            break;
        }
        case 'A': {
            char buffer[150];
            uint8_t bufferLength = sizeof(buffer);
            memset(buffer, 0, bufferLength);

            result = deviceinfo_architecture(buffer, &bufferLength);
            if (result==0) {
                Trace("Architecure: %s", buffer);
                memset(buffer, 0, bufferLength);
           } else if (result == 16) {
                Trace("Buffer too small, should be at least of size %d ",bufferLength);
            } else {
                Trace("Instance or buffer is null.Error code = %d ", result);
            }
            break;
        }
        case 'R': {
            deviceinfo_output_resolution_t res = DEVICEINFO_RESOLUTION_UNKNOWN;
            deviceinfo_maximum_output_resolution(DEVICEINFO_VIDEO_HDMI, &res);
            Trace("Output Resolution: %d", res);
            break;
        }
        case 'S': {
            uint8_t audMaxLen = 0;
            uint8_t index = 0;

            deviceinfo_audio_output_t audio_outputs[DEVICEINFO_AUDIO_LENGTH];
            memset(audio_outputs, 0, sizeof(audio_outputs));

            audMaxLen = sizeof(audio_outputs) / sizeof(deviceinfo_audio_output_t);
            deviceinfo_audio_outputs(audio_outputs, &audMaxLen);

            Trace("\n audio outputs: [");
            for (index = 0; index < audMaxLen; index++) {
                Trace("\b %d", audio_outputs[index]);
                deviceinfo_audio_capability_t audioCapabilities[10];
                uint8_t audCapabilitiesMaxLen = sizeof(audioCapabilities);
                deviceinfo_audio_capabilities(audio_outputs[index], audioCapabilities, &audCapabilitiesMaxLen);

                Trace("\b audio capabilities:");
                for (uint8_t i = 0; i < audCapabilitiesMaxLen; ++i) {
                    Trace("\b %d", audioCapabilities[i]);
                }
                deviceinfo_audio_ms12_capability_t ms12Capabilities[10];
                uint8_t ms12CapabilitiesMaxLen = sizeof(ms12Capabilities);
                deviceinfo_audio_ms12_capabilities(audio_outputs[index], ms12Capabilities, &ms12CapabilitiesMaxLen);

                Trace("\b ms12 capabilities:");
                for (uint8_t i = 0; i < ms12CapabilitiesMaxLen; ++i) {
                    Trace("\b %d", ms12Capabilities[i]);
                }

                deviceinfo_audio_ms12_profile_t ms12Profiles[10];
                uint8_t ms12ProfilesMaxLen = sizeof(ms12Profiles);
                deviceinfo_audio_ms12_audio_profiles(audio_outputs[index], ms12Profiles, &ms12ProfilesMaxLen);
                Trace("\b ms12 profiles:");
                for (uint8_t i = 0; i < ms12ProfilesMaxLen; ++i) {
                    Trace("\b %d", ms12Profiles[i]);
                }
            }
            Trace("\b ]\n");

            break;
        }

        case 'V': {
            bool atmos = false;
            bool hdr = false;
            bool cec = false;
            uint8_t vidMaxLen = 0;
            uint8_t index = 0;
            char edid[50];
            uint8_t edidMaxLen = sizeof(edid);
            memset(edid, 0, edidMaxLen);

            deviceinfo_video_output_t video_outputs[DEVICEINFO_VIDEO_LENGTH];
            memset(video_outputs, 0, sizeof(video_outputs));

            vidMaxLen = sizeof(video_outputs) / sizeof(deviceinfo_video_output_t);
            deviceinfo_video_outputs(video_outputs, &vidMaxLen);

            deviceinfo_cec(&cec);
            deviceinfo_hdr(&hdr);
            deviceinfo_atmos(&atmos);
            deviceinfo_host_edid(edid, &edidMaxLen);

            Trace("Summary:\n atmos: %s \nhdr: %s, \ncec: %s \nedid: %s", atmos ? "available" : "unavailable", hdr ? "available" : "unavailable", cec ? "available" : "unavailable", edid);
            Trace("\nvideo outputs: [");
            for (index = 0; index < vidMaxLen; index++) {
                Trace("\b %d ", video_outputs[index]);

                deviceinfo_hdcp_t hdcp = DEVICEINFO_HDCP_UNAVAILABLE;
                deviceinfo_hdcp(video_outputs[index], &hdcp);

                Trace("\b hdcp : %d", hdcp);

                deviceinfo_output_resolution_t resolutions[DEVICEINFO_RESOLUTION_LENGTH];
                memset(resolutions, 0, sizeof(resolutions));
                uint8_t resMaxLen = sizeof(resolutions) / sizeof(deviceinfo_output_resolution_t);
                deviceinfo_output_resolutions(video_outputs[index], resolutions, &resMaxLen);

                Trace("\b supported resolutions: ");
                for (uint8_t i = 0; i < resMaxLen; ++i) {
                    Trace(" %d", resolutions[i]);
                }
                deviceinfo_output_resolution_t defaultResolution;
                deviceinfo_default_output_resolution(video_outputs[index], &defaultResolution);
                Trace("\b default resolution: %d", defaultResolution);

                deviceinfo_output_resolution_t maxResolution;
                deviceinfo_maximum_output_resolution(video_outputs[index], &maxResolution);
                Trace("\b maximum resolution: %d", maxResolution);
            }
            Trace("\b ]\n");
            break;
        }
        case 'G': {
            char buffer[25];
            uint8_t bufferLength = sizeof(buffer);
            memset(buffer, 0, bufferLength);

            result = deviceinfo_device_type(buffer, &bufferLength);
            if (result == 0) {
                Trace("device type: %s", buffer);
                memset(buffer, 0, bufferLength);
            } else if (result == 16) {
                Trace("Buffer too small, should be at least of size %d ",bufferLength);
            } else {
                Trace("Instance or buffer is null.Error code = %d ", result);
            }
            break;
        }
        case 'E': {
            char buffer[25];
            uint8_t bufferLength = sizeof(buffer);
            memset(buffer, 0, bufferLength);

            result = deviceinfo_make(buffer, &bufferLength);
            if (result == 0) {
                Trace("make: %s", buffer);
                memset(buffer, 0, bufferLength);
            } else if (result == 16) {
                Trace("Buffer too small, should be at least of size %d ",bufferLength);
            } else {
                Trace("Instance or buffer is null.Error code = %d ", result);
            }
            break;
        }
        case 'K': {
            char buffer[25];
            uint8_t bufferLength = sizeof(buffer);
            memset(buffer, 0, bufferLength);

            result = deviceinfo_sku(buffer, &bufferLength);
            if (result == 0) {
                Trace("sku: %s", buffer);
                memset(buffer, 0, bufferLength);
            } else if (result == 16) {
                Trace("Buffer too small, should be at least of size %d ",bufferLength);
            } else {
                Trace("Instance or buffer is null.Error code = %d ", result);
            }
            break;
        }
        case 'N': {
            char buffer[25];
            uint8_t bufferLength = sizeof(buffer);
            memset(buffer, 0, bufferLength);

            result = deviceinfo_serial_number(buffer, &bufferLength);
            if (result == 0) {
                Trace("serial number: %s", buffer);
                memset(buffer, 0, bufferLength);
            } else if (result == 16) {
                Trace("Buffer too small, should be at least of size %d ",bufferLength);
            } else {
                Trace("Instance or buffer is null.Error code = %d ", result);
            }
            break;
        }

        case 'M': {
            char buffer[25];
            uint8_t bufferLength = sizeof(buffer);
            memset(buffer, 0, bufferLength);

            result = deviceinfo_model_name(buffer, &bufferLength);
            if (result == 0) {
                Trace("model name: %s", buffer);
                memset(buffer, 0, bufferLength);
            } else if (result == 16) {
                Trace("Buffer too small, should be at least of size %d ",bufferLength);
            } else {
                Trace("Instance or buffer is null.Error code = %d ", result);
            }
            break;
        }
        case 'Y': {
            char buffer[25];
            uint8_t bufferLength = sizeof(buffer);
            memset(buffer, 0, bufferLength);

            result = deviceinfo_model_year(buffer, &bufferLength);
            if (result == 0) {
                Trace("model year: %s", buffer);
                memset(buffer, 0, bufferLength);
            } else if (result == 16) {
                Trace("Buffer too small, should be at least of size %d ",bufferLength);
            } else {
                Trace("Instance or buffer is null.Error code = %d ", result);
            }
            break;
        }
        case 'X': {
            char buffer[25];
            uint8_t bufferLength = sizeof(buffer);
            memset(buffer, 0, bufferLength);

            result = deviceinfo_system_integrator_name(buffer, &bufferLength);
            if (result == 0) {
                Trace("system integrator name: %s", buffer);
                memset(buffer, 0, bufferLength);
            } else if (result == 16) {
                Trace("Buffer too small, should be at least of size %d ",bufferLength);
            } else {
                Trace("Instance or buffer is null.Error code = %d ", result);
            }
            break;
        }
        case 'D': {
            char buffer[25];
            uint8_t bufferLength = sizeof(buffer);
            memset(buffer, 0, bufferLength);

            result = deviceinfo_friendly_name(buffer, &bufferLength);
              if (result == 0) {
                Trace("friendly name: %s", buffer);
                memset(buffer, 0, bufferLength);
            } else if (result == 16) {
                Trace("Buffer too small, should be at least of size %d ",bufferLength);
            } else {
                Trace("Instance or buffer is null.Error code = %d ", result);
            }
            break;
        }
        case 'P': {
            char buffer[25];
            uint8_t bufferLength = sizeof(buffer);
            memset(buffer, 0, bufferLength);

            result = deviceinfo_platform_name(buffer, &bufferLength);
             if (result == 0) {
                Trace("platform name: %s", buffer);
                memset(buffer, 0, bufferLength);
            } else if (result == 16) {
                Trace("Buffer too small, should be at least of size %d ",bufferLength);
            } else {
                Trace("Instance or buffer is null.Error code = %d ", result);
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

    Trace("Done");

    deviceinfo_dispose();

    return 0;
}
