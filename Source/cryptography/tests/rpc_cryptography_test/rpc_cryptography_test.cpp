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

#include <gtest/gtest.h>

#include <com/com.h>
#include <core/core.h>
#include <plugins/plugins.h>

#include <cryptography.h>

namespace Thunder = WPEFramework;

static constexpr uint32_t TimeOut = 1000; //Thunder::Core::infinite;
static constexpr const TCHAR* PluginConnector = "/tmp/communicator";
static constexpr const TCHAR* PluginController = "Controller";

template <typename T, size_t size>
::testing::AssertionResult ArraysMatch(const T (&expected)[size], const T (&actual)[size])
{
    for (size_t i(0); i < size; ++i) {
        if (expected[i] != actual[i]) {
            return ::testing::AssertionFailure() << "array[" << i
                                                 << "] (" << actual[i] << ") != expected[" << i
                                                 << "] (" << expected[i] << ")";
        }
    }

    return ::testing::AssertionSuccess();
}

class Controller {
private:
    using Engine = Thunder::RPC::InvokeServerType<1, 0, 6>;

public:
    Controller() = delete;
    Controller(const uint32_t waitTime, const Thunder::Core::NodeId& thunder, const string& callsign)
        : _engine(Thunder::Core::ProxyType<Engine>::Create())
        , _client(Thunder::Core::ProxyType<Thunder::RPC::CommunicatorClient>::Create(
              thunder,
              Thunder::Core::ProxyType<Thunder::Core::IIPCServer>(_engine)))
    {
        _engine->Announcements(_client->Announcement());
    }
    virtual ~Controller()
    {
    }

public:
    uint32_t ActivatePlugin(string callsign)
    {
        uint32_t result(Thunder::Core::ERROR_NONE);

        if (IsPluginActive(callsign) == false) {
            Thunder::PluginHost::IShell* plugin = _client->Open<Thunder::PluginHost::IShell>(callsign, ~0, TimeOut);

            if (plugin != nullptr) {
                result = plugin->Activate(Thunder::PluginHost::IShell::REQUESTED);
                plugin->Release();
            }

            _client->Close(TimeOut);
        }

        return result;
    }

    uint32_t DeactivatePlugin(string callsign)
    {
        uint32_t result(Thunder::Core::ERROR_NONE);

        if (IsPluginActive(callsign) == true) {
            Thunder::PluginHost::IShell* plugin = _client->Open<Thunder::PluginHost::IShell>(callsign, ~0, TimeOut);

            if (plugin != nullptr) {
                result = plugin->Deactivate(Thunder::PluginHost::IShell::REQUESTED);
                plugin->Release();
            }

            _client->Close(TimeOut);
        }

        return result;
    }

    bool IsPluginActive(string callsign)
    {
        Thunder::PluginHost::IShell::state state(Thunder::PluginHost::IShell::DESTROYED);

        Thunder::PluginHost::IShell* plugin = _client->Open<Thunder::PluginHost::IShell>(callsign, ~0, TimeOut);

        if (plugin != nullptr) {
            state = plugin->State();
            plugin->Release();
        }

        _client->Close(TimeOut);

        return (state == Thunder::PluginHost::IShell::ACTIVATED) ? true : false;
    }

private:
    Thunder::Core::ProxyType<Engine> _engine;
    Thunder::Core::ProxyType<Thunder::RPC::CommunicatorClient> _client;
};

namespace TestData {
const string nodeId("/tmp/svalbard");

const string plugin("Svalbard");

const char data[] = "super secret string of secrets with even more secrets";

const uint8_t expectedSHA1HashOfData[] = {
    0xE8, 0x3D, 0x50, 0x5C, 0x94,
    0x77, 0xD7, 0xA7, 0x73, 0x68,
    0x7B, 0xE0, 0x46, 0xD0, 0xF8,
    0xEE, 0x2D, 0xC2, 0xDB, 0x68
};

const uint8_t hashkey[] = {
    0x7C, 0xF3, 0xA6, 0x2F, 0xB3, 0xC6, 0xB6, 0x43,
    0xA4, 0x2B, 0x18, 0x9B, 0x97, 0xBC, 0x59, 0x5D,
    0x51, 0x77, 0x51, 0xEC, 0x7C, 0x8B, 0x4B, 0xFE,
    0x68, 0xFE, 0xD5, 0xD8, 0x1C, 0x0A, 0xEC, 0x26
};

const uint8_t cipherkey[] = {
    0x7C, 0xF3, 0xA6, 0x2F, 0xB3, 0xC6, 0xB6, 0x43,
    0x68, 0xFE, 0xD5, 0xD8, 0x1C, 0x0A, 0xEC, 0x26
};

const uint8_t dhModulus[] = {
    0xC6, 0x37, 0x5D, 0xC5, 0x55, 0x78, 0x49, 0x8D,
    0x7F, 0xFA, 0x85, 0xF5, 0x7F, 0xF8, 0x10, 0x0A,
    0x77, 0x88, 0x2D, 0xA6, 0x14, 0x71, 0xFC, 0x10,
    0x57, 0x12, 0xFE, 0xAA, 0x4F, 0x62, 0xC3, 0x43
};
}

