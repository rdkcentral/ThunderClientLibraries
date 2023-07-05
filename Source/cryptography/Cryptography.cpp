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

#include "Module.h"

#include <ICryptography.h>

#include "implementation/cipher_implementation.h"
#include "implementation/diffiehellman_implementation.h"
#include "implementation/hash_implementation.h"
#include "implementation/vault_implementation.h"
#include "implementation/persistent_implementation.h"

#include <com/com.h>
#include <plugins/Types.h>

namespace WPEFramework {
namespace Implementation {
    static constexpr uint16_t TimeOut = 3000;
    static constexpr const TCHAR* PluginConnector = "/tmp/communicator";
    static constexpr const TCHAR* Callsign = "Svalbard";
    static constexpr const TCHAR* CryptographyConnector = "/tmp/svalbard";

    class CryptographyLink : public RPC::SmartInterfaceType<PluginHost::IPlugin> {
    private:
        using BaseClass = RPC::SmartInterfaceType<PluginHost::IPlugin>;

    public:
        CryptographyLink(const uint32_t waitTime, const Core::NodeId& thunder, const string& callsign)
            : BaseClass()
            , _adminLock()
        {
            ASSERT(_singleton==nullptr);
            _singleton=this;
            BaseClass::Open(waitTime, thunder, callsign);
        }
        ~CryptographyLink() override
        {
            _interfaces.Clear();
            BaseClass::Close(Core::infinite);
        }
        static CryptographyLink& Instance(const std::string& callsign = Callsign)
        {
            static CryptographyLink *instance = new CryptographyLink(TimeOut, PluginConnector, callsign);
            ASSERT(instance!=nullptr);
            return *instance;
        }
        static void Dispose()
        {
            ASSERT(_singleton != nullptr);

            if(_singleton != nullptr)
            {
                delete _singleton;
            }
        }
        Cryptography::ICryptography* Acquire(const Core::NodeId& nodeId)
        {
            return BaseClass::Acquire<Cryptography::ICryptography>(3000, nodeId, _T(""), ~0);
        }
        Cryptography::ICryptography* Cryptography(const std::string& connectionPoint);

        template <typename TYPE, typename... Args>
        Core::ProxyType<Core::IUnknown> Register(Args&&... args)
        {
            return (Core::ProxyType<Core::IUnknown>(_interfaces.template Instance<TYPE>(std::forward<Args>(args)...)));
        }

    private:
        void Operational(const bool upAndRunning) override
        {
            if (upAndRunning == false) {
                Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
                _interfaces.Clear();
            }
        }

    private:
        mutable Core::CriticalSection _adminLock;
        Core::ProxyListType<Core::IUnknown> _interfaces;
        static CryptographyLink* _singleton;
    };

    CryptographyLink* CryptographyLink::_singleton = nullptr;

    class RPCDiffieHellmanImpl : public Cryptography::IDiffieHellman {
    public:
        RPCDiffieHellmanImpl(Cryptography::IDiffieHellman* iface)
            : _accessor(iface)
        {
            if (_accessor != nullptr) {
                _accessor->AddRef();
            }
        }
        ~RPCDiffieHellmanImpl() override = default;

        BEGIN_INTERFACE_MAP(RPCDiffieHellmanImpl)
        INTERFACE_ENTRY(Cryptography::IDiffieHellman)
        END_INTERFACE_MAP

    public:
        uint32_t Generate(const uint8_t generator,
            const uint16_t modulusSize, const uint8_t modulus[],
            uint32_t& privKeyId, uint32_t& pubKeyId) override
        {
            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
            return (_accessor != nullptr) ? _accessor->Generate(generator, modulusSize, modulus, privKeyId, pubKeyId) : 0;
        }

        uint32_t Derive(const uint32_t privateKey, const uint32_t peerPublicKeyId, uint32_t& secretId) override
        {
            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
            return (_accessor != nullptr) ? _accessor->Derive(privateKey, peerPublicKeyId, secretId) : 0;
        }

        void Unlink()
        {
            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
            if (_accessor != nullptr) {
                _accessor->Release();
                _accessor = nullptr;
            }
        }

    private:
        Core::CriticalSection _adminLock;
        Cryptography::IDiffieHellman* _accessor;
    };

