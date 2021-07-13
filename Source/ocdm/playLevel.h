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

#include "Module.h"

using namespace WPEFramework::Core ;

/** Playback protection levels struct. */
struct PlayLevels {

    class PlayLevelsJSON : public JSON::Container {
    private:
        PlayLevelsJSON(const PlayLevelsJSON&) = delete;
        PlayLevelsJSON& operator=(const PlayLevelsJSON&) = delete;

    public:
        PlayLevelsJSON()
            : WPEFramework::Core::JSON::Container()
            , m_CompressedVideo(0)
            , m_UncompressedVideo(0)
            , m_AnalogVideo(0)
            , m_CompressedAudio(0)
            , m_UncompressedAudio(0)
            , m_maxDecodeWidth(0)
            , m_maxDecodeHeigth(0)
        {
            Add(_T("compressed-video"), &m_CompressedVideo);
            Add(_T("uncompressed-video"), &m_UncompressedVideo);
            Add(_T("analog-video"), &m_AnalogVideo);
            Add(_T("compressed-audio"), &m_CompressedAudio);
            Add(_T("uncompressed-audio"), &m_UncompressedAudio);
            Add(_T("max-decode-width"), &m_maxDecodeWidth);
            Add(_T("max-decode-height"), &m_maxDecodeHeigth);
        }

    public:
        JSON::DecUInt16 m_CompressedVideo;
        JSON::DecUInt16 m_UncompressedVideo;
        JSON::DecUInt16 m_AnalogVideo;
        JSON::DecUInt16 m_CompressedAudio;
        JSON::DecUInt16 m_UncompressedAudio;
        JSON::DecUInt32 m_maxDecodeWidth;
        JSON::DecUInt32 m_maxDecodeHeigth;
    };


    uint16_t compressedDigitalVideoLevel_;   //!< Compressed digital video output protection level.
    uint16_t uncompressedDigitalVideoLevel_; //!< Uncompressed digital video output protection level.
    uint16_t analogVideoLevel_;              //!< Analog video output protection level.
    uint16_t compressedDigitalAudioLevel_;   //!< Compressed digital audio output protection level.
    uint16_t uncompressedDigitalAudioLevel_; //!< Uncompressed digital audio output protection level.
    uint32_t maxResDecodeWidth_;             //!< Max res decode width in pixels.
    uint32_t maxResDecodeHeight_;            //!< Max res decode height in pixels.
};