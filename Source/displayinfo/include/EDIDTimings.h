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

#pragma once

namespace WPEFramework {
namespace Plugin {
namespace EDID {

    enum class displayaspectratiotype: uint8_t {
        DAR_4_TO_3,
        DAR_16_TO_9,
        DAR_64_TO_27,
        DAR_256_TO_135,
    };

    enum class verticalfrequencytype: uint16_t {
        // in milliHertzs
        VF_23980_OR_24000 = (1 << 0),
        VF_25000 = (1 << 1),
        VF_29970_OR_30000 = (1 << 2),
        VF_47960_OR_48000 = (1 << 3),
        VF_50000 = (1 << 4),
        VF_59826 = (1 << 5),
        VF_59940 = (1 << 6),
        VF_60000 = (1 << 7),
        VF_100000 = (1 << 8),
        VF_119880_OR_120000 = (1 << 9),
        VF_200000 = (1 << 10),
        VF_239760 = (1 << 11),
    };

    struct StandardTiming {
        uint8_t vic; // Video Idendification Code
        displayaspectratiotype dar;
        verticalfrequencytype verticalFrequency;
        uint16_t activeWidth;
        uint16_t activeHeight;
    };

    static constexpr StandardTiming standardTimingMap[] =
    {
        {1, displayaspectratiotype::DAR_4_TO_3, verticalfrequencytype::VF_59940, 640, 480},
        {2, displayaspectratiotype::DAR_4_TO_3, verticalfrequencytype::VF_59940, 720, 480},
        {3, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_59940, 720, 480},
        {4, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_60000, 1280, 720},
        {5, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_60000, 1920, 540},
        {6, displayaspectratiotype::DAR_4_TO_3, verticalfrequencytype::VF_59940, 1440, 240},
        {7, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_59940, 1440, 240},
        {8, displayaspectratiotype::DAR_4_TO_3, verticalfrequencytype::VF_59826, 1440, 240},
        {9, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_59826, 1440, 240},
        {10, displayaspectratiotype::DAR_4_TO_3, verticalfrequencytype::VF_59940, 2880, 240},
        {11, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_59940, 2880, 240},
        {12, displayaspectratiotype::DAR_4_TO_3, verticalfrequencytype::VF_60000, 2880, 240},
        {13, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_60000, 2880, 240},
        {14, displayaspectratiotype::DAR_4_TO_3, verticalfrequencytype::VF_59940, 1440, 480},
        {15, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_59940, 1440, 480},
        {16, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_60000, 1920, 1080},
        {17, displayaspectratiotype::DAR_4_TO_3, verticalfrequencytype::VF_50000, 720, 576},
        {18, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_50000, 720, 576},
        {19, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_50000, 1280, 720},
        {20, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_50000, 1920, 540},
        {21, displayaspectratiotype::DAR_4_TO_3, verticalfrequencytype::VF_50000, 1440, 288},
        {22, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_50000, 1440, 288},
        {23, displayaspectratiotype::DAR_4_TO_3, verticalfrequencytype::VF_50000, 1440, 288},
        {24, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_50000, 1440, 288},
        {25, displayaspectratiotype::DAR_4_TO_3, verticalfrequencytype::VF_50000, 2880, 288},
        {26, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_50000, 2880, 288},
        {27, displayaspectratiotype::DAR_4_TO_3, verticalfrequencytype::VF_50000, 2880, 288},
        {28, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_50000, 2880, 288},
        {29, displayaspectratiotype::DAR_4_TO_3, verticalfrequencytype::VF_50000, 1440, 576},
        {30, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_50000, 1440, 576},
        {31, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_50000, 1920, 1080},
        {32, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_23980_OR_24000, 1920, 1080},
        {33, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_25000, 1920, 1080},
        {34, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_29970_OR_30000, 1920, 1080},
        {35, displayaspectratiotype::DAR_4_TO_3, verticalfrequencytype::VF_59940, 2880, 240},
        {36, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_59940, 2880, 240},
        {37, displayaspectratiotype::DAR_4_TO_3, verticalfrequencytype::VF_50000, 2880, 576},
        {38, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_50000, 2880, 576},
        {39, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_50000, 1920, 540},
        {40, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_100000, 1920, 540},
        {41, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_100000, 1280, 720},
        {42, displayaspectratiotype::DAR_4_TO_3, verticalfrequencytype::VF_100000, 720, 576},
        {43, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_100000, 720, 576},
        {44, displayaspectratiotype::DAR_4_TO_3, verticalfrequencytype::VF_100000, 1440, 576},
        {45, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_100000, 1440, 576},
        {46, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_119880_OR_120000, 1920, 540},
        {47, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_119880_OR_120000, 1280, 720},
        {48, displayaspectratiotype::DAR_4_TO_3, verticalfrequencytype::VF_119880_OR_120000, 720, 576},
        {49, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_119880_OR_120000, 720, 576},
        {50, displayaspectratiotype::DAR_4_TO_3, verticalfrequencytype::VF_119880_OR_120000, 1440, 576},
        {51, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_119880_OR_120000, 1440, 576},
        {52, displayaspectratiotype::DAR_4_TO_3, verticalfrequencytype::VF_200000, 720, 576},
        {53, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_200000, 720, 576},
        {54, displayaspectratiotype::DAR_4_TO_3, verticalfrequencytype::VF_200000, 1440, 288},
        {55, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_200000, 1440, 288},
        {56, displayaspectratiotype::DAR_4_TO_3, verticalfrequencytype::VF_239760, 720, 480},
        {57, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_239760, 720, 480},
        {58, displayaspectratiotype::DAR_4_TO_3, verticalfrequencytype::VF_239760, 1440, 240},
        {59, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_239760, 1440, 240},
        {60, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_23980_OR_24000, 1280, 720},
        {61, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_25000, 1280, 720},
        {62, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_29970_OR_30000, 1280, 720},
        {63, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_119880_OR_120000, 1920, 1080},
        {64, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_100000, 1920, 1080},
        {65, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_23980_OR_24000, 1280, 720},
        {66, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_25000, 1280, 720},
        {67, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_29970_OR_30000, 1280, 720},
        {68, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_50000, 1280, 720},
        {69, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_60000, 1650, 750},
        {70, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_100000, 1280, 720},
        {71, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_119880_OR_120000, 1280, 720},
        {72, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_23980_OR_24000, 1920, 1080},
        {73, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_25000, 1920, 1080},
        {74, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_29970_OR_30000, 1920, 1080},
        {75, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_50000, 1920, 1080},
        {76, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_60000, 1920, 1080},
        {77, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_100000, 1920, 1080},
        {78, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_119880_OR_120000, 1920, 1080},
        {79, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_23980_OR_24000, 1680, 720},
        {80, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_25000, 1680, 720},
        {81, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_29970_OR_30000, 1680, 720},
        {82, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_50000, 1680, 720},
        {83, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_60000, 1680, 720},
        {84, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_100000, 1680, 720},
        {85, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_119880_OR_120000, 1680, 720},
        {86, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_23980_OR_24000, 2560, 1080},
        {87, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_25000, 2560, 1080},
        {88, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_29970_OR_30000, 2560, 1080},
        {89, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_50000, 2560, 1080},
        {90, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_60000, 2560, 1080},
        {91, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_100000, 2560, 1080},
        {92, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_119880_OR_120000, 2560, 1080},
        {93, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_23980_OR_24000, 3840, 2160},
        {94, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_25000, 3840, 2160},
        {95, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_29970_OR_30000, 3840, 2160},
        {96, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_50000, 3840, 2160},
        {97, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_60000, 3840, 2160},
        {98, displayaspectratiotype::DAR_256_TO_135, verticalfrequencytype::VF_23980_OR_24000, 4096, 2160},
        {99, displayaspectratiotype::DAR_256_TO_135, verticalfrequencytype::VF_25000, 4096, 2160},
        {100, displayaspectratiotype::DAR_256_TO_135, verticalfrequencytype::VF_29970_OR_30000, 4096, 2160},
        {101, displayaspectratiotype::DAR_256_TO_135, verticalfrequencytype::VF_50000, 4096, 2160},
        {102, displayaspectratiotype::DAR_256_TO_135, verticalfrequencytype::VF_60000, 4096, 2160},
        {103, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_23980_OR_24000, 3840, 2160},
        {104, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_25000, 3840, 2160},
        {105, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_29970_OR_30000, 3840, 2160},
        {106, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_50000, 3840, 2160},
        {107, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_60000, 3840, 2160},
        {108, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_47960_OR_48000, 1280, 720},
        {109, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_47960_OR_48000, 1280, 720},
        {110, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_47960_OR_48000, 1680, 720},
        {111, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_47960_OR_48000, 1920, 1080},
        {112, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_47960_OR_48000, 1920, 1080},
        {113, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_47960_OR_48000, 2560, 1080},
        {114, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_47960_OR_48000, 3840, 2160},
        {115, displayaspectratiotype::DAR_256_TO_135, verticalfrequencytype::VF_47960_OR_48000, 4096, 2160},
        {116, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_47960_OR_48000, 3840, 2160},
        {117, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_100000, 3840, 2160},
        {118, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_119880_OR_120000, 3840, 2160},
        {119, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_100000, 3840, 2160},
        {120, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_119880_OR_120000, 3840, 2160},
        {121, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_23980_OR_24000, 5120, 2160},
        {122, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_25000, 5120, 2160},
        {123, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_29970_OR_30000, 5120, 2160},
        {124, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_47960_OR_48000, 5120, 2160},
        {125, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_50000, 5120, 2160},
        {126, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_60000, 5120, 2160},
        {127, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_100000, 5120, 2160},
        {193, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_119880_OR_120000, 5120, 2160},
        {194, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_23980_OR_24000, 7680, 4320},
        {195, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_25000, 7680, 4320},
        {196, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_29970_OR_30000, 7680, 4320},
        {197, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_47960_OR_48000, 7680, 4320},
        {198, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_50000, 7680, 4320},
        {199, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_60000, 7680, 4320},
        {200, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_100000, 7680, 4320},
        {201, displayaspectratiotype::DAR_16_TO_9, verticalfrequencytype::VF_119880_OR_120000, 7680, 4320},
        {202, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_23980_OR_24000, 7680, 4320},
        {203, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_25000, 7680, 4320},
        {204, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_29970_OR_30000, 7680, 4320},
        {205, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_47960_OR_48000, 7680, 4320},
        {206, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_50000, 7680, 4320},
        {207, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_60000, 7680, 4320},
        {208, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_100000, 7680, 4320},
        {209, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_119880_OR_120000, 7680, 4320},
        {210, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_23980_OR_24000, 10240, 4320},
        {211, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_25000, 10240, 4320},
        {212, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_29970_OR_30000, 10240, 4320},
        {213, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_47960_OR_48000, 10240, 4320},
        {214, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_50000, 10240, 4320},
        {215, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_60000, 10240, 4320},
        {216, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_100000, 10240, 4320},
        {217, displayaspectratiotype::DAR_64_TO_27, verticalfrequencytype::VF_119880_OR_120000, 10240, 4320},
        {218, displayaspectratiotype::DAR_256_TO_135, verticalfrequencytype::VF_100000, 4096, 2160},
        {219, displayaspectratiotype::DAR_256_TO_135, verticalfrequencytype::VF_119880_OR_120000, 4096, 2160},
    };

    const StandardTiming* VICToStandardTiming(uint8_t vic)
    {
        const StandardTiming* result = nullptr;

        for(uint8_t index = 0; index < (sizeof(standardTimingMap)/sizeof(StandardTiming)); ++index) {
            if(standardTimingMap[index].vic == vic) {
                result = &standardTimingMap[index];
                break;
            }
        }
        return result;
    }

} // EDID
} // Plugin
} // WPEFramework