    class RPCCipherImpl : public Cryptography::ICipher {
    public:
        RPCCipherImpl(Cryptography::ICipher* iface)
            : _accessor(iface)
        {
            if (_accessor != nullptr) {
                _accessor->AddRef();
            }
        }
        ~RPCCipherImpl() override = default;

        BEGIN_INTERFACE_MAP(RPCCipherImpl)
        INTERFACE_ENTRY(Cryptography::ICipher)
        END_INTERFACE_MAP

    public:
        int32_t Encrypt(const uint8_t ivLength, const uint8_t iv[],
            const uint32_t inputLength, const uint8_t input[],
            const uint32_t maxOutputLength, uint8_t output[]) const override
        {
            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
            return (_accessor != nullptr) ? _accessor->Encrypt(ivLength, iv, inputLength, input, maxOutputLength, output) : 0;
        }

        int32_t Decrypt(const uint8_t ivLength, const uint8_t iv[],
            const uint32_t inputLength, const uint8_t input[],
            const uint32_t maxOutputLength, uint8_t output[]) const override
        {
            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
            return (_accessor != nullptr) ? _accessor->Decrypt(ivLength, iv, inputLength, input, maxOutputLength, output) : 0;
        }

        void Unlink()
        {
            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
            if (_accessor != nullptr) {
                _accessor->Release();
                _accessor = nullptr;
            }
        }

    private:
        mutable Core::CriticalSection _adminLock;
        Cryptography::ICipher* _accessor;
    };

    class RPCHashImpl : public Cryptography::IHash {
    public:
        RPCHashImpl(Cryptography::IHash* hash)
            : _accessor(hash)
        {
            if (_accessor != nullptr) {
                _accessor->AddRef();
            }
        }
        ~RPCHashImpl() override = default;

        BEGIN_INTERFACE_MAP(RPCHashImpl)
        INTERFACE_ENTRY(Cryptography::IHash)
        END_INTERFACE_MAP

    public:
        /* Ingest data into the hash calculator (multiple calls possible) */
        uint32_t Ingest(const uint32_t length, const uint8_t data[] /* @length:length */) override
        {
            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
            return (_accessor != nullptr ? _accessor->Ingest(length, data) : 0);
        }

        /* Calculate the hash from all ingested data */
        uint8_t Calculate(const uint8_t maxLength, uint8_t data[] /* @out @maxlength:maxLength */) override
        {
            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
            return (_accessor != nullptr ? _accessor->Calculate(maxLength, data) : 0);
        }

        void Unlink()
        {
            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
            if (_accessor != nullptr) {
                _accessor->Release();
                _accessor = nullptr;
            }
        }

    private:
        Core::CriticalSection _adminLock;
        Cryptography::IHash* _accessor;
    };

    class RPCVaultImpl : public Cryptography::IVault {
    public:
        RPCVaultImpl(Cryptography::IVault* vault)
            : _accessor(vault)
        {
            if (_accessor != nullptr) {
                _accessor->AddRef();
            }
        }
        ~RPCVaultImpl() override = default;

        BEGIN_INTERFACE_MAP(RPCVaultImpl)
        INTERFACE_ENTRY(Cryptography::IVault)
        END_INTERFACE_MAP

    public:
        // Return size of a vault data blob
        // (-1 if the blob exists in the vault but is not extractable and 0 if the ID does not exist)
        uint16_t Size(const uint32_t id) const override
        {
            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
            return (_accessor != nullptr ? _accessor->Size(id) : 0);
        }

        // Import unencrypted data blob into the vault (returns blob ID)
        // Note: User IDs are always greater than 0x80000000, values below 0x80000000 are reserved for implementation-specific internal data blobs.
        uint32_t Import(const uint16_t length, const uint8_t blob[] /* @length:length */) override
        {
            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
            return (_accessor != nullptr ? _accessor->Import(length, blob) : 0);
        }

        // Export unencrypted data blob out of the vault (returns blob ID), only public blobs are exportable
        uint16_t Export(const uint32_t id, const uint16_t maxLength, uint8_t blob[] /* @out @maxlength:maxLength */) const override
        {
            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
            return (_accessor != nullptr ? _accessor->Export(id, maxLength, blob) : 0);
        }

