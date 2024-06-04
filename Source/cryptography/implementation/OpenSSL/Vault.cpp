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

#include "../../Module.h"

#include <persistent_implementation.h>
#include <vault_implementation.h>

#include <cryptalgo/cryptalgo.h>

#if defined(USE_PROVISIONING)
#include <interfaces/IProvisioning.h>
#include <provision/DRMInfo.h>

using namespace Thunder;
#endif

#include <openssl/evp.h>
#include <openssl/rand.h>

#include "Derive.h"
#include "Vault.h"

namespace Implementation {

#if defined(USE_PROVISIONING)
static string GetEndPoint()
{
    TCHAR* value = ::getenv(_T("PROVISION_PATH"));

#ifdef __WINDOWS__
    return (value == nullptr ? _T("127.0.0.1:7777") : value);
#else
    return (value == nullptr ? _T("/tmp/provision") : value);
#endif
}
#endif

static constexpr uint8_t IV_SIZE = 16;

namespace Netflix {

    static constexpr uint32_t KPE_ID = 1;
    static constexpr uint32_t KPH_ID = 2;
    static constexpr uint32_t KPW_ID = 3;
    // static constexpr uint32_t KPW_TEST_ID = 4;
    static constexpr uint32_t ESN_ID = 16;

    static constexpr uint8_t MAX_ESN_SIZE = 64;

} // namespace Netflix

/* static */ Vault& Vault::NetflixInstance()
{
#if defined(USE_PROVISIONING)
    static const uint8_t key[] = { 0x41, 0xde, 0xba, 0x86, 0x9f, 0xcf, 0x2e, 0xb1, 0xcc, 0x0c, 0x15, 0xb7, 0x4e, 0xd0, 0x9d, 0x90 };

    auto ctor = [](Vault& vault) {
        auto engine = Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create();
        auto client = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId(GetEndPoint().c_str()), Core::ProxyType<Core::IIPCServer>(engine));

        if ((client.IsValid() == true) && (client->IsOpen() == false)) {

            Exchange::IProvisioning* provisioning = client->Open<Exchange::IProvisioning>("Provisioning");

            if (provisioning != nullptr) {
                std::string label;
                Core::SystemInfo::GetEnvironment(_T("NETFLIX_VAULT"), label);

                if (label.empty()) {
                    label = "netflix";
                    TRACE_L1(_T("Set label to default \"%s\""), label.c_str());
                }

PUSH_WARNING(DISABLE_WARNING_NON_STANDARD_EXTENSION_USED, DISABLE_WARNING_PEDANTIC)
                struct NetflixData {
                    uint8_t salt[16];
                    uint8_t kpe[16];
                    uint8_t kph[32];
                    uint8_t esn[0];
                };
POP_WARNING()

                uint8_t encrypted_data[1024];
                uint16_t encrypted_data_size = sizeof(encrypted_data);

                uint32_t error = provisioning->DRMId(label, encrypted_data_size, encrypted_data);

                if (error == Core::ERROR_NONE) {
                    uint8_t netflix_data[256];
                    uint16_t netflix_data_size = sizeof(netflix_data);

                    netflix_data_size = ClearBlob(encrypted_data_size, reinterpret_cast<const char*>(encrypted_data), sizeof(netflix_data), reinterpret_cast<char*>(netflix_data));

                    if (netflix_data_size > 0) {
                        TRACE_L1(_T("Received Provision Info for '%s' with length [%d]."), label.c_str(), netflix_data_size);

                        NetflixData* data = reinterpret_cast<NetflixData*>(netflix_data);

                        VARIABLE_IS_NOT_USED uint32_t kpeId = vault.Import(sizeof(NetflixData::kpe), data->kpe, false);
                        ASSERT(kpeId == Netflix::KPE_ID);

                        VARIABLE_IS_NOT_USED uint32_t kphId = vault.Import(sizeof(NetflixData::kph), data->kph, false);
                        ASSERT(kphId == Netflix::KPH_ID);

                        uint8_t kpw[32];
                        // kpe and kph are already concatenated in the correct order
                        Netflix::DeriveWrappingKey(data->kpe, (sizeof(data->kpe) + sizeof(data->kph)), sizeof(kpw), kpw);
                        VARIABLE_IS_NOT_USED uint32_t kdwId = vault.Import(16, kpw, false); // take the first 16 bytes only!
                        ASSERT(kdwId == Netflix::KPW_ID);

                        // Let's (ab)use the vault to hold the ESN as well
                        vault._lastHandle = (Netflix::ESN_ID - 1);
                        VARIABLE_IS_NOT_USED uint32_t esnId = vault.Import((netflix_data_size - sizeof(NetflixData)), data->esn, true);
                        ASSERT(esnId == Netflix::ESN_ID);

                        TRACE_L1("Imported pre-shared keys and ESN () into the Netflix vault");

                    } else {
                        TRACE_L1(_T("Failed to get %s"), label.c_str());
                    }
                } else {
                    TRACE_L1(_T("Failed to extract %s provisioning. Error code %d."), label.c_str(), error);
                }

                provisioning->Release();
            }
        }
    };
#else
    static const uint8_t key[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x11 };

