/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Metrological
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

#include "../../Module.h"
#include "../random_implementation.h"
#include <openssl/rand.h>

namespace Implementation {

static uint16_t Generate(const uint16_t length, uint8_t data[])
{
    uint16_t result = 0;

    if (RAND_bytes(data, length) == 1) {
        result = length;
    }

    return (result);
}

} // namespace Implementation

extern "C" {

uint16_t random_generate(const uint16_t length, uint8_t data[])
{
    return (Implementation::Generate(length, data));
}

} // extern "C"
