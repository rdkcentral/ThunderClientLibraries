/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Metrological
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

namespace Thunder {
namespace Compositor {
    struct Character {
        int codePoint;
        int x;
        int y;
        int width;
        int height;
        int originX;
        int originY;
    };

    struct Font {
        const char* name;
        const char* file;
        int size;
        int bold;
        int italic;
        int width;
        int height;
        int characterCount;
        const Character* characters;
        
        Font(const char* n, const char* f, int s, int b, int i, 
             int w, int h, int count, const Character* chars)
            : name(n), file(f), size(s), bold(b), italic(i)
            , width(w), height(h), characterCount(count), characters(chars)
        {
        }
    };
} // namespace Compositor
} // namespace Thunder