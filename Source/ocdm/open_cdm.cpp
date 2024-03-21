/*
 * Copyright 2016-2017 TATA ELXSI
 * Copyright 2016-2017 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Module.h"
#include "open_cdm.h"
#include <interfaces/IOCDM.h>
#include "open_cdm_impl.h"

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

using namespace WPEFramework;

Core::CriticalSection _systemLock;
const char EmptyString[] = { '\0' };

namespace 
{

template<typename SIZETYPE> 
OpenCDMError StringToAllocatedBuffer(const std::string& source, char* destinationBuffer, SIZETYPE& bufferSize)
{
    using sizetype = SIZETYPE;

    OpenCDMError result = ERROR_NONE;
    
    sizetype sizeneeded = ( (source.size()+1) > std::numeric_limits<sizetype>::max() ) ? std::numeric_limits<sizetype>::max() : static_cast<sizetype>(source.size()+1);

    if(destinationBuffer == nullptr) {
        bufferSize = sizeneeded;
        result = ERROR_MORE_DATA_AVAILABLE;
    } else if ( bufferSize < sizeneeded ) {
        strncpy(destinationBuffer, source.c_str(), bufferSize-1);
        destinationBuffer[bufferSize-1] = '\0';
        bufferSize = sizeneeded;
        result = ERROR_MORE_DATA_AVAILABLE;      
    } else { // buffersize >= sizeneeded
        strncpy(destinationBuffer, source.c_str(), sizeneeded-1);
        destinationBuffer[sizeneeded-1] = '\0';
        bufferSize = sizeneeded;
    }
    return result;
}

} // namespace

/* static */ OpenCDMAccessor* OpenCDMAccessor::Instance()
{
    static class TheOne {
    public:
        TheOne(const TheOne&) = delete;
        TheOne& operator= (const TheOne&) = delete;

        TheOne() {
            string connector;
            if ((Core::SystemInfo::GetEnvironment(_T("OPEN_CDM_SERVER"), connector) == false) || (connector.empty() == true)) {
                connector = _T("/tmp/ocdm");
            }
            Core::SingletonType<OpenCDMAccessor>::Create(connector.c_str());
        }
        ~TheOne() {

            if( Core::SingletonType<OpenCDMAccessor>::Dispose() == true ) {
                // if the accessor was disposed here because the destructor of the static instance was called there
                // was no proper dispose before (opencdm_dispose and/or Singleton::Dispose). 
                // The static dispose might be incomplete or have side effects (e.g. Threads could already be killed)
                TRACE_L1(_T("OpenCDM Accessor was not disposed properly"));
            }
        }

    public:
        OpenCDMAccessor& Instance() {
            return (Core::SingletonType<OpenCDMAccessor>::Instance());
        }

    } singleton;

    OpenCDMAccessor& result = singleton.Instance();
    return &result;
}


KeyStatus CDMState(const Exchange::ISession::KeyStatus state)
{

    switch (state) {
    case Exchange::ISession::StatusPending:
        return KeyStatus::StatusPending;
    case Exchange::ISession::Usable:
        return KeyStatus::Usable;
    case Exchange::ISession::InternalError:
        return KeyStatus::InternalError;
    case Exchange::ISession::Released:
        return KeyStatus::Released;
    case Exchange::ISession::Expired:
        return KeyStatus::Expired;
    case Exchange::ISession::OutputRestricted:
        return KeyStatus::OutputRestricted;
    case Exchange::ISession::OutputRestrictedHDCP22:
        return KeyStatus::OutputRestrictedHDCP22;
    case Exchange::ISession::OutputDownscaled:
        return KeyStatus::OutputDownscaled;
    case Exchange::ISession::HWError:
        return KeyStatus::HWError;
    default:
        assert(false);
    }

    return KeyStatus::InternalError;
}


/**
 * Destructs an \ref OpenCDMAccessor instance.
 * \param system \ref OpenCDMAccessor instance to desctruct.
 * \return Zero on success, non-zero on error.
 */
OpenCDMError opencdm_destruct_system(struct OpenCDMSystem* system)
{
    OpenCDMAccessor::Instance()->SystemBeingDestructed(system);
    assert(system != nullptr);
    if (system != nullptr) {
       delete system;
    }
    return (OpenCDMError::ERROR_NONE);
}

/**
 * \brief Checks if a DRM system is supported.
 *
 * \param keySystem Name of required key system (e.g.
 * "com.microsoft.playready").
 * \param mimeType MIME type.
 * \return Zero if supported, Non-zero otherwise.
 * \remark mimeType is currently ignored.
 */
OpenCDMError opencdm_is_type_supported(const char keySystem[],
    const char mimeType[])
{
    OpenCDMAccessor * accessor = OpenCDMAccessor::Instance();
    OpenCDMError result(OpenCDMError::ERROR_KEYSYSTEM_NOT_SUPPORTED);

    if ((accessor != nullptr) && (accessor->IsTypeSupported(std::string(keySystem), std::string(mimeType)) == true)) {
        result = OpenCDMError::ERROR_NONE;
    }
    return (result);
}

/**
 * \brief Retrieves DRM system specific metadata.
 *
 * \param system Instance of \ref OpenCDMAccessor.
 * \param metadata, buffer to write metadata into, always 0 terminated (also when not large enough to hold all data) except when metadata is
 *     Null of course. Null allowed to retrieve required size needed for this buffer in metadataSize to be able to allocate required buffer 
 *     for subsequent call to opencdm_is_type_supported
 * \param metadataSize, in: size of metadata buffer, out: required size to hold all data available when return value is ERROR_MORE_DATA_AVAILABLE,
 *     , number of characters written into metadata (incl 0 terminator) otherwise. Note in case metadata could not hold all data but was not of zero
 *     length it is filled up to the maximum size (still zero terminated) but also ERROR_MORE_DATA_AVAILABLE is returned with the required size needed
 *     to hold all data
 * \return Zero on success, non-zero on error. ERROR_MORE_DATA_AVAILABLE when the buffer was not large enough to hold all the data available. 
 */
OpenCDMError opencdm_system_get_metadata(struct OpenCDMSystem* system, 
    char metadata[], 
    uint16_t* metadataSize)
{
    OpenCDMError result(ERROR_INVALID_ACCESSOR);

    if(system != nullptr) {
        result = StringToAllocatedBuffer(system->Metadata(), metadata, *metadataSize);
    }
    return result;
}

/**
 * \brief Get metrics associated with a DRM system.
 *
 * Some DRMs (e.g. WideVine) offer metric data that can be used for any
 * analyses. This function retrieves the metric data of the passed in
 * system. It is up to the callee to interpret the baniary data correctly.
 * \param system Instance of \ref OpenCDMAccessor.
 * \param bufferLength Actual buffer length of the buffer parameter, on return
 *                     it holds the number of bytes actually written in it.
 * \param buffer Buffer length of buffer that can hold the metric data.
 * \return Zero on success, non-zero on error.
 */

EXTERNAL OpenCDMError opencdm_get_metric_system_data(struct OpenCDMSystem* system,
    uint32_t* bufferLength,
    uint8_t* buffer) {
    OpenCDMError result(ERROR_INVALID_ACCESSOR);
    OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();

    if (accessor != nullptr) {
	result = static_cast<OpenCDMError>(accessor->Metricdata(system->keySystem(), *bufferLength, buffer));
    }

    return (result);
}

/**
 * \brief Maps key ID to \ref OpenCDMSession instance.
 *
 * In some situations we only have the key ID, but need the specific \ref
 * OpenCDMSession instance that
 * belongs to this key ID. This method facilitates this requirement.
 * \param keyId Array containing key ID.
 * \param length Length of keyId array.
 * \param maxWaitTime Maximum allowed time to block (in miliseconds).
 * \return \ref OpenCDMSession belonging to key ID, or NULL when not found or
 * timed out. This instance
 *         also needs to be destructed using \ref opencdm_session_destruct.
 * REPLACING: void* acquire_session(const uint8_t* keyId, const uint8_t
 * keyLength, const uint32_t waitTime);
 */
struct OpenCDMSession* opencdm_get_session(const uint8_t keyId[],
    const uint8_t length,
    const uint32_t waitTime)
{
    return opencdm_get_system_session(nullptr, keyId, length, waitTime);
}

struct OpenCDMSession* opencdm_get_system_session(struct OpenCDMSystem* system, const uint8_t keyId[],
    const uint8_t length, const uint32_t waitTime)
{
    OpenCDMAccessor * accessor = OpenCDMAccessor::Instance();
    struct OpenCDMSession* result = nullptr;

    std::string sessionId;
    if ((accessor != nullptr) && (accessor->WaitForKey(length, keyId, waitTime, Exchange::ISession::Usable, sessionId, system) == true)) {
        result = accessor->Session(sessionId);
    }

    return (result);
}

/**
 * \brief Gets support server certificate.
 *
 * Some DRMs (e.g. WideVine) use a system-wide server certificate. This method
 * gets if system has support for that certificate.
 * \param system Instance of \ref OpenCDMAccessor.
 * \return Non-zero on success, zero on error.
 */
EXTERNAL OpenCDMBool opencdm_system_supports_server_certificate(
    struct OpenCDMSystem*)
{
  return OPENCDM_BOOL_FALSE;
}

/**
 * \brief Sets server certificate.
 *
 * Some DRMs (e.g. WideVine) use a system-wide server certificate. This method
 * will set that certificate. Other DRMs will ignore this call.
 * \param serverCertificate Buffer containing certificate data.
 * \param serverCertificateLength Buffer length of certificate data.
 * \return Zero on success, non-zero on error.
 */
OpenCDMError opencdm_system_set_server_certificate(struct OpenCDMSystem* system,
    const uint8_t serverCertificate[], const uint16_t serverCertificateLength)
{
    OpenCDMAccessor * accessor = OpenCDMAccessor::Instance();
    OpenCDMError result(ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        result = static_cast<OpenCDMError>(accessor->SetServerCertificate(
            system->keySystem(), serverCertificate, serverCertificateLength));
    }
    return (result);
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
    ASSERT(system != nullptr);
    OpenCDMError result(OpenCDMError::ERROR_INVALID_SESSION);

    TRACE_L1("Creating a Session for %s", system->keySystem().c_str());

    result = OpenCDMSession::CreateSession(system,
                                            licenseType,
                                            initDataType,
                                            initData, initDataLength,
                                            CDMData, CDMDataLength,
                                            callbacks, userData,
                                            session
    );

    TRACE_L1("Created a Session, result %p, %d", *session, result);
    return result;
}

/**
 * Destructs an \ref OpenCDMSession instance.
 * \param system \ref OpenCDMSession instance to desctruct.
 * \return Zero on success, non-zero on error.
 * REPLACING: void release_session(void* session);
 */
OpenCDMError opencdm_destruct_session(struct OpenCDMSession* session)
{
    OpenCDMError result(OpenCDMError::ERROR_INVALID_SESSION);

    if (session != nullptr) {
        result = OpenCDMError::ERROR_NONE;
        session->Release();
    }

    return (result);
}

/**
 * Loads the data stored for a specified OpenCDM session into the CDM context.
 * \param session \ref OpenCDMSession instance.
 * \return Zero on success, non-zero on error.
 */
OpenCDMError opencdm_session_load(struct OpenCDMSession* session)
{
    OpenCDMError result(ERROR_INVALID_SESSION);

    if (session != nullptr) {
        result = static_cast<OpenCDMError>(session->Load());
    }

    return (result);
}

/**
 * Retrieves DRM session specific metadata of a session.
 * \param session \ref OpenCDMSession instance.
* \param metadata, buffer to write metadata into, always 0 terminated (also when not large enough to hold all data) except when metadata is
 *     Null of course. Null allowed to retrieve required size needed for this buffer in metadataSize to be able to allocate required buffer 
 *     for subsequent call to opencdm_session_metadata
 * \param metadataSize, in: size of metadata buffer, out: required size to hold all data available when return value is ERROR_MORE_DATA_AVAILABLE,
 *     , number of characters written into metadata (incl 0 terminator) otherwise. Note in case metadata could not hold all data but was not of zero
 *     length it is filled up to the maximum size (still zero terminated) but also ERROR_MORE_DATA_AVAILABLE is returned with the required size needed
 *     to hold all data
 * \return Zero on success, non-zero on error. ERROR_MORE_DATA_AVAILABLE when the buffer was not large enough to hold all the data available. 

 */
OpenCDMError opencdm_session_metadata(const struct OpenCDMSession* session, 
    char metadata[], 
    uint16_t* metadataSize)
{
    OpenCDMError result(ERROR_INVALID_SESSION);

    if(session != nullptr) {
        result = StringToAllocatedBuffer(session->Metadata(), metadata, *metadataSize);
    }
    return result;
}

/**
 * Gets session ID for a session.
 * \param session \ref OpenCDMSession instance.
 * \return session ID, valid as long as \ref session is valid.
 */
const char* opencdm_session_id(const struct OpenCDMSession* session)
{
    const char* result = EmptyString;
    if (session != nullptr) {
        result = session->SessionId().c_str();
    }
    return (result);
}

/**
 * Gets buffer ID for a session.
 * \param session \ref OpenCDMSession instance.
 * \return Buffer ID, valid as long as \ref session is valid.
 */
const char* opencdm_session_buffer_id(const struct OpenCDMSession* session)
{
    const char* result = EmptyString;
    if (session != nullptr) {
        result = session->BufferId().c_str();
    }
    return (result);
}

/**
 * Checks if a session has a specific keyid. Will check both BE/LE
 * \param session \ref OpenCDMSession instance.
 * \param length Length of key ID buffer (in bytes).
 * \param keyId Key ID.
 * \return 1 if keyID found else 0.
 */
uint32_t opencdm_session_has_key_id(struct OpenCDMSession* session, 
    const uint8_t length, const uint8_t keyId[])
{
    bool result = false;
    if (session != nullptr) {
        result = session->HasKeyId(length, keyId);
    }
    
    return result ? 1 : 0;
}

/**
 * Returns status of a particular key assigned to a session.
 * \param session \ref OpenCDMSession instance.
 * \param keyId Key ID.
 * \param length Length of key ID buffer (in bytes).
 * \return key status.
 */
KeyStatus opencdm_session_status(const struct OpenCDMSession* session,
    const uint8_t keyId[], uint8_t length)
{
    KeyStatus result(KeyStatus::InternalError);

    if (session != nullptr) {
        result = CDMState(session->Status(length, keyId));
    }

    return (result);
}

/**
 * Returns error for key (if any).
 * \param session \ref OpenCDMSession instance.
 * \param keyId Key ID.
 * \param length Length of key ID buffer (in bytes).
 * \return Key error (zero if no error, non-zero if error).
 */
uint32_t opencdm_session_error(const struct OpenCDMSession* session,
    const uint8_t keyId[], uint8_t length)
{
    uint32_t result(~0);

    if (session != nullptr) {
        result = session->Error(keyId, length);
    }

    return (result);
}

/**
 * Returns system error. This reference general system, instead of specific key.
 * \param session \ref OpenCDMSession instance.
 * \return System error code, zero if no error.
 */
OpenCDMError
opencdm_session_system_error(const struct OpenCDMSession* session)
{
    OpenCDMError result(ERROR_INVALID_SESSION);

    if (session != nullptr) {
        result = static_cast<OpenCDMError>(session->Error());
    }

    return (result);
}

/**
 * Process a key message response.
 * \param session \ref OpenCDMSession instance.
 * \param keyMessage Key message to process.
 * \param keyLength Length of key message buffer (in bytes).
 * \return Zero on success, non-zero on error.
 */
OpenCDMError opencdm_session_update(struct OpenCDMSession* session,
    const uint8_t keyMessage[],
    uint16_t keyLength)
{
    OpenCDMError result(ERROR_INVALID_SESSION);

    if (session != nullptr) {
        session->Update(keyMessage, keyLength);
        result = OpenCDMError::ERROR_NONE;
    }

    return (result);
}

/**
 * Removes all keys/licenses related to a session.
 * \param session \ref OpenCDMSession instance.
 * \return Zero on success, non-zero on error.
 */
OpenCDMError opencdm_session_remove(struct OpenCDMSession* session)
{
    OpenCDMError result(ERROR_INVALID_SESSION);

    if (session != nullptr) {
        result = static_cast<OpenCDMError>(session->Remove());
    }

    return (result);
}

/**
 * Set a name/value pair into the CDM
 * \param session \ref OpenCDMSession instance.
 * \return Zero on success, non-zero on error.
 */
OpenCDMError opencdm_session_set_parameter(struct OpenCDMSession* session,
    const std::string& name,
    const std::string& value)
{
    OpenCDMError result(ERROR_INVALID_SESSION);

    if (session != nullptr) {
        session->SetParameter(name, value);
        result = OpenCDMError::ERROR_NONE;
    }

    return (result);
}

/**
 * Let CDM know playback stopped and reset output protection
 * \param session \ref OpenCDMSession instance.
 * \return Zero on success, non-zero on error.
 */
OpenCDMError opencdm_session_resetoutputprotection(struct OpenCDMSession* session)
{
    OpenCDMError result(ERROR_INVALID_SESSION);

    if (session != nullptr) {
        session->ResetOutputProtection();
        result = OpenCDMError::ERROR_NONE;
    }

    return (result);
}

/**
 * Closes a session.
 * \param session \ref OpenCDMSession instance.
 * \return zero on success, non-zero on error.
 */
OpenCDMError opencdm_session_close(struct OpenCDMSession* session)
{

    OpenCDMError result(ERROR_INVALID_SESSION);

    if (session != nullptr) {
        session->Close();
        result = OpenCDMError::ERROR_NONE;
    }

    return (result);
}

/**
 * \brief Performs decryption.
 *
 * This method accepts encrypted data and will typically decrypt it
 * out-of-process (for security reasons). The actual data copying is performed
 * using a memory-mapped file (for performance reasons). If the DRM system
 * allows access to decrypted data (i.e. decrypting is not
 * performed in a TEE), the decryption is performed in-place.
 * \param session \ref OpenCDMSession instance.
 * \param encrypted Buffer containing encrypted data. If applicable, decrypted
 * data will be stored here after this call returns.
 * \param encryptedLength Length of encrypted data buffer (in bytes).
 * \param IV Initial vector (IV) used during decryption.
 * \param IVLength Length of IV buffer (in bytes).
 * \return Zero on success, non-zero on error.
 * REPLACING: uint32_t decrypt(void* session, uint8_t*, const uint32_t, const
 * uint8_t*, const uint16_t);
 */
OpenCDMError opencdm_session_decrypt(struct OpenCDMSession* session,
    uint8_t encrypted[],
    const uint32_t encryptedLength,
    const EncryptionScheme encScheme,
    const EncryptionPattern pattern,
    const uint8_t* IV, const uint16_t IVLength,
    const uint8_t* keyId, const uint16_t keyIdLength,
    uint32_t initWithLast15 /* = 0 */)
{
    OpenCDMError result(ERROR_INVALID_SESSION);
    if (session != nullptr) {
        SampleInfo sampleInfo;
        sampleInfo.subSample = nullptr;
        sampleInfo.subSampleCount = 0;
        sampleInfo.scheme = encScheme;
        sampleInfo.pattern.clear_blocks = pattern.clear_blocks;
        sampleInfo.pattern.encrypted_blocks = pattern.encrypted_blocks;
        sampleInfo.iv = const_cast<uint8_t*>(IV);
        sampleInfo.ivLength = static_cast<uint8_t>(IVLength);
        sampleInfo.keyId = const_cast<uint8_t*>(keyId);
        sampleInfo.keyIdLength = static_cast<uint8_t>(keyIdLength);
        result = encryptedLength > 0 ? static_cast<OpenCDMError>(session->Decrypt(
            encrypted, encryptedLength, const_cast<const SampleInfo*>(&sampleInfo), initWithLast15, nullptr)) : ERROR_NONE;
    }

    return (result);
}


OpenCDMError opencdm_session_decrypt_v2(struct OpenCDMSession* session,
    uint8_t encrypted[],
    const uint32_t encryptedLength,
    const SampleInfo* sampleInfo,
    const MediaProperties* properties) {

    OpenCDMError result(ERROR_INVALID_SESSION);
    if (session != nullptr) {
        uint32_t initWithLast15 = 0;
        result = encryptedLength > 0 ? static_cast<OpenCDMError>(session->Decrypt(
            encrypted, encryptedLength, sampleInfo, initWithLast15, properties)) : ERROR_NONE;
    }

    return (result);
}

/**
 * \brief Get metrics associated with a DRM session.
 *
 * Some DRMs (e.g. WideVine) offer metric data that can be used for any
 * analyses. This function retrieves the metric data of the passed in
 * system. It is up to the callee to interpret the baniary data correctly.
 * \param session Instance of \ref OpenCDMSession.
 * \param bufferLength Actual buffer length of the buffer parameter, on return
 *                     it holds the number of bytes actually written in it.
 * \param buffer Buffer length of buffer that can hold the metric data.
 * \return Zero on success, non-zero on error.
 */

OpenCDMError opencdm_get_metric_session_data(struct OpenCDMSession* session,
    uint32_t* bufferLength,
    uint8_t* buffer) {
    OpenCDMError result(ERROR_INVALID_SESSION);
    if (session != nullptr) {
        result = static_cast<OpenCDMError>(session->Metricdata(
            *bufferLength, buffer));
    }

    return (result);
}

void opencdm_dispose() {
    Core::SingletonType<OpenCDMAccessor>::Dispose();
}

bool OpenCDMAccessor::WaitForKey(const uint8_t keyLength, const uint8_t keyId[],
        const uint32_t waitTime,
        const Exchange::ISession::KeyStatus status,
        std::string& sessionId, OpenCDMSystem* system) const
    {
        bool result = false;
        uint64_t timeOut(Core::Time::Now().Add(waitTime).Ticks());

        do {
            _adminLock.Lock();

            KeyMap::const_iterator session(_sessionKeys.begin());

            for  (; session != _sessionKeys.end(); ++session) {
                if (!system || session->second->BelongsTo(system) == true) {
                    if(session->second->Status(keyLength, keyId) == status) {
                        result = true;
                        break;
                    }
                }
            }

            if (result == false) {
                Exchange::KeyId paramKey(keyId, keyLength);
                _interested++;

                _adminLock.Unlock();

                TRACE_L1("Waiting for KeyId: %s", paramKey.ToString().c_str());

                uint64_t now(Core::Time::Now().Ticks());

                if (now < timeOut) {
                    _signal.Lock(static_cast<uint32_t>((timeOut - now) / Core::Time::TicksPerMillisecond));
                }

                Core::InterlockedDecrement(_interested);
            } else {
                sessionId = session->first;
                _adminLock.Unlock();
            }
        } while ((result == false) && (timeOut > Core::Time::Now().Ticks()));

        return (result);
    }
    OpenCDMSession* OpenCDMAccessor::Session(const std::string& sessionId)
    {
        OpenCDMSession* result = nullptr;
        KeyMap::iterator index = _sessionKeys.find(sessionId);

        if(index != _sessionKeys.end()){
            result = index->second;
            result->AddRef();
        }

        return (result);
    }

    void OpenCDMAccessor::AddSession(OpenCDMSession* session)
    {
        string sessionId = session->SessionId();

        _adminLock.Lock();

        KeyMap::iterator index(_sessionKeys.find(sessionId));

        if (index == _sessionKeys.end()) {
            _sessionKeys.insert(std::pair<string, OpenCDMSession*>(sessionId, session));
        } else {
            TRACE_L1("Same session created, again ???? Keep the old one than. [%s]",
                sessionId.c_str());
        }

        _adminLock.Unlock();
    }
    void OpenCDMAccessor::RemoveSession(const string& sessionId)
    {

        _adminLock.Lock();

        KeyMap::iterator index(_sessionKeys.find(sessionId));

        if (index != _sessionKeys.end()) {
            _sessionKeys.erase(index);
        } else {
            TRACE_L1("A session is destroyed of which we were not aware [%s]",
                sessionId.c_str());
        }

        _adminLock.Unlock();
    }

    void OpenCDMAccessor::SystemBeingDestructed(OpenCDMSystem* system)
    {
        _adminLock.Lock();
        for (auto& sessionKey : _sessionKeys) {
            if (sessionKey.second->BelongsTo(system) == true) {
                TRACE_L1("System the session %s belongs to is being destructed. Destruct the session before destructing the system!", sessionKey.second->SessionId().c_str());
            }
        }
        _adminLock.Unlock();
    }