    auto ctor = [](Vault& vault) {
        std::string path;
        Thunder::Core::SystemInfo::GetEnvironment(_T("NETFLIX_VAULT"), path);
        Thunder::Core::File file(path);

        if (file.Open(true) == true) {
PUSH_WARNING(DISABLE_WARNING_NON_STANDARD_EXTENSION_USED, DISABLE_WARNING_PEDANTIC)
            struct NetflixData {
                uint8_t salt[16];
                uint8_t kpe[16];
                uint8_t kph[32];
                uint8_t esn[0];
            };
POP_WARNING()

            uint64_t fileSize = file.Size();

            if ((fileSize > (IV_SIZE + sizeof(NetflixData))) && (fileSize <= (IV_SIZE + sizeof(NetflixData) + Netflix::MAX_ESN_SIZE))) {
                uint16_t blobSize = (static_cast<uint16_t>(fileSize) - IV_SIZE);
                uint8_t* decryptedBlob = reinterpret_cast<uint8_t*>(ALLOCA(blobSize));
                uint8_t* input = reinterpret_cast<uint8_t*>(ALLOCA(fileSize));
                file.Read(input, static_cast<uint32_t>(fileSize));
                blobSize = vault.Cipher(false, (IV_SIZE + blobSize), input, blobSize, decryptedBlob);

                if (blobSize >= sizeof(NetflixData)) {
                    NetflixData* data = reinterpret_cast<NetflixData*>(decryptedBlob);

                    VARIABLE_IS_NOT_USED uint32_t kpeId = vault.Import(sizeof(NetflixData::kpe), data->kpe, false);
                    ASSERT(kpeId == Netflix::KPE_ID);

                    VARIABLE_IS_NOT_USED uint32_t kphId = vault.Import(sizeof(NetflixData::kph), data->kph, false);
                    ASSERT(kphId == Netflix::KPH_ID);

                    uint8_t kpw[32];
                    // kpe and kph are already concatenated in the correct order
                    Netflix::DeriveWrappingKey(data->kpe, (sizeof(data->kpe) + sizeof(data->kph)), sizeof(kpw), kpw);
                    VARIABLE_IS_NOT_USED uint32_t kdwId = vault.Import(16, kpw, false); // take the first 16 bytes only!
                    ASSERT(kdwId == Netflix::KPW_ID);

                    // Let's (ab)use the vault to hold the ESN as well
                    vault._lastHandle = (Netflix::ESN_ID - 1);
                    VARIABLE_IS_NOT_USED uint32_t esnId = vault.Import((blobSize - sizeof(NetflixData)), data->esn, true);
                    ASSERT(esnId == Netflix::ESN_ID);

                    TRACE_L1("Imported pre-shared keys and ESN into the Netflix vault");
                }
            }

            file.Close();
        }
    };

#endif

    auto dtor = [](Vault& vault) {
        vault.Delete(Netflix::ESN_ID);
        vault.Delete(Netflix::KPW_ID);
        vault.Delete(Netflix::KPH_ID);
        vault.Delete(Netflix::KPE_ID);
    };

    static Vault instance(string(reinterpret_cast<const char*>(key), sizeof(key)), ctor, dtor);
    return (instance);
}

/* static */ Vault& Vault::PlatformInstance()
{
    static const uint8_t key[] = { 0x42, 0x71, 0x7b, 0x85, 0x98, 0x61, 0xe3, 0x19, 0x16, 0xd1, 0xc7, 0x28, 0x02, 0x9a, 0xc4, 0x07 };

    static Vault instance(string(reinterpret_cast<const char*>(key), sizeof(key)));
    return (instance);
}

Vault::Vault(const string key, const Callback& ctor, const Callback& dtor)
    : _lock()
    , _items()
    , _lastHandle(0)
    , _vaultKey(key)
    , _dtor(dtor)
{
    if (ctor != nullptr) {
        ctor(*this);
    }

    _lastHandle = 0x80000000;
}

Vault::~Vault()
{
    if (_dtor != nullptr) {
        _dtor(*this);
    }
}

uint16_t Vault::Cipher(bool encrypt, const uint16_t inSize, const uint8_t input[], const uint16_t maxOutSize, uint8_t output[]) const
{
    uint16_t result = 0;

    ASSERT(maxOutSize >= (inSize + (encrypt ? IV_SIZE : -IV_SIZE)));

    if (maxOutSize >= (inSize + (encrypt ? IV_SIZE : -IV_SIZE))) {
        uint8_t newIv[IV_SIZE];
        const uint8_t* iv = nullptr;
        const uint8_t* inputBuffer = nullptr;
        uint16_t inputSize = 0;
        uint8_t* outputBuffer = nullptr;
        uint16_t totalLen = 0;

        if (encrypt) {
            RAND_bytes(newIv, sizeof(newIv));
            iv = newIv;
            inputBuffer = input;
            inputSize = inSize;
            outputBuffer = output + IV_SIZE;
            ::memcpy(output, iv, IV_SIZE);
            totalLen += IV_SIZE;
        } else {
            iv = input;
            inputBuffer = (input + IV_SIZE);
            inputSize = (inSize - IV_SIZE);
            outputBuffer = output;
        }

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        ASSERT(ctx != nullptr);

        // AES-CTR ensures same buffer size after encryption
        if (EVP_CipherInit_ex(ctx, EVP_aes_128_ctr(), nullptr, reinterpret_cast<const unsigned char*>(_vaultKey.data()), iv, encrypt) != 0) {

            int outLen = 0;

            if (EVP_CipherUpdate(ctx, outputBuffer, &outLen, inputBuffer, inputSize) != 0) {
                totalLen += outLen;
                outLen = 0;

                if (EVP_CipherFinal_ex(ctx, (outputBuffer + totalLen), &outLen) != 0) {
                    totalLen += outLen;
                    result = totalLen;
                }
            }
        }

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
        EVP_CIPHER_CTX_reset(ctx);
#else
        EVP_CIPHER_CTX_cleanup(ctx);
#endif
    }

    return (result);
}

uint16_t Vault::Size(const uint32_t id, bool allowSealed) const
{
    uint16_t size = 0;

    _lock.Lock();
    auto it = _items.find(id);
    if (it != _items.end()) {
        if ((allowSealed == true) || (*it).second.IsExportable() == true) {
            size = ((*it).second.Size() - IV_SIZE);
            TRACE_L2("%sBlob id 0x%08x size: %i",
                (((allowSealed == true) || ((*it).second.IsExportable() == false)) ? "Internal: " : ""), id, size);
        } else {
            TRACE_L2("Blob id 0x%08x is sealed, won't tell its size", id);
            size = USHRT_MAX;
        }
    } else {
        TRACE_L1("Failed to look up blob id 0x%08x", id);
    }
    _lock.Unlock();

    return (size);
}

