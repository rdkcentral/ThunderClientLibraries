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

#include "Module.h"

#include <core/core.h>

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <interfaces/ICryptography.h>
#include <interfaces/INetflixSecurity.h>

namespace {

using namespace WPEFramework;

static bool GenerateAESKey(const std::string passphrase, const uint32_t iterations, const uint16_t sizeBits, const std::string name,
                           const Exchange::CryptographyVault vaultId, const string connector = "")
{
    bool result = false;

    Exchange::ICryptography* crypto = Exchange::ICryptography::Instance(connector);

    printf("Generating %d-bit AES key with PKBDF2 (HMAC-256, %d iterations)...\n", sizeBits, iterations);

    if (crypto == nullptr) {
        printf("Cryptography not available!\n");
    }
    else if ((iterations != 0) && (sizeBits % 8 == 0) && (sizeBits <= 512)) {

        Exchange::IVault* vault = crypto->Vault(vaultId);

        const uint16_t size = (sizeBits / 8);

        if (vault != nullptr) {
            uint8_t salt[16];
            uint8_t* hash = new uint8_t[size];

            RAND_bytes(salt, sizeof(salt));

            if (PKCS5_PBKDF2_HMAC(passphrase.c_str(), passphrase.size(), salt, sizeof(salt), iterations, EVP_sha256(), size, hash)) {

                const uint32_t keyId = vault->Import(size, hash);

                ::memset(hash, 0xFF, size);

                uint8_t encryptedKey[64];
                const uint16_t encryptedKeySize = vault->Get(keyId, sizeof(encryptedKey), encryptedKey);

                Core::File keyFile(name);

                if ((keyFile.Exists() == false) && (keyFile.Create() == true)) {
                    keyFile.Write(encryptedKey, encryptedKeySize);
                    keyFile.Close();

                    printf("Genenerated AES key %s\n", keyFile.Name().c_str());
                    result = true;
                }

                vault->Delete(keyId);
            }

            delete[] hash;

            vault->Release();
        }
    }
    else {
        printf("Invalid parameters!\n");
    }

    if (crypto != nullptr) {
        crypto->Release();
    }

    return (result);
}

}


int main(const int argc, const char* argv[])
{
    int result = 0;

    if ((argc == 6) || (argc == 5)) {
        uint32_t iterations = (argc == 6? atoi(argv[5]) : 500000);

        Exchange::CryptographyVault vaultId = static_cast<Exchange::CryptographyVault>(~0);

        if (::strcmp(argv[4], "netflix") == 0) {
            vaultId = Exchange::CRYPTOGRAPHY_VAULT_NETFLIX;
        }
        else if (::strcmp(argv[4], "provisionig") == 0) {
            vaultId = Exchange::CRYPTOGRAPHY_VAULT_PROVISIONING;
        }
        else if (::strcmp(argv[4], "platform") == 0) {
            vaultId = Exchange::CRYPTOGRAPHY_VAULT_PLATFORM;
        }
        else if (::strcmp(argv[4], "default") == 0) {
            vaultId = Exchange::CRYPTOGRAPHY_VAULT_DEFAULT;
        }

        if (vaultId != static_cast<Exchange::CryptographyVault>(~0)) {
            if (GenerateAESKey(argv[2], iterations, atoi(argv[3]), argv[1], vaultId) == false) {
                printf("FAILED to generate a key!\n");
                result = 1;
            }
        }
        else {
            printf("invalid vault (must be default, netflix, provisioning or platform)\n");
            result = 1;
        }
    }
    else {
        printf("usage: %s <filename> <passphrase> <sizebits> <vault> [iterations]\n", argv[0]);
    }

    return (result);
}

