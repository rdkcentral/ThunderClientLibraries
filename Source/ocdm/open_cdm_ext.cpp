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
#include "open_cdm_ext.h"

#include "open_cdm_impl.h"

#define ASSERT_NOT_EXECUTED()                                         \
    {                                                                 \
        fprintf(stderr, "Error: didn't expect to use %s (%s:%d)!!\n", \
            __PRETTY_FUNCTION__, __FILE__, __LINE__);                 \
        abort();                                                      \
    }

DEPRECATED struct OpenCDMSystem* opencdm_create_system(const char keySystem[])
{
    struct OpenCDMSystem* result = nullptr;
    opencdm_create_system_extended(keySystem, &result);
    return result;
}

OpenCDMError opencdm_create_system_extended(const char keySystem[], struct OpenCDMSystem** system)
{
    OpenCDMError result(OpenCDMError::ERROR_INVALID_ARG);

    ASSERT(system != nullptr);

    if (system != nullptr) {
        *system = nullptr;

        ASSERT(keySystem != nullptr);

        if (keySystem != nullptr) {
            std::string metadata;
            OpenCDMError result = static_cast<OpenCDMError>(OpenCDMAccessor::Instance()->Metadata(std::string(keySystem), metadata));

            if (result == OpenCDMError::ERROR_NONE) {
                *system = new OpenCDMSystem(keySystem, metadata);
            }
        }
    }

    return result;
}

OpenCDMError opencdm_system_get_version(struct OpenCDMSystem* system,
    char versionStr[])
{
    OpenCDMError result(OpenCDMError::ERROR_INVALID_ARG);

    ASSERT(system != nullptr);
    ASSERT(versionStr != nullptr);

    if ((system != nullptr) && (versionStr != nullptr)) {
        std::string versionStdStr = OpenCDMAccessor::Instance()->GetVersionExt(system->keySystem());

        assert(versionStdStr.length() < 64);

        versionStr[0] = '\0';
        strncat(versionStr, versionStdStr.c_str(), 63);

        result = OpenCDMError::ERROR_NONE;
    }

    return (result);
}

OpenCDMError opencdm_system_ext_get_ldl_session_limit(OpenCDMSystem* system,
    uint32_t* ldlLimit)
{
    OpenCDMError result(OpenCDMError::ERROR_INVALID_ARG);

    ASSERT(system != nullptr);
    ASSERT(ldlLimit != nullptr);

    if ((system != nullptr) && (ldlLimit != nullptr)) {
        std::string keySystem = system->keySystem();

        *ldlLimit = OpenCDMAccessor::Instance()->GetLdlSessionLimit(keySystem);

        result = OpenCDMError::ERROR_NONE;
    }

    return (result);
}

uint32_t opencdm_system_ext_is_secure_stop_enabled(
    struct OpenCDMSystem* system)
{
    OpenCDMError result(OpenCDMError::ERROR_INVALID_ARG);

    ASSERT(system != nullptr);

    if (system != nullptr) {
        result = static_cast<OpenCDMError>(OpenCDMAccessor::Instance()->IsSecureStopEnabled(system->keySystem()));
    }

    return (result);
}

OpenCDMError opencdm_system_ext_enable_secure_stop(struct OpenCDMSystem* system,
    uint32_t use)
{
    OpenCDMError result(OpenCDMError::ERROR_INVALID_ARG);

    ASSERT(system != nullptr);

    if (system != nullptr) {
        result = static_cast<OpenCDMError>(OpenCDMAccessor::Instance()->EnableSecureStop(system->keySystem(),
                    (use != 0)));
    }

    return (result);
}

uint32_t opencdm_system_ext_reset_secure_stop(struct OpenCDMSystem* system)
{
    OpenCDMError result(OpenCDMError::ERROR_INVALID_ARG);

    ASSERT(system != nullptr);

    if (system != nullptr) {
        result = static_cast<OpenCDMError>(OpenCDMAccessor::Instance()->ResetSecureStops(system->keySystem()));
    }

    return (result);
}

OpenCDMError opencdm_system_ext_get_secure_stop_ids(OpenCDMSystem* system,
    uint8_t ids[],
    uint16_t idsLength,
    uint32_t* count)
{
    OpenCDMError result(OpenCDMError::ERROR_INVALID_ARG);

    ASSERT(system != nullptr);

    if (system != nullptr) {
        result = static_cast<OpenCDMError>(OpenCDMAccessor::Instance()->GetSecureStopIds(system->keySystem(),
                    ids, idsLength, *count));
    }

    return (result);
}

OpenCDMError opencdm_system_ext_get_secure_stop(OpenCDMSystem* system,
    const uint8_t sessionID[],
    uint32_t sessionIDLength,
    uint8_t rawData[],
    uint16_t* rawSize)
{
    OpenCDMError result(OpenCDMError::ERROR_INVALID_ARG);

    ASSERT(system != nullptr);

    if (system != nullptr) {
        result = static_cast<OpenCDMError>(OpenCDMAccessor::Instance()->GetSecureStop(system->keySystem(),
                    sessionID, sessionIDLength, rawData, *rawSize));
    }

    return (result);
}

OpenCDMError opencdm_system_ext_commit_secure_stop(
    OpenCDMSystem* system, const uint8_t sessionID[],
    uint32_t sessionIDLength, const uint8_t serverResponse[],
    uint32_t serverResponseLength)
{
    OpenCDMError result(OpenCDMError::ERROR_INVALID_ARG);

    ASSERT(system != nullptr);

    if (system != nullptr) {
        result = static_cast<OpenCDMError>(OpenCDMAccessor::Instance()->CommitSecureStop(
                    system->keySystem(), sessionID, sessionIDLength, serverResponse,
                    serverResponseLength));
    }

    return (result);
}

OpenCDMError opencdm_system_get_drm_time(struct OpenCDMSystem* system,
    uint64_t* time)
{
    OpenCDMError result(OpenCDMError::ERROR_INVALID_ARG);

    ASSERT(system != nullptr);
    ASSERT(time != nullptr);

    if ((system != nullptr) && (time != nullptr)) {
        *time = OpenCDMAccessor::Instance()->GetDrmSystemTime(system->keySystem());
        result = OpenCDMError::ERROR_NONE;
    }

    return result;
}

uint32_t opencdm_session_get_session_id_ext(struct OpenCDMSession* opencdmSession)
{
    uint32_t result(OpenCDMError::ERROR_INVALID_SESSION);

    ASSERT(opencdmSession != nullptr);

    if (opencdmSession != nullptr) {
       result = opencdmSession->SessionIdExt();
    }

    return result;
}

OpenCDMError opencdm_destruct_session_ext(OpenCDMSession* opencdmSession)
{
    OpenCDMError result(OpenCDMError::ERROR_INVALID_SESSION);

    ASSERT(opencdmSession != nullptr);

    if (opencdmSession != nullptr) {
        result = OpenCDMError::ERROR_NONE;
        opencdmSession->Release();
    }

    return result;
}

OpenCDMError
opencdm_session_set_drm_header(struct OpenCDMSession* opencdmSession,
    const uint8_t drmHeader[],
    uint32_t drmHeaderSize)
{
    OpenCDMError result(OpenCDMError::ERROR_INVALID_SESSION);

    ASSERT(opencdmSession != nullptr);

    if (opencdmSession != nullptr) {
        result = static_cast<OpenCDMError>(opencdmSession->SetDrmHeader(drmHeader, drmHeaderSize));
    }

    return (result);
}

OpenCDMError
opencdm_session_get_challenge_data(struct OpenCDMSession* opencdmSession,
    uint8_t* challenge, uint32_t* challengeSize,
    uint32_t isLDL)
{
    OpenCDMError result(OpenCDMError::ERROR_INVALID_SESSION);

    ASSERT(opencdmSession != nullptr);
    ASSERT(challengeSize != nullptr);

    if ((opencdmSession != nullptr) && (challengeSize != nullptr)) {
        ASSERT((*challengeSize) < 0xFFFF);
        uint16_t realLength = static_cast<uint16_t>(*challengeSize);

        result = static_cast<OpenCDMError>(opencdmSession->GetChallengeDataExt(challenge, realLength, isLDL));
        *challengeSize = realLength;
    }

    return (result);
}