        // Set encrypted data blob in the vault (returns blob ID)
        uint32_t Set(const uint16_t length, const uint8_t blob[] /* @length:length */) override
        {
            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
            return (_accessor != nullptr ? _accessor->Set(length, blob) : 0);
        }

        // Get encrypted data blob out of the vault (data identified by ID, returns size of the retrieved data)
        uint16_t Get(const uint32_t id, const uint16_t maxLength, uint8_t blob[] /* @out @maxlength:maxLength */) const override
        {
            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
            return (_accessor != nullptr ? _accessor->Get(id, maxLength, blob) : 0);
        }

        // Delete a data blob from the vault
        bool Delete(const uint32_t id) override
        {
            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
            return (_accessor != nullptr ? _accessor->Delete(id) : false);
        }

        // Crypto operations using the vault for key storage
        // -----------------------------------------------------

        // Retrieve a HMAC calculator
        Cryptography::IHash* HMAC(const Cryptography::hashtype hashType, const uint32_t keyId) override
        {
            Cryptography::IHash* iface = nullptr;

            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);

            if (_accessor != nullptr) {

                iface = _accessor->HMAC(hashType, keyId);

                if (iface != nullptr) {
                    Core::ProxyType<Core::IUnknown> object = CryptographyLink::Instance().Register<RPCHashImpl>(iface);

                    ASSERT(object.IsValid() == true);

                    iface->Release();

                    iface = reinterpret_cast<Cryptography::IHash*>(object->QueryInterface(Cryptography::IHash::ID));
                }
            }

            return iface;
        }

        // Retrieve an AES encryptor/decryptor
        Cryptography::ICipher* AES(const Cryptography::aesmode aesMode, const uint32_t keyId) override
        {
            Cryptography::ICipher* iface = nullptr;

            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);

            if (_accessor != nullptr) {

                iface = _accessor->AES(aesMode, keyId);

                if (iface != nullptr) {
                    Core::ProxyType<Core::IUnknown> object = CryptographyLink::Instance().Register<RPCCipherImpl>(iface);

                    ASSERT(object.IsValid() == true);

                    iface->Release();

                    iface = reinterpret_cast<Cryptography::ICipher*>(object->QueryInterface(Cryptography::ICipher::ID));
                }
            }

            return iface;
        }

        // Retrieve a Diffie-Hellman key creator
        Cryptography::IDiffieHellman* DiffieHellman() override
        {
            Cryptography::IDiffieHellman* iface = nullptr;

            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);

            if (_accessor != nullptr) {

                iface = _accessor->DiffieHellman();

                if (iface != nullptr) {
                    Core::ProxyType<Core::IUnknown> object = CryptographyLink::Instance().Register<RPCDiffieHellmanImpl>(iface);

                    ASSERT(object.IsValid() == true);

                    iface->Release();

                    iface = reinterpret_cast<Cryptography::IDiffieHellman*>(object->QueryInterface(Cryptography::IDiffieHellman::ID));
                }
            }

