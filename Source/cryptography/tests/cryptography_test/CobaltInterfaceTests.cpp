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

#include <climits>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <cstring>

#include <nexus_config.h>
#include <bstd.h>
#include <berr.h>
#include <bkni.h>
#include <nxclient.h>
#include <BSEAV/lib/security/sage/utility/starboard_tl.h>

#include "Helpers.h"
#include "Test.h"

#include <cryptography.h>
#include <core/core.h>

#ifdef VALIDATE_VALUES
#ifdef TEST_PLATFORM_APOLLOV1PLUS
#include "CobaltVaultHardcodesApolloV1Plus.h"
#else
#error Define values for validation
#endif
#endif

#define HMAC_SHA256_SIG_SIZE        32
#define ENV_COBALT_CERT_KEY_NAME    "COBALT_CERT_KEY_NAME"
#define ENV_COBALT_CERT_KEY_INDEX   "COBALT_CERT_KEY_INDEX"
#define DEFAULT_INPUT_DATA_LEN      100

static WPEFramework::Cryptography::ICryptography* cg = nullptr;

std::string getKeyName()
{
    std::string key_name;
    const char *env = std::getenv(ENV_COBALT_CERT_KEY_NAME);
    if ( env != nullptr )\
    {
        key_name = env;
        printf("Use key name: %s\n", key_name.c_str());
    }
    else
    {
        exit(-1);
    }
    return key_name;
}

uint32_t getKeyIndex()
{
    uint32_t result = 0xFF;
    const char * internal_key_index = getenv(ENV_COBALT_CERT_KEY_INDEX);
    if (internal_key_index)
    {
        char *end;
        long index_value = strtol(internal_key_index, &end, 10);
        if (*end == '\0' && index_value >= 0 && index_value < 3)
        {
            result = (uint32_t)index_value;
        }
    }
    return result;
}

bool isAllSetZero(const uint32_t length, const uint8_t data[])
{
    bool result = (length > 0);
    for (uint32_t i = 0; i < length; i++)
    {
        if (data[i] != 0x00)
        {
            result = false;
            break;
        }
    }
    return result;
}

bool areEqual(const uint32_t length, const uint8_t data1[], const uint8_t data2[])
{
    return (memcmp(data1, data2, length) == 0);
}

static uint8_t pure_starboard_result[DEFAULT_INPUT_DATA_LEN];
static bool pure_starboard_result_calculated = false;

TEST(Cobalt, PureStarboardTL)
{
    NxClient_JoinSettings joinSettings;
    NxClient_GetDefaultJoinSettings(&joinSettings);
    snprintf(joinSettings.name, NXCLIENT_MAX_NAME, "%s", "starboard-test");
    NEXUS_Error err = NxClient_Join(&joinSettings);
    EXPECT_EQ(err, NEXUS_SUCCESS);
    if (err != NEXUS_SUCCESS) return;

    StarBoardTl_Handle handle = 0;
    StarBoardSettings settings;
    StarBoardTl_GetDefaultSettings(&settings);
    std::string key_name = getKeyName();
    uint32_t key_index = getKeyIndex();
    snprintf(settings.drm_binfile_path, 255, "%s", key_name.c_str());
    BERR_Code ret = StarBoardTl_Init(&handle, &settings);
    EXPECT_EQ(ret, BERR_SUCCESS);

    if (ret == BERR_SUCCESS)
    {
        for (uint32_t idx = 0; idx < 3; idx++)
        {
            uint8_t cert_data[STARBOARD_TL_MAX_CERTIFICATION_SCOPES_DATA_SIZE];
            uint32_t cert_size = STARBOARD_TL_MAX_CERTIFICATION_SCOPES_DATA_SIZE;
            memset(cert_data, 0, STARBOARD_TL_MAX_CERTIFICATION_SCOPES_DATA_SIZE);
            ret = StarBoardTl_GetPropertyCert(handle, idx, cert_data, &cert_size);
            EXPECT_EQ(ret, BERR_SUCCESS);
            EXPECT_GT(cert_size, 0);
            printf(_T("StarBoardTl_GetPropertyCert: %d\n"), ret);

            if (ret == BERR_SUCCESS)
            {
                printf(_T("StarBoardTl_GetPropertyCert: %u %u\n"), idx, cert_size);
                DumpBuffer(cert_data, cert_size);
#ifdef VALIDATE_VALUES
                EXPECT_EQ(cert_size, PROPERTY_CERTS[idx].first);
                EXPECT_EQ(areEqual(cert_size, cert_data, PROPERTY_CERTS[idx].second), true);
#endif
#if 0
                for (uint32_t i = 0; i < cert_size; i++)
                {
                    printf(_T("StarBoardTl_GetPropertyCert: %u %03u %02x %03d %c\n"), idx, i, cert_data[i], cert_data[i], isprint(cert_data[i]) ? cert_data[i] : '.');
                    // values are libertyglobal-2023-apollov1plus
                }
#endif
            }
        }
        for (uint32_t idx = 0; idx < 3; idx++) {

            uint8_t signature[STARBOARD_TL_HMAC_SHA256_SIG_SIZE];
            uint32_t signature_size = STARBOARD_TL_HMAC_SHA256_SIG_SIZE;
            memset(signature, 0, STARBOARD_TL_HMAC_SHA256_SIG_SIZE);

            uint8_t src_data[DEFAULT_INPUT_DATA_LEN];
            uint32_t src_data_size = DEFAULT_INPUT_DATA_LEN;
            memset(src_data, 0, src_data_size);
            ret = StarBoardTl_SignWithCertKey(handle, src_data, src_data_size, idx, signature, signature_size);
            EXPECT_EQ(ret, BERR_SUCCESS);
            printf(_T("StarBoardTl_SignWithCertKey: %d\n"), ret);
#ifdef VALIDATE_VALUES
            EXPECT_EQ(areEqual(STARBOARD_TL_HMAC_SHA256_SIG_SIZE, signature, SIGN_VALUES[idx]), true);
#endif
            if (ret == BERR_SUCCESS)
            {
                if (idx == key_index)
                {
                    // store pure starboard result to crosscheck with the one from API
                    pure_starboard_result_calculated = true;
                    memcpy(pure_starboard_result, signature, STARBOARD_TL_HMAC_SHA256_SIG_SIZE);
                }
                printf(_T("StarBoardTl_SignWithCertKey: %u\n"), idx);
                DumpBuffer(signature, STARBOARD_TL_HMAC_SHA256_SIG_SIZE);
#if 0
                for (int i = 0; i < STARBOARD_TL_HMAC_SHA256_SIG_SIZE; i++)
                {
                    printf(_T("StarBoardTl_SignWithCertKey: %u %03u %02x %03d\n"), idx, i, signature[i], signature[i]);
                }
#endif
            }
        }
        StarBoardTl_Uninit(handle);
    }
    NxClient_Uninit();
}

TEST(Cobalt, CobaltCertKeyNameExistAccessibleAndNotZeroSize)
{
    const char * cert_key_name = getenv(ENV_COBALT_CERT_KEY_NAME);
    EXPECT_NE(cert_key_name, nullptr);
    if (cert_key_name == nullptr)
    {
        printf("Please export valid key name: %s env variable - terminate tests\n", ENV_COBALT_CERT_KEY_NAME);
        exit(-1);
    }
    printf("Cobalt certificate key name: %s\n", cert_key_name);
    int r = access(cert_key_name, R_OK);
    EXPECT_EQ(r, 0);
    if (r != 0)
    {
        printf("Cobalt certificate file not accessible: %s - terminate tests\n", cert_key_name);
        exit(-1);
    }
    struct stat file_status;
    r = stat(cert_key_name, &file_status);
    EXPECT_EQ(r, 0);
    EXPECT_GT(file_status.st_size, 0);
    printf("Cobalt certificate key: %s, size: %ld\n", cert_key_name, file_status.st_size);
    if (file_status.st_size == 0)
    {
        printf("Cobalt certificate file zero size: %s - terminate tests\n", cert_key_name);
        exit(-1);
    }
    if (!S_ISREG(file_status.st_mode))
    {
        printf("Cobalt certificate is not a file: %s - terminate tests\n", cert_key_name);
        exit(-1);
    }
}

TEST(Cobalt, CobaltCertKeyIndexDefinedAndValid)
{
    const char * internal_key_index = getenv(ENV_COBALT_CERT_KEY_INDEX);
    EXPECT_NE(internal_key_index, nullptr);
    if (internal_key_index == nullptr)
    {
        printf("Please export valid key name: %s env variable - terminate tests\n", ENV_COBALT_CERT_KEY_INDEX);
        exit(-1);
    }
    else
    {
        printf("COBALT_CERT_KEY_INDEX=%s\n", internal_key_index);
    }
    char *end;
    long index_value = strtol(internal_key_index, &end, 10);
    EXPECT_EQ(*end, '\0');
    if (*end != '\0')
    {
        printf("Please export valid key name: %s env variable: %s - terminate tests\n", ENV_COBALT_CERT_KEY_INDEX, internal_key_index);
        exit(-1);
    }
    printf("Converted to int key index: %ld\n", index_value);
    EXPECT_EQ((index_value >= 0), true); //EXPECT_GE(index_value, 0);
    EXPECT_LE(index_value, 2);
    if (index_value < 0 || index_value > 2)
    {
        printf("Please export valid key name: %s env variable (shoud be between 0 and 2) - terminate tests\n", ENV_COBALT_CERT_KEY_INDEX);
        exit(-1);
    }
}

TEST(Cobalt, CobaltVaultLoadFlush)
{
    WPEFramework::Cryptography::IVault *vault = cg->Vault(cryptographyvault::CRYPTOGRAPHY_VAULT_COBALT);
    EXPECT_NE(vault, nullptr);
    WPEFramework::Cryptography::IPersistent *persistent = vault->QueryInterface<WPEFramework::Cryptography::IPersistent>();
    EXPECT_NE(persistent, nullptr);
    std::string key_name = getKeyName();
    uint32_t key_id = 0;
    uint32_t rc = persistent->Load(key_name, key_id);
    EXPECT_EQ(rc, WPEFramework::Core::ERROR_NONE);
    EXPECT_NE(key_id, 0);
    printf("key_id %x\n", key_id);
    rc = persistent->Flush();
    EXPECT_EQ(rc, WPEFramework::Core::ERROR_NONE);

    // second Flush should not fail
    rc = persistent->Flush();
    EXPECT_EQ(rc, WPEFramework::Core::ERROR_NONE);

    // second Load should not fail, return previous values
    rc = persistent->Load(key_name, key_id);
    EXPECT_EQ(rc, WPEFramework::Core::ERROR_NONE);
    EXPECT_NE(key_id, 0);
    printf("key_id %x\n", key_id);
    uint32_t second_key_id = 0;
    rc = persistent->Load(key_name, second_key_id);
    EXPECT_EQ(rc, WPEFramework::Core::ERROR_NONE);
    EXPECT_EQ(second_key_id, key_id);
    rc = persistent->Flush();
    EXPECT_EQ(rc, WPEFramework::Core::ERROR_NONE);
    vault->Release();
}

TEST(Cobalt, CobaltValultLoadFailed)
{
    WPEFramework::Cryptography::IVault *vault = cg->Vault(cryptographyvault::CRYPTOGRAPHY_VAULT_COBALT);
    EXPECT_NE(vault, nullptr);
    WPEFramework::Cryptography::IPersistent *persistent = vault->QueryInterface<WPEFramework::Cryptography::IPersistent>();
    EXPECT_NE(persistent, nullptr);

    // Load should fail with not existing file
    std::string key_name = "not_existing_file";
    uint32_t key_id = 0;
    uint32_t rc = persistent->Load(key_name, key_id);
    EXPECT_EQ(rc, WPEFramework::Core::ERROR_GENERAL);
    EXPECT_EQ(key_id, 0);

    // Load should fail with too long file name
    char test_path[257];
    memset(test_path, 'a', 256);
    key_name = test_path;
    key_id = 0;
    rc = persistent->Load(key_name, key_id);
    EXPECT_EQ(rc, WPEFramework::Core::ERROR_GENERAL);
    EXPECT_EQ(key_id, 0);
    vault->Release();
}

TEST(Cobalt, CheckVaultStubs)
{
    WPEFramework::Cryptography::IVault *vault = cg->Vault(cryptographyvault::CRYPTOGRAPHY_VAULT_COBALT);
    EXPECT_NE(vault, nullptr);

    uint8_t blob[10];
    EXPECT_EQ(vault->Size(0), 0);
    EXPECT_EQ(vault->Import(10, blob), 0);
    EXPECT_EQ(vault->Export(0, 10, blob), 0);
    EXPECT_EQ(vault->Set(10, blob), 0);
    EXPECT_EQ(vault->Get(0, 10, blob), 0);
    EXPECT_EQ(vault->Delete(0), false);
    EXPECT_EQ(vault->AES(WPEFramework::Cryptography::aesmode::CBC, 0), nullptr);
    //EXPECT_EQ(vault->DiffieHellman(), nullptr);
    vault->Release();
}

TEST(Cobalt, CheckPersistentStubs)
{
    WPEFramework::Cryptography::IVault *vault = cg->Vault(cryptographyvault::CRYPTOGRAPHY_VAULT_COBALT);
    EXPECT_NE(vault, nullptr);

    WPEFramework::Cryptography::IPersistent *persistent = vault->QueryInterface<WPEFramework::Cryptography::IPersistent>();
    EXPECT_NE(persistent, nullptr);
    bool result = false;
    EXPECT_EQ(persistent->Exists("example_locator", result), WPEFramework::Core::ERROR_UNAVAILABLE);
    EXPECT_EQ(result, false);
    uint32_t key_id = 0;
    EXPECT_EQ(persistent->Create("example_locator", WPEFramework::Cryptography::IPersistent::keytype::AES256, key_id), WPEFramework::Core::ERROR_UNAVAILABLE);
    EXPECT_EQ(key_id, 0);
    vault->Release();
}

TEST(Cobalt, HMACOnlySHA256Supported)
{
    WPEFramework::Cryptography::IVault *vault = cg->Vault(cryptographyvault::CRYPTOGRAPHY_VAULT_COBALT);
    EXPECT_NE(vault, nullptr);

    WPEFramework::Cryptography::IPersistent *persistent = vault->QueryInterface<WPEFramework::Cryptography::IPersistent>();
    EXPECT_NE(persistent, nullptr);

    std::string key_name = getKeyName();
    uint32_t key_id = 0;
    uint32_t rc = persistent->Load(key_name, key_id);
    EXPECT_EQ(rc, WPEFramework::Core::ERROR_NONE);
    EXPECT_NE(key_id, 0);

    WPEFramework::Cryptography::IHash* hash_impl = vault->HMAC(WPEFramework::Cryptography::hashtype::SHA256, key_id);
    EXPECT_NE(hash_impl, nullptr);

    EXPECT_EQ(vault->HMAC(WPEFramework::Cryptography::hashtype::SHA1, key_id), nullptr);
    EXPECT_EQ(vault->HMAC(WPEFramework::Cryptography::hashtype::SHA224, key_id), nullptr);
    EXPECT_EQ(vault->HMAC(WPEFramework::Cryptography::hashtype::SHA384, key_id), nullptr);
    EXPECT_EQ(vault->HMAC(WPEFramework::Cryptography::hashtype::SHA512, key_id), nullptr);

    rc = persistent->Flush();
    EXPECT_EQ(rc, WPEFramework::Core::ERROR_NONE);
    vault->Release();
}

