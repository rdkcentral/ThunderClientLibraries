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

#ifndef VAULT_IMPLEMENTATION_H
#define VAULT_IMPLEMENTATION_H

#include <stdint.h>
#include <stdbool.h>

#undef EXTERNAL
#if defined(WIN32) || defined(_WINDOWS) || defined (__CYGWIN__) || defined(_WIN64)
#ifdef CRYPTOGRAPHY_EXPORTS
#define EXTERNAL __declspec(dllexport)
#else
#define EXTERNAL __declspec(dllimport)
#pragma comment(lib, "cryptography.lib")
#endif
#else
#define EXTERNAL __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum cryptographyvault : uint8_t {
    CRYPTOGRAPHY_VAULT_DEFAULT = 0,
    CRYPTOGRAPHY_VAULT_PLATFORM = 1,
    CRYPTOGRAPHY_VAULT_PROVISIONING = 0x10,
    CRYPTOGRAPHY_VAULT_NETFLIX = 0x11,
    CRYPTOGRAPHY_VAULT_COBALT = 0x12
};

struct VaultImplementation;

EXTERNAL struct VaultImplementation* vault_instance(const enum cryptographyvault id);

EXTERNAL uint16_t vault_size(const struct VaultImplementation* vault, const uint32_t id);

EXTERNAL uint32_t vault_import(struct VaultImplementation* vault, const uint16_t length, const uint8_t blob[]);

EXTERNAL uint16_t vault_export(const struct VaultImplementation* vault, const uint32_t id, const uint16_t max_length, uint8_t blob[]);

EXTERNAL uint32_t vault_set(struct VaultImplementation* vault, const uint16_t length, const uint8_t blob[]);

EXTERNAL uint16_t vault_get(const struct VaultImplementation* vault, const uint32_t id, const uint16_t max_length, uint8_t blob[]);

EXTERNAL uint32_t vault_generate(struct VaultImplementation* vault, const uint16_t length);

EXTERNAL bool vault_delete(struct VaultImplementation* vault, const uint32_t id);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // VAULT_IMPLEMENTATION_H