OpenCDMError
opencdm_session_cancel_challenge_data(struct OpenCDMSession* opencdmSession)
{
    OpenCDMError result(OpenCDMError::ERROR_INVALID_SESSION);

    ASSERT(opencdmSession != nullptr);

    if (opencdmSession != nullptr) {
        result = static_cast<OpenCDMError>(opencdmSession->CancelChallengeDataExt());
    }

    return (result);
}

OpenCDMError opencdm_session_store_license_data(
    struct OpenCDMSession* opencdmSession, const uint8_t licenseData[],
    uint32_t licenseDataSize, uint8_t* secureStopId)
{
    OpenCDMError result(OpenCDMError::ERROR_INVALID_SESSION);

    ASSERT(opencdmSession != nullptr);

    if (opencdmSession != nullptr) {
        result = static_cast<OpenCDMError>(opencdmSession->StoreLicenseData(licenseData, licenseDataSize, secureStopId));
    }

    return (result);
}

OpenCDMError opencdm_session_select_key_id(
    struct OpenCDMSession* opencdmSession, uint8_t keyLength, const uint8_t keyId[])
{
    OpenCDMError result(OpenCDMError::ERROR_INVALID_SESSION);

    ASSERT(opencdmSession != nullptr);

    if (opencdmSession != nullptr) {
        result = static_cast<OpenCDMError>(opencdmSession->SelectKeyId(keyLength, keyId));
    }

    return (result);
}

OpenCDMError opencdm_session_clean_decrypt_context(struct OpenCDMSession* opencdmSession)
{
    OpenCDMError result(OpenCDMError::ERROR_INVALID_SESSION);

    ASSERT(opencdmSession != nullptr);

    if (opencdmSession != nullptr) {
        result = static_cast<OpenCDMError>(opencdmSession->CleanDecryptContext());
    }

    return (result);
}

OpenCDMError opencdm_delete_key_store(struct OpenCDMSystem* system)
{
    OpenCDMError result(OpenCDMError::ERROR_INVALID_ARG);

    ASSERT(system != nullptr);

    if (system != nullptr) {
        std::string keySystem = system->keySystem();
        result = static_cast<OpenCDMError>(OpenCDMAccessor::Instance()->DeleteKeyStore(keySystem));
    }

    return result;
}

OpenCDMError opencdm_delete_secure_store(struct OpenCDMSystem* system)
{
    OpenCDMError result(OpenCDMError::ERROR_INVALID_ARG);

    ASSERT(system != nullptr);

    if (system != nullptr) {
        std::string keySystem = system->keySystem();
        result = static_cast<OpenCDMError>(OpenCDMAccessor::Instance()->DeleteSecureStore(keySystem));
    }

    return result;
}

OpenCDMError opencdm_get_key_store_hash_ext(struct OpenCDMSystem* system,
    uint8_t keyStoreHash[],
    uint32_t keyStoreHashLength)
{
    OpenCDMError result(OpenCDMError::ERROR_INVALID_ARG);

    ASSERT(system != nullptr);

    if (system != nullptr) {
        std::string keySystem = system->keySystem();
        result = static_cast<OpenCDMError>(OpenCDMAccessor::Instance()->GetKeyStoreHash(keySystem,
                    keyStoreHash, keyStoreHashLength));
    }

    return result;
}

OpenCDMError opencdm_get_secure_store_hash_ext(struct OpenCDMSystem* system,
    uint8_t secureStoreHash[],
    uint32_t secureStoreHashLength)
{
    OpenCDMError result(OpenCDMError::ERROR_INVALID_ARG);

    ASSERT(system != nullptr);

    if (system != nullptr) {
        std::string keySystem = system->keySystem();
        result = static_cast<OpenCDMError>(OpenCDMAccessor::Instance()->GetSecureStoreHash(keySystem,
                    secureStoreHash, secureStoreHashLength));
    }

    return result;
}