            return iface;
        }

        void Unlink()
        {
            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
            if (_accessor != nullptr) {
                _accessor->Release();
                _accessor = nullptr;
            }
        }

    private:
        mutable Core::CriticalSection _adminLock;
        Cryptography::IVault* _accessor;
    };

    class RPCCryptographyImpl : public Cryptography::ICryptography {
    public:
        RPCCryptographyImpl() = delete;
        RPCCryptographyImpl(const RPCCryptographyImpl&) = delete;
        RPCCryptographyImpl& operator=(const RPCCryptographyImpl&) = delete;

        RPCCryptographyImpl(Cryptography::ICryptography* iface)
            : _accessor(iface)
        {
            _accessor->AddRef();
        }
        ~RPCCryptographyImpl() override = default;

        BEGIN_INTERFACE_MAP(RPCCryptographyImpl)
        INTERFACE_ENTRY(Cryptography::ICryptography)
        END_INTERFACE_MAP

    public:
        // Retrieve a hash calculator
        Cryptography::IHash* Hash(const Cryptography::hashtype hashType) override
        {
            Cryptography::IHash* iface = nullptr;

            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);

            if (_accessor != nullptr) {

                iface = _accessor->Hash(hashType);

                if (iface != nullptr) {
                    Core::ProxyType<Core::IUnknown> object = CryptographyLink::Instance().Register<RPCHashImpl>(iface);

                    ASSERT(object.IsValid() == true);

                    iface->Release();

                    iface = reinterpret_cast<Cryptography::IHash*>(object->QueryInterface(Cryptography::IHash::ID));
                }
            }

            return iface;
        }

        // Retrieve a vault (TEE identified by ID)
        Cryptography::IVault* Vault(const cryptographyvault id) override
        {
            Cryptography::IVault* iface = nullptr;

            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);

            if (_accessor != nullptr) {

                iface = _accessor->Vault(id);

                if (iface != nullptr) {
                    Core::ProxyType<Core::IUnknown> object = CryptographyLink::Instance().Register<RPCVaultImpl>(iface);

                    ASSERT(object.IsValid() == true);

                    iface->Release();

                    iface = reinterpret_cast<Cryptography::IVault*>(object->QueryInterface(Cryptography::IVault::ID));
                }
            }

            return iface;
        }

        void Unlink()
        {
            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
            if (_accessor != nullptr) {
                _accessor->Release();
                _accessor = nullptr;
            }
        }

    private:
        mutable Core::CriticalSection _adminLock;
        Cryptography::ICryptography* _accessor;
    };

    Cryptography::ICryptography* CryptographyLink::Cryptography(const std::string& connectionPoint)
    {
        Cryptography::ICryptography* iface = BaseClass::Acquire<Cryptography::ICryptography>(3000, Core::NodeId(connectionPoint.c_str()), _T(""), ~0);

        // Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);

        if (iface != nullptr) {
            Core::ProxyType<Core::IUnknown> object = Register<RPCCryptographyImpl>(iface);

            ASSERT(object.IsValid() == true);

            iface->Release();

            iface = reinterpret_cast<Cryptography::ICryptography*>(object->QueryInterface(Cryptography::ICryptography::ID));
        }

        return iface;
    }

    class HashImpl : public WPEFramework::Cryptography::IHash {
    public:
        HashImpl() = delete;
        HashImpl(const HashImpl&) = delete;
        HashImpl& operator=(const HashImpl&) = delete;

        HashImpl(HashImplementation* impl)
            : _implementation(impl)
        {
            ASSERT(_implementation != nullptr);
        }

        ~HashImpl() override
        {
            hash_destroy(_implementation);
        }

    public:
        uint32_t Ingest(const uint32_t length, const uint8_t data[]) override
        {
            return (hash_ingest(_implementation, length, data));
        }

        uint8_t Calculate(const uint8_t maxLength, uint8_t data[]) override
        {
            return (hash_calculate(_implementation, maxLength, data));
        }

    public:
        BEGIN_INTERFACE_MAP(HashImpl)
        INTERFACE_ENTRY(WPEFramework::Cryptography::IHash)
        END_INTERFACE_MAP

    private:
        HashImplementation* _implementation;
    }; // class HashImpl

    class VaultImpl : public WPEFramework::Cryptography::IVault, public WPEFramework::Cryptography::IPersistent {
    public:
        VaultImpl() = delete;
        VaultImpl(const VaultImpl&) = delete;
        VaultImpl& operator=(const VaultImpl&) = delete;

        VaultImpl(VaultImplementation* impl)
            : _implementation(impl)
        {
            ASSERT(_implementation != nullptr);
        }

        ~VaultImpl() override = default;

    public:
        uint16_t Size(const uint32_t id) const override
        {
            return (vault_size(_implementation, id));
        }

        uint32_t Import(const uint16_t length, const uint8_t blob[] ) override
        {
            return (vault_import(_implementation, length, blob));
        }

        uint16_t Export(const uint32_t id, const uint16_t maxLength, uint8_t blob[]) const override
        {
            return (vault_export(_implementation, id, maxLength, blob));
        }

        uint32_t Set(const uint16_t length, const uint8_t blob[]) override
        {
            return (vault_set(_implementation, length, blob));
        }

        uint16_t Get(const uint32_t id, const uint16_t maxLength, uint8_t blob[]) const override
        {
            return (vault_get(_implementation, id, maxLength, blob));
        }

        bool Delete(const uint32_t id) override
        {
            return (vault_delete(_implementation, id));
        }
   
        uint32_t Exists(const string& locator,bool& result) const override
        {
            return(persistent_key_exists(_implementation,locator.c_str(),&result));
        }

        uint32_t Load(const string& locator,uint32_t& id) override
        {
            return(persistent_key_load(_implementation,locator.c_str(),&id));
        }

        uint32_t Create(const string& locator, const keytype keyType,uint32_t&  id ) override
        {
            return(persistent_key_create(_implementation,locator.c_str(), static_cast<key_type>(keyType),&id));
        }

        uint32_t Flush() override
        {
            return(persistent_flush(_implementation));
        }
 
        VaultImplementation* Implementation()
        {
            return _implementation;
        }

        const VaultImplementation* Implementation() const
        {
            return _implementation;
        }

        class HMACImpl : public HashImpl {
        public:
            HMACImpl() = delete;
            HMACImpl(const HMACImpl&) = delete;
            HMACImpl& operator=(const HMACImpl&) = delete;

            HMACImpl(VaultImpl* vault, HashImplementation* implementation)
                : HashImpl(implementation)
                , _vault(vault)
            {
                ASSERT(vault != nullptr);
                _vault->AddRef();
            }

            ~HMACImpl() override
            {
                _vault->Release();
            }

        private:
            VaultImpl* _vault;
        }; // class HMACImpl

        class CipherImpl : public WPEFramework::Cryptography::ICipher {
        public:
            CipherImpl() = delete;
            CipherImpl(const CipherImpl&) = delete;
            CipherImpl& operator=(const CipherImpl&) = delete;

            CipherImpl(VaultImpl* vault, CipherImplementation* implementation)
                : _vault(vault)
                , _implementation(implementation)
            {
                ASSERT(_implementation != nullptr);
                ASSERT(_vault != nullptr);
                _vault->AddRef();
            }

            ~CipherImpl() override
            {
                cipher_destroy(_implementation);
                _vault->Release();
            }

        public:
            int32_t Encrypt(const uint8_t ivLength, const uint8_t iv[],
                const uint32_t inputLength, const uint8_t input[],
                const uint32_t maxOutputLength, uint8_t output[]) const override
            {
                return (cipher_encrypt(_implementation, ivLength, iv, inputLength, input, maxOutputLength, output));
            }

            int32_t Decrypt(const uint8_t ivLength, const uint8_t iv[],
                const uint32_t inputLength, const uint8_t input[],
                const uint32_t maxOutputLength, uint8_t output[]) const override
            {
                return (cipher_decrypt(_implementation, ivLength, iv, inputLength, input, maxOutputLength, output));
            }

        public:
            BEGIN_INTERFACE_MAP(CipherImpl)
            INTERFACE_ENTRY(WPEFramework::Cryptography::ICipher)
            END_INTERFACE_MAP

        private:
            VaultImpl* _vault;
            CipherImplementation* _implementation;
        }; // class CipherImpl

        class DiffieHellmanImpl : public WPEFramework::Cryptography::IDiffieHellman {
        public:
            DiffieHellmanImpl() = delete;
            DiffieHellmanImpl(const DiffieHellmanImpl&) = delete;
            DiffieHellmanImpl& operator=(const DiffieHellmanImpl&) = delete;

            DiffieHellmanImpl(VaultImpl* vault)
                : _vault(vault)
            {
                ASSERT(_vault != nullptr);
                _vault->AddRef();
            }

            ~DiffieHellmanImpl() override
            {
                _vault->Release();
            }

        public:
            uint32_t Generate(const uint8_t generator, const uint16_t modulusSize, const uint8_t modulus[],
                uint32_t& privKeyId, uint32_t& pubKeyId) override
            {
                return (diffiehellman_generate(_vault->Implementation(), generator, modulusSize, modulus, &privKeyId, &pubKeyId));
            }

            uint32_t Derive(const uint32_t privateKeyId, const uint32_t peerPublicKeyId, uint32_t& secretId) override
            {
                return (diffiehellman_derive(_vault->Implementation(), privateKeyId, peerPublicKeyId, &secretId));
            }

        public:
            BEGIN_INTERFACE_MAP(DiffieHellmanImpl)
            INTERFACE_ENTRY(WPEFramework::Cryptography::IDiffieHellman)
            END_INTERFACE_MAP

        private:
            VaultImpl* _vault;
        }; // class DiffieHellmanImpl

        WPEFramework::Cryptography::IHash* HMAC(const WPEFramework::Cryptography::hashtype hashType,
            const uint32_t secretId) override
        {
            WPEFramework::Cryptography::IHash* hmac(nullptr);

            HashImplementation* impl = hash_create_hmac(_implementation, static_cast<hash_type>(hashType), secretId);

            if (impl != nullptr) {
                hmac = WPEFramework::Core::Service<HMACImpl>::Create<WPEFramework::Cryptography::IHash>(this, impl);
                ASSERT(hmac != nullptr);

                if (hmac == nullptr) {
                    hash_destroy(impl);
                }
            }

            return (hmac);
        }

        WPEFramework::Cryptography::ICipher* AES(const WPEFramework::Cryptography::aesmode aesMode,
            const uint32_t keyId) override
        {
            WPEFramework::Cryptography::ICipher* cipher(nullptr);

            CipherImplementation* impl = cipher_create_aes(_implementation, static_cast<aes_mode>(aesMode), keyId);

            if (impl != nullptr) {
                cipher = Core::Service<CipherImpl>::Create<WPEFramework::Cryptography::ICipher>(this, impl);
                ASSERT(cipher != nullptr);

                if (cipher == nullptr) {
                    cipher_destroy(impl);
                }
            }

            return (cipher);
        }

        WPEFramework::Cryptography::IDiffieHellman* DiffieHellman() override
        {
            WPEFramework::Cryptography::IDiffieHellman* dh = Core::Service<DiffieHellmanImpl>::Create<WPEFramework::Cryptography::IDiffieHellman>(this);
            ASSERT(dh != nullptr);
            return (dh);
        }

    public:
        BEGIN_INTERFACE_MAP(VaultImpl)
        INTERFACE_ENTRY(WPEFramework::Cryptography::IVault)
        INTERFACE_ENTRY(WPEFramework::Cryptography::IPersistent)
        END_INTERFACE_MAP

    private:
        VaultImplementation* _implementation;
    }; // class VaultImpl

    class CryptographyImpl : public WPEFramework::Cryptography::ICryptography {
    public:
        CryptographyImpl(const CryptographyImpl&) = delete;
        CryptographyImpl& operator=(const CryptographyImpl&) = delete;
        CryptographyImpl() = default;
        ~CryptographyImpl() override = default;

    public:
        WPEFramework::Cryptography::IHash* Hash(const WPEFramework::Cryptography::hashtype hashType) override
        {
            WPEFramework::Cryptography::IHash* hash(nullptr);

            HashImplementation* impl = hash_create(static_cast<hash_type>(hashType));
            if (impl != nullptr) {
                hash = WPEFramework::Core::Service<Implementation::HashImpl>::Create<WPEFramework::Cryptography::IHash>(impl);
                ASSERT(hash != nullptr);

                if (hash == nullptr) {
                    hash_destroy(impl);
                }
            }

            return (hash);
        }

        WPEFramework::Cryptography::IVault* Vault(const cryptographyvault id) override
        {
            WPEFramework::Cryptography::IVault* vault(nullptr);
            VaultImplementation* impl = vault_instance(id);

            if (impl != nullptr) {
                vault = WPEFramework::Core::Service<Implementation::VaultImpl>::Create<WPEFramework::Cryptography::IVault>(impl);
                ASSERT(vault != nullptr);
            }

            return (vault);
        }

    public:
        BEGIN_INTERFACE_MAP(CryptographyImpl)
        INTERFACE_ENTRY(WPEFramework::Cryptography::ICryptography)
        END_INTERFACE_MAP
    }; // class CryptographyImpl

} // namespace Implementation

/* static */ Cryptography::ICryptography* Cryptography::ICryptography::Instance(const std::string& connectionPoint)
{
    Cryptography::ICryptography* result(nullptr);

    if (connectionPoint.empty() == true) {
        result = Core::Service<Implementation::CryptographyImpl>::Create<Cryptography::ICryptography>();
    } else {
        // Seems we received a connection point
        result = Implementation::CryptographyLink::Instance().Cryptography(connectionPoint);
    }

    return result;
}

} // namespace WPEFramework
