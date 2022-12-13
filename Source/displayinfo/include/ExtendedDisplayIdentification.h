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
#include <cstring>

#include "displayinfo.h"


namespace WPEFramework {
namespace Plugin {

    class ExtendedDisplayIdentification {
    public:
        // http://standards-oui.ieee.org/oui/oui.txt
        static constexpr uint32_t OUI_HDMI_LICENSING = 0x000C03;    // HDMI Licensing, LLC, data block contains HDMI 1.4 info
        static constexpr uint32_t OUI_HDMI_FORUM = 0xC45DD8;        // HDMI Forum, data block contains HDMI 2.0 info
        static constexpr uint32_t OUI_HDMI_HDR10PLUS = 0x90848B;    // HDR10+ Technologies, LLC, data block contains HDR10+ info as part of HDMI 2.1 Amendment A1 standard
        static constexpr uint32_t OUI_HDMI_DOLBY = 0x00D046;        // DOLBY LABORATORIES, INC.i, data block contains Dolby Vision Info

        class Buffer {
        public:
            static constexpr uint16_t edid_block_size = 128;
        public:
            Buffer()
            {
                constexpr uint8_t init_value = 0;
                std::memcpy(_data, &init_value, sizeof(_data));
            }
            Buffer(const Buffer& copy)
            {
                std::memcpy(_data, copy._data, sizeof(_data));
            }
            ~Buffer() = default;

            Buffer& operator=(const Buffer& copy)
            {
                std::memcpy(_data, copy._data, sizeof(copy._data));
                return (*this);
            }

        public:
            uint16_t Length() const
            {
                return (edid_block_size);
            }
            operator uint8_t* ()
            {
                return (_data);
            }
            operator const uint8_t* () const
            {
                return (_data);
            }