/**
 * \brief Create DRM session (for actual decrypting of data).
 *
 * Creates an instance of \ref OpenCDMSession using initialization data.
 * \param keySystem DRM system to create the session for.
 * \param licenseType DRM specifc signed integer selecting License Type (e.g.
 * "Limited Duration" for PlayReady).
 * \param initDataType Type of data passed in \ref initData.
 * \param initData Initialization data.
 * \param initDataLength Length (in bytes) of initialization data.
 * \param CDMData CDM data.
 * \param CDMDataLength Length (in bytes) of \ref CDMData.
 * \param session Output parameter that will contain pointer to instance of \ref
 * OpenCDMSession.
 * \return Zero on success, non-zero on error.
 */
OpenCDMError
opencdm_construct_session(struct OpenCDMSystem* system,
    const LicenseType licenseType, const char initDataType[],
    const uint8_t initData[], const uint16_t initDataLength,
    const uint8_t CDMData[], const uint16_t CDMDataLength,
    OpenCDMSessionCallbacks* callbacks, void* userData,
    struct OpenCDMSession** session)
{
    OpenCDMError result(OpenCDMError::ERROR_INVALID_ARG);

    ASSERT(system != nullptr);
    ASSERT(session != nullptr);

    if ((system != nullptr) && (session != nullptr)) {
        TRACE_L1("Creating a Session for %s", system->keySystem().c_str());

        *session = new OpenCDMSession(system, std::string(initDataType),
                            initData, initDataLength, CDMData,
                            CDMDataLength, licenseType, callbacks, userData);

        result = (*session != nullptr ? OpenCDMError::ERROR_NONE
                                      : OpenCDMError::ERROR_INVALID_SESSION);

        TRACE_L1("Created a Session, result %p, %d", *session, result);
    }

    return result;
}

OpenCDMError opencdm_system_ext_get_properties(struct PlayLevels* system, const char* propertiesJSONText) 
{
    using namespace Core ;
    class PlayLevelsJSON : public JSON::Container {
    private:
        PlayLevelsJSON(const PlayLevelsJSON&) = delete;
        PlayLevelsJSON& operator=(const PlayLevelsJSON&) = delete;

    public:
        PlayLevelsJSON()
            : WPEFramework::Core::JSON::Container()
            , _compressedVideo()
            , _uncompressedVideo()
            , _analogVideo()
            , _compressedAudio()
            , _uncompressedAudio()
            , _maxDecodeWidth()
            , _maxDecodeHeight()
        {
            Add(_T("compressed-video"), &_compressedVideo);
            Add(_T("uncompressed-video"), &_uncompressedVideo);
            Add(_T("analog-video"), &_analogVideo);
            Add(_T("compressed-audio"), &_compressedAudio);
            Add(_T("uncompressed-audio"), &_uncompressedAudio);
            Add(_T("max-decode-width"), &_maxDecodeWidth);
            Add(_T("max-decode-height"), &_maxDecodeHeight);
        }

    public:
        JSON::DecUInt16 _compressedVideo;
        JSON::DecUInt16 _uncompressedVideo;
        JSON::DecUInt16 _analogVideo;
        JSON::DecUInt16 _compressedAudio;
        JSON::DecUInt16 _uncompressedAudio;
        JSON::DecUInt32 _maxDecodeWidth;
        JSON::DecUInt32 _maxDecodeHeight;
    };

    OpenCDMError result(OpenCDMError::ERROR_INVALID_ARG);

    ASSERT(system != nullptr);
    ASSERT(propertiesJSONText != nullptr);

    if ((system != nullptr) && (propertiesJSONText!= nullptr)) {
        string properties= std::string(propertiesJSONText);
        PlayLevelsJSON playlevelJson;
        playlevelJson.FromString(properties);

        system->_compressedDigitalVideoLevel = playlevelJson._compressedVideo.Value();
        system->_uncompressedDigitalVideoLevel = playlevelJson._uncompressedVideo.Value();
        system->_analogVideoLevel = playlevelJson._analogVideo.Value();
        system->_compressedDigitalAudioLevel = playlevelJson._compressedAudio.Value();
        system->_uncompressedDigitalAudioLevel = playlevelJson._uncompressedAudio.Value();
        system->_maxResDecodeWidth = playlevelJson._maxDecodeWidth.Value();
        system->_maxResDecodeHeight = playlevelJson._maxDecodeHeight.Value();

        result = OpenCDMError::ERROR_NONE;
    }

    return result;
}