TEST(Cobalt, SignatureBufferErrors)
{
    WPEFramework::Cryptography::IVault *vault = cg->Vault(cryptographyvault::CRYPTOGRAPHY_VAULT_COBALT);
    EXPECT_NE(vault, nullptr);

    WPEFramework::Cryptography::IPersistent *persistent = vault->QueryInterface<WPEFramework::Cryptography::IPersistent>();
    EXPECT_NE(persistent, nullptr);

    std::string key_name = getKeyName();
    uint32_t key_id = 0;
    uint32_t rc = persistent->Load(key_name, key_id);
    EXPECT_EQ(rc, WPEFramework::Core::ERROR_NONE);
    EXPECT_NE(key_id, 0);

    WPEFramework::Cryptography::IHash* hash_impl = vault->HMAC(WPEFramework::Cryptography::hashtype::SHA256, key_id);
    EXPECT_NE(hash_impl, nullptr);

    uint8_t signature[1];
    uint8_t src_data[DEFAULT_INPUT_DATA_LEN];
    uint32_t src_data_size = DEFAULT_INPUT_DATA_LEN;
    memset(src_data, 0, src_data_size);
    uint32_t ingest_res = hash_impl->Ingest(src_data_size, src_data);
    EXPECT_EQ(ingest_res, src_data_size);

    uint8_t result = hash_impl->Calculate(1, signature);
    EXPECT_EQ(result, 0);    

    result = hash_impl->Calculate(1, nullptr);
    EXPECT_EQ(result, 0);

    result = hash_impl->Calculate(0, signature);
    EXPECT_EQ(result, 0);

    result = hash_impl->Calculate(0, nullptr);
    EXPECT_EQ(result, 0);

    result = hash_impl->Calculate(HMAC_SHA256_SIG_SIZE, nullptr);
    EXPECT_EQ(result, 0);

    uint8_t signature_too_long[HMAC_SHA256_SIG_SIZE + 1];
    result = hash_impl->Calculate(HMAC_SHA256_SIG_SIZE + 1, signature_too_long);
    EXPECT_EQ(result, HMAC_SHA256_SIG_SIZE);

    rc = persistent->Flush();
    EXPECT_EQ(rc, WPEFramework::Core::ERROR_NONE);
    vault->Release();
}