class BasicTest : public ::testing::Test {
protected:
    BasicTest()
        : cryptography(nullptr)
        , controller(TimeOut, PluginConnector, PluginController)
    {
    }

    ~BasicTest() override
    {
    }

    virtual void SetUp()
    {
        cryptography = Thunder::Cryptography::ICryptography::Instance(TestData::nodeId);
    }

    virtual void TearDown()
    {
        if (cryptography != nullptr) {
            cryptography->Release();
            cryptography = nullptr;
        }
    }

    Thunder::Cryptography::ICryptography* cryptography;
    Controller controller;
};

TEST_F(BasicTest, PluginActive)
{
    ASSERT_EQ(controller.ActivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);

    ASSERT_TRUE(controller.IsPluginActive(TestData::plugin));
}

TEST_F(BasicTest, PluginActivation)
{
    ASSERT_EQ(controller.ActivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);

    ASSERT_TRUE(controller.IsPluginActive(TestData::plugin));

    ASSERT_EQ(controller.ActivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);

    ASSERT_EQ(controller.DeactivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);

    ASSERT_EQ(controller.ActivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);
}

TEST_F(BasicTest, Cryptrography)
{
    ASSERT_EQ(controller.ActivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);
    ASSERT_TRUE(controller.IsPluginActive(TestData::plugin));
    ASSERT_NE(nullptr, cryptography);
}

TEST_F(BasicTest, Vault)
{
    ASSERT_TRUE(controller.IsPluginActive(TestData::plugin));
    ASSERT_NE(nullptr, cryptography);

    Thunder::Cryptography::IVault* vault = cryptography->Vault(CRYPTOGRAPHY_VAULT_PLATFORM);

    ASSERT_NE(nullptr, vault);

    if (vault != nullptr) {
        vault->Release();
        vault = nullptr;
    }
}

TEST_F(BasicTest, VaultImportExport)
{
    uint32_t id(0);
    char exportBuffer[400];
    memset(exportBuffer, 0, sizeof(exportBuffer));

    ASSERT_EQ(controller.ActivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);
    ASSERT_TRUE(controller.IsPluginActive(TestData::plugin));
    ASSERT_NE(nullptr, cryptography);

    Thunder::Cryptography::IVault* vault = cryptography->Vault(CRYPTOGRAPHY_VAULT_PLATFORM);

    ASSERT_NE(nullptr, vault);

    id = vault->Import(sizeof(TestData::data), reinterpret_cast<const uint8_t*>(TestData::data));

    EXPECT_GT(id, 0x8000000);

    EXPECT_EQ(vault->Size(id), sizeof(TestData::data));

    EXPECT_EQ(vault->Export(id, sizeof(exportBuffer), reinterpret_cast<uint8_t*>(exportBuffer)), sizeof(TestData::data));

    EXPECT_STREQ(TestData::data, exportBuffer);

    EXPECT_TRUE(vault->Delete(id));

    EXPECT_EQ(vault->Size(id), 0);

    if (vault != nullptr) {
        vault->Release();
        vault = nullptr;
    }
}

TEST_F(BasicTest, VaultSetGet)
{
    uint32_t id(0);
    char exportBuffer[400];
    memset(exportBuffer, 0, sizeof(exportBuffer));

    ASSERT_EQ(controller.ActivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);
    ASSERT_TRUE(controller.IsPluginActive(TestData::plugin));
    ASSERT_NE(nullptr, cryptography);

    Thunder::Cryptography::IVault* vault = cryptography->Vault(CRYPTOGRAPHY_VAULT_PLATFORM);

    ASSERT_NE(nullptr, vault);

    id = vault->Set(sizeof(TestData::data), reinterpret_cast<const uint8_t*>(TestData::data));

    EXPECT_GT(id, 0x8000000);

    EXPECT_EQ(vault->Size(id), USHRT_MAX);

    EXPECT_EQ(vault->Get(id, sizeof(exportBuffer), reinterpret_cast<uint8_t*>(exportBuffer)), sizeof(TestData::data));

    EXPECT_STREQ(TestData::data, exportBuffer);

    EXPECT_TRUE(vault->Delete(id));

    EXPECT_EQ(vault->Size(id), 0);

    if (vault != nullptr) {
        vault->Release();
        vault = nullptr;
    }
}

