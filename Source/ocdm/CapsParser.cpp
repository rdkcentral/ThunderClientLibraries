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

#include "CapsParser.h"

static constexpr TCHAR StartingCharacter = ')';
static constexpr TCHAR EndingCharacter   = ',';
static constexpr TCHAR WidthTag[]        = _T("width");
static constexpr TCHAR HeightTag[]       = _T("height");
static constexpr TCHAR MediaTag[]        = _T("original-media-type");

namespace Thunder {
    namespace Plugin {

        CapsParser::CapsParser() 
            : _lastHash(0)
            , _mediaType(CDMi::Unknown)
            , _width(0)
            , _height(0) {
        }

        CapsParser::~CapsParser() {
        }

        void CapsParser::Parse(const uint8_t* info, const uint16_t infoLength) /* override */ 
        {
            if(infoLength > 0) {
                std::string infoStr(reinterpret_cast<const char*>(info), infoLength);

                std::hash<::string> hash_fn;
                size_t info_hash = hash_fn(infoStr);
                if(_lastHash != info_hash) {
                    _lastHash = info_hash;

                    // Parse the data
                    std::string result = FindMarker(infoStr, MediaTag);
                    if(!result.empty()) {
                        if(result.find("video") != ::string::npos) {
                            _mediaType = CDMi::Video;
                        }
                        else if(result.find("audio") != ::string::npos) {
                            _mediaType = CDMi::Audio;
                        }
                        else {
                            TRACE(Trace::Error, (Core::Format(_T("Found and unknown media type %s\n"), result.c_str())));
                            _mediaType = CDMi::Unknown;
                        }
                    }
                    else {
                        TRACE(Trace::Warning, (_T("No result for media type")));
                    }

                    if(_mediaType == CDMi::Video) {

                        result = FindMarker(infoStr, WidthTag);

                        if (result.length() > 0) {
                            _width = Core::NumberType<uint16_t>(result.c_str(), result.length(), NumberBase::BASE_DECIMAL);
                        }
                        else {
                            _width = 0;
                            TRACE(Trace::Warning, (_T("No result for width")));
                        }

                        result = FindMarker(infoStr, HeightTag);

                        if (result.length() > 0) {
                            _height = Core::NumberType<uint16_t>(result.c_str(), result.length(), NumberBase::BASE_DECIMAL);
                        }
                        else {
                            _height = 0;
                            TRACE(Trace::Warning, (_T("No result for height")));
                        }
                    }
                    else {
                        // Audio
                        _width  = 0;
                        _height = 0;
                    }
                }
            }
        }

        std::string CapsParser::FindMarker(const std::string& data, const TCHAR tag[]) const
        {
            std::string retVal;

            size_t found = data.find(tag);
            TRACE(Trace::Warning, (Core::Format(_T("Found tag <%s> in <%s> at location %ld"), tag, data.c_str(), found)));
            if(found != ::string::npos) {
                // Found the marker
                // Find the end of the gst caps type identifier
                size_t start = data.find(StartingCharacter, found) + 1;  // step over the ")"
                size_t end = data.find(EndingCharacter, start);
                if(end == ::string::npos) {
                    // Went past the end of the string
                    end = data.length();
                }
                retVal = data.substr(start, end - start);
                TRACE(Trace::Warning, (Core::Format(_T("Found substr <%s>"), retVal.c_str())));
            }
            return retVal;
        }
    }
}
