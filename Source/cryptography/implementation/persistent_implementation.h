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

#include <stdint.h>
#include <stdbool.h>
#include "cryptography_vault_ids.h"
#include "vault_implementation.h" 

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
        AES128,
        AES256,
        HMAC128,
        HMAC160,
        HMAC256
}key_type;

EXTERNAL uint32_t persistent_key_exists( struct VaultImplementation* vault ,const char locator[],bool* result);

EXTERNAL uint32_t persistent_key_load(struct VaultImplementation* vault,const char locator[],uint32_t*  id);

EXTERNAL uint32_t persistent_key_create( struct VaultImplementation* vault,const char locator[],const key_type keyType,uint32_t* id);

EXTERNAL uint32_t persistent_flush(struct VaultImplementation* vault);

#ifdef __cplusplus
} // extern "C"
#endif

