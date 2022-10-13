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

#pragma once
#include "Module.h"

#include <list>
#include <string>
#include <vector>

#include "displayinfo.h"


namespace WPEFramework {
namespace Plugin {

    class ExtendedDisplayIdentification {
    public:
        static constexpr uint32_t OUI_HDMI_LICENSING = 0x000C03;
        static constexpr uint32_t OUI_HDMI_FORUM = 0xC45DD8;

        class Buffer {
        public:
            static constexpr uint16_t edid_block_size = 128;
        public:
            Buffer() {
                constexpr uint8_t init_value = 0;
                ::memcpy(_data, &init_value, sizeof(_data));
            }
            Buffer(const Buffer& copy) {
                ::memcpy(_data, copy._data, sizeof(_data));
            }
            ~Buffer() = default;

            Buffer& operator=(const Buffer& copy) {
                ::memcpy(_data, copy._data, sizeof(copy._data));
                return (*this);
            }

        public:
            uint16_t Length() const {
                return (edid_block_size);
            }
            operator uint8_t* () {
                return (_data);
            }
            operator const uint8_t* () const {
                return (_data);
            }

            bool IsValid() const {
                uint16_t sum;

                for (uint16_t index = 0; index < Length(); index++){
                    sum = (sum + _data[index]) % 256;
                }

                // The byte sum of each segment of 128 bytes equals 0 for base EDID and CEA segemnts
                return sum == 0;
            }

        private:
            uint8_t _data[edid_block_size];
        };

        using BufferList = std::list<Buffer>;

        class Iterator {
        public:
            Iterator()
                : _segments(nullptr)
                , _index()
                , _reset(true) {
            }
            Iterator(const BufferList& rhs)
                : _segments(&rhs)
                , _index(rhs.begin())
                , _reset(true) {
            }
            Iterator(const Iterator& copy)
                : _segments(copy._segments)
                , _index()
                , _reset(true) {
                if (_segments != nullptr) {
                    _index = _segments->cbegin();
                }
            }
            ~Iterator() = default;

            Iterator& operator= (const Iterator& rhs) {
                _segments = rhs._segments;
                if(_segments != nullptr) {
                    _index = rhs._segments->cbegin();
                }
                _reset = true;

                return (*this);
            }

        public:
            bool IsValid() const {
                return ((_reset == false) && (_segments != nullptr) && (_index != _segments->cend()));
            }
            void Reset() {
                _reset = true;
                if (_segments != nullptr) {
                    _index = _segments->cbegin();
                }
            }
            bool Next() {
                if (_reset == true) {
                    _reset = false;
                }
                else if ((_segments != nullptr) && (_index != _segments->cend())) {
                    _index++;
                }
                return ((_segments != nullptr) && (_index != _segments->cend()));
            }
            uint8_t Type() const {
                /*
                    0x00             : Timing Extention
                    0x02 CEA_EXT     : CEA 861 Series Extension
                    0x10 VTB-EXT     : Video Timing Block Extension
                    0x20             : EDID 2.0 Extention
                    0x40 DI-EXT      : Display Information Extension
                    0x50 LS-EXT      : Localized String Extension
                    0x60 DPVL-EXT    : Digital Packet Video Link Extension
                    0XA7, 0xAF, 0xBF : Display Transfer Characteristics Data Block (DTCDB) A7 AF BF
                    0xF0             : Extention block map
                    0xFF             : Extentions defined by the display manufacturer / Display Device Data Block (DDDB)
                */
// TODO:
                // On error anything not being a tag value previously mentioned
                return (IsValid() ? (*_index)[0] : 0xFF);
            }
            const Buffer& Current() const {
                ASSERT(IsValid() == true);
                return (*_index);
            }

        private:
            const BufferList* _segments;
            BufferList::const_iterator _index;
            bool _reset;
        };

        class CEA {
        public:
            static constexpr uint8_t extension_tag = 0x02;
        public:
            // Iterator for the CTA Data Block Colection
            class DataBlockIterator {
            public:
                /*
                    Bits 7-5 of byte 1 describes the tag
                    0       Reserved
                    1       Audio Data Block (includes one or more Short Audio Descriptors)
                    2       Video Data Block (includes one or more Short Video Descriptors)
                    3       Vendor-Specific Data Block
                    4       Speaker Allocation Data Block
                    5       VESA Display Transfer Characteristic Data Block [99]
                    6       Reserved
                    7       Use Extended Tag
                */
                /*
                    Byte 2 contains the extended tag code
                    0       Video Capability Data Block
                    1       Vendor-Specific Video Data Block
                    2       VESA Display Device Data Block (100)
                    3       VESA Video Timing Block Extension
                    4       Reserved for HDMI Video Data Block
                    5       Colorimetry Data Block
                    6       HDR Static Metadata Data Block
                    7       HDR Dynamic Metadata Data Block
                    8..12   Reserved for video-related blocks
                    13      Video Format Preference Data Block
                    14      YCBCR 4:2:0 Video Data Block
                    15      YCBCR 4:2:0 Capability Map Data Block
                    16      Reserved for CTA Miscellaneous Audio Fields
                    17      Vendor-Specific Audio Data Block
                    18      Reserved for HDMI Audio Data Block
                    19      Room Configuration Data Block
                    20      Speaker Location Data Block
                    21-31   Reserved for audio-related blocks
                    32      InfoFrame Data Block (includes one or more Short InfoFrame Descriptors)
                    33..255 Reserved
                */

