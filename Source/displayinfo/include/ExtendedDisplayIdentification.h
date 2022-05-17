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

#include <list>
#include <string>
#include <vector>


namespace WPEFramework {
namespace Plugin {

    class ExtendedDisplayIdentification {
    public:
        enum Type {
            Undefined = 0,
            HDMIa = 2,
            HDMIb = 3,
            MDDI = 4,
            DisplayPort = 5
        };

        enum class AudioFormatType : uint32_t {
            LPCM = (1 << 0),
            AC3 = (1 << 1),
            MPEG1 = (1 << 2),
            MP3 = (1 << 3),
            MPEG2 = (1 << 4),
            AAC_LC = (1 << 5),
            DTS = (1 << 6),
            ATRAC = (1 << 7),
            SUPER_AUDIO_CD = (1 << 8),
            EAC3 = (1 << 9),
            DTSHD = (1 << 10),
            DOLBY_TRUEHD = (1 << 11),
            DST_AUDIO = (1 << 12),
            MS_WMA_PRO = (1 << 13),
            MPEG4_HEAAC = (1 << 14),
            MPEG4_HEAAC_V2 = (1 << 15),
            MPEG4_ACC_LC = (1 << 16),
            DRA = (1 << 17),
            MPEG4_HEAAC_MPEG_SURROUND = (1 << 18),
            MPEG4_HEAAC_LC_MPEG_SURROUND = (1 << 19),
            MPEGH_3DAUDIO = (1 << 20),
            LPCM_3DAUDIO = (1 << 21),
            AC4 = (1 << 22),
            DOLBY_ATMOS = (1 << 23),
        };


        enum class ColorSpaceType : uint16_t {
            SRGB = (1 << 0),
            XVYCC_601 = (1 << 1), // ITU-R BT.601
            XVYCC_709 = (1 << 2), // ITU-R BT.709
            SYCC_601 = (1 << 3),
            OP_YCC_601 = (1 << 4), // or ADOBE_YCC_601
            OP_RGB = (1 << 5), // or ADOBE_RGB
            ITUR_BT_2020_CYCC = (1 << 6), // ITU-R BT.2020 Yc Cbc Crc
            ITUR_BT_2020_YCC = (1 << 7), // ITU-R BT.2020 RGB or YCbCr
            ITUR_BT_2020_RGB = (1 << 8),
            DCI_P3 = (1 << 9),
        };

        enum class ColorDepthType : uint8_t {
            BPC_UNDEFINED = (0 << 0),
            BPC_6 = (1 << 0),
            BPC_8 = (1 << 1),
            BPC_10 = (1 << 2),
            BPC_12 = (1 << 3),
            BPC_14 = (1 << 4),
            BPC_16 = (1 << 5),
        };

        enum class ColorFormatType : uint8_t {
            UNDEFINED = (0 << 0),
            RGB = (1 << 0),
            YCBCR_4_2_2 = (1 << 1),
            YCBCR_4_4_4 = (1 << 2),
            YCBCR_4_2_0 = (1 << 3)
        };

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

            Buffer& operator= (const Buffer& copy) = delete;

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

                DataBlockIterator& operator= (const DataBlockIterator& rhs) {
                    return (*this);
                }

            public:
                uint8_t BlockSize() const
                {
                    return (IsValid() ? _segment[_index] & 0x1F : 0);
                }