uint32_t Vault::Import(const uint16_t size, const uint8_t blob[], bool exportable)
{
    uint32_t id = 0;

    if (size > 0) {
        _lock.Lock();
        id = (_lastHandle + 1);

        if (id != 0) {
            uint8_t* buf = reinterpret_cast<uint8_t*>(ALLOCA(USHRT_MAX));
            uint16_t len = Cipher(true, size, blob, USHRT_MAX, buf);

            _items.emplace(std::piecewise_construct,
                std::forward_as_tuple(id),
                std::forward_as_tuple(exportable, len, buf));

            _lastHandle = id;

            TRACE_L2("Added a %s data blob of size %i as id 0x%08x", (exportable ? "clear" : "sealed"), (len - IV_SIZE), id);
        }
        _lock.Unlock();
    }

    return (id);
}

uint16_t Vault::Export(const uint32_t id, const uint16_t size, uint8_t blob[], bool allowSealed) const
{
    uint16_t outSize = 0;

    if (size > 0) {
        _lock.Lock();
        auto it = _items.find(id);
        if (it != _items.end()) {
            if ((allowSealed == true) || ((*it).second.IsExportable() == true)) {
                outSize = Cipher(false, (*it).second.Size(), (*it).second.Buffer(), size, blob);

                TRACE_L2("%sExported %i bytes from blob id 0x%08x",
                    (((allowSealed == true) || ((*it).second.IsExportable() == false)) ? "Internal: " : ""), outSize, id);
            } else {
                TRACE_L1("Blob id 0x%08x is sealed, can't export", id);
            }
        } else {
            TRACE_L1("Failed to look up blob id 0x%08x", id);
        }
        _lock.Unlock();
    }

    return (outSize);
}

uint32_t Vault::Put(const uint16_t size, const uint8_t blob[])
{
    uint32_t id = 0;

    if (size > 0) {
        _lock.Lock();
        id = (_lastHandle + 1);
        if (id != 0) {
            _items.emplace(std::piecewise_construct,
                std::forward_as_tuple(id),
                std::forward_as_tuple(false, size, blob));

            _lastHandle = id;

            TRACE_L2("Inserted a sealed data blob of size %i as id 0x%08x", size, id);
        }
        _lock.Unlock();
    }

    return (id);
}

uint16_t Vault::Get(const uint32_t id, const uint16_t size, uint8_t blob[]) const
{
    uint16_t result = 0;

    if (size > 0) {
        _lock.Lock();
        auto it = _items.find(id);
        if (it != _items.end()) {
            result = std::min(size, static_cast<uint16_t>((*it).second.Size()));
            ::memcpy(blob, (*it).second.Buffer(), result);
            TRACE_L2("Retrieved a sealed data blob id 0x%08x of size %i bytes", id, result);
        }
        _lock.Unlock();
    }

    return (result);
}

uint32_t Vault::Generate(const uint16_t size)
{
    uint32_t id = 0;

    if (size > 0) {
        uint8_t* buffer = static_cast<uint8_t*>(ALLOCA(size));
        ASSERT(buffer != nullptr);

        if (RAND_bytes(buffer, size) != 0) {
            id = Import(size, buffer);
            TRACE_L2("Generated random blob id 0x%08x of size %d bytes", id, size);
        }
        else {
            TRACE_L2("PRNG not available");
        }

        ::memset(buffer, 0x00, size);
    }

    return (id);
}

bool Vault::Delete(const uint32_t id)
{
    bool result = false;

    _lock.Lock();
    auto it = _items.find(id);
    if (it != _items.end()) {
        _items.erase(it);
        result = true;
    }
    _lock.Unlock();

    return (result);
}

} // namespace Implementation

