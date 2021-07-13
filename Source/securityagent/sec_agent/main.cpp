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
#include <securityagent.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <cstring>

#define Trace(fmt, ...)                                 \
    do {                                                \
        fprintf(stdout, "<< " fmt "\n", ##__VA_ARGS__); \
        fflush(stdout);                                 \
    } while (0)


void ShowMenu()
{
    printf("Enter\n"
           "\tG : Get Token.\n"
           "\t? : Help\n"
           "\tQ : Quit\n");
}

int main(int argc, char* argv[])
{

    ShowMenu();

    int character;
    do {
        character = toupper(getc(stdin));

        switch (character) {
        case 'G': {
             uint8_t buffer[2 * 1024];
             std::string url("https://www.google.com");

             std::string tokenAsString;
             if (url.length() < sizeof(buffer)) {
                ::memset (buffer, 0, sizeof(buffer));
                ::memcpy (buffer, url.c_str(), url.length());
                int length = GetToken(static_cast<uint16_t>(sizeof(buffer)), url.length(), buffer);
                if (length > 0) {
                   tokenAsString = std::string(reinterpret_cast<const char*>(buffer), length);
                   printf("\nToken: %s\n",tokenAsString.c_str());
                }
                else
                   printf("Error Getting token - %d\n",length);
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

    return 0;
}