                uint8_t BlockTag() const
                {
                    return (IsValid() ? ((_segment[_index] & 0xE0 ) >> 5) : 0x00);
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
                        _index += BlockSize() + 1;
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
                const Buffer& _segment;
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
            {
                ASSERT(_segment[0] == extension_tag);
            }
            ~CEA() {
            }

        public:
            uint8_t Version() const
            {
                return (_segment[1]);
            }
            uint8_t DetailedTimingDescriptorStart() const
            {
                return(_segment[2]);
            }

            void SupportedColorDepths(uint8_t& colorDepthMap) const
            {
                DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());
                while(dataBlock.Next()) {
                    if(dataBlock.IsValid() && (dataBlock.BlockTag() == 0x03) && (dataBlock.BlockSize() > 6)) {
                        const uint32_t registrationId = (dataBlock.Current()[1]) + (dataBlock.Current()[2] << 8) + (dataBlock.Current()[3] << 16);
                        // HDMI Licensing, LLC -- HDMI 1.4 info
                        if(registrationId == 0x000C03) {
                            if(dataBlock.Current()[6] & (1 << 6)) {
                                colorDepthMap |= static_cast<uint8_t>(ColorDepthType::BPC_16);
                            }
                            if(dataBlock.Current()[6] & (1 << 5)) {
                                colorDepthMap |= static_cast<uint8_t>(ColorDepthType::BPC_12);
                            }
                            if(dataBlock.Current()[6] & (1 << 4)) {
                                colorDepthMap |= static_cast<uint8_t>(ColorDepthType::BPC_10);
                            }
                        }
                    }
                }
            }

            ColorFormatType SupportedColorFormat() const
            {
                ColorFormatType result = ColorFormatType::UNDEFINED;
                if(Version() > 2) {
                    uint8_t colorFormat = (_segment[3] & 0x07) >> 4;
                    switch(colorFormat) {
                        case 0x00:
                            result = ColorFormatType::RGB;
                            break;
                        case 0x01:
                            result = ColorFormatType::YCBCR_4_2_2;
                            break;
                        case 0x02:
                            result = ColorFormatType::YCBCR_4_4_4;
                            break;
                        case 0x03:
                            result = ColorFormatType::YCBCR_4_2_0;
                            break;
                        default:
                            result = ColorFormatType::UNDEFINED;
                            break;
                    }
                }
                return result;
            }

            void SupportedColorFormats(uint8_t& colorFormatMap) const
            {
                DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());
                while(dataBlock.Next()) {
                    if(dataBlock.IsValid() && (dataBlock.BlockTag() == 0x03) && (dataBlock.BlockSize() > 3)) {
                        const uint32_t registrationId = (dataBlock.Current()[1]) + (dataBlock.Current()[2] << 8) + (dataBlock.Current()[3] << 16);
                        // HDMI Forum -- HDMI 2.0 info
                        if(registrationId == 0xC45DD8) {
                            if((dataBlock.Current()[3]) & (1 << 1)) {
                                colorFormatMap |= static_cast<uint8_t>(ColorFormatType::YCBCR_4_2_0);
                            }
                            if((dataBlock.Current()[3]) & (1 << 2)) {
                                colorFormatMap |= static_cast<uint8_t>(ColorFormatType::YCBCR_4_2_0);
                            }
                            if((dataBlock.Current()[3]) & (1 << 3)) {
                                colorFormatMap |= static_cast<uint8_t>(ColorFormatType::YCBCR_4_2_0);
                            }
                        }
                    }
                }
            }