TEST(Cobalt, CobaltApplicationFlow)
{
    WPEFramework::Cryptography::IVault *vault = cg->Vault(cryptographyvault::CRYPTOGRAPHY_VAULT_COBALT);
    EXPECT_NE(vault, nullptr);

    WPEFramework::Cryptography::IPersistent *persistent = vault->QueryInterface<WPEFramework::Cryptography::IPersistent>();
    EXPECT_NE(persistent, nullptr);

    std::string key_name = getKeyName();
    uint32_t key_id = 0;
    uint32_t rc = persistent->Load(key_name, key_id);
    EXPECT_EQ(rc, WPEFramework::Core::ERROR_NONE);
    EXPECT_NE(key_id, 0);

    WPEFramework::Cryptography::IHash* hash_impl = vault->HMAC(WPEFramework::Cryptography::hashtype::SHA256, key_id);
    EXPECT_NE(hash_impl, nullptr);
    WPEFramework::Cryptography::IHash* hash_impl_second = vault->HMAC(WPEFramework::Cryptography::hashtype::SHA256, key_id);
    EXPECT_NE(hash_impl_second, nullptr);

    uint8_t signature1[HMAC_SHA256_SIG_SIZE];
    uint8_t signature2[HMAC_SHA256_SIG_SIZE];
    uint8_t signature3[HMAC_SHA256_SIG_SIZE];
    uint8_t signature2_after_recalculate[HMAC_SHA256_SIG_SIZE];
    uint8_t signature2_after_ingest_0_bytes[HMAC_SHA256_SIG_SIZE];
    uint8_t signature2_on_second_instance[HMAC_SHA256_SIG_SIZE];

    // set all signatures to zero
    memset(signature1, 0, HMAC_SHA256_SIG_SIZE);
    memset(signature2, 0, HMAC_SHA256_SIG_SIZE);
    memset(signature3, 0, HMAC_SHA256_SIG_SIZE);
    memset(signature2_after_ingest_0_bytes, 0, HMAC_SHA256_SIG_SIZE);
    memset(signature2_after_recalculate, 0, HMAC_SHA256_SIG_SIZE);
    memset(signature2_on_second_instance, 0, HMAC_SHA256_SIG_SIZE);

    // signature1 - calculate without Ingest - on 0 bytes array input - should be legal
    // underlying implementation StarBoardTl_SignWithCertKey return 2: BERR_INVALID_PATAMETER: for 0 size inputs
    // in practice not used case, just documented here in testcases
    uint8_t result = hash_impl->Calculate(HMAC_SHA256_SIG_SIZE, signature1);
    EXPECT_EQ(result, 0); //EXPECT_EQ(result, HMAC_SHA256_SIG_SIZE);

    // source data - all set to zero
    uint8_t src_data[DEFAULT_INPUT_DATA_LEN];
    uint32_t src_data_size = DEFAULT_INPUT_DATA_LEN;
    memset(src_data, 0, src_data_size);

    // Ingest 100 bytes
    uint32_t ingest_res = hash_impl->Ingest(src_data_size, src_data);
    EXPECT_EQ(ingest_res, src_data_size);
    // ... also on second instance
    ingest_res = hash_impl_second->Ingest(src_data_size, src_data);
    EXPECT_EQ(ingest_res, src_data_size);

    // signature2 - calculate after Ingest 100 bytes
    result = hash_impl->Calculate(HMAC_SHA256_SIG_SIZE, signature2);
    EXPECT_EQ(result, HMAC_SHA256_SIG_SIZE);
    result = hash_impl_second->Calculate(HMAC_SHA256_SIG_SIZE, signature2_on_second_instance);
    EXPECT_EQ(result, HMAC_SHA256_SIG_SIZE);

    // signature2_after_recalculate - re-calculate with previously Ingest data (100 bytes), should be equal to signature2
    result = hash_impl->Calculate(HMAC_SHA256_SIG_SIZE, signature2_after_recalculate);
    EXPECT_EQ(result, HMAC_SHA256_SIG_SIZE);

    // Ingest 0 bytes
    ingest_res = hash_impl->Ingest(0, src_data);
    EXPECT_EQ(ingest_res, 0);

    // signature2_after_ingest_0_bytes should be equal to signature2
    result = hash_impl->Calculate(HMAC_SHA256_SIG_SIZE, signature2_after_ingest_0_bytes);
    EXPECT_EQ(result, HMAC_SHA256_SIG_SIZE);

    // Ingest second time - 100 bytes (we have 200 bytes)
    ingest_res = hash_impl->Ingest(src_data_size, src_data);
    EXPECT_EQ(ingest_res, src_data_size);

    // signature3 - calculated on 200 bytes
    result = hash_impl->Calculate(HMAC_SHA256_SIG_SIZE, signature3);
    EXPECT_EQ(result, HMAC_SHA256_SIG_SIZE);

    // signature2, signature3, signature2_after_ingest_0_bytes, signature2_on_second_instance, signature2_after_recalculate - all of them not set to zeros
    // exceptional signature1
    EXPECT_EQ(isAllSetZero(HMAC_SHA256_SIG_SIZE, signature1), true); //EXPECT_EQ(isAllSetZero(HMAC_SHA256_SIG_SIZE, signature1), false);
    EXPECT_EQ(isAllSetZero(HMAC_SHA256_SIG_SIZE, signature2), false);
    EXPECT_EQ(isAllSetZero(HMAC_SHA256_SIG_SIZE, signature3), false);
    EXPECT_EQ(isAllSetZero(HMAC_SHA256_SIG_SIZE, signature2_after_ingest_0_bytes), false);
    EXPECT_EQ(isAllSetZero(HMAC_SHA256_SIG_SIZE, signature2_after_recalculate), false);
    EXPECT_EQ(isAllSetZero(HMAC_SHA256_SIG_SIZE, signature2_on_second_instance), false);

    // signature2 == signature2_after_ingest_0_bytes == signature2_after_recalculate == signature2_on_second_instance
    EXPECT_EQ(areEqual(HMAC_SHA256_SIG_SIZE, signature2, signature2_after_ingest_0_bytes), true);
    EXPECT_EQ(areEqual(HMAC_SHA256_SIG_SIZE, signature2, signature2_after_recalculate), true);
    EXPECT_EQ(areEqual(HMAC_SHA256_SIG_SIZE, signature2, signature2_on_second_instance), true);

    // signature1 != signature2
    // signature1 != signature3
    // signature2 != signature3
    EXPECT_EQ(areEqual(HMAC_SHA256_SIG_SIZE, signature1, signature2), false);
    EXPECT_EQ(areEqual(HMAC_SHA256_SIG_SIZE, signature1, signature3), false);
    EXPECT_EQ(areEqual(HMAC_SHA256_SIG_SIZE, signature2, signature3), false);

    if (pure_starboard_result_calculated)
    {
        EXPECT_EQ(areEqual(HMAC_SHA256_SIG_SIZE, signature2, pure_starboard_result), true);
        DumpBuffer(signature2, HMAC_SHA256_SIG_SIZE);
        DumpBuffer(pure_starboard_result, HMAC_SHA256_SIG_SIZE);
    }
    // not sure if could assume anything more here

    rc = persistent->Flush();
    EXPECT_EQ(rc, WPEFramework::Core::ERROR_NONE);
    vault->Release();
}

