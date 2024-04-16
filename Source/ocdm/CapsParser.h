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

#include <interfaces/IDRM.h>

namespace Thunder {
    namespace Plugin {

        class CapsParser {
        public:
            CapsParser(const CapsParser&) = delete;
            CapsParser& operator= (const CapsParser&) = delete;

            CapsParser();
            ~CapsParser();

        public:
            void Parse(const uint8_t* info, const uint16_t infoLength); 

            uint16_t GetHeight() const { 
                return _height; 
            } 
            uint16_t GetWidth() const { 
                return _width; 
            } 
            CDMi::MediaType GetMediaType() const { 
                return _mediaType; 
            } 
        
        private:
            std::string FindMarker(const std::string& data, const TCHAR* tag) const;

        private:
            size_t _lastHash;

            CDMi::MediaType _mediaType;
            uint16_t _width;
            uint16_t _height;
        };
    }
}