            void SupportedColorSpaces(uint16_t& colorSpaceMap)
            {
                DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());
                while(dataBlock.Next()) {
                    if(dataBlock.IsValid() && (dataBlock.BlockTag() == 0x07) && (dataBlock.BlockSize() > 1)) {
                        const uint8_t extendedTag = dataBlock.Current()[1];
                        if((extendedTag == 0x05) && (dataBlock.BlockSize() >= 3)) {
                            if(dataBlock.Current()[2]  & (1 << 0)) {
                                colorSpaceMap |= static_cast<uint16_t>(ColorSpaceType::XVYCC_601);
                            }
                            if(dataBlock.Current()[2]  & (1 << 1)) {
                                colorSpaceMap |= static_cast<uint16_t>(ColorSpaceType::XVYCC_709);
                            }
                            if(dataBlock.Current()[2]  & (1 << 2)) {
                                colorSpaceMap |= static_cast<uint16_t>(ColorSpaceType::SYCC_601);
                            }
                            if(dataBlock.Current()[2]  & (1 << 3)) {
                                colorSpaceMap |= static_cast<uint16_t>(ColorSpaceType::OP_YCC_601);
                            }
                            if(dataBlock.Current()[2]  & (1 << 4)) {
                                colorSpaceMap |= static_cast<uint16_t>(ColorSpaceType::OP_RGB);
                            }
                            if(dataBlock.Current()[2]  & (1 << 5)) {
                                colorSpaceMap |= static_cast<uint16_t>(ColorSpaceType::ITUR_BT_2020_CYCC);
                            }
                            if(dataBlock.Current()[2]  & (1 << 6)) {
                                colorSpaceMap |= static_cast<uint16_t>(ColorSpaceType::ITUR_BT_2020_YCC);
                            }
                            if(dataBlock.Current()[2]  & (1 << 7)) {
                                colorSpaceMap |= static_cast<uint16_t>(ColorSpaceType::ITUR_BT_2020_RGB);
                            }
                            if(dataBlock.Current()[3]  & (1 << 7)) {
                                colorSpaceMap |= static_cast<uint16_t>(ColorSpaceType::DCI_P3);
                            }
                        }
                    }
                }
            }