                enum blocktype : uint16_t {
                    INVALID = 0,
                    AUDIO = 1,
                    VIDEO = 2,
                    VENDOR_SPECIFIC = 3,
                    SPEAKER_ALLOCATION = 4,
                    VESA = 5,
                    // Use extended Tag
                    EXTENDED = 7,
                    // All subsequent values exceed the max of first byte, eg max (255) + 1 + value
                    MARKER = 256,
                    VIDEO_CAPABILITY = (MARKER | 0),
                    VENDOR_SPECIFIC_VIDEO = (MARKER | 1),
                    VESA_DISPLAY_DEVICE = (MARKER | 2),
                    COLORIMETRY = (MARKER | 5),
                    HDR_STATIC_METADATA = (MARKER | 6),
                    HDR_DYNAMIC_METADATA = (MARKER | 7),
                    VIDEO_FORMAT_PREFERENCE = (MARKER | 13),
                    YCBCR420_VIDEO = (MARKER | 14),
                    YCBCR420_CAPABILITY_MAP = (MARKER | 15)
                };

            public:
                DataBlockIterator() = delete;
                DataBlockIterator(const Buffer& rhs, uint8_t dtdBegin)
                    : _segment(rhs)
                    , _index(4)
                    , _dtdBegin(dtdBegin) {
                        ASSERT(_index >= 4);
                }
                DataBlockIterator(const DataBlockIterator& copy)
                    : _segment(copy._segment)
                    , _index(copy._index)
                    , _dtdBegin(copy._dtdBegin) {
                        ASSERT(_index >= 4);
                }
                ~DataBlockIterator() = default;

                const DataBlockIterator& operator=(const DataBlockIterator& rhs)
                {
                    _segment = rhs._segment;
                    _index = rhs._index;
                    ASSERT(_index >= 4);
                    _dtdBegin = rhs._dtdBegin;

                    return (*this);
                }

            public:
                uint8_t BlockSize() const
                {
                    // Total data block size (following bytes + this byte) is limited to 30 or 31 bytes
                    return (IsValid() ? ((_segment[_index] & 0x1F) + 1) : 0);
                }

                blocktype BlockTag() const
                {
                    blocktype type = INVALID;

                    if (IsValid()) {
                        const uint8_t tag = (_segment[_index] >> 5);
                        if (tag == EXTENDED) {
                            if (BlockSize() >= 2) {
                                type = static_cast<blocktype>(MARKER | _segment[_index + 1]);
                            }
                        } else {
                            type = static_cast<blocktype>(tag);
                        }
                    }

                    return (type);
                }

                // Organizational Unique Identifier
                uint32_t OUI() const
                {
                    // for vendor specific only
                    if (BlockSize() >= 4) {
                        return ((_segment[_index + 1]) + (_segment[_index + 2] << 8) + (_segment[_index + 3] << 16));
                    } else {
                        return 0;
                    }
                }

                bool IsValid() const
                {
                    return (IsInRange());
                }

                bool IsInRange() const
                {
                    return ((_index < Buffer::edid_block_size) && (_index < _dtdBegin) && (_index >= 4));
                }

                void Reset()
                {
                    _index = 4;
                }

                bool Next()
                {
                    bool success = true;
                    if(IsInRange()){
                        _index += BlockSize();
                    }
                    if(BlockSize() == 0) {
                       success = false;
                    }
                    success &= IsInRange();
                    return success;
                }

                const uint8_t* Current() const
                {
                    ASSERT(IsValid() == true);
                    return (&_segment[_index]);
                }

            private:
                Buffer _segment;
                uint16_t _index;
                uint8_t _dtdBegin;
            };

        public:
            CEA() = delete;
            CEA(const CEA&) = delete;
            CEA& operator= (const CEA&) = delete;

            CEA(const Buffer& data)
                : _segment(data)
                , _colorFormats(GetColorFormats())
            {
                ASSERT(_segment[0] == extension_tag);
            }
            ~CEA() = default;

        public:
            // Revision Number
            uint8_t Version() const
            {
                return (_segment[1]);
            }

            displayinfo_edid_color_format_map_t ColorFormats() const
            {
                return (_colorFormats);
            }

            displayinfo_edid_color_depth_map_t RGBColorDepths() const
            {
                displayinfo_edid_color_depth_map_t colorDepthMap = DISPLAYINFO_EDID_COLOR_DEPTH_8_BPC;
                DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());