extern "C" {

// Vault

VaultImplementation* vault_instance(const cryptographyvault id)
{
    Implementation::Vault* vault = nullptr;

    switch (id) {
    case CRYPTOGRAPHY_VAULT_NETFLIX:
        vault = &Implementation::Vault::NetflixInstance();
        break;
    case CRYPTOGRAPHY_VAULT_PLATFORM:
        vault = &Implementation::Vault::PlatformInstance();
        break;
    default:
        TRACE_L1("Vault not supported: %d", static_cast<uint32_t>(id));
        break;
    }

    return reinterpret_cast<VaultImplementation*>(vault);
}

uint16_t vault_size(const VaultImplementation* vault, const uint32_t id)
{
    ASSERT(vault != nullptr);
    const Implementation::Vault* vaultImpl = reinterpret_cast<const Implementation::Vault*>(vault);
    return (vaultImpl->Size(id));
}

uint32_t vault_import(VaultImplementation* vault, const uint16_t length, const uint8_t data[])
{
    ASSERT(vault != nullptr);
    Implementation::Vault* vaultImpl = reinterpret_cast<Implementation::Vault*>(vault);
    return (vaultImpl->Import(length, data, true /* imported in clear is always exportable */));
}

uint16_t vault_export(const VaultImplementation* vault, const uint32_t id, const uint16_t max_length, uint8_t data[])
{
    ASSERT(vault != nullptr);
    const Implementation::Vault* vaultImpl = reinterpret_cast<const Implementation::Vault*>(vault);
    return (vaultImpl->Export(id, max_length, data));
}

uint32_t vault_set(VaultImplementation* vault, const uint16_t length, const uint8_t data[])
{
    ASSERT(vault != nullptr);
    Implementation::Vault* vaultImpl = reinterpret_cast<Implementation::Vault*>(vault);
    return (vaultImpl->Put(length, data));
}

uint16_t vault_get(const VaultImplementation* vault, const uint32_t id, const uint16_t max_length, uint8_t data[])
{
    ASSERT(vault != nullptr);
    const Implementation::Vault* vaultImpl = reinterpret_cast<const Implementation::Vault*>(vault);
    return (vaultImpl->Get(id, max_length, data));
}

uint32_t vault_generate(VaultImplementation* vault, const uint16_t length)
{
    ASSERT(vault != nullptr);
    Implementation::Vault* vaultImpl = reinterpret_cast<Implementation::Vault*>(vault);
    return (vaultImpl->Generate(length));
}

bool vault_delete(VaultImplementation* vault, const uint32_t id)
{
    ASSERT(vault != nullptr);
    Implementation::Vault* vaultImpl = reinterpret_cast<Implementation::Vault*>(vault);
    return (vaultImpl->Delete(id));
}

// Netflix Security

uint16_t netflix_security_esn(const uint16_t max_length, uint8_t data[])
{
    if (data == nullptr) {
        return (Implementation::Vault::NetflixInstance().Size(Implementation::Netflix::ESN_ID));
    } else {
        return (Implementation::Vault::NetflixInstance().Export(Implementation::Netflix::ESN_ID, max_length, data));
    }
}

uint32_t netflix_security_encryption_key(void)
{
    return (Implementation::Vault::NetflixInstance().Size(Implementation::Netflix::KPE_ID) != 0 ? Implementation::Netflix::KPE_ID : 0);
}

uint32_t netflix_security_hmac_key(void)
{
    return (Implementation::Vault::NetflixInstance().Size(Implementation::Netflix::KPH_ID) != 0 ? Implementation::Netflix::KPH_ID : 0);
}

uint32_t netflix_security_wrapping_key(void)
{
    return (Implementation::Vault::NetflixInstance().Size(Implementation::Netflix::KPW_ID) != 0 ? Implementation::Netflix::KPW_ID : 0);
}

uint32_t persistent_key_exists(struct VaultImplementation* /* vault */, const char* /* locator[] */, bool* /* result */)
{
    return (Thunder::Core::ERROR_UNAVAILABLE);
}

uint32_t persistent_key_load(struct VaultImplementation* /* vault */, const char* /* locator[] */, uint32_t* /* id */)
{
    return (Thunder::Core::ERROR_UNAVAILABLE);
}

uint32_t persistent_key_create(struct VaultImplementation* /* vault */, const char* /* locator[] */, const key_type /* keyType */, uint32_t* /* id */)
{
    return (Thunder::Core::ERROR_UNAVAILABLE);
}

uint32_t persistent_flush(struct VaultImplementation* /* vault */)
{
    return (Thunder::Core::ERROR_UNAVAILABLE);
}

} // extern "C"