            void SupportedTimings(std::vector<uint8_t>& vicList)
            {
                DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());
                while(dataBlock.Next()) {
                    if(dataBlock.IsValid() && (dataBlock.BlockTag() == 0x02)) {
                        for(uint8_t index = 0; index < dataBlock.BlockSize(); index++) {
                            const uint8_t vic = dataBlock.Current()[index] & 0x7F;
                            vicList.push_back(vic);
                        }
                    }
                }
            }

            void SupportedAudioFormats(uint32_t& audioFormatMap)
            {
                DataBlockIterator dataBlock = DataBlockIterator(_segment, DetailedTimingDescriptorStart());
                while(dataBlock.Next()) {
                    if(dataBlock.IsValid() && (dataBlock.BlockTag() == 0x01)) {
                        uint8_t sadCount = dataBlock.BlockSize() % 3;
                        for(uint8_t index = 0; index < dataBlock.BlockSize(); index+=3) {
                            const uint8_t sad = (dataBlock.Current()[index + 1] & 0x78) >> 3;
                            // switch(((blockStart[sadStart + 1] & 0x78) >> 3)) {
                            switch(sad){
                            case 0x01:
                                audioFormatMap |= static_cast<uint32_t>(AudioFormatType::LPCM);
                                break;
                            case 0x02:
                                audioFormatMap |= static_cast<uint32_t>(AudioFormatType::AC3);
                                break;
                            case 0x03:
                                audioFormatMap |= static_cast<uint32_t>(AudioFormatType::MPEG1);
                                break;
                            case 0x04:
                                audioFormatMap |= static_cast<uint32_t>(AudioFormatType::MP3);
                                break;
                            case 0x05:
                                audioFormatMap |= static_cast<uint32_t>(AudioFormatType::MPEG2);
                                break;
                            case 0x06:
                                audioFormatMap |= static_cast<uint32_t>(AudioFormatType::AAC_LC);
                                break;
                            case 0x07:
                                audioFormatMap |= static_cast<uint32_t>(AudioFormatType::DTS);
                                break;
                            case 0x08:
                                audioFormatMap |= static_cast<uint32_t>(AudioFormatType::ATRAC);
                                break;
                            case 0x09:
                                audioFormatMap |= static_cast<uint32_t>(AudioFormatType::SUPER_AUDIO_CD);
                                break;
                            case 0x0A:
                                audioFormatMap |= static_cast<uint32_t>(AudioFormatType::EAC3);
                                // if MPEG surround implicitly and explicitly supported: assume ATMOS
                                if((dataBlock.Current()[index + 3] & 0x01)) {
                                    audioFormatMap |= static_cast<uint32_t>(AudioFormatType::DOLBY_ATMOS);
                                }
                                break;
                            case 0x0B:
                                audioFormatMap |= static_cast<uint32_t>(AudioFormatType::DTSHD);
                                break;
                            case 0x0C:
                                audioFormatMap |= static_cast<uint32_t>(AudioFormatType::DOLBY_TRUEHD);
                                break;
                            case 0x0D:
                                audioFormatMap |= static_cast<uint32_t>(AudioFormatType::DST_AUDIO);
                                break;
                            case 0x0E:
                                audioFormatMap |= static_cast<uint32_t>(AudioFormatType::MS_WMA_PRO);
                                break;
                            case 0x0F:
                                switch((dataBlock.Current()[index + 3] & 0xF8) >> 3) {
                                    case 0x04:
                                        audioFormatMap |= static_cast<uint32_t>(AudioFormatType::MPEG4_HEAAC);
                                        break;
                                    case 0x05:
                                        audioFormatMap |= static_cast<uint32_t>(AudioFormatType::MPEG4_HEAAC_V2);
                                        break;
                                    case 0x06:
                                        audioFormatMap |= static_cast<uint32_t>(AudioFormatType::MPEG4_ACC_LC);
                                        break;
                                    case 0x07:
                                        audioFormatMap |= static_cast<uint32_t>(AudioFormatType::DRA);
                                        break;
                                    case 0x08:
                                        audioFormatMap |= static_cast<uint32_t>(AudioFormatType::MPEG4_HEAAC_MPEG_SURROUND);
                                        break;
                                    case 0x0A:
                                        audioFormatMap |= static_cast<uint32_t>(AudioFormatType::MPEG4_HEAAC_LC_MPEG_SURROUND);
                                        break;
                                    case 0x0B:
                                        audioFormatMap |= static_cast<uint32_t>(AudioFormatType::MPEGH_3DAUDIO);
                                        break;
                                    case 0x0C:
                                        audioFormatMap |= static_cast<uint32_t>(AudioFormatType::AC4);
                                        break;
                                    case 0x0D:
                                        audioFormatMap |= static_cast<uint32_t>(AudioFormatType::LPCM_3DAUDIO);
                                        break;
                                    default:
                                        break;
                                }
                            default:
                                break;
                        }
                    }
                }
            }
        }

        private:
            Buffer _segment;
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
        std::string Manufacturer() const {
            std::string result;
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
            return ((_base[14] & 0x80) != 0);
        }
        uint8_t BitsPerColor() const {
            static uint8_t bitsPerColor[] = { 0, 6, 8, 10, 12, 14, 16, 255 };
            return (bitsPerColor[(_base[0x14] >> 4) & 0x7]);
        }

        ColorDepthType ColorDepth() const {
            ColorDepthType colorDepth = ColorDepthType::BPC_UNDEFINED;
            if((_base[0x14] >> 7) & 0x01) {
                switch((_base[0x14] >> 4) & 0x07) {
                    case 0x01:
                        colorDepth = ColorDepthType::BPC_6;
                        break;
                    case 0x02:
                        colorDepth = ColorDepthType::BPC_8;
                        break;
                    case 0x03:
                        colorDepth = ColorDepthType::BPC_10;
                        break;
                    case 0x04:
                        colorDepth = ColorDepthType::BPC_12;
                        break;
                    case 0x05:
                        colorDepth = ColorDepthType::BPC_14;
                        break;
                    case 0x06:
                        colorDepth = ColorDepthType::BPC_16;
                        break;
                    default:
                        colorDepth = ColorDepthType::BPC_UNDEFINED;
                        break;
                }
            }

            return colorDepth;
        }

        Type VideoInterface() const {
            return (static_cast<Type> (_base[0x14] & 0x0F));
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

        void SupportedColorDepths(uint8_t& colorDepthMap) const {
            colorDepthMap |= static_cast<uint8_t>(ColorDepth());

            Iterator segment = Iterator(_segments);
            while(segment.Next() == true) {
                if(segment.Type() == CEA::extension_tag) {
                    CEA cae(segment.Current());
                    cae.SupportedColorDepths(colorDepthMap);
                }
            }
        }

        ColorFormatType SupportedColorFormat() const {
            ColorFormatType result = ColorFormatType::UNDEFINED;

            Iterator segment = Iterator(_segments);
            while(segment.Next() == true) {
                if(segment.Type() == CEA::extension_tag) {
                    CEA cae(segment.Current());
                    result = cae.SupportedColorFormat();
                }
            }

            return result;
        }

        void SupportedDigitalDisplayTypes(uint8_t& colorFormatMap) const {
            if((_base[0x14] >> 7) & 0x01){
                switch((_base[0x18] >> 3) & 0x07) {
                    case 0x00:
                        colorFormatMap |= static_cast<uint8_t>(ColorFormatType::RGB);
                        break;
                    case 0x01:
                        colorFormatMap |= static_cast<uint8_t>(ColorFormatType::RGB);
                        colorFormatMap |= static_cast<uint8_t>(ColorFormatType::YCBCR_4_4_4);
                        break;
                    case 0x02:
                        colorFormatMap |= static_cast<uint8_t>(ColorFormatType::RGB);
                        colorFormatMap |= static_cast<uint8_t>(ColorFormatType::YCBCR_4_2_2);
                        break;
                    case 0x03:
                        colorFormatMap |= static_cast<uint8_t>(ColorFormatType::RGB);
                        colorFormatMap |= static_cast<uint8_t>(ColorFormatType::YCBCR_4_2_2);
                        colorFormatMap |= static_cast<uint8_t>(ColorFormatType::YCBCR_4_4_4);
                        break;
                }
            }
        }

        void SupportedColorFormats(uint8_t& colorFormatMap) const {
            Iterator segment = Iterator(_segments);
            while(segment.Next() == true) {
                if(segment.Type() == CEA::extension_tag) {
                    CEA cae(segment.Current());
                    colorFormatMap |= static_cast<uint8_t>(cae.SupportedColorFormat());
                    cae.SupportedColorFormats(colorFormatMap);
                }
            }
            SupportedDigitalDisplayTypes(colorFormatMap);
        }

        void SupportedColorSpace(uint16_t& colorSpaceMap) const {
            if((_base[0x18] & (1 << 2))) {
                colorSpaceMap |= static_cast<uint16_t>(ColorSpaceType::SRGB);
            }

            Iterator segment = Iterator(_segments);
            while(segment.Next() == true) {
                if(segment.Type() == CEA::extension_tag) {
                    CEA cae(segment.Current());
                    cae.SupportedColorSpaces(colorSpaceMap);
                }
            }
        }

        void SupportedTimings(std::vector<uint8_t>& vicList) const {
            Iterator segment = Iterator(_segments);
            while(segment.Next() == true) {
                if(segment.Type() == CEA::extension_tag) {
                    CEA cae(segment.Current());
                    cae.SupportedTimings(vicList);
                }
            }
        }

        void SupportedAudioFormats(uint32_t& audioFormatMap) const {
            Iterator segment = Iterator(_segments);
            while(segment.Next() == true) {
                if(segment.Type() == CEA::extension_tag) {
                    CEA cae(segment.Current());
                    cae.SupportedAudioFormats(audioFormatMap);
                }
            }
        }

        void Clear() {
            _base[0] = 0x55;
            _segments.clear();
        }

    private:
        inline char ManufactereChar(uint8_t value) const {
            return static_cast<char>('A' + ((value - 1) & 0x1F));
        }

    private:
        Buffer _base;
        BufferList _segments;
    };
}
}