int main()
{
    CALL(Cobalt, CobaltCertKeyNameExistAccessibleAndNotZeroSize); // validate environment COBALT_CERT_KEY_NAME
    CALL(Cobalt, CobaltCertKeyIndexDefinedAndValid); // validate environment COBALT_CERT_KEY_INDEX
    CALL(Cobalt, PureStarboardTL); // pure starboard_tl check

    cg = WPEFramework::Cryptography::ICryptography::Instance("");
    if (cg != nullptr)
    {
        CALL(Cobalt, CobaltVaultLoadFlush); // WPEFramework::Cryptography::IPersistent (Load/Flush) testcases
        CALL(Cobalt, CobaltValultLoadFailed); // WPEFramework::Cryptography::IPersistent (Load) testcases
        CALL(Cobalt, HMACOnlySHA256Supported); // HMAC supported only for WPEFramework::Cryptography::hashtype::SHA256
        CALL(Cobalt, CheckVaultStubs); // check stub implementation
        CALL(Cobalt, CheckPersistentStubs); // check WPEFramework::Cryptography::IPersistent stub implementation for Exists and Create

        CALL(Cobalt, SignatureBufferErrors);
        // ckeck complete Cobalt application flow, reference: https://code.rdkcentral.com/r/plugins/gitiles/rdk/components/generic/cobalt/+/refs/heads/master/src/third_party/starboard/rdk/shared/system/system_sign_with_certification_secret_key.cc
        CALL(Cobalt, CobaltApplicationFlow);
        cg->Release();
    }
    else
    {
        printf("FATAL: Failed to acquire ICryptographic, no tests can't be performed\n");
    }
    printf("TOTAL: %i tests; %i PASSED, %i FAILED\n", TotalTests, TotalTestsPassed, (TotalTests - TotalTestsPassed));

    return (TotalTests - TotalTestsPassed);
}
