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
            class DataBlockIterator {
            public:
                enum blocktype : uint16_t {
                    INVALID = 0,
                    AUDIO = 1,
                    VIDEO = 2,
                    VENDOR_SPECIFIC = 3,
                    SPEAKER_ALLOCATION = 4,
                    VESA = 5,
                    EXTENDED = 7,
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
                    , _reset(true) {
                        ASSERT(_index >= 4);
                }
                DataBlockIterator(const DataBlockIterator& copy)
                    : _segment(copy._segment)
                    , _index(copy._index)
                    , _dtdBegin(copy._dtdBegin)
                    , _reset(true) {
                        ASSERT(_index >= 4);
                }
                ~DataBlockIterator() = default;

                const DataBlockIterator& operator=(const DataBlockIterator& rhs)
                {
                    _segment = rhs._segment;
                    _index = rhs._index;
                    ASSERT(_index >= 4);
                    _dtdBegin = rhs._dtdBegin;
                    _reset = rhs._reset;

                    return (*this);
                }

            public:
                uint8_t BlockSize() const
                {
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
                    return ((_reset == false) && IsInRange());
                }

                bool IsInRange() const
                {
                    return ((_index < Buffer::edid_block_size) && (_index < _dtdBegin) && (_index >= 4));
                }

                void Reset()
                {
                    _reset = true;
                    _index = 4;
                }

                bool Next()
                {
                    bool success = true;
                    if(_reset == true) {
                        _reset = false;
                    }
                    else if(IsInRange()){
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
                bool _reset;
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

                while(dataBlock.Next()) {
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
                }

                return colorDepthMap;
            }

            displayinfo_edid_color_depth_map_t YCbCr444ColorDepths() const
            {
                displayinfo_edid_color_depth_map_t colorDepthMap = DISPLAYINFO_EDID_COLOR_DEPTH_UNDEFINED;

                if (_colorFormats & DISPLAYINFO_EDID_COLOR_FORMAT_YCBCR444) {
                    colorDepthMap = DISPLAYINFO_EDID_COLOR_DEPTH_8_BPC;
                    DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());

                    while(dataBlock.Next()) {
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
                    }
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

                    while(dataBlock.Next()) {
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
                    }
                }

                return colorDepthMap;
            }

            displayinfo_edid_color_space_map_t ColorSpaces() const
            {
                displayinfo_edid_color_space_map_t colorSpaceMap = DISPLAYINFO_EDID_COLOR_SPACE_UNDEFINED;

                DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());
                while(dataBlock.Next()) {
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
                }
                return colorSpaceMap;
            }

            void Timings(std::vector<uint8_t>& vicList) const
            {
                DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());
                while(dataBlock.Next()) {
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
                }
            }

            displayinfo_edid_audio_format_map_t AudioFormats() const
            {
                displayinfo_edid_audio_format_map_t audioFormatMap = 0;

                DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());
                while(dataBlock.Next()) {
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
                }
                return audioFormatMap;
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
                while(dataBlock.Next()) {
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
                }
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
                     (_base[7] == 0x00) );
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
                result += ManufactereChar(value);
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
            Iterator index = Iterator(_segments);
            while(index.Next() == true) {
                if(index.Type() == CEA::extension_tag) {
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