                do {
                    if(dataBlock.IsValid() && (dataBlock.BlockTag() == DataBlockIterator::VENDOR_SPECIFIC) && (dataBlock.BlockSize() > 6)) {
                        if(dataBlock.OUI() == OUI_HDMI_LICENSING) {
                            if(dataBlock.Current()[6] & (1 << 6)) {
                                colorDepthMap |= DISPLAYINFO_EDID_COLOR_DEPTH_16_BPC;
                            }
                            if(dataBlock.Current()[6] & (1 << 5)) {
                                colorDepthMap |= DISPLAYINFO_EDID_COLOR_DEPTH_12_BPC;
                            }
                            if(dataBlock.Current()[6] & (1 << 4)) {
                                colorDepthMap |= DISPLAYINFO_EDID_COLOR_DEPTH_10_BPC;
                            }
                            break;
                        }
                    }
                } while(dataBlock.Next());

                return colorDepthMap;
            }

            displayinfo_edid_color_depth_map_t YCbCr444ColorDepths() const
            {
                displayinfo_edid_color_depth_map_t colorDepthMap = DISPLAYINFO_EDID_COLOR_DEPTH_UNDEFINED;

                if (_colorFormats & DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR444) {
                    colorDepthMap = DISPLAYINFO_EDID_COLOR_DEPTH_8_BPC;
                    DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());

                    do {
                        if(dataBlock.IsValid() && (dataBlock.BlockTag() == DataBlockIterator::VENDOR_SPECIFIC) && (dataBlock.BlockSize() > 6)) {
                            if(dataBlock.OUI() == OUI_HDMI_LICENSING) {
                                if(dataBlock.Current()[6] & (1 << 3)) {
                                    if(dataBlock.Current()[6] & (1 << 6)) {
                                        colorDepthMap |= DISPLAYINFO_EDID_COLOR_DEPTH_16_BPC;
                                    }
                                    if(dataBlock.Current()[6] & (1 << 5)) {
                                        colorDepthMap |= DISPLAYINFO_EDID_COLOR_DEPTH_12_BPC;
                                    }
                                    if(dataBlock.Current()[6] & (1 << 4)) {
                                        colorDepthMap |= DISPLAYINFO_EDID_COLOR_DEPTH_10_BPC;
                                    }
                                }
                                break;
                            }
                        }
                    } while(dataBlock.Next());
                }

                return colorDepthMap;
            }

            displayinfo_edid_color_depth_map_t YCbCr422ColorDepths() const
            {
                displayinfo_edid_color_depth_map_t colorDepthMap = DISPLAYINFO_EDID_COLOR_DEPTH_UNDEFINED;

                if (_colorFormats & DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR422) {
                    colorDepthMap = (DISPLAYINFO_EDID_COLOR_DEPTH_8_BPC | DISPLAYINFO_EDID_COLOR_DEPTH_10_BPC | DISPLAYINFO_EDID_COLOR_DEPTH_12_BPC);
                }

                return colorDepthMap;
            }

            displayinfo_edid_color_depth_map_t YCbCr420ColorDepths() const
            {
                displayinfo_edid_color_depth_map_t colorDepthMap = DISPLAYINFO_EDID_COLOR_DEPTH_UNDEFINED;

                if (_colorFormats & DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR420) {
                    colorDepthMap = DISPLAYINFO_EDID_COLOR_DEPTH_8_BPC;
                    DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());

                    do {
                        if(dataBlock.IsValid()) {
                            if ((dataBlock.BlockTag() == DataBlockIterator::VENDOR_SPECIFIC) && (dataBlock.BlockSize() > 7)) {
                                if(dataBlock.OUI() == OUI_HDMI_FORUM) {
                                    if((dataBlock.Current()[7]) & 1) {
                                        colorDepthMap |= DISPLAYINFO_EDID_COLOR_DEPTH_10_BPC;
                                        break;
                                    }
                                    if((dataBlock.Current()[7]) & 2) {
                                        colorDepthMap |= DISPLAYINFO_EDID_COLOR_DEPTH_12_BPC;
                                        break;
                                    }
                                    if((dataBlock.Current()[7]) & 4) {
                                        colorDepthMap |= DISPLAYINFO_EDID_COLOR_DEPTH_16_BPC;
                                        break;
                                    }
                                    break;
                                }
                            }
                        }
                    } while(dataBlock.Next());
                }

                return colorDepthMap;
            }

            displayinfo_edid_color_space_map_t ColorSpaces() const
            {
                displayinfo_edid_color_space_map_t colorSpaceMap = DISPLAYINFO_EDID_COLOR_SPACE_UNDEFINED;

                DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());