TEST_F(BasicTest, VaultDiffieHellman)
{
    ASSERT_EQ(controller.ActivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);
    ASSERT_TRUE(controller.IsPluginActive(TestData::plugin));
    ASSERT_NE(nullptr, cryptography);

    Thunder::Cryptography::IVault* vault = cryptography->Vault(CRYPTOGRAPHY_VAULT_PLATFORM);

    ASSERT_NE(nullptr, vault);

    Thunder::Cryptography::IDiffieHellman* dh = vault->DiffieHellman();

    EXPECT_NE(nullptr, dh);

    if (dh != nullptr) {
        dh->Release();
        dh = nullptr;
    }

    if (vault != nullptr) {
        vault->Release();
        vault = nullptr;
    }
}

TEST_F(BasicTest, VaultHMAC)
{
    ASSERT_EQ(controller.ActivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);
    ASSERT_TRUE(controller.IsPluginActive(TestData::plugin));
    ASSERT_NE(nullptr, cryptography);

    Thunder::Cryptography::IVault* vault = cryptography->Vault(CRYPTOGRAPHY_VAULT_PLATFORM);

    ASSERT_NE(nullptr, vault);

    uint32_t keyId = vault->Import(sizeof(TestData::cipherkey), reinterpret_cast<const uint8_t*>(TestData::cipherkey));

    Thunder::Cryptography::IHash* iface = vault->HMAC(Thunder::Cryptography::SHA1, keyId);

    EXPECT_NE(nullptr, iface);

    if (iface != nullptr) {
        iface->Release();
        iface = nullptr;
    }

    if (vault != nullptr) {
        vault->Release();
        vault = nullptr;
    }
}

TEST_F(BasicTest, VaultHMACIngestCalculate)
{
    uint8_t hashBuffer[20];
    memset(hashBuffer, 0, sizeof(hashBuffer));

    uint8_t hashExpected[] = {
        130, 119, 142, 1, 91,
        110, 173, 90, 240, 86,
        248, 76, 153, 38, 170,
        216, 150, 153, 123, 119
    };

    ASSERT_EQ(controller.ActivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);
    ASSERT_TRUE(controller.IsPluginActive(TestData::plugin));
    ASSERT_NE(nullptr, cryptography);

    Thunder::Cryptography::IVault* vault = cryptography->Vault(CRYPTOGRAPHY_VAULT_PLATFORM);

    ASSERT_NE(nullptr, vault);

    uint32_t keyId = vault->Import(sizeof(TestData::cipherkey), TestData::cipherkey);

    Thunder::Cryptography::IHash* iface = vault->HMAC(Thunder::Cryptography::SHA1, keyId);

    if (vault != nullptr) {
        vault->Release();
        vault = nullptr;
    }

    ASSERT_NE(nullptr, iface);

    uint32_t ingestSize = iface->Ingest(sizeof(TestData::data), reinterpret_cast<const uint8_t*>(TestData::data));

    EXPECT_EQ(ingestSize, sizeof(TestData::data));

    ingestSize = iface->Calculate(sizeof(hashBuffer), hashBuffer);

    EXPECT_EQ(ingestSize, Thunder::Cryptography::SHA1);

    EXPECT_TRUE(ArraysMatch(hashBuffer, hashExpected));

    if (iface != nullptr) {
        iface->Release();
        iface = nullptr;
    }
}