            bool IsValid() const
            {
                uint16_t sum = 0;

                for (uint16_t index = 0, count = Length(); index < count; index++){
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
                , _reset(true)
            {
            }
            Iterator(const BufferList& rhs)
                : _segments(&rhs)
                , _index(rhs.begin())
                , _reset(true)
            {
            }
            Iterator(const Iterator& copy)
                : _segments(copy._segments)
                , _index()
                , _reset(true)
            {
                if (_segments != nullptr) {
                    _index = _segments->cbegin();
                }
            }
            ~Iterator() = default;

            Iterator& operator= (const Iterator& rhs)
            {
                _segments = rhs._segments;
                if(_segments != nullptr) {
                    _index = rhs._segments->cbegin();
                }
                _reset = true;

                return (*this);
            }

        public:
            bool IsValid() const
            {
                return (   !_reset
                        && (_segments != nullptr)
                        && (_index != _segments->cend())
                       );
            }
            void Reset()
            {
                _reset = true;
                if (_segments != nullptr) {
                    _index = _segments->cbegin();
                }
            }
            bool Next()
            {
                if (_reset) {
                    _reset = false;
                }
                else if ((_segments != nullptr) && (_index != _segments->cend())) {
                    _index++;
                }
                return ((_segments != nullptr) && (_index != _segments->cend()));
            }
            uint8_t Type() const
            {
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
// FIXME:
                // On error anything not being a tag value previously depicted
                return (IsValid() ? (*_index)[0] : 0xFF);
            }
            const Buffer& Current() const
            {
                ASSERT(IsValid());
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
                    , _dtdBegin(dtdBegin)
                {
                    ASSERT(_index >= 4);
                }
                DataBlockIterator(const DataBlockIterator& copy)
                    : _segment(copy._segment)
                    , _index(copy._index)
                    , _dtdBegin(copy._dtdBegin)
                {
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
                    return success && IsInRange();
                }

                const uint8_t* Current() const
                {
                    ASSERT(IsValid());
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
                displayinfo_edid_color_depth_map_t colorDepthMap = DISPLAYINFO_EDID_COLOR_DEPTH_UNDEFINED;

                if ((_colorFormats & DISPLAYINFO_EDID_COLOR_FORMAT_RGB) == DISPLAYINFO_EDID_COLOR_FORMAT_RGB) {
                    DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());

                    do {
                        if (   dataBlock.IsValid()
                            && (dataBlock.BlockTag() == DataBlockIterator::VENDOR_SPECIFIC)
                            && (dataBlock.BlockSize() > 6)
                           ) {
                            if (dataBlock.OUI() == OUI_HDMI_LICENSING) {
                                // HDMI 1.0
                                displayinfo_edid_color_depth_map_t colorDepthMap = DISPLAYINFO_EDID_COLOR_DEPTH_8_BPC;

                                if (dataBlock.Current()[6] & (1 << 6)) {
                                    colorDepthMap |= DISPLAYINFO_EDID_COLOR_DEPTH_16_BPC;
                                }
                                if (dataBlock.Current()[6] & (1 << 5)) {
                                    colorDepthMap |= DISPLAYINFO_EDID_COLOR_DEPTH_12_BPC;
                                }
                                if (dataBlock.Current()[6] & (1 << 4)) {
                                    colorDepthMap |= DISPLAYINFO_EDID_COLOR_DEPTH_10_BPC;
                                }
                                break;
                            }
                        }
                    } while (dataBlock.Next());
                }

                return colorDepthMap;
            }

            displayinfo_edid_color_depth_map_t YCbCr444ColorDepths() const
            {
                displayinfo_edid_color_depth_map_t colorDepthMap = DISPLAYINFO_EDID_COLOR_DEPTH_UNDEFINED;

                if ((_colorFormats & DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR444) == DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR444) {
                    DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());

                    do {
                        if (   dataBlock.IsValid()
                            && (dataBlock.BlockTag() == DataBlockIterator::VENDOR_SPECIFIC)
                            && (dataBlock.BlockSize() > 6)
                           ) {
                            if (dataBlock.OUI() == OUI_HDMI_LICENSING) {
                                // HDMI 1.0
                                displayinfo_edid_color_depth_map_t colorDepthMap = DISPLAYINFO_EDID_COLOR_DEPTH_8_BPC;

                                if (dataBlock.Current()[6] & (1 << 3)) {
                                    if (dataBlock.Current()[6] & (1 << 6)) {
                                        colorDepthMap |= DISPLAYINFO_EDID_COLOR_DEPTH_16_BPC;
                                    }
                                    if (dataBlock.Current()[6] & (1 << 5)) {
                                        colorDepthMap |= DISPLAYINFO_EDID_COLOR_DEPTH_12_BPC;
                                    }
                                    if (dataBlock.Current()[6] & (1 << 4)) {
                                        colorDepthMap |= DISPLAYINFO_EDID_COLOR_DEPTH_10_BPC;
                                    }
                                }
                                break;
                            }
                        }
                    } while (dataBlock.Next());
                }

                return colorDepthMap;
            }

            displayinfo_edid_color_depth_map_t YCbCr422ColorDepths() const
            {
                displayinfo_edid_color_depth_map_t colorDepthMap = DISPLAYINFO_EDID_COLOR_DEPTH_UNDEFINED;

                if ((_colorFormats & DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR422) == DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR422) {
                    DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());

                    do {
                        if (   dataBlock.IsValid()
                            && (dataBlock.BlockTag() == DataBlockIterator::VENDOR_SPECIFIC)
                            && (dataBlock.BlockSize() > 6)
                           ) {
                            if (dataBlock.OUI() == OUI_HDMI_LICENSING) {
                                // HDMI 1.0
                                colorDepthMap = (DISPLAYINFO_EDID_COLOR_DEPTH_8_BPC | DISPLAYINFO_EDID_COLOR_DEPTH_10_BPC | DISPLAYINFO_EDID_COLOR_DEPTH_12_BPC);

                                if (dataBlock.Current()[6] & (1 << 3)) {
                                    if (dataBlock.Current()[6] & (1 << 6)) {
                                        colorDepthMap |= DISPLAYINFO_EDID_COLOR_DEPTH_16_BPC;
                                    }
                                    if (dataBlock.Current()[6] & (1 << 5)) {
                                        colorDepthMap |= DISPLAYINFO_EDID_COLOR_DEPTH_12_BPC;
                                    }
                                    if (dataBlock.Current()[6] & (1 << 4)) {
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

            displayinfo_edid_color_depth_map_t YCbCr420ColorDepths() const
            {
                displayinfo_edid_color_depth_map_t colorDepthMap = DISPLAYINFO_EDID_COLOR_DEPTH_UNDEFINED;

                if ((_colorFormats & DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR420) == DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR420) {
                    colorDepthMap = DISPLAYINFO_EDID_COLOR_DEPTH_8_BPC;
                    DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());

                    do {
                        if (   dataBlock.IsValid()
                            && (dataBlock.BlockTag() == DataBlockIterator::VENDOR_SPECIFIC)
                                && (dataBlock.BlockSize() > 7)
                           ) {
                            if (dataBlock.OUI() == OUI_HDMI_FORUM) {
                                if ((dataBlock.Current()[7]) & 1) {
                                    colorDepthMap |= DISPLAYINFO_EDID_COLOR_DEPTH_10_BPC;
                                    break;
                                }
                                if ((dataBlock.Current()[7]) & 2) {
                                    colorDepthMap |= DISPLAYINFO_EDID_COLOR_DEPTH_12_BPC;
                                    break;
                                }
                                if ((dataBlock.Current()[7]) & 4) {
                                    colorDepthMap |= DISPLAYINFO_EDID_COLOR_DEPTH_16_BPC;
                                    break;
                                }
                                break;
                            }
                        }
                    } while (dataBlock.Next());
                }

                return colorDepthMap;
            }

            displayinfo_edid_color_space_map_t ColorSpaces() const
            {
                displayinfo_edid_color_space_map_t colorSpaceMap = DISPLAYINFO_EDID_COLOR_SPACE_UNDEFINED;

                DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());

                do {
                    if (   dataBlock.IsValid()
                        && (dataBlock.BlockTag() == DataBlockIterator::COLORIMETRY)
                        && (dataBlock.BlockSize() > 3)
                       ) {
                        if (dataBlock.Current()[2]  & (1 << 0)) {
                            colorSpaceMap |= DISPLAYINFO_EDID_COLOR_SPACE_XVYCC_601;
                        }
                        if (dataBlock.Current()[2]  & (1 << 1)) {
                            colorSpaceMap |= DISPLAYINFO_EDID_COLOR_SPACE_XVYCC_709;
                        }
                        if (dataBlock.Current()[2]  & (1 << 2)) {
                            colorSpaceMap |= DISPLAYINFO_EDID_COLOR_SPACE_SYCC_601;
                        }
                        if (dataBlock.Current()[2]  & (1 << 3)) {
                            colorSpaceMap |= DISPLAYINFO_EDID_COLOR_SPACE_OP_YCC_601;
                        }
                        if (dataBlock.Current()[2]  & (1 << 4)) {
                            colorSpaceMap |= DISPLAYINFO_EDID_COLOR_SPACE_OP_RGB;
                        }
                        if (dataBlock.Current()[2]  & (1 << 5)) {
                            colorSpaceMap |= DISPLAYINFO_EDID_COLOR_SPACE_ITUR_BT_2020_CYCC;
                        }
                        if (dataBlock.Current()[2]  & (1 << 6)) {
                            colorSpaceMap |= DISPLAYINFO_EDID_COLOR_SPACE_ITUR_BT_2020_YCC;
                        }
                        if (dataBlock.Current()[2]  & (1 << 7)) {
                            colorSpaceMap |= DISPLAYINFO_EDID_COLOR_SPACE_ITUR_BT_2020_RGB;
                        }
                        if (dataBlock.Current()[3]  & (1 << 7)) {
                            colorSpaceMap |= DISPLAYINFO_EDID_COLOR_SPACE_DCI_P3;
                        }
                        break;
                    }
                } while (dataBlock.Next());
                return colorSpaceMap;
            }

            void Timings(std::vector<uint8_t>& vicList) const
            {
                DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());

                do {
                    if (   dataBlock.IsValid()
                        && (dataBlock.BlockTag() == DataBlockIterator::VIDEO)
                       ) {
                        for (uint8_t index = 1; index < dataBlock.BlockSize(); index++) {
                            const uint8_t vic = dataBlock.Current()[index];
                            if ((vic >= 128) && (vic <= 192)) {
                                vicList.push_back(vic & 0x7F);
                            } else {
                                vicList.push_back(vic);
                            }
                        }
                        break;
                    }
                } while (dataBlock.Next());
            }

            displayinfo_edid_audio_format_map_t AudioFormats() const
            {
                displayinfo_edid_audio_format_map_t audioFormatMap = 0;

                DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());

                do {
                    if (   dataBlock.IsValid()
                        && (dataBlock.BlockTag() == DataBlockIterator::AUDIO)
                       ) {
                        for (uint8_t index = 1; index < dataBlock.BlockSize(); index += 3) {
                            const uint8_t sad = (dataBlock.Current()[index] & 0x78) >> 3;
                            switch(sad) {
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
                                if ((dataBlock.Current()[index + 2] & 0x01)) {
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
                } while (dataBlock.Next());
                return audioFormatMap;
            }

            displayinfo_edid_hdr_static_metadata_t HDRStaticMetadata() const
            {
                displayinfo_edid_hdr_static_metadata_t metadata = {DISPLAYINFO_EDID_EOT_UNDEFINED, DISPLAYINFO_EDID_HDR_STATIC_METATTYPE_UNDEFINED, {0, 0, 0}};

                DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());

                do {
                    // Bytes 5 to 7 are optional
                    if (   dataBlock.IsValid()
                        && (dataBlock.BlockTag() == DataBlockIterator::HDR_STATIC_METADATA)
                        && (dataBlock.BlockSize() >= 4)
                       ) {
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

                        if (dataBlock.Current()[2] & (1 << 0)) {
                            metadata.eot |= DISPLAYINFO_EDID_EOT_TRADITIONAL_GAMMA_SDR;
                        }
                        if (dataBlock.Current()[2] & (1 << 1)) {
                            metadata.eot |= DISPLAYINFO_EDID_EOT_TRADITIONAL_GAMMA_HDR;
                        }
                        if (dataBlock.Current()[2] & (1 << 2)) {
                            metadata.eot |= DISPLAYINFO_EDID_EOT_SMPTE_ST_2084;
                        }
                        if (dataBlock.Current()[2] & (1 << 3)) {
                            metadata.eot |= DISPLAYINFO_EDID_EOT_TRADITIONAL_GAMMA_HDR;
                        }
                        if (dataBlock.Current()[3] & (1 << 0)) {
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
                } while (dataBlock.Next());

                return metadata;
            }

            displayinfo_edid_hdr_dynamic_metadata_t HDRDynamicMetadata() const
            {
                DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());

                // Optional fields are not present (see below)
                std::map<uint16_t, std::vector<uint8_t>> map;

                do {
                    if (   dataBlock.IsValid()
                        && (dataBlock.BlockTag() == DataBlockIterator::HDR_DYNAMIC_METADATA)
                        && (dataBlock.BlockSize() >= 6)
                       ) {
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

                        const uint16_t type = dataBlock.Current()[4] << 8 & dataBlock.Current()[4];

                        switch (type) {
                        case 0x0001 :   // type_1_hdr_metadata_version
                        case 0x0002 :   // ts_103_433_spec_version
                        case 0x0004 :   // type_4_hdr_metadata_version
                                        ASSERT (dataBlock.Current()[2] == 0x3);
                                        map[type].push_back(dataBlock.Current()[5] & 0x0F);
                                        break;
                        case 0x0003 :   // No support flags, no optional fields
                                        ASSERT (dataBlock.Current()[2] == 0x2);
                                        map[type].clear();
                                        break;
                        default     :   // Not specified / unknown
                                        ASSERT(false);
                        }

                        // No break since multiple dynamic blocks may exist
                    }
                } while (dataBlock.Next());


                displayinfo_edid_hdr_dynamic_metadata_t metadata = {static_cast<displayinfo_edid_hdr_dynamic_metatype_t*>(malloc(map.size() * sizeof(displayinfo_edid_hdr_dynamic_metatype_t))), map.size()};

                if (   metadata.type != nullptr
                    && metadata.count > 0
                   ) {
                    auto it = map.cbegin();

                    for (size_t index = 0; index < map.size(); index++, it++) {
                        if (it == map.cend()) {
                            break;
                        }

                        displayinfo_edid_hdr_dynamic_metatype_t& metatype = metadata.type[index];

                        metatype.type = DISPLAYINFO_EDID_HDR_DYNAMIC_FLAG_TYPE_UNDEFINED;

                        const auto& list = map[it->first];

                        switch (it->first) {
                        case 0x0001     :   {
                                                using unit_t = decltype(metatype.type_1_hdr_metadata_version);

                                                const size_t count = list.size() * sizeof(unit_t);

                                                metatype.type_1_hdr_metadata_version = static_cast<unit_t>(malloc(count));

                                                if (metatype.type_1_hdr_metadata_version != nullptr) {
                                                    std::memcpy(metatype.type_1_hdr_metadata_version, list.data(), count);
                                                    metatype.type = DISPLAYINFO_EDID_HDR_DYNAMIC_FLAG_TYPE_1_HDR_METADATA_VERSION;
                                                }

                                                break;
                                            }
                        case 0x0002     :   {
                                                using unit_t = decltype(metatype.ts_103_433_spec_version);

                                                const size_t count = list.size() * sizeof(unit_t);

                                                metatype.ts_103_433_spec_version = static_cast<unit_t>(malloc(count));

                                                if (metatype.ts_103_433_spec_version != nullptr) {
                                                    std::memcpy(metatype.ts_103_433_spec_version, list.data(), count);
                                                    metatype.type = DISPLAYINFO_EDID_HDR_DYNAMIC_FLAG_TYPE_TS_103__433_SPEC_VERSION;
                                                }

                                                break;
                                            }
                        case 0x0004     :   {
                                                using unit_t = decltype(metatype.type_4_hdr_metadata_version);

                                                const size_t count = list.size() * sizeof(unit_t);

                                                metatype.type_4_hdr_metadata_version = static_cast<unit_t>(malloc(count));

                                                if (metatype.type_4_hdr_metadata_version != nullptr) {
                                                    std::memcpy(metatype.type_4_hdr_metadata_version, list.data(), count);
                                                    metatype.type = DISPLAYINFO_EDID_HDR_DYNAMIC_FLAG_TYPE_4_HDR_METADATA_VERSION;
                                                }

                                                break;
                                            }
                        case 0x0003     :   {   // No interpretation specified
                                                metatype.type = DISPLAYINFO_EDID_HDR_DYNAMIC_FLAG_TYPE_3_HDR_METADATA_VERSION;
                                                break;
                                            }
                        default         :   // Unknown
                                            ;
                        }
                    }
                } else {
                    free(metadata.type);
                    metadata.type = nullptr;
                    metadata.count = 0;
                }

                return metadata;
            }

            displayinfo_edid_hdr_licensor_map_t HDRSupportLicensors() const
            {
                displayinfo_edid_hdr_licensor_map_t licensors = DISPLAYINFO_EDID_HDR_LICENSOR_NONE;

                DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());

                do {
                    if (   dataBlock.IsValid()
                        && (dataBlock.BlockTag() == DataBlockIterator::VENDOR_SPECIFIC)
                        && (dataBlock.BlockSize() > 6)
                       ) {
                        switch (dataBlock.OUI()) {
                        case OUI_HDMI_LICENSING : licensors |= DISPLAYINFO_EDID_HDR_LICENSOR_HDMI_LICENSING_LLC;
                                                  break;
                        case OUI_HDMI_FORUM     : licensors |= DISPLAYINFO_EDID_HDR_LICENSOR_HDMI_FORUM;
                                                  break;
                        case OUI_HDMI_HDR10PLUS : licensors |= DISPLAYINFO_EDID_HDR_LICENSOR_HDR10PLUS_LLC;
                                                  break;
                        case OUI_HDMI_DOLBY     : licensors |= DISPLAYINFO_EDID_HDR_LICENSOR_DOLBY_LABORATORIES_INC;
                                                  break;
                        default                 :;
                        }
                    }
                } while (dataBlock.Next());

                return licensors;
            }

        private:

            uint8_t DetailedTimingDescriptorStart() const
            {
                return(_segment[2]);
            }

            displayinfo_edid_color_format_map_t GetColorFormats() const
            {
                displayinfo_edid_color_format_map_t colorFormatMap = DISPLAYINFO_EDID_COLOR_FORMAT_UNDEFINED;

                if (_segment.IsValid()) {
                    // CEA default
                    colorFormatMap |= DISPLAYINFO_EDID_COLOR_FORMAT_RGB;
                }

                if (Version() >= 2) {
                    if(_segment[3] & (1 << 4)) {
                        colorFormatMap |= DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR444;
                    }
                    if(_segment[3] & (1 << 5)) {
                        colorFormatMap |= DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR422;
                    }
                }

                DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());

                do {
                    if (dataBlock.IsValid()) {
                        if (   (dataBlock.BlockTag() == DataBlockIterator::VENDOR_SPECIFIC)
                            && (dataBlock.BlockSize() > 7)
                           ) {
                            if (dataBlock.OUI() == OUI_HDMI_FORUM) {
                                if ((dataBlock.Current()[7]) & 7) {
                                    colorFormatMap |= DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR420;
                                    break;
                                }
                            }
                        } else if (dataBlock.BlockTag() == DataBlockIterator::YCBCR420_CAPABILITY_MAP) {
                            colorFormatMap |= DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR420;
                            break;
                        }
                    }
                } while (dataBlock.Next());
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
            : _segments()
        {
            // It is already invalid if the first byte != 0x00
            _base[0] = 0x55;
        }
        ~ExtendedDisplayIdentification()
        {
        }

    public:
        // -------------------------------------------------------------
        // Only use the accessors if this method return true!!!
        // -------------------------------------------------------------
        inline bool IsValid() const
        {
            return (   (_base[0] == 0x00)
                    && (_base[1] == 0xFF)
                    && (_base[2] == 0xFF)
                    && (_base[3] == 0xFF)
                    && (_base[4] == 0xFF)
                    && (_base[5] == 0xFF)
                    && (_base[6] == 0xFF)
                    && (_base[7] == 0x00)
                     // Only checksum calculation of the base EDID and CEA are known
                    && _base.IsValid());
        }

        uint16_t Raw(const uint16_t length, uint8_t data[]) const
        {
            uint16_t written = 0;

            // We always have a base if it is valid...
            if (IsValid()) {
                uint16_t segment = std::min(Length(), length);
                BufferList::const_iterator index(_segments.cbegin());

                // By definition, we can copy the base...
                std::memcpy(data, _base, segment);
                written = segment;

                while ( (written < length) && (index != _segments.cend()) ) {
                    segment = std::min(Length(), static_cast<uint16_t>(length - written));
                    std::memcpy(&(data[written]), *index, segment);
                    index++;
                    written += segment;
                }
            }

            return (written);
        }

        // -------------------------------------------------------------
        // Accessors to the base information of the EDID raw buffer.
        // -------------------------------------------------------------
        string Manufacturer() const
        {
            string result;
            if (IsValid()) {
                uint16_t value = ((_base[0x08] << 8) | (_base[0x09]));
                result  = ManufactereChar(value >> 10);
                result += ManufactereChar(value >> 5);
                result += ManufactereChar(static_cast<uint8_t>(value));
            }
            return (result);
        }
        uint16_t ProductCode() const
        {
            uint16_t result = ~0;

            if (IsValid()) {
                result =   _base[0x0B] << 8
                         | _base[0x0A];
            }
            return (result);
        }
        uint32_t Serial() const
        {
            uint32_t result = ~0;
            if (IsValid()) {
                result =   _base[0x0F] << 24
                         | _base[0x0E] << 16
                         | _base[0x0D] << 8
                         | _base[0x0C];
            }
            return (result);
        }
        // If the Week = 0xFF, it means the year is the year when this model was
        // release. If 0 <= week <= 53 the year represnt the year when the device
        // was manufactured.
        uint8_t Week() const
        {
            return (IsValid() ? _base[0x10] : 0x00);
        }
        uint16_t Year() const
        {
            return (IsValid() ? 1990 + _base[0x11] : 0x00);
        }
        uint8_t Major() const
        {
            return (IsValid() ? _base[0x12] : 0x00);
        }
        uint8_t Minor() const
        {
            return (IsValid() ? _base[0x13] : 0x00);
        }
        bool Digital() const
        {
            return ((_base[0x14] & 0x80) != 0);
        }

        // Bits per primary color channel
        uint8_t BitsPerColor() const
        {
            static const uint8_t bitsPerColor[] = { 0, 6, 8, 10, 12, 14, 16, 255 };
            // EDID 1.3 requires bits 6-1 set to 0 for bit 7 equals 1
            // See Table 3.9 Release A Rev 1
            return Digital() ? (bitsPerColor[(_base[0x14] >> 4) & 0x7]) : bitsPerColor [0 + 2 * (_base[0x14] & 0x1)];
        }

        uint8_t ColorDepth() const
        {
            uint8_t colorDepth = DISPLAYINFO_EDID_COLOR_DEPTH_UNDEFINED;
            if (   Digital()
                && (Minor() >= 4)
               ) {
                switch ((_base[0x14] >> 4) & 0x07) {
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
                case 0x0:
                default:
                    colorDepth = DISPLAYINFO_EDID_COLOR_DEPTH_UNDEFINED;
                    break;
                }
            }

            return colorDepth;
        }

        displayinfo_edid_color_space_map_t DefaultColorSpace() const
        {
            return (IsValid() && (_base[0x18] & 0x4)) ? DISPLAYINFO_EDID_COLOR_SPACE_SRGB : DISPLAYINFO_EDID_COLOR_SPACE_UNDEFINED;
        }

        uint8_t DisplayType() const
        {
            displayinfo_edid_color_format_map_t displayType = DISPLAYINFO_EDID_COLOR_FORMAT_UNDEFINED;

            if (   Digital()
                && Minor() >= 4
               ) {
                /*
                    Color Endcoding Formats

                    Bit 4-3
                    00 RGB4444
                    01 RGB4444 & YCrCb444
                    10 RGB4444 & YCrCb422
                    11 RGB4444 & YCrCb444 & YCrCb422
                */

                if ((_base[0x18] & 0x18) == 0x0) {
                    displayType = static_cast<displayinfo_edid_color_format_map_t>(DISPLAYINFO_EDID_COLOR_FORMAT_RGB);
                }

                if ((_base[0x18] & 0x18) == 0x8) {
                    // 01 RGB4444 & YCrCb444
                    displayType |= static_cast<displayinfo_edid_color_format_map_t>(DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR444);
                }

                if ((_base[0x18] & 0x18) == 0x10) {
                    displayType |= static_cast<displayinfo_edid_color_format_map_t>(DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR422);
                }
            } else {
                /*
                    Display (Color) Type

                    Bit 4-3
                    00 Monochrome yyor Grayscale display
                    01 RGB color display
                    10 Non-RGB color display
                    11 Display Color Type is Undefined
                */

                switch (_base[0x18] & 0x18) {
                case 0x0    :   // 00 Monochrome or Grayscale display
                                displayType = DISPLAYINFO_EDID_DISPLAY_MONOCHROME_GRAYSCALE;
                                break;
                case 0x8    :   // 01 RGB_color display
                                displayType = DISPLAYINFO_EDID_DISPLAY_RGB;
                                break;
                case 0x10   :   // 10 Non-RGB color display
                                displayType = DISPLAYINFO_EDID_DISPLAY_NONE_RGB;
                                break;
                case 0x18   :   // 11 Display Color Type is Undefined
                default     :   // Error
                                displayType = DISPLAYINFO_EDID_DISPLAY_UNDEFINED;
                }
            }

            return static_cast<uint8_t>(displayType);
        }

        displayinfo_edid_rgb_colorspace_coordinates ColorspaceRGBCoordinates() const
        {
            const uint16_t Rx = (static_cast<uint16_t>(_base[0x1B]) << 2) | ((_base[0x19] & 0xC0) >> 6);
            const uint16_t Ry = (static_cast<uint16_t>(_base[0x1C]) << 2) | ((_base[0x19] & 0x30) >> 4);

            const uint16_t Gx = (static_cast<uint16_t>(_base[0x1D]) << 2) | ((_base[0x19] & 0xC) >> 2);
            const uint16_t Gy = (static_cast<uint16_t>(_base[0x1E]) << 2) | ((_base[0x19] & 0x3) >> 0);

            const uint16_t Bx = (static_cast<uint16_t>(_base[0x1F]) << 2) | ((_base[0x1A] & 0xC0) >> 6);
            const uint16_t By = (static_cast<uint16_t>(_base[0x20]) << 2) | ((_base[0x1A] & 0x30) >> 4);

            const uint16_t Wx = (static_cast<uint16_t>(_base[0x21]) << 2) | ((_base[0x1A] & 0xC) >> 2);
            const uint16_t Wy = (static_cast<uint16_t>(_base[0x22]) << 2) | ((_base[0x1A] & 0x3) >> 0);


            return {Rx, Ry, Gx, Gy, Bx, By, Wx, Wy};
        }

        bool HDRProfileSupport(displayinfo_hdr_common_t format) const
        {
            bool result = false;

            displayinfo_edid_hdr_multitype_map_t data;

            data.legacy = DISPLAYINFO_HDR_MULTITYPE | DISPLAYINFO_HDR_COMMONTYPE;
            data.version |= static_cast<uint8_t>(0x1);
            data.count = sizeof(displayinfo_hdr_t);
            data.common = new uint8_t[data.count];
            data.pseudo = nullptr;
            data.vesa = nullptr;

            if (data.common != nullptr) {
                /* void * */ std::memcpy(data.common, &format, data.count);

                result = HDRProfileSupport(data);

                delete[] data.common;
            }

            return result;
        }

        bool HDRProfileSupport(displayinfo_hdr_pseudo_t format) const
        {
            bool result = false;

            displayinfo_edid_hdr_multitype_map_t data;

            data.legacy = DISPLAYINFO_HDR_MULTITYPE | DISPLAYINFO_HDR_PSEUDOTYPE;
            data.version |= static_cast<uint8_t>(0x1);
            data.count = sizeof(displayinfo_hdr_pseudo_t);
            data.common = nullptr;
            data.pseudo = new uint8_t[data.count];
            data.vesa = nullptr;

            if (data.pseudo != nullptr) {
                /* void * */ std::memcpy(data.pseudo, &format, data.count);

                result = HDRProfileSupport(data);

                delete[] data.pseudo;
            }

            return result;
        }

        bool HDRProfileSupport(displayinfo_hdr_vesa_t format) const
        {
            bool result = false;

            displayinfo_edid_hdr_multitype_map_t data;

            data.legacy = DISPLAYINFO_HDR_MULTITYPE | DISPLAYINFO_HDR_VESATYPE;
            data.version |= static_cast<uint8_t>(0x1);
            data.count = sizeof(displayinfo_hdr_vesa_t);
            data.common = nullptr;
            data.pseudo = nullptr;
            data.vesa = new uint8_t[data.count];

            if (data.vesa != nullptr) {
                /* void * */ std::memcpy(data.vesa, &format, data.count);

                result = HDRProfileSupport(data);

                delete[] data.vesa;
            }

            return result;
        }

        // Legacy
        bool HDRProfileSupport(displayinfo_hdr_t format) const
        {
            bool result = false;

            switch (format) {
            case DISPLAYINFO_HDR_10          :  result = HDRProfileSupport(DISPLAYINFO_HDR_COMMON_10);
                                                break;
            case DISPLAYINFO_HDR_10PLUS      :  result = HDRProfileSupport(DISPLAYINFO_HDR_COMMON_10PLUS);
                                                break;
            case DISPLAYINFO_HDR_DOLBYVISION :  result = HDRProfileSupport(DISPLAYINFO_HDR_COMMON_DOLBYVISION);
                                                break;
            case DISPLAYINFO_HDR_TECHNICOLOR :  result = HDRProfileSupport(DISPLAYINFO_HDR_COMMON_TECHNICOLOR);
                                                break;
            case DISPLAYINFO_HDR_HLG         :  result = HDRProfileSupport(DISPLAYINFO_HDR_COMMON_HLG);
                                                break;
            case DISPLAYINFO_HDR_400         :  result = HDRProfileSupport(DISPLAYINFO_HDR_PSEUDO_400);
                                                break;
            case DISPLAYINFO_HDR_500         :  result = HDRProfileSupport(DISPLAYINFO_HDR_PSEUDO_500);
                                                break;
            case DISPLAYINFO_HDR_600         :  result = HDRProfileSupport(DISPLAYINFO_HDR_PSEUDO_600);
                                                break;
            case DISPLAYINFO_HDR_1000        :  result = HDRProfileSupport(DISPLAYINFO_HDR_PSEUDO_1000);
                                                break;
            case DISPLAYINFO_HDR_1400        :  result = HDRProfileSupport(DISPLAYINFO_HDR_PSEUDO_1400);
                                                break;
            case DISPLAYINFO_HDR_TB_400      :  result = HDRProfileSupport(DISPLAYINFO_HDR_PSEUDO_TB_400);
                                                break;
            case DISPLAYINFO_HDR_TB_500      :  result = HDRProfileSupport(DISPLAYINFO_HDR_PSEUDO_TB_500);
                                                break;
            case DISPLAYINFO_HDR_TB_600      :  result = HDRProfileSupport(DISPLAYINFO_HDR_PSEUDO_TB_600);
                                                break;
            case DISPLAYINFO_DISPLAYHDR_400  :  result = HDRProfileSupport(DISPLAYINFO_HDR_VESA_DISPLAYHDR_400);
                                                break;
            case DISPLAYINFO_HDR_OFF         :  __attribute__((fallthrough));
            case DISPLAYINFO_HDR_UNKNOWN     :  __attribute__((fallthrough));
            default                          :;
            }

            return result;
        }

        displayinfo_edid_video_interface_t VideoInterface() const
        {
            return (Minor() >= 4 ? static_cast<displayinfo_edid_video_interface_t> (_base[0x14] & 0x0F) : DISPLAYINFO_EDID_VIDEO_INTERFACE_UNDEFINED);
        }

        Iterator Extensions() const
        {
            return (Iterator(_segments));
        }

        // -------------------------------------------------------------
        // Operators to get access to the EDID strorage raw information.
        // -------------------------------------------------------------
        uint16_t Length () const
        {
            return (_base.Length());
        }
        // Total segments including the base EDID
        uint8_t Segments() const
        {
            return (IsValid() ? _base[0x7e] + 1 : 1);
        }
        uint8_t* Segment(const uint8_t index)
        {
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

        Iterator CEASegment() const
        {
            return CEASegment(Iterator(_segments));
        }

        // Multiple CEA segments may exist
        Iterator CEASegment(const Iterator& it) const
        {
            Iterator index = it;
            while (index.Next()) {
                if (   index.Type() == CEA::extension_tag
                    && index.Current().IsValid()
                   ) {
                    break;
                }
            }
            return index;
        }

        uint8_t WidthInCentimeters() const
        {
            return IsValid() ? _base[21] : 0;
        }

        uint8_t HeightInCentimeters() const
        {
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
        uint16_t PreferredWidthInPixels() const
        {
            return IsValid() ? (((_base[0x3A] & 0xF0 ) << 4) + _base[0x38]) : 0;
        }

        uint16_t PreferredHeightInPixels() const
        {
            return IsValid() ? (((_base[0x3D] & 0xF0 ) << 4) + _base[0x3B]) : 0;
        }

        void Clear()
        {
            _base[0] = 0x55;
            _segments.clear();
        }

    private:

        inline TCHAR ManufactereChar(uint8_t value) const
        {
            return static_cast<TCHAR>('A' + ((value - 1) & 0x1F));
        }

        // Users should set the common, pseudo or vesa fields
        bool HDRProfileSupport(displayinfo_edid_hdr_multitype_map_t format) const
        {
            bool result = false;

            const auto it = CEASegment();

            // CEA and its data blocks are optional
            if (it.IsValid()) {
                const CEA& cea = ExtendedDisplayIdentification::CEA(it.Current());

                const auto dc_max = [](uint8_t cv) -> uint64_t {
                    uint64_t result = 0;
                    /*
                        2 ^ ( cv / 32 ) = 2 ^ ( m * 32 / 32 + n / 32 ) = 2 ^ (m + n / 32 ), where m = cv / 32, and n = cv % 32;
                                        = 2 ^ m * 2 ^ ( n / 32 )
                                        = 2 ^ m * 2 ^ ( 1 / 32 ) ^ n = alpha * beta
                    */

                    const uint8_t m = cv / 32;
                    const uint8_t n = cv % 32;

                    uint64_t alpha = 1;

                    if (m > 0) {
                        alpha = alpha << m;
                    }

                    // A very naive method
                    float beta = 1.0;
                    for (uint8_t i = 0; i < n; i++) {
                        beta *= 1.021897148654117;
                    }

                    // Truncation to underestimate
                    result = static_cast<float>(50 * alpha) * beta;

                    return result;
                };

                const auto dc_min = [&](uint8_t cv_min, uint8_t cv_max) -> uint64_t {
                    const float val = static_cast<float>(cv_min) / 255.0;
                    return static_cast<float>(dc_max(cv_max)) * val * val / 100.0;
                };

                const auto dynamic = [](const displayinfo_edid_hdr_dynamic_metadata_t& data) -> bool {
                    bool result = false;

                    for (uint8_t i = 0; i < data.count; i++) {
                        result =    result
                                 || data.type[i].type != DISPLAYINFO_EDID_HDR_DYNAMIC_FLAG_TYPE_UNDEFINED;
                    }

                    return result;
                };


                // Allow for some inaccuracies
                constexpr uint8_t gamut_threshold = 95;
                uint8_t gamut_match = 0;

                const displayinfo_edid_color_space_map_t spaces = DefaultColorSpace() | cea.ColorSpaces();

                // ITU-R BT.2020 defined 10 or 12 bits per primary and YCbCr spaces are derived from RGB values
                if (   (spaces & DISPLAYINFO_EDID_COLOR_SPACE_ITUR_BT_2020_RGB) != DISPLAYINFO_EDID_COLOR_SPACE_ITUR_BT_2020_RGB
                    && (spaces & DISPLAYINFO_EDID_COLOR_SPACE_ITUR_BT_2020_CYCC) != DISPLAYINFO_EDID_COLOR_SPACE_ITUR_BT_2020_CYCC
                    && (spaces & DISPLAYINFO_EDID_COLOR_SPACE_ITUR_BT_2020_YCC) != DISPLAYINFO_EDID_COLOR_SPACE_ITUR_BT_2020_YCC
                   ) {
                    gamut_match = ColorSpaceGamutMatch(DISPLAYINFO_EDID_COLOR_SPACE_D65_P3);
                } else {
                    // Wide color gamut support
                    //gamut_match = ColorSpaceGamutMatch(DISPLAYINFO_EDID_COLOR_SPACE_D65_P3);
                    gamut_match = 100; // Represent >= 100
                }

                const displayinfo_edid_color_format_map_t color_format = DisplayType() | cea.ColorFormats();

                const displayinfo_edid_color_depth_map_t color_depth =    ColorDepth()
                                                                       | ((color_format & DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR444) == DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR444 ? cea.YCbCr444ColorDepths() : DISPLAYINFO_EDID_COLOR_DEPTH_UNDEFINED)
                                                                       | ((color_format & DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR422) == DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR422 ? cea.YCbCr422ColorDepths() : DISPLAYINFO_EDID_COLOR_DEPTH_UNDEFINED)
                                                                       | ((color_format & DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR420) == DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR420 ? cea.YCbCr420ColorDepths() : DISPLAYINFO_EDID_COLOR_DEPTH_UNDEFINED)
                                                                       | ((color_format & DISPLAYINFO_EDID_COLOR_FORMAT_RGB) == DISPLAYINFO_EDID_COLOR_FORMAT_RGB ? cea.RGBColorDepths() : DISPLAYINFO_EDID_COLOR_DEPTH_UNDEFINED);

                // Transfer function and luminance
                const displayinfo_edid_hdr_static_metadata_t hdr_static = cea.HDRStaticMetadata();

                const displayinfo_edid_hdr_dynamic_metadata_t hdr_dynamic = cea.HDRDynamicMetadata();

                displayinfo_edid_hdr_type_map_t data = DISPLAYINFO_HDR_OFF;

                // displayinfo_hdr_common_t;
                if ((format.legacy & (DISPLAYINFO_HDR_MULTITYPE | DISPLAYINFO_HDR_COMMONTYPE))
                     ==
                    (DISPLAYINFO_HDR_MULTITYPE | DISPLAYINFO_HDR_COMMONTYPE)
                    ) {
                    data = DISPLAYINFO_HDR_COMMON_OFF;

                    for (size_t i = format.count; i > 0; i--) {
                        data |= format.common[i - 1] ;
                        if (i > 1) {
                            data <<= 8;
                        }
                    }

                    // https://en.wikipedia.org/wiki/High-dynamic-range_television
                    switch (data) {
                    case DISPLAYINFO_HDR_COMMON_10              :   /*
                                                                        Developed by            : CTA
                                                                        Transfer function       : PQ
                                                                        Bit depth               : 10 bit
                                                                        Peak luminance          : Technical limit 10000 nits, common 1000-4000 nits
                                                                        Color primaries         : Technical limit Rec.2020, contents P3-D65 (common)
                                                                        Metadata                : static
                                                                    */
                                                                    result =     (hdr_static.eot & DISPLAYINFO_EDID_EOT_SMPTE_ST_2084) == DISPLAYINFO_EDID_EOT_SMPTE_ST_2084
                                                                              && color_depth >= static_cast<displayinfo_edid_color_depth_map_t>(DISPLAYINFO_EDID_COLOR_DEPTH_10_BPC)
                                                                              && dc_max(hdr_static.luminance.max_cv) >= 0
                                                                              && dc_max(hdr_static.luminance.average_cv) >= 0
                                                                              && dc_min(hdr_static.luminance.min_cv, hdr_static.luminance.max_cv) >= 0
                                                                              && gamut_match >= gamut_threshold;
                                                                    break;
                    case DISPLAYINFO_HDR_COMMON_10PLUS          :   /*
                                                                        Developed by            : Samsung
                                                                        Transfer function       : PQ
                                                                        Bit depth               : 10 bit or more
                                                                        Peak luminance          : Technical limit 10000 nits, common 1000-4000 nits
                                                                        Color primaries         : Technical limit Rec.2020, contents P3-D65 (common)
                                                                        Metadata                : static and dynamic
                                                                    */
                                                                    result =    (hdr_static.eot & DISPLAYINFO_EDID_EOT_SMPTE_ST_2084) == DISPLAYINFO_EDID_EOT_SMPTE_ST_2084
                                                                             && dynamic(hdr_dynamic)
                                                                             && color_depth >= static_cast<displayinfo_edid_color_depth_map_t>(DISPLAYINFO_EDID_COLOR_DEPTH_10_BPC)
                                                                             && dc_max(hdr_static.luminance.max_cv) >= 0
                                                                             && dc_max(hdr_static.luminance.average_cv) >= 0
                                                                             && dc_min(hdr_static.luminance.min_cv, hdr_static.luminance.max_cv) >= 0
                                                                             && gamut_match >= gamut_threshold;
                                                                    break;
                    case DISPLAYINFO_HDR_COMMON_DOLBYVISION    :    /*
                                                                        Developed by            : Dolby
                                                                        Transfer function       : PQ (most profiles), SDR (profiles 4.8.2, and 9), HLG (profile 8.4)
                                                                        Bit depth               : 10 bit or 12 bit using FEL
                                                                        Peak luminance          : Technical limit 10000 nits, at least 1000 nits, common 4000 nits
                                                                        Color primaries         : Technical limit Rec.2020, contents P3-D65 (at least)
                                                                        Metadata                : static and dynamic
                                                                    */
                                                                    result =    (   (hdr_static.eot & DISPLAYINFO_EDID_EOT_SMPTE_ST_2084) == DISPLAYINFO_EDID_EOT_SMPTE_ST_2084
                                                                                 || (hdr_static.eot & DISPLAYINFO_EDID_EOT_HYBRID_LOG_GAMMA_ITU_R_BT_2100) == DISPLAYINFO_EDID_EOT_HYBRID_LOG_GAMMA_ITU_R_BT_2100
                                                                                )
                                                                             && dynamic(hdr_dynamic)
                                                                             && color_depth >= static_cast<displayinfo_edid_color_depth_map_t>(DISPLAYINFO_EDID_COLOR_DEPTH_10_BPC)
                                                                             && dc_max(hdr_static.luminance.max_cv) >= 1000
                                                                             && dc_max(hdr_static.luminance.average_cv) >= 1000
                                                                             && dc_min(hdr_static.luminance.min_cv, hdr_static.luminance.max_cv) >= 1000
                                                                             && gamut_match >= 100;
                                                                    break;
                    case DISPLAYINFO_HDR_COMMON_HLG            :    /*
                                                                        Developed by            : NHK and BBC
                                                                        Transfer function       : HLG
                                                                        Bit depth               : 10 bit
                                                                        Peak luminance          : Technical limit variable nits, common 1000 nits
                                                                        Color primaries         : Technical limit Rec.2020, contents P3-D65 (common)
                                                                        Metadata                : none
                                                                    */
                                                                    result =    (hdr_static.eot & DISPLAYINFO_EDID_EOT_HYBRID_LOG_GAMMA_ITU_R_BT_2100) == DISPLAYINFO_EDID_EOT_HYBRID_LOG_GAMMA_ITU_R_BT_2100
                                                                             && color_depth >= static_cast<displayinfo_edid_color_depth_map_t>(DISPLAYINFO_EDID_COLOR_DEPTH_10_BPC)
                                                                             && dc_max(hdr_static.luminance.max_cv) >= 0
                                                                             && dc_max(hdr_static.luminance.average_cv) >= 0
                                                                             && dc_min(hdr_static.luminance.min_cv, hdr_static.luminance.max_cv) >= 0
                                                                             && gamut_match >= gamut_threshold;
                                                                    break;
                    case DISPLAYINFO_HDR_COMMON_TECHNICOLOR     :   /*
                                                                       Currently unknown
                                                                    */
                    case DISPLAYINFO_HDR_COMMON_OFF             :   __attribute__((fallthrough));
                    case DISPLAYINFO_HDR_COMMON_UNKNOWN         :   __attribute__((fallthrough));
                    default                                     :;
                    }

                }

                // displayinfo_hdr_pseudo_t;
                if ((format.legacy & (DISPLAYINFO_HDR_MULTITYPE | DISPLAYINFO_HDR_PSEUDOTYPE))
                    ==
                    (DISPLAYINFO_HDR_MULTITYPE | DISPLAYINFO_HDR_PSEUDOTYPE)
                   ) {
                    data = DISPLAYINFO_HDR_PSEUDO_OFF;

                    for (size_t i = format.count; i > 0; i--) {
                        data |= format.pseudo[i - 1] ;
                        if (i > 1) {
                            data <<= 8;
                        }
                    }

                    switch (data) {
                    case DISPLAYINFO_HDR_PSEUDO_400     :   /*
                                                                Exceeds SDR, minium peak 400 nits, but may otherwise not meet HDR10 specifications
                                                                https://displayhdr.org/not-all-hdr-is-created-equal/
                                                            */
                                                            gamut_match = (color_format & DISPLAYINFO_EDID_DISPLAY_RGB) == DISPLAYINFO_EDID_DISPLAY_RGB ? 100 : ColorSpaceGamutMatch(DISPLAYINFO_EDID_COLOR_SPACE_SRGB);

                                                            result =    color_depth >= static_cast<displayinfo_edid_color_depth_map_t>(DISPLAYINFO_EDID_COLOR_DEPTH_8_BPC)
                                                                     && dc_max(hdr_static.luminance.max_cv) >= 400
                                                                     && dc_max(hdr_static.luminance.average_cv) >= 400
                                                                     && dc_min(hdr_static.luminance.min_cv, hdr_static.luminance.max_cv) >= 0
                                                                     // Exceed SDR
                                                                     && gamut_match >= gamut_threshold;
                                                            break;
                    case DISPLAYINFO_HDR_PSEUDO_OFF     :   __attribute__((fallthrough));
                    default                             :;
                    }
                }

                // displayinfo_hdr_vesa_t;
                if ((format.legacy & (DISPLAYINFO_HDR_MULTITYPE | DISPLAYINFO_HDR_VESATYPE))
                    ==
                    (DISPLAYINFO_HDR_MULTITYPE | DISPLAYINFO_HDR_VESATYPE)
                   ) {
                    data = DISPLAYINFO_HDR_VESA_OFF;

                    for (size_t i = format.count; i > 0; i--) {
                        data |= format.vesa[i - 1] ;
                        if (i > 1) {
                            data <<= 8;
                        }
                    }

                    switch (data) {
                    case DISPLAYINFO_HDR_VESA_DISPLAYHDR_400    :   /*
                                                                        Developed by            : CTA
                                                                        Transfer function       : PQ
                                                                        Bit depth               : 10 bit
                                                                        Peak luminance          : Technical limit 10000 nits, common 1000-4000 nits
                                                                        Color primaries         : Technical limit Rec.2020, contents P3-D65 (common)
                                                                        Metadata                : static

                                                                        Minimal HDR_10, with maximum luminance values exceeding 400
                                                                    */
                                                                    result =    (hdr_static.eot & DISPLAYINFO_EDID_EOT_SMPTE_ST_2084) == DISPLAYINFO_EDID_EOT_SMPTE_ST_2084
                                                                             && color_depth >= static_cast<displayinfo_edid_color_depth_map_t>(DISPLAYINFO_EDID_COLOR_DEPTH_10_BPC)
                                                                             && dc_max(hdr_static.luminance.max_cv) >= 400
                                                                             && dc_max(hdr_static.luminance.average_cv) >= 400
                                                                             && dc_min(hdr_static.luminance.min_cv, hdr_static.luminance.max_cv) >= 0
                                                                             && gamut_match >= gamut_threshold;
                                                                    break;
                    case DISPLAYINFO_HDR_VESA_OFF               :   __attribute__((fallthrough));
                    default                                     :;
                    }
                }
            }

            return result;
        }

        class Coordinate {
        public:
            Coordinate() = delete;

            // White point
            Coordinate(displayinfo_edid_color_space_t space)
            {
                switch (space) {
                case DISPLAYINFO_EDID_COLOR_SPACE_D65_P3 : __attribute__((fallthrough));
                case DISPLAYINFO_EDID_COLOR_SPACE_SRGB   : _x = 0x140; _y = 0x151;
                                                            break;
                default                                  : _x = -1; _y = -1;
                }
            }

            Coordinate(int64_t x, int64_t y) : _x{x}, _y{y}
            {
            }

            Coordinate(const Coordinate&) = default;
            Coordinate& operator=(const Coordinate&) = default;

            ~Coordinate() = default;

            bool operator!=(const Coordinate& rhs) const {
                return    _x != rhs._x
                       || _y != rhs._y;
            }

            bool operator==(const Coordinate& rhs) const {
                return !(*this != rhs);
            }

            bool Equal(const Coordinate& rhs) const
            {
                return *this == rhs;
            }

            bool IsValid() const
            {
                // EDID condition
                return    _x > -1
                       && _y > -1;
            }

            const int64_t& X() const
            {
                return _x;
            }

            const int64_t& Y() const
            {
                return _y;
            }

        private:
            int64_t _x;
            int64_t _y;
        };

        class Polygon {
        public:
            // Empty
            Polygon()
            {
            }

            // Predefined (reference) gamut
            Polygon(displayinfo_edid_color_space_t space)
            {
                switch (space) {
                case DISPLAYINFO_EDID_COLOR_SPACE_D65_P3 : _poly = {Coordinate{0x2B8, 0x148}, Coordinate{0x10F, 0x2C3}, Coordinate{0x9A, 0x3D}};
                                                           break;
                case DISPLAYINFO_EDID_COLOR_SPACE_SRGB   : _poly = {Coordinate{0x28F, 0x152}, Coordinate{0x133, 0x266}, Coordinate{0x9A, 0x3D}};
                                                           break;
                default                                  :;
                }
            }

            ~Polygon() = default;

            Polygon(const Polygon&) = default;
            Polygon& operator=(const Polygon&) = default;

            bool IsValid() const
            {
                const size_t count = Count();

                // Gamut condition
                bool result = count > 2;

                for (size_t i = 0; i < count; i++) {
                    result =    result
                             && _poly[i].IsValid();
                }

                return result;
            }

            bool Add(const Coordinate& point)
            {
                bool result = point.IsValid();

                if (result) {
                    _poly.push_back(point);
                }

                return result;
            }

            bool Next(Coordinate& point)
            {
                bool result = false;

                size_t count = Count();

                if (   0 < count
                    && _index < count
                   ) {
                    point = _poly[_index];
                    _index = (_index + 1) % count;
                    result = true;
                }

                return result;
            }

            size_t Count() const
            {
                return _poly.size();
            }

            bool Area(float& sum) const
            {
                bool result = false;

                Polygon poly;

                result =    IsValid()
                         && SortCounterClockWise(poly)
                         && poly.IsValid()
                         && poly.Convex();

                if (result) {
                    Coordinate A{-1, -1}, B{-1, -1};

                    result =    poly.Next(A)
                             && A.IsValid();

                    uint64_t total = 0;

                    // http://www.mathwords.com/a/area_convex_polygon.htm
                    for (size_t i = 1, end = poly.Count(); i <= end; i++) {
                        result =    poly.Next(B)
                                 && B.IsValid();

                        if (result) {
                            total += (A.X() * B.Y()) - (B.X() * A.Y());
                        }
                        else {
                            break;
                        }

                        A = B;
                    }

                    sum = total / 2.0;
                }

                return result;
            }

            bool Enclosed(const Coordinate& point) const
            {
                bool result =    IsValid()
                              && point.IsValid();

                if (result) {
                    const size_t count = Count();

                    // The total is the sum of the parts
                    switch (count) {
                    case 0  :   __attribute__((fallthrough));
                    case 1  :   __attribute__((fallthrough));
                    case 2  :   result = false;
                                break;
                    case 3  :   {
                                    float sum = 0.0;

                                    for (size_t i = 0; i < count; i++) {
                                        Polygon poly;

                                        const Coordinate& A = _poly[i % count];
                                        const Coordinate& B = _poly[(i + 1) % count];

                                        float delta = 0.0;

                                        result =    poly.Add(point)
                                                 && poly.Add(A)
                                                 && poly.Add(B)
                                                 && poly.IsValid()
                                                 && poly.Area(delta);

                                        if (result) {
                                            sum += delta;
                                        } else {
                                            break;
                                        }
                                    }

                                    float total = 0.0;

                                    result =    result
                                             && Area(total)
                                             && static_cast<int64_t>(total) > 0
                                             && static_cast<int64_t>(total) == static_cast<int64_t>(sum);
                                }
                                break;
                    default :   // Algorithm
                                // For now default to false
                                result = false;
                                ;
                    }
                }

                return result;
            }

            bool Convex() const
            {
                bool result = IsValid();

                if (result) {
                    const size_t count = Count();

                    switch (count) {
                    case 0  :   __attribute__((fallthrough));
                    case 1  :   __attribute__((fallthrough));
                    case 2  :   result = false;
                                break;
                    case 3  :   // A triangle is convex
                                for (size_t i = 0; i < (count - 1); i++) {
                                    for (size_t j = i + 1; j < count; j++) {
                                        result =    result
                                                 && !_poly[i].Equal(_poly[j]);
                                    }
                                }
                                break;
                    default :   // Algorithm required
                                // For now default to false
                                result = false;
                    }
                }

                return result;
            }

            Coordinate Centroid() const
            {
                int64_t x = 0;
                int64_t y = 0;

                if (IsValid()) {
                    const size_t count = Count();

                    for (size_t i = 0; i < count; i++) {
                        Coordinate point = _poly[i];

                        x += point.X();
                        y += point.Y();
                    }

                    // Integer truncation
                    x /= count;
                    y /= count;
                } else {
                    x = -1;
                    y = -1;
                }

                return Coordinate{x, y};
            }

            Polygon Intersection(const Polygon& poly) const
            {
                const auto LineIntersection = [](const Polygon& polyL, const Polygon& polyR) -> Coordinate {
                    Coordinate point = {-1, -1};

                    if (   polyL.Count() == 2
                        && polyR.Count() == 2
                       ) {
                        const Coordinate& C = polyL._poly[0], &D = polyL._poly[1], &E = polyR._poly[0], &F = polyR._poly[1];;

                        // Invalid in a positive quadrant
                        Coordinate coordinate[2] = { Coordinate{-1, -1}, Coordinate{-1, -1} };

                        const int64_t num_t = ((C.X() - E.X()) * (E.Y() - F.Y())) - ((E.X() - F.X()) * (C.Y() - E.Y()));
                        const int64_t num_u = ((C.X() - E.X()) * (C.Y() - D.Y())) - ((C.X() - D.X()) * (C.Y() - E.Y()));
                        const int64_t den = ((C.X() - D.X()) * (E.Y() - F.Y())) - ((E.X() - F.X()) * (C.Y() - D.Y()));

                        if (den == 0) {
                            // parallel or coincident
                        } else {
                            // Within first line segment
                            if (   (   0 <= num_t
                                    && 0 <= den
                                    && num_t <= den
                                   )
                                ||
                                   (   num_t <= 0
                                    && den <= 0
                                    && den <= num_t
                                   )
                               ) {
                                const float fraction = static_cast<float>(num_t) / static_cast<float>(den);

                                const float x = C.X() + fraction * (D.X() - C.X());
                                const float y = C.Y() + fraction * (D.Y() - C.Y());

                                // Truncation 'rounds' towards (0, 0)

                                // std::round includes unwanted 0.5 and -0.5

                                int64_t ix = static_cast<int64_t>(x);
                                int64_t iy = static_cast<int64_t>(y);

                                if ((ix - x) < -0.5) {
                                    ix++;
                                }

                                if ((ix - x) > 0.5) {
                                    ix--;
                                }

                                if ((iy - y) < -0.5) {
                                    iy++;
                                }

                                if ((iy - y) > 0.5) {
                                    iy--;
                                }

                                coordinate[0] = Coordinate{ix, iy};
                            }

                            // Within second line segment
                            if (   (   0 <= num_u
                                    && 0 <= den
                                    && num_u <= den
                                   )
                                ||
                                   (   num_u <= 0
                                    && den <= 0
                                    && den <= num_u
                                   )
                               ) {
                                const float fraction = static_cast<float>(num_u) / static_cast<float>(den);

                                const float x = E.X() + fraction * (F.X() - E.X());
                                const float y = E.Y() + fraction * (F.Y() - E.Y());

                                // Truncation 'rounds' towards (0, 0)

                                // std::round includes unwanted 0.5 and -0.5

                                int64_t ix = static_cast<int64_t>(x);
                                int64_t iy = static_cast<int64_t>(y);

                                if ((ix - x) < -0.5) {
                                    ix++;
                                }

                                if ((ix - x) > 0.5) {
                                    ix--;
                                }

                                if ((iy - y) < -0.5) {
                                    iy++;
                                }

                                if ((iy - y) > 0.5) {
                                    iy--;
                                }

                                coordinate[1] = Coordinate{ix, iy};
                            }
                        }

                        // A valid point falls on both line segments

                        if (   coordinate[0].IsValid()
                            && coordinate[1].IsValid()
                           ) {
                            // 'Average out' differences
                            point = Coordinate{(coordinate[0].X() + coordinate[1].X()) / 2, (coordinate[0].Y() + coordinate[1].Y()) / 2};
                        }
                    }

                    return point;
                };


                Polygon points, polyAB = *this, polyCD = poly;

                Coordinate A{-1, -1}, B{-1, -1}, C{-1, -1}, D{-1, -1};

                if (   polyAB.IsValid()
                    && polyCD.IsValid()
                    && polyAB.Next(A)
                   ) {
                    for (size_t i = 0, end = polyAB.Count(); i < end; i++) {
                       if (polyAB.Next(B)) {
                            Polygon lineA;

                            if (   lineA.Add(A)
                                && lineA.Add(B)
                                && polyCD.Next(C)
                               ) {
                                for (size_t j = 0, end = polyCD.Count(); j < end; j++) {
                                    if (polyCD.Next(D)) {
                                        Polygon lineB;

                                        if (   lineB.Add(C)
                                            && lineB.Add(D)
                                           ) {
                                            const Coordinate coordinate(LineIntersection(lineA, lineB));

                                            if (coordinate.IsValid()) {
                                                points.Add(coordinate);
                                            }
                                        }
                                    }

                                    C = D;
                                }
                            }

                            A = B;
                        }
                    }
                }

                return points;
            }

        private:

            bool SortCounterClockWise(Polygon& poly) const
            {
                const Coordinate centroid = Centroid();

                bool result =    IsValid()
                              && centroid.IsValid();

                if (result) {
                    std::vector<float> angles_r, angles_l;
                    std::vector<Coordinate> poly_r, poly_l;

                    // All left or all right, all top or all bottom
                    bool left = true, right = true, top = true, bottom = true;

                    const int64_t cx = centroid.X();
                    const int64_t cy = centroid.Y();

                    for (size_t i = 0, end = _poly.size(); i < end; i++) {
                        const Coordinate& point = _poly[i];

                        const int64_t& x = point.X();;
                        const int64_t& y = point.Y();;

                        left &= x <= cx;
                        right &= x >= cx;
                        top &= y >= cy;
                        bottom &= y <= cy;

                        int64_t dx = x - cx;
                        int64_t dy = y - cy;

                        if (dx != 0) {
                            if (dx > 0) {
                                if (dy == 0) {
                                    // positive horizontal axis
                                    angles_r.push_back(0.0);
                                } else {
                                    // quadrant 1 or 4
                                    angles_r.push_back(static_cast<float>(dy) / static_cast<float>(dx));
                                }

                                poly_r.push_back(point);
                            }

                            if (dx < 0) {
                                if (dy == 0) {
                                    // negative horizontal axis
                                    angles_l.push_back(0.0);
                                } else {
                                    // quadrant 2 or 3
                                    angles_l.push_back(static_cast<float>(dy) / static_cast<float>(dx));
                                }

                                poly_l.push_back(point);
                            }
                        } else { // dx = 0
                            if (dy > 0) {
                                // positive vertical axis
                                angles_r.push_back(std::numeric_limits<float>::max());
                                poly_r.push_back(point);
                            }

                            if (dy < 0) {
                                // negative vertical axis
                                angles_l.push_back(std::numeric_limits<float>::lowest());
                                poly_l.push_back(point);
                            }

                            if (dy == 0) {
                                // ill-defined because point coexists with origin
                                left = true; right = true; top = true; bottom = true;
                                break;
                            }
                        }
                    }

                    result = !(   top
                               && bottom
                               && left
                               && right
                              );

                    if (result) {
                        for (size_t i = 0, count = angles_l.size(); i < (count - 1); i++) {
                            for (size_t j = i + 1; j < count; j++) {
                                if (angles_l[j] < angles_l[i]) {
                                    std::swap(angles_l[j], angles_l[i]);
                                    std::swap(poly_l[j], poly_l[i]);
                                }
                            }
                        }

                        for (size_t i = 0, count = angles_r.size(); i < (count - 1); i++) {
                            for (size_t j = i + 1; j < count; j++) {
                                if (angles_r[j] < angles_r[i]) {
                                    std::swap(angles_r[j], angles_r[i]);
                                    std::swap(poly_r[j], poly_r[i]);
                                }
                            }
                        }

                        if (poly_l.size() + poly_r.size() > 0) {
                            for (size_t i = 0, end = poly_l.size(); i < end; i++) {
                                const Coordinate& point = poly_l[i];
                                result =    result
                                         && point.IsValid()
                                         && poly.Add(point);
                            }

                            for (size_t i = 0, end = poly_r.size(); i < end; i++) {
                                const Coordinate& point = poly_r[i];
                                result =    result
                                         && point.IsValid()
                                         && poly.Add(point);
                            }
                        }
                    }

                    result = poly.IsValid();
                }
                return result;
            }

            std::vector<Coordinate> _poly;
            size_t _index = 0;
        };

        uint8_t ColorSpaceGamutMatch(displayinfo_edid_color_space_t space) const
        {
            /*
                https://en.wikipedia.org/wiki/Rec._2020
                https://en.wikipedia.org/wiki/DCI-P3
                https://en.wikipedia.org/wiki/SRGB
                VESA Enhanced EDID Standard

                // Converted values!

                Color space             White point   CCT     Primary colors
                                        xW     yW     K       xR     yR     xG     yG     xB    yB
                ITU-R BT.2020           0x140  0x151          0x2D5  0x12B  0xAE   0x330  0x86  0x2F
                P3-D65 (Display)        0x140  0x151  0x1968  0x2B8  0x148  0x10F  0x2C3  0x9A  0x3D
                P3-DCI (Theater)        0x142  0x167  0x189C  0x2B8  0x148  0x10F  0x2C3  0x9A  0x3D
                P3-D60 (ACES Cinema)    0x149  0x15A  0x1770  0x2B8  0x148  0x10F  0x2C3  0x9A  0x3D

                // Negative values cannot be represented in EDID, rounded to 0x0
                // Positive values exceeding 0,9990234375 are rounded to 0,9990234375
                DCI-P3+                 0x142  0x167  0x189C  0x2F6  0x114  0xE1   0x31F  0x5C  0x0
                Cinema Gamut	        0x140  0x151  0x1968  0x2F6  0x114  0xAE   0x48F  0x52  0x0

                sRGB/ITU-R BT.709       0x140  0x151          0x28F  0x152  0x133  0x266  0x9A  0x3D
            */

            const displayinfo_edid_rgb_colorspace_coordinates xyz = ColorspaceRGBCoordinates();

            // Display color space trianglei and white point
            Polygon gamut;
            const Coordinate white_gamut(xyz.Wx, xyz.Wy);

            // Reference color space triagle and white point
            Polygon gamut_ref(space);
            const Coordinate white_ref(space);

            bool status =    gamut.Add(Coordinate{xyz.Rx, xyz.Ry})
                          && gamut.Add(Coordinate{xyz.Gx, xyz.Gy})
                          && gamut.Add(Coordinate{xyz.Bx, xyz.By})
                          && gamut.IsValid()
                          && white_gamut.IsValid()
                          && gamut_ref.IsValid()
                          && white_ref.IsValid();

            // The white points relates to the spectral distribution and one can correct for color recorded under a different illuminant and hence the gamut changes under that transformation
            // Here, it is assumed the distance between gamut white points and reference white points is negligible, hence, they represent the same illuminant
            // No guarantuee the white point is enclosed

// FIXME: transformation / correction

            // Determine intersections
            Polygon polygon;

            if (status) {
                polygon = gamut.Intersection(gamut_ref);

                // Add points to complete the shape
                // Non-intersecting points that are enclosed by either gamut

                for (size_t i = 0, end = gamut.Count(); i < end; i++) {
                    Coordinate A{-1, -1};

                    if (   gamut.Next(A)
                        && A.IsValid()
                        && gamut_ref.Enclosed(A)
                       ) {
                        polygon.Add(A);
                    }
                }

                for (size_t i = 0, end = gamut_ref.Count(); i < end; i++) {
                    Coordinate A{-1, -1};

                    if (   gamut_ref.Next(A)
                        && A.IsValid()
                        && gamut.Enclosed(A)
                       ) {
                        polygon.Add(A);
                    }
                }
            }

            // The combined set of polygon extended with enclosed points is convex see 'rubber band theorem'

// FIXME: area overlap, distance white points

            uint8_t ratio = 0;

            if (   polygon.IsValid()
                && polygon.Count() > 2
               ) {
                float den = 0.0;
                float num = 0.0;

                if (   gamut_ref.Area(den)
                    && polygon.Area(num)
                   ) {
                    // Max is 100%
                    ratio = den > num ? static_cast<uint8_t>(num / den * 100) : 100;
                }
            }

            return ratio;
        }

    private:
        Buffer _base;
        BufferList _segments;
    };
}
}