                do {
                    if(dataBlock.IsValid() && (dataBlock.BlockTag() == DataBlockIterator::COLORIMETRY) && (dataBlock.BlockSize() > 3)) {
                        if(dataBlock.Current()[2]  & (1 << 0)) {
                            colorSpaceMap |= DISPLAYINFO_EDID_COLOR_SPACE_XVYCC_601;
                        }
                        if(dataBlock.Current()[2]  & (1 << 1)) {
                            colorSpaceMap |= DISPLAYINFO_EDID_COLOR_SPACE_XVYCC_709;
                        }
                        if(dataBlock.Current()[2]  & (1 << 2)) {
                            colorSpaceMap |= DISPLAYINFO_EDID_COLOR_SPACE_SYCC_601;
                        }
                        if(dataBlock.Current()[2]  & (1 << 3)) {
                            colorSpaceMap |= DISPLAYINFO_EDID_COLOR_SPACE_OP_YCC_601;
                        }
                        if(dataBlock.Current()[2]  & (1 << 4)) {
                            colorSpaceMap |= DISPLAYINFO_EDID_COLOR_SPACE_OP_RGB;
                        }
                        if(dataBlock.Current()[2]  & (1 << 5)) {
                            colorSpaceMap |= DISPLAYINFO_EDID_COLOR_SPACE_ITUR_BT_2020_CYCC;
                        }
                        if(dataBlock.Current()[2]  & (1 << 6)) {
                            colorSpaceMap |= DISPLAYINFO_EDID_COLOR_SPACE_ITUR_BT_2020_YCC;
                        }
                        if(dataBlock.Current()[2]  & (1 << 7)) {
                            colorSpaceMap |= DISPLAYINFO_EDID_COLOR_SPACE_ITUR_BT_2020_RGB;
                        }
                        if(dataBlock.Current()[3]  & (1 << 7)) {
                            colorSpaceMap |= DISPLAYINFO_EDID_COLOR_SPACE_DCI_P3;
                        }
                        break;
                    }
                } while(dataBlock.Next());
                return colorSpaceMap;
            }

            void Timings(std::vector<uint8_t>& vicList) const
            {
                DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());

                do {
                    if(dataBlock.IsValid() && (dataBlock.BlockTag() == DataBlockIterator::VIDEO)) {
                        for(uint8_t index = 1; index < dataBlock.BlockSize(); index++) {
                            const uint8_t vic = dataBlock.Current()[index];
                            if ((vic >= 128) && (vic <= 192)) {
                                vicList.push_back(vic & 0x7F);
                            } else {
                                vicList.push_back(vic);
                            }
                        }
                        break;
                    }
                } while(dataBlock.Next());
            }

            displayinfo_edid_audio_format_map_t AudioFormats() const
            {
                displayinfo_edid_audio_format_map_t audioFormatMap = 0;

                DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());

                do {
                    if(dataBlock.IsValid() && (dataBlock.BlockTag() == DataBlockIterator::AUDIO)) {
                        for(uint8_t index = 1; index < dataBlock.BlockSize(); index += 3) {
                            const uint8_t sad = (dataBlock.Current()[index] & 0x78) >> 3;
                            switch(sad){
                            case 0x01:
                                audioFormatMap |= DISPLAYINFO_EDID_AUDIO_FORMAT_LPCM;
                                break;
                            case 0x02:
                                audioFormatMap |= DISPLAYINFO_EDID_AUDIO_FORMAT_AC3;
                                break;
                            case 0x03:
                                audioFormatMap |= DISPLAYINFO_EDID_AUDIO_FORMAT_MPEG1;
                                break;
                            case 0x04:
                                audioFormatMap |= DISPLAYINFO_EDID_AUDIO_FORMAT_MP3;
                                break;
                            case 0x05:
                                audioFormatMap |= DISPLAYINFO_EDID_AUDIO_FORMAT_MPEG2;
                                break;
                            case 0x06:
                                audioFormatMap |= DISPLAYINFO_EDID_AUDIO_FORMAT_AAC_LC;
                                break;
                            case 0x07:
                                audioFormatMap |= DISPLAYINFO_EDID_AUDIO_FORMAT_DTS;
                                break;
                            case 0x08:
                                audioFormatMap |= DISPLAYINFO_EDID_AUDIO_FORMAT_ATRAC;
                                break;
                            case 0x09:
                                audioFormatMap |= DISPLAYINFO_EDID_AUDIO_FORMAT_SUPER_AUDIO_CD;
                                break;
                            case 0x0A:
                                audioFormatMap |= DISPLAYINFO_EDID_AUDIO_FORMAT_EAC3;
                                // if MPEG surround implicitly and explicitly supported: assume ATMOS
                                if((dataBlock.Current()[index + 2] & 0x01)) {
                                    audioFormatMap |= DISPLAYINFO_EDID_AUDIO_FORMAT_DOLBY_ATMOS;
                                }
                                break;
                            case 0x0B:
                                audioFormatMap |= DISPLAYINFO_EDID_AUDIO_FORMAT_DTSHD;
                                break;
                            case 0x0C:
                                audioFormatMap |= DISPLAYINFO_EDID_AUDIO_FORMAT_DOLBY_TRUEHD;
                                break;
                            case 0x0D:
                                audioFormatMap |= DISPLAYINFO_EDID_AUDIO_FORMAT_DST_AUDIO;
                                break;
                            case 0x0E:
                                audioFormatMap |= DISPLAYINFO_EDID_AUDIO_FORMAT_MS_WMA_PRO;
                                break;
                            case 0x0F:
                                switch((dataBlock.Current()[index + 2] & 0xF8) >> 3) {
                                case 0x04:
                                    audioFormatMap |= DISPLAYINFO_EDID_AUDIO_FORMAT_MPEG4_HEAAC;
                                    break;
                                case 0x05:
                                    audioFormatMap |= DISPLAYINFO_EDID_AUDIO_FORMAT_MPEG4_HEAAC_V2;
                                    break;
                                case 0x06:
                                    audioFormatMap |= DISPLAYINFO_EDID_AUDIO_FORMAT_MPEG4_ACC_LC;
                                    break;
                                case 0x07:
                                    audioFormatMap |= DISPLAYINFO_EDID_AUDIO_FORMAT_DRA;
                                    break;
                                case 0x08:
                                    audioFormatMap |= DISPLAYINFO_EDID_AUDIO_FORMAT_MPEG4_HEAAC_MPEG_SURROUND;
                                    break;
                                case 0x0A:
                                    audioFormatMap |= DISPLAYINFO_EDID_AUDIO_FORMAT_MPEG4_HEAAC_LC_MPEG_SURROUND;
                                    break;
                                case 0x0B:
                                    audioFormatMap |= DISPLAYINFO_EDID_AUDIO_FORMAT_MPEGH_3DAUDIO;
                                    break;
                                case 0x0C:
                                    audioFormatMap |= DISPLAYINFO_EDID_AUDIO_FORMAT_AC4;
                                    break;
                                case 0x0D:
                                    audioFormatMap |= DISPLAYINFO_EDID_AUDIO_FORMAT_LPCM_3DAUDIO;
                                    break;
                                default:
                                    break;
                                }
                            default:
                                break;
                            }
                        }
                        break;
                    }
                } while(dataBlock.Next());
                return audioFormatMap;
            }

            displayinfo_edid_hdr_static_metadata_t HDRStaticMetadata() const {
                displayinfo_edid_hdr_static_metadata_t metadata = {DISPLAYINFO_EDID_EOT_UNDEFINED, DISPLAYINFO_EDID_HDR_STATIC_METATTYPE_UNDEFINED, {0, 0, 0}};

                DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());

                do {
                    // Bytes 5 to 7 are optional
                    if(dataBlock.IsValid() && (dataBlock.BlockTag() == DataBlockIterator::HDR_STATIC_METADATA) && (dataBlock.BlockSize() >= 4)) {
                        /*
                        dataBlock.Current()[0] Tag code 0x7 + Length of following data block
                        dataBlock.Current()[1] Extended tag code 0x6
                        dataBlock.Current()[2] Electro-Optical Transfer Functions
                            bit 0 Traditional Gamma - SDR Luminance Range See section 6.9
                            bit 1 Traditional Gamma - HDR Luminance Range See section 6.9
                            bit 2 EOTF support as defined in SMPTE ST 2084
                            bit 3 Hybrid Log-Gamma (HLG) defined in Recommendation ITU-R BT.2100
                            bit 4 Set to 0
                            bit 5 Set to 0
                            bit 6 F37=0
                            bit 7 F37=0
                        dataBlock.Current()[3] Static Metadata Descriptors
                            bit 0 if 1 Static MetaData Type 1
                            bit 1-7 Reserved, set to 0

                        dataBlock.Current()[4] Desired Content Max Luminance Data
                            Perceptually coded value (>=0)
                        dataBlock.Current()[5] Desired Max Frame-average Luminance
                            Perceptually coded value (>=0)
                        dataBlock.Current()[6] Desired Content Min Luminance
                            Perceptually Coded Value (>=0)
                        */

                        if ((dataBlock.Current()[2] & (1 << 0)) == 1) {
                            metadata.eot |= DISPLAYINFO_EDID_EOT_TRADITIONAL_GAMMA_SDR;
                        }
                        if ((dataBlock.Current()[2] & (1 << 1)) == 1) {
                            metadata.eot |= DISPLAYINFO_EDID_EOT_TRADITIONAL_GAMMA_HDR;
                        }
                        if ((dataBlock.Current()[2] & (1 << 2)) == 1) {
                            metadata.eot |= DISPLAYINFO_EDID_EOT_SMPTE_ST_2084;
                        }
                        if ((dataBlock.Current()[2] & (1 << 3)) == 1) {
                            metadata.eot |= DISPLAYINFO_EDID_EOT_TRADITIONAL_GAMMA_HDR;
                        }

                        if ((dataBlock.Current()[3] & (1 << 0)) == 1) {
                            metadata.type |= DISPLAYINFO_EDID_HDR_STATIC_METATAYPE_TYPE0;
                        }

                        // Next are optional

                        if (dataBlock.BlockSize() >= 5) {
                            metadata.luminance.max_cv = dataBlock.Current()[4];
                        }
                        if (dataBlock.BlockSize() >= 6) {
                            metadata.luminance.average_cv = dataBlock.Current()[5];
                        }
                        if (dataBlock.BlockSize() >= 7) {
                            metadata.luminance.min_cv = dataBlock.Current()[6];
                        }

                        break;
                    }
                } while(dataBlock.Next());

                return metadata;
            }

            displayinfo_edid_hdr_dynamic_metadata_t HDRDynamicMetadata() const {
                DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());

                // Optional fields are not present (see below)
                std::map<uint16_t, std::vector<uint8_t>> map;

                do {
                    if(dataBlock.IsValid() && (dataBlock.BlockTag() == DataBlockIterator::HDR_DYNAMIC_METADATA) && (dataBlock.BlockSize() >= 6)) {
                        /*
                        dataBlock.Current()[0]  Tag code 0x7 + Length of following data block in bytes
                        dataBlock.Current()[1]  Extended tag code 0x07
                        dataBlock.Current()[2]  Length of following data for supported HDR Dynamic Metadata Type n
                        dataBlock.Current()[3]  Supported HDR Dynamic Metadata Type n LSB
                                                    Interpreted at/by Support flags
                        dataBlock.Current()[4]  Supported HDR Dynamic Metadata Type n MSB
                                                    Interpreted at/by Support flags
                        dataBlock.Current()[5]  Support Flags for HDR Dynamic Metadata Type n
                                                    bit 0-3 =   1 type_1_hdr_metadata_version
                                                                2 ts_103_433_spec_version
                                                                3 "No support flags or Optional Fields are defined and thus the length of following data is 2"
                                                                4 type_4_hdr_metadata_version
                                                    bit 4-7 Reserved
                        dataBlock.Current()[6]  Optional Fields for HDR Dynamic Metadata Type n
                                                    None are specified in CEA 861 hence the length of a Metatdata Type equals 3 bytes (LSB + MSB + Support flags)

                        2-6 are repeated for a subsequent HDR Metadata Type until all bytes in this block are processed
                        */

                        uint16_t type = dataBlock.Current()[4] << 8 & dataBlock.Current()[4];

                        switch (type) {
                            case 0x0001 :   // type_1_hdr_metadata_version
                            case 0x0002 :   // ts_103_433_spec_version
                            case 0x0004 :   // type_4_hdr_metadata_version
                                            ASSERT (dataBlock.Current()[2] == 3);
                                            map[type].push_back(dataBlock.Current()[5] & 0x0F);
                                            break;
                            case 0x0003 :   // No support flags, no optional fields
                                            ASSERT (dataBlock.Current()[2] == 2);
                                            map[type].clear();
                                            break;
                            default     :   // Not specified / unknown
                                            ASSERT(false);
                        }

                            // No break since multiple dynamic blocks may exist
                    }
                } while(dataBlock.Next());


                displayinfo_edid_hdr_dynamic_metadata_t metadata = {static_cast<displayinfo_edid_hdr_dynamic_metatype_t*>(malloc(map.size() * sizeof(displayinfo_edid_hdr_dynamic_metatype_t))), map.size()};

                if (metadata.type != nullptr && metadata.count > 0) {
                    auto it = map.cbegin();

                    for (size_t index = 0; index < map.size(); index++, it++) {
                        if (it == map.cend()) {
                            break;
                        }

                        displayinfo_edid_hdr_dynamic_metatype_t & metatype = metadata.type[index];

                        metatype.type = DISPLAYINFO_EDID_HDR_DYNAMIC_FLAG_TYPE_UNDEFINED;

                        auto & list = map[it->first];

                        switch (it->first) {
                            case 0x0001     :   {
                                                using unit_t = decltype(metatype.type_1_hdr_metadata_version);

                                                size_t count = list.size() * sizeof(unit_t);

                                                metatype.type_1_hdr_metadata_version = static_cast<unit_t>(malloc(count));

                                                if (metatype.type_1_hdr_metadata_version != nullptr) {
                                                    memcpy(metatype.type_1_hdr_metadata_version, list.data(), count);
                                                    metatype.type = DISPLAYINFO_EDID_HDR_DYNAMIC_FLAG_TYPE_1_HDR_METADATA_VERSION;
                                                }

                                                break;
                                                }
                            case 0x0002     :   {
                                                using unit_t = decltype(metatype.ts_103_433_spec_version);

                                                size_t count = list.size() * sizeof(unit_t);

                                                metatype.ts_103_433_spec_version = static_cast<unit_t>(malloc(count));

                                                if (metatype.ts_103_433_spec_version != nullptr) {
                                                    memcpy(metatype.ts_103_433_spec_version, list.data(), count);
                                                    metatype.type = DISPLAYINFO_EDID_HDR_DYNAMIC_FLAG_TYPE_TS_103__433_SPEC_VERSION;
                                                }

                                                break;
                                                }
                            case 0x0004     :   {
                                                using unit_t = decltype(metatype.type_4_hdr_metadata_version);

                                                size_t count = list.size() * sizeof(unit_t);

                                                metatype.type_4_hdr_metadata_version = static_cast<unit_t>(malloc(count));

                                                if (metatype.type_4_hdr_metadata_version != nullptr) {
                                                    memcpy(metatype.type_4_hdr_metadata_version, list.data(), count);
                                                    metatype.type = DISPLAYINFO_EDID_HDR_DYNAMIC_FLAG_TYPE_4_HDR_METADATA_VERSION;
                                                }

                                                break;
                                                }
                            case 0x0003     :   {// No interpretation specified
                                                metatype.type = DISPLAYINFO_EDID_HDR_DYNAMIC_FLAG_TYPE_3_HDR_METADATA_VERSION;
                                                break;
                                                }
                            default         :   // Unknown
                                                ;
                        }
                    }
                }
                else {
                    free(metadata.type);
                    metadata.type = nullptr;
                    metadata.count = 0;
                }

                return metadata;
            }

        private:

            uint8_t DetailedTimingDescriptorStart() const
            {
                return(_segment[2]);
            }

            displayinfo_edid_color_format_map_t GetColorFormats() const
            {
                displayinfo_edid_color_format_map_t colorFormatMap = DISPLAYINFO_EDID_COLOR_FORMAT_RGB;

                if(Version() >= 2) {
                    if(_segment[3] & (1 << 4)) {
                        colorFormatMap |= DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR444;
                    }
                    if(_segment[3] & (1 << 5)) {
                        colorFormatMap |= DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR422;
                    }
                }

                DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());

                do {
                    if(dataBlock.IsValid()) {
                        if((dataBlock.BlockTag() == DataBlockIterator::VENDOR_SPECIFIC) && (dataBlock.BlockSize() > 7)) {
                            if(dataBlock.OUI() == OUI_HDMI_FORUM) {
                                if((dataBlock.Current()[7]) & 7) {
                                    colorFormatMap |= DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR420;
                                    break;
                                }
                            }
                        } else if(dataBlock.BlockTag() == DataBlockIterator::YCBCR420_CAPABILITY_MAP) {
                            colorFormatMap |= DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR420;
                            break;
                        }
                    }
                } while(dataBlock.Next());
                return colorFormatMap;
            }

        private:
            Buffer _segment;
            displayinfo_edid_color_format_map_t _colorFormats;
        };

    public:
        ExtendedDisplayIdentification (const ExtendedDisplayIdentification&) = delete;
        ExtendedDisplayIdentification& operator= (const ExtendedDisplayIdentification&) = delete;

        ExtendedDisplayIdentification()
            : _segments() {
            // It is already invalid if the first byte != 0x00
            _base[0] = 0x55;
        }
        ~ExtendedDisplayIdentification() {
        }

    public:
        // -------------------------------------------------------------
        // Only use the accessors if this method return true!!!
        // -------------------------------------------------------------
        inline bool IsValid() const {
            return ( (_base[0] == 0x00) &&
                     (_base[1] == 0xFF) &&
                     (_base[2] == 0xFF) &&
                     (_base[3] == 0xFF) &&
                     (_base[4] == 0xFF) &&
                     (_base[5] == 0xFF) &&
                     (_base[6] == 0xFF) &&
                     (_base[7] == 0x00) &&
                     // Only checksum calculation of the base EDID and CEA are known
                     _base.IsValid() != false);
        }

        uint16_t Raw(const uint16_t length, uint8_t data[]) const {
            uint16_t written = 0;

            // We always have a base if it is valid...
            if (IsValid() == true) {
                uint16_t segment = std::min(Length(), length);
                BufferList::const_iterator index(_segments.cbegin());

                // By definition, we can copy the base...
                ::memcpy(data, _base, segment);
                written = segment;

                while ( (written < length) && (index != _segments.cend()) ) {
                    segment = std::min(Length(), static_cast<uint16_t>(length - written));
                    ::memcpy(&(data[written]), *index, segment);
                    index++;
                    written += segment;
                }
            }

            return (written);
        }

        // -------------------------------------------------------------
        // Accessors to the base information of the EDID raw buffer.
        // -------------------------------------------------------------
        string Manufacturer() const {
            string result;
            if (IsValid() == true) {
                uint16_t value = ((_base[0x08] << 8) | (_base[0x09]));
                result  = ManufactereChar(value >> 10);
                result += ManufactereChar(value >> 5);
                result += ManufactereChar(static_cast<uint8_t>(value));
            }
            return (result);
        }
        uint16_t ProductCode() const {
            uint16_t result = ~0;

            if (IsValid() == true) {
                result = _base[0x0B] << 8 |
                         _base[0x0A];
            }
            return (result);
        }
        uint32_t Serial() const {
            uint32_t result = ~0;
            if (IsValid() == true) {
                result = _base[0x0F] << 24 |
                         _base[0x0E] << 16 |
                         _base[0x0D] << 8  |
                         _base[0x0C];
            }
            return (result);
        }
        // If the Week = 0xFF, it means the year is the year when this model was
        // release. If 0 <= week <= 53 the year represnt the year when the device
        // was manufactured.
        uint8_t Week() const {
            return (IsValid() ? _base[0x10] : 0x00);
        }
        uint16_t Year() const {
            return (IsValid() ? 1990 + _base[0x11] : 0x00);
        }
        uint8_t Major() const {
            return (IsValid() ? _base[0x12] : 0x00);
        }
        uint8_t Minor() const {
            return (IsValid() ? _base[0x13] : 0x00);
        }
        bool Digital() const {
            return ((_base[0x14] & 0x80) != 0);
        }

        uint8_t BitsPerColor() const {
            static uint8_t bitsPerColor[] = { 0, 6, 8, 10, 12, 14, 16, 255 };
            return (bitsPerColor[(_base[0x14] >> 4) & 0x7]);
        }

        uint8_t ColorDepth() const {
            uint8_t colorDepth = DISPLAYINFO_EDID_COLOR_DEPTH_UNDEFINED;
            if(Digital() && (Minor() >= 4)) {
                switch((_base[0x14] >> 4) & 0x07) {
                case 0x01:
                    colorDepth = DISPLAYINFO_EDID_COLOR_DEPTH_6_BPC;
                    break;
                case 0x02:
                    colorDepth = DISPLAYINFO_EDID_COLOR_DEPTH_8_BPC;
                    break;
                case 0x03:
                    colorDepth = DISPLAYINFO_EDID_COLOR_DEPTH_10_BPC;
                    break;
                case 0x04:
                    colorDepth = DISPLAYINFO_EDID_COLOR_DEPTH_12_BPC;
                    break;
                case 0x05:
                    colorDepth = DISPLAYINFO_EDID_COLOR_DEPTH_14_BPC;
                    break;
                case 0x06:
                    colorDepth = DISPLAYINFO_EDID_COLOR_DEPTH_16_BPC;
                    break;
                default:
                    colorDepth = DISPLAYINFO_EDID_COLOR_DEPTH_UNDEFINED;
                    break;
                }
            }

            return colorDepth;
        }

        uint8_t DisplayType() const {
            uint8_t displayType = 0;

            if(Digital()){
                if (Minor() >= 4) {
                    displayType = static_cast<uint8_t>(DISPLAYINFO_EDID_COLOR_FORMAT_RGB);
                    if(_base[0x18] & 8) {
                        displayType |= static_cast<uint8_t>(DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR444);
                    }
                    if(_base[0x18] & 16) {
                        displayType |= static_cast<uint8_t>(DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR422);
                    }
                } else {
                    if (_base[0x18] & 8) {
                        displayType = static_cast<uint8_t>(DISPLAYINFO_EDID_COLOR_FORMAT_RGB);
                    }
                }
            }

            return displayType;
        }

        displayinfo_edid_video_interface_t VideoInterface() const {
            return (Minor() >= 4? static_cast<displayinfo_edid_video_interface_t> (_base[0x14] & 0x0F) : DISPLAYINFO_EDID_VIDEO_INTERFACE_UNDEFINED);
        }

        Iterator Extensions() const {
            return (Iterator(_segments));
        }

        // -------------------------------------------------------------
        // Operators to get access to the EDID strorage raw information.
        // -------------------------------------------------------------
        uint16_t Length () const {
            return (_base.Length());
        }
        // Total segments including the base EDID
        uint8_t Segments() const {
            return (IsValid() ? _base[0x7e] + 1 : 1);
        }
        uint8_t* Segment(const uint8_t index) {
            uint8_t* result = nullptr;

            ASSERT (index <= Segments());

            if (index == 0) {
                result = _base.operator uint8_t* ();
            }
            else if (index <= Segments()) {
                BufferList::iterator pointer = _segments.begin();
                uint8_t current = 1;
                while (current <= index) {
                    if (pointer != _segments.end()) {
                        pointer++;
                    }
                    else {
                        _segments.emplace_back();
                    }
                    current++;
                }
                result = (pointer != _segments.end() ? pointer->operator uint8_t*() : _segments.back().operator uint8_t* ());
            }

            return (result);
        }

        Iterator CEASegment() const {
            return CEASegment(Iterator(_segments));
        }

        // Multiple CEA segments may exist
        Iterator CEASegment(const Iterator& it) const {
            Iterator index = it;
            while(index.Next() == true) {
                if(index.Type() == CEA::extension_tag && index.Current().IsValid() != false) {
                    break;
                }
            }
            return index;
        }

        uint8_t WidthInCentimeters() const {
            return IsValid() ? _base[21] : 0;
        }

        uint8_t HeightInCentimeters() const {
            return IsValid() ? _base[22] : 0;
        }

        // EDID v1.3 - https://glenwing.github.io/docs/VESA-EEDID-A1.pdf
        // EDID v1.4 - https://glenwing.github.io/docs/VESA-EEDID-A2.pdf
        // According the VESA standard:
        //
        // 3.10.1 First Detailed Timing Descriptor Block
        // The first Detailed Timing (at addresses 36h→47h) shall only be used to indicate the mode
        // that the monitor vendor has determined will give an optimal image. For LCD monitors,
        // this will in most cases be the panel "native timing" and “native resolution”.
        // Use of the EDID Preferred Timing bit shall be used to indicate that the timing indeed
        // conforms to this definition.
        uint16_t PreferredWidthInPixels() const {
            return IsValid() ? (((_base[0x3A] & 0xF0 ) << 4) + _base[0x38]) : 0;
        }

        uint16_t PreferredHeightInPixels() const {
            return IsValid() ? (((_base[0x3D] & 0xF0 ) << 4) + _base[0x3B]) : 0;
        }

        void Clear() {
            _base[0] = 0x55;
            _segments.clear();
        }

    private:
        inline TCHAR ManufactereChar(uint8_t value) const {
            return static_cast<TCHAR>('A' + ((value - 1) & 0x1F));
        }

    private:
        Buffer _base;
        BufferList _segments;
    };
}
}