TEST_F(BasicTest, VaultHMACIngest65K)
{
    uint8_t hashBuffer[20];
    memset(hashBuffer, 0, sizeof(hashBuffer));

    ASSERT_EQ(controller.ActivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);
    ASSERT_TRUE(controller.IsPluginActive(TestData::plugin));
    ASSERT_NE(nullptr, cryptography);

    Thunder::Cryptography::IVault* vault = cryptography->Vault(CRYPTOGRAPHY_VAULT_PLATFORM);

    ASSERT_NE(nullptr, vault);

    uint32_t keyId = vault->Import(sizeof(TestData::cipherkey), TestData::cipherkey);

    Thunder::Cryptography::IHash* iface = vault->HMAC(Thunder::Cryptography::SHA1, keyId);

    if (vault != nullptr) {
        vault->Release();
        vault = nullptr;
    }

    ASSERT_NE(nullptr, iface);

    uint32_t totalSize(0);
    uint32_t ingestSize(0);

    while (totalSize < UINT16_MAX) {
        totalSize += iface->Ingest(sizeof(TestData::data), reinterpret_cast<const uint8_t*>(TestData::data));
    }

    ingestSize = iface->Calculate(sizeof(hashBuffer), hashBuffer);

    EXPECT_EQ(ingestSize, Thunder::Cryptography::SHA1);

    if (iface != nullptr) {
        iface->Release();
        iface = nullptr;
    }
}

TEST_F(BasicTest, VaultAES)
{
    ASSERT_EQ(controller.ActivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);
    ASSERT_TRUE(controller.IsPluginActive(TestData::plugin));
    ASSERT_NE(nullptr, cryptography);

    Thunder::Cryptography::IVault* vault = cryptography->Vault(CRYPTOGRAPHY_VAULT_PLATFORM);

    ASSERT_NE(nullptr, vault);

    uint32_t keyId = vault->Import(sizeof(TestData::cipherkey), TestData::cipherkey);

    Thunder::Cryptography::ICipher* iface = vault->AES(Thunder::Cryptography::CBC, keyId);

    ASSERT_NE(nullptr, iface);

    if (iface != nullptr) {
        iface->Release();
        iface = nullptr;
    }

    if (vault != nullptr) {
        vault->Release();
        vault = nullptr;
    }
}

TEST_F(BasicTest, VaultAESEncryptDecrypt)
{
    uint8_t encryptBuffer[128];
    uint8_t clearBuffer[128];

    Thunder::Cryptography::ICipher* iface = nullptr;

    memset(encryptBuffer, 0x00, sizeof(encryptBuffer));
    memset(clearBuffer, 0x00, sizeof(clearBuffer));

    ASSERT_EQ(controller.ActivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);
    ASSERT_TRUE(controller.IsPluginActive(TestData::plugin));
    ASSERT_NE(nullptr, cryptography);

    Thunder::Cryptography::IVault* vault = cryptography->Vault(CRYPTOGRAPHY_VAULT_PLATFORM);

    ASSERT_NE(nullptr, vault);

    uint32_t keyId = vault->Import(sizeof(TestData::cipherkey), TestData::cipherkey);

    iface = vault->AES(Thunder::Cryptography::CBC, keyId);

    ASSERT_NE(nullptr, iface);

    uint32_t encryptedSize = iface->Encrypt(
        sizeof(TestData::cipherkey), TestData::cipherkey,
        sizeof(TestData::data), reinterpret_cast<const uint8_t*>(TestData::data),
        sizeof(encryptBuffer), encryptBuffer);

    ASSERT_EQ(encryptedSize, 64);

    uint32_t clearSize = iface->Decrypt(
        sizeof(TestData::cipherkey), TestData::cipherkey,
        encryptedSize, encryptBuffer,
        sizeof(clearBuffer), clearBuffer);

    EXPECT_EQ(memcmp(clearBuffer, TestData::data, sizeof(TestData::data)), 0);

    EXPECT_EQ(clearSize, sizeof(TestData::data));

    if (iface != nullptr) {
        iface->Release();
        iface = nullptr;
    }

    if (vault != nullptr) {
        vault->Release();
        vault = nullptr;
    }
}

TEST_F(BasicTest, VaultAESEncryptDecryptDisablePlugin)
{
    uint8_t encryptBuffer[128];
    memset(encryptBuffer, 0, sizeof(encryptBuffer));

    ASSERT_EQ(controller.ActivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);
    ASSERT_TRUE(controller.IsPluginActive(TestData::plugin));
    ASSERT_NE(nullptr, cryptography);

    Thunder::Cryptography::IVault* vault = cryptography->Vault(CRYPTOGRAPHY_VAULT_PLATFORM);

    cryptography->Release();
    cryptography = nullptr;

    ASSERT_NE(nullptr, vault);

    uint32_t keyId = vault->Import(sizeof(TestData::cipherkey), TestData::cipherkey);

    Thunder::Cryptography::ICipher* iface = vault->AES(Thunder::Cryptography::CBC, keyId);

    ASSERT_NE(nullptr, iface);

    EXPECT_EQ(controller.DeactivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);

    uint32_t encryptedSize = iface->Encrypt(
        sizeof(TestData::cipherkey), TestData::cipherkey,
        sizeof(TestData::data), reinterpret_cast<const uint8_t*>(TestData::data),
        sizeof(encryptBuffer), encryptBuffer);

    EXPECT_EQ(encryptedSize, 0);

    uint32_t clearSize = iface->Decrypt(
        sizeof(TestData::cipherkey), TestData::cipherkey,
        sizeof(TestData::data), reinterpret_cast<const uint8_t*>(TestData::data),
        sizeof(encryptBuffer), encryptBuffer);

    EXPECT_EQ(clearSize, 0);

    if (vault != nullptr) {
        vault->Release();
        vault = nullptr;
    }

    if (iface != nullptr) {
        iface->Release();
        iface = nullptr;
    }

    ASSERT_EQ(controller.ActivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);
}

TEST_F(BasicTest, Hash)
{
    ASSERT_EQ(controller.ActivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);
    ASSERT_TRUE(controller.IsPluginActive(TestData::plugin));
    ASSERT_NE(nullptr, cryptography);

    Thunder::Cryptography::IHash* hash = cryptography->Hash(Thunder::Cryptography::SHA1);

    ASSERT_NE(nullptr, hash);

    if (hash != nullptr) {
        hash->Release();
        hash = nullptr;
    }
}

TEST_F(BasicTest, HashSHA1Calculate)
{
    uint8_t exportBuffer[Thunder::Cryptography::SHA1];
    memset(exportBuffer, 0, sizeof(exportBuffer));

    ASSERT_EQ(controller.ActivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);
    ASSERT_TRUE(controller.IsPluginActive(TestData::plugin));
    ASSERT_NE(nullptr, cryptography);

    Thunder::Cryptography::IHash* hash = cryptography->Hash(Thunder::Cryptography::SHA1);

    ASSERT_NE(nullptr, hash);

    EXPECT_EQ(hash->Ingest(sizeof(TestData::data), reinterpret_cast<const uint8_t*>(TestData::data)), sizeof(TestData::data));

    EXPECT_EQ(hash->Calculate(sizeof(exportBuffer), exportBuffer), Thunder::Cryptography::SHA1);

    EXPECT_TRUE(ArraysMatch(exportBuffer, TestData::expectedSHA1HashOfData));

    if (hash != nullptr) {
        hash->Release();
        hash = nullptr;
    }
}

TEST_F(BasicTest, HashSHA1CalculateDeactivate)
{
    uint8_t exportBuffer[Thunder::Cryptography::SHA1];
    memset(exportBuffer, 0, sizeof(exportBuffer));

    ASSERT_EQ(controller.ActivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);
    ASSERT_TRUE(controller.IsPluginActive(TestData::plugin));
    ASSERT_NE(nullptr, cryptography);

    Thunder::Cryptography::IHash* hash = cryptography->Hash(Thunder::Cryptography::SHA1);

    ASSERT_NE(nullptr, hash);

    EXPECT_EQ(hash->Ingest(sizeof(TestData::data), reinterpret_cast<const uint8_t*>(TestData::data)), sizeof(TestData::data));

    EXPECT_EQ(controller.DeactivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);

    EXPECT_EQ(hash->Calculate(sizeof(exportBuffer), exportBuffer), 0);

    EXPECT_EQ(controller.ActivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);

    if (hash != nullptr) {
        hash->Release();
        hash = nullptr;
    }
}

TEST_F(BasicTest, HashSHA1CalculateDeactivateAndRecover)
{
    uint8_t exportBuffer[Thunder::Cryptography::SHA1];
    memset(exportBuffer, 0, sizeof(exportBuffer));

    ASSERT_EQ(controller.ActivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);
    ASSERT_TRUE(controller.IsPluginActive(TestData::plugin));
    ASSERT_NE(nullptr, cryptography);

    Thunder::Cryptography::IHash* hash = cryptography->Hash(Thunder::Cryptography::SHA1);
    ASSERT_NE(nullptr, hash);

    EXPECT_EQ(hash->Ingest(sizeof(TestData::data), reinterpret_cast<const uint8_t*>(TestData::data)), sizeof(TestData::data));

    EXPECT_EQ(controller.DeactivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);

    EXPECT_EQ(hash->Calculate(sizeof(exportBuffer), exportBuffer), 0);

    cryptography->Release();
    hash->Release();

    ASSERT_EQ(controller.ActivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);
    ASSERT_TRUE(controller.IsPluginActive(TestData::plugin));

    cryptography = Thunder::Cryptography::ICryptography::Instance(TestData::nodeId);
    ASSERT_NE(nullptr, cryptography);

    hash = cryptography->Hash(Thunder::Cryptography::SHA1);
    ASSERT_NE(nullptr, hash);

    EXPECT_EQ(hash->Ingest(sizeof(TestData::data), reinterpret_cast<const uint8_t*>(TestData::data)), sizeof(TestData::data));

    EXPECT_EQ(hash->Calculate(sizeof(exportBuffer), exportBuffer), Thunder::Cryptography::SHA1);

    EXPECT_TRUE(ArraysMatch(exportBuffer, TestData::expectedSHA1HashOfData));

    if (hash != nullptr) {
        hash->Release();
        hash = nullptr;
    }
}

TEST_F(BasicTest, VaultDiffieHellmanDeavtivated)
{
    ASSERT_EQ(controller.ActivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);
    ASSERT_TRUE(controller.IsPluginActive(TestData::plugin));
    ASSERT_NE(nullptr, cryptography);

    Thunder::Cryptography::IVault* vault = cryptography->Vault(CRYPTOGRAPHY_VAULT_PLATFORM);

    ASSERT_NE(nullptr, vault);

    EXPECT_EQ(controller.DeactivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);

    Thunder::Cryptography::IDiffieHellman* df = vault->DiffieHellman();

    EXPECT_EQ(nullptr, df);

    if (vault != nullptr) {
        vault->Release();
        vault = nullptr;
    }

    ASSERT_EQ(controller.ActivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);
}

TEST_F(BasicTest, VaultDiffieHellmanGenerate)
{
    ASSERT_EQ(controller.ActivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);
    ASSERT_TRUE(controller.IsPluginActive(TestData::plugin));
    ASSERT_NE(nullptr, cryptography);

    Thunder::Cryptography::IVault* vault = cryptography->Vault(CRYPTOGRAPHY_VAULT_PLATFORM);

    ASSERT_NE(nullptr, vault);

    Thunder::Cryptography::IDiffieHellman* dh = vault->DiffieHellman();

    vault->Release();
    vault = nullptr;

    ASSERT_NE(nullptr, dh);

    uint32_t privKeyId = 0;
    uint32_t pubKeyId = 0;

    EXPECT_EQ(dh->Generate(5,
                  sizeof(TestData::dhModulus), TestData::dhModulus,
                  privKeyId, pubKeyId),
        Thunder::Core::ERROR_NONE);

    EXPECT_GT(privKeyId, 0x8000000);
    EXPECT_GT(pubKeyId, 0x8000000);

    if (dh != nullptr) {
        dh->Release();
        dh = nullptr;
    }
}

TEST_F(BasicTest, VaultDiffieHellmanGenerateDeactivatedPlugin)
{
    ASSERT_EQ(controller.ActivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);
    ASSERT_TRUE(controller.IsPluginActive(TestData::plugin));
    ASSERT_NE(nullptr, cryptography);

    Thunder::Cryptography::IVault* vault = cryptography->Vault(CRYPTOGRAPHY_VAULT_PLATFORM);

    ASSERT_NE(nullptr, vault);

    Thunder::Cryptography::IDiffieHellman* dh = vault->DiffieHellman();

    if (vault != nullptr) {
        vault->Release();
        vault = nullptr;
    }

    ASSERT_EQ(controller.DeactivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);

    ASSERT_NE(nullptr, dh);

    uint32_t privKeyId = 0;
    uint32_t pubKeyId = 0;

    dh->Generate(5,
        sizeof(TestData::dhModulus), TestData::dhModulus,
        privKeyId, pubKeyId);

    EXPECT_EQ(privKeyId, 0);
    EXPECT_EQ(pubKeyId, 0);

    if (dh != nullptr) {
        dh->Release();
        dh = nullptr;
    }

    ASSERT_EQ(controller.ActivatePlugin(TestData::plugin), Thunder::Core::ERROR_NONE);
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);

    int result = RUN_ALL_TESTS();

    Thunder::Core::Singleton::Dispose();

    return result;
}
