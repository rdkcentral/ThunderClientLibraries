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


#include <interfaces/IContentDecryption.h>
#include <interfaces/IOCDM.h>
#include "Module.h"
#include "open_cdm.h"

#include <atomic>

using namespace WPEFramework;

extern Core::CriticalSection _systemLock;

struct OpenCDMSystem {
    OpenCDMSystem(const char system[], const std::string& metadata) : _keySystem(system), _metadata(metadata) {}
    ~OpenCDMSystem() = default;
    OpenCDMSystem(const OpenCDMSystem&) = default;
    OpenCDMSystem(OpenCDMSystem&&) = default;
    OpenCDMSystem& operator=(OpenCDMSystem&&) = default;
    OpenCDMSystem& operator=(const OpenCDMSystem&) = default;
    const std::string& keySystem() const { return _keySystem; }
    const std::string& Metadata() const { return _metadata; }

 private:
    std::string _keySystem;
    std::string _metadata;
};

struct OpenCDMAccessor : public Exchange::IAccessorOCDM {
private:
    typedef std::map<string, OpenCDMSession*> KeyMap;

protected:
    OpenCDMAccessor(const TCHAR domainName[])
        : _refCount(1)
        , _domain(domainName)
        , _engine(Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create())
        , _client()
        , _remote(nullptr)
        , _adminLock()
        , _signal(false, true)
        , _interested(0)
        , _sessionKeys()
    {
        TRACE_L1("Trying to open an OCDM connection @ %s\n", domainName);
        Reconnect(); // make sure ResourceMonitor singleton is created before OpenCDMAccessor so the destruction order is correct
    }

    void Reconnect() const
    {
        if (_client.IsValid() == false) {
            _client = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId(_domain.c_str()), Core::ProxyType<Core::IIPCServer>(_engine));
        }

        if ((_client.IsValid() == true) && (_client->IsOpen() == false)) {
            if (_remote != nullptr) {
                _remote->Release();
            }
            _remote = _client->Open<Exchange::IAccessorOCDM>(_T("OpenCDMImplementation"));

            ASSERT(_remote != nullptr);

            if (_remote == nullptr) {
                if (_client.IsValid()) {
                  _client.Release();
                }
            }
        }
    }

public:
    OpenCDMAccessor() : _signal(false, true) { ASSERT(false); }
    OpenCDMAccessor(const OpenCDMAccessor&) = delete;
    OpenCDMAccessor& operator=(const OpenCDMAccessor&) = delete;

    static OpenCDMAccessor* Instance();

    ~OpenCDMAccessor()
    {
        if (_remote != nullptr) {
            _remote->Release();
        }

        if (_client.IsValid()) {
            _client.Release();
        }

        TRACE_L1("Destructed the OpenCDMAccessor %p", this);
    }
    bool WaitForKey(const uint8_t keyLength, const uint8_t keyId[],
        const uint32_t waitTime,
        const Exchange::ISession::KeyStatus status,
        std::string& sessionId, OpenCDMSystem* system = nullptr) const;

public:
    uint32_t AddRef() const override
    {
        Core::InterlockedIncrement(_refCount);
        return (Core::ERROR_NONE);
    }
    uint32_t Release() const override
    {
        uint32_t result = Core::ERROR_NONE;

        _systemLock.Lock();

        if (Core::InterlockedDecrement(_refCount) == 0) {
            delete this;
            result = Core::ERROR_DESTRUCTION_SUCCEEDED;
        }

        _systemLock.Unlock();

        return (result);
    }
    BEGIN_INTERFACE_MAP(OpenCDMAccessor)
    INTERFACE_ENTRY(Exchange::IAccessorOCDM)
    END_INTERFACE_MAP

    virtual bool IsTypeSupported(const std::string& keySystem,
        const std::string& mimeType) const override
    {
        // Do reconnection here again if server is down.
        // This is first call from WebKit when new session is started
        // If ProxyStub return error for this call, there will be not next call from WebKit
        Reconnect();
        bool result = false;
        if (_remote != nullptr) {
            result = _remote->IsTypeSupported(keySystem, mimeType);
        }
        return result;
    }

    virtual Exchange::OCDM_RESULT Metadata(const string& keySystem, string& metadata) const override
    {
        return(_remote->Metadata(keySystem, metadata));
    }

    virtual Exchange::OCDM_RESULT Metricdata(const string& keySystem, uint32_t& length, uint8_t buffer[]) const override {
        return(_remote->Metricdata(keySystem, length, buffer));
    }

    // Create a MediaKeySession using the supplied init data and CDM data.
    virtual Exchange::OCDM_RESULT
    CreateSession(const string& keySystem, const int32_t licenseType,
        const std::string& initDataType, const uint8_t* initData,
        const uint16_t initDataLength, const uint8_t* CDMData,
        const uint16_t CDMDataLength,
        Exchange::ISession::ICallback* callback, std::string& sessionId, 
        Exchange::ISession*& session) override
    {
        return (_remote->CreateSession(
            keySystem, licenseType, initDataType, initData, initDataLength, CDMData,
            CDMDataLength, callback, sessionId, session));
    }

    // Set Server Certificate
    virtual Exchange::OCDM_RESULT
    SetServerCertificate(const string& keySystem, const uint8_t* serverCertificate,
        const uint16_t serverCertificateLength) override
    {
        return (_remote->SetServerCertificate(keySystem, serverCertificate,
            serverCertificateLength));
    }

    OpenCDMSession* Session(const std::string& sessionId);

    void AddSession(OpenCDMSession* sessionId);
    void RemoveSession(const string& sessionId);
    void KeyUpdate()
    {
        _adminLock.Lock();

        if (_interested != 0) {
            // We need to notify the "other side", they are expecting an update
            _signal.SetEvent();

            while (_interested != 0) {
                ::SleepMs(0);
            }

            _signal.ResetEvent();
        }
        _adminLock.Unlock();
    }

    uint64_t GetDrmSystemTime(const std::string& keySystem) const override
    {
        ASSERT(_remote && "This method only works on IAccessorOCDM implementations.");
        return _remote->GetDrmSystemTime(keySystem);
    }

    std::string GetVersionExt(const std::string& keySystem) const override
    {
        ASSERT(_remote && "This method only works on IAccessorOCDM implementations.");
        return _remote->GetVersionExt(keySystem);
    }

    uint32_t GetLdlSessionLimit(const std::string& keySystem) const override
    {
        ASSERT(_remote && "This method only works on IAccessorOCDM implementations.");
        return _remote->GetLdlSessionLimit(keySystem);
    }

    bool IsSecureStopEnabled(const std::string& keySystem) override
    {
        ASSERT(_remote && "This method only works on IAccessorOCDM implementations.");
        return _remote->IsSecureStopEnabled(keySystem);
    }

    Exchange::OCDM_RESULT EnableSecureStop(const std::string& keySystem, bool enable) override
    {
        ASSERT(_remote && "This method only works on IAccessorOCDM implementations.");
        return _remote->EnableSecureStop(keySystem, enable);
    }

    uint32_t ResetSecureStops(const std::string& keySystem) override
    {
        ASSERT(_remote && "This method only works on IAccessorOCDM implementations.");
        return _remote->ResetSecureStops(keySystem);
    }

    Exchange::OCDM_RESULT GetSecureStopIds(const std::string& keySystem,
        uint8_t ids[], uint16_t idsLength,
        uint32_t& count) override
    {
        ASSERT(_remote && "This method only works on IAccessorOCDM implementations.");
        return _remote->GetSecureStopIds(keySystem, ids, idsLength, count);
    }

    Exchange::OCDM_RESULT GetSecureStop(const std::string& keySystem,
        const uint8_t sessionID[],
        uint16_t sessionIDLength,
        uint8_t rawData[],
        uint16_t& rawSize) override
    {
        ASSERT(_remote && "This method only works on IAccessorOCDM implementations.");
        return _remote->GetSecureStop(keySystem, sessionID, sessionIDLength,
            rawData, rawSize);
    }

    Exchange::OCDM_RESULT
    CommitSecureStop(const std::string& keySystem, const uint8_t sessionID[],
        uint16_t sessionIDLength, const uint8_t serverResponse[],
        uint16_t serverResponseLength) override
    {
        ASSERT(_remote && "This method only works on IAccessorOCDM implementations.");
        return _remote->CommitSecureStop(keySystem, sessionID, sessionIDLength,
            serverResponse, serverResponseLength);
    }

    Exchange::OCDM_RESULT
    DeleteKeyStore(const std::string& keySystem) override
    {
        ASSERT(_remote && "This method only works on IAccessorOCDM implementations.");
        return _remote->DeleteKeyStore(keySystem);
    }

    Exchange::OCDM_RESULT
    DeleteSecureStore(const std::string& keySystem) override
    {
        ASSERT(_remote && "This method only works on IAccessorOCDM implementations.");
        return _remote->DeleteSecureStore(keySystem);
    }

    Exchange::OCDM_RESULT
    GetKeyStoreHash(const std::string& keySystem, uint8_t keyStoreHash[],
        uint16_t keyStoreHashLength) override
    {
        ASSERT(_remote && "This method only works on IAccessorOCDM implementations.");
        return _remote->GetKeyStoreHash(keySystem, keyStoreHash,
            keyStoreHashLength);
    }

    Exchange::OCDM_RESULT
    GetSecureStoreHash(const std::string& keySystem, uint8_t secureStoreHash[],
        uint16_t secureStoreHashLength) override
    {
        ASSERT(_remote && "This method only works on IAccessorOCDM implementations.");
        return _remote->GetSecureStoreHash(keySystem, secureStoreHash,
            secureStoreHashLength);
    }

    void SystemBeingDestructed(OpenCDMSystem* system);

private:
    mutable uint32_t _refCount;
    string _domain;
    Core::ProxyType<RPC::InvokeServerType<1, 0, 4> > _engine;
    mutable Core::ProxyType<RPC::CommunicatorClient> _client;
    mutable Exchange::IAccessorOCDM* _remote;
    mutable Core::CriticalSection _adminLock;
    mutable Core::Event _signal;
    mutable volatile uint32_t _interested;
    KeyMap _sessionKeys;
};

struct OpenCDMSession {
private:
    using KeyStatusesMap = std::list<Exchange::KeyId>;

    class Sink : public Exchange::ISession::ICallback {
    //private:
    public:
        Sink() = delete;
        Sink(const Sink&) = delete;
        Sink& operator=(const Sink&) = delete;

    public:
        Sink(OpenCDMSession* parent)
            : _parent(*parent)
        {
            ASSERT(parent != nullptr);
        }
        virtual ~Sink() {}

    public:
        // Event fired when a key message is successfully created.
        void OnKeyMessage(const uint8_t* keyMessage, //__in_bcount(f_cbKeyMessage)
            const uint16_t keyLength, //__in
            const std::string& URL) override
        {
            _parent.OnKeyMessage(keyMessage, keyLength, URL);
        }

        void OnError(const int16_t error, const Exchange::OCDM_RESULT sysError, const std::string& errorMessage) override
        {
            _parent.OnError(error, sysError, errorMessage);
        }

        // Event fired on key status update
        void OnKeyStatusUpdate(const uint8_t keyID[], const uint8_t keyIDLength,const Exchange::ISession::KeyStatus keyMessage) override
        {
            _parent.OnKeyStatusUpdate(keyID, keyIDLength, keyMessage);
        }

        void OnKeyStatusesUpdated() const override
        {
            _parent.OnKeyStatusesUpdated();
        }

        BEGIN_INTERFACE_MAP(Sink)
        INTERFACE_ENTRY(Exchange::ISession::ICallback)
        END_INTERFACE_MAP

    private:
        OpenCDMSession& _parent;
    };

    class DataExchange : public Exchange::DataExchange {
    private:
        DataExchange() = delete;
        DataExchange(const DataExchange&) = delete;
        DataExchange& operator=(DataExchange&) = delete;

    public:
        DataExchange(const string& bufferName)
            : Exchange::DataExchange(bufferName)
            , _busy(false)
        {

            TRACE_L1("Constructing buffer client side: %p - %s", this,
                bufferName.c_str());
        }
        virtual ~DataExchange()
        {
            if (_busy == true) {
                TRACE_L1("Destructed a DataExchange while still in progress. %p", this);
            }
            TRACE_L1("Destructing buffer client side: %p - %s", this,
                 Exchange::DataExchange::Name().c_str());
        }

    public:
        uint32_t Decrypt(uint8_t* encryptedData, uint32_t encryptedDataLength,
            const ::SampleInfo* sampleInfo,
            uint32_t initWithLast15,
            const ::MediaProperties* properties)
        {
            int ret = 0;

            // This works, because we know that the Audio and the Video streams are
            // fed from
            // the same process, so they will use the same critial section and thus
            // will
            // not interfere with each-other. If Audio and video will be located into
            // two
            // different processes, start using the administartion space to share a
            // lock.
            _systemLock.Lock();

            _busy = true;

            if (RequestProduce(Core::infinite) == Core::ERROR_NONE) {

                CDMi::SubSampleInfo* subSample = nullptr;
                uint8_t subSampleCount = 0;
                CDMi::EncryptionScheme encScheme = CDMi::EncryptionScheme::AesCtr_Cenc;
                CDMi::EncryptionPattern pattern = {0 , 0};
                uint8_t* ivData = nullptr;
                uint8_t ivDataLength = 0;
                uint8_t* keyId = nullptr;
                uint8_t keyIdLength = 0;

                if(sampleInfo != nullptr) {
                    subSample = reinterpret_cast<CDMi::SubSampleInfo*>(sampleInfo->subSample);
                    subSampleCount = sampleInfo->subSampleCount;
                    ivData = sampleInfo->iv;
                    ivDataLength = sampleInfo->ivLength;
                    keyId = sampleInfo->keyId;
                    keyIdLength = sampleInfo->keyIdLength;
                    encScheme = static_cast<CDMi::EncryptionScheme>(sampleInfo->scheme);
                    pattern.clear_blocks = sampleInfo->pattern.clear_blocks;
                    pattern.encrypted_blocks = sampleInfo->pattern.encrypted_blocks;
                }

                SetIV(static_cast<uint8_t>(ivDataLength), ivData);
                KeyId(static_cast<uint8_t>(keyIdLength), keyId);
                SubSample(subSampleCount, subSample);
                SetEncScheme(static_cast<uint8_t>(encScheme));
                SetEncPattern(pattern.encrypted_blocks,pattern.clear_blocks);
                InitWithLast15(initWithLast15);
                if(properties != nullptr) {
                    SetMediaProperties(properties->height, properties->width, properties->media_type);
                }

                Write(encryptedDataLength, encryptedData);

                // This will trigger the OpenCDMIServer to decrypt this memory...
                Produced();

                // Now we should wait till it is decrypted, that happens if the
                // Producer, can run again.
                if (RequestProduce(Core::infinite) == Core::ERROR_NONE) {

                    // For nowe we just copy the clear data..
                    Read(encryptedDataLength, encryptedData);

                    // Get the status of the last decrypt.
                    ret = Status();

                    // And free the lock, for the next production Scenario..
                    Consumed();
                }
            }

            _busy = false;

            _systemLock.Unlock();

            return (ret);
        }

    private:
        bool _busy;
    };

public:
    OpenCDMSession(const OpenCDMSession&) = delete;
    OpenCDMSession& operator= (const OpenCDMSession&) = delete;
    OpenCDMSession() = delete;

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)

    OpenCDMSession(OpenCDMSystem* system,
        const string& initDataType,
        const uint8_t* pbInitData, const uint16_t cbInitData,
        const uint8_t* pbCustomData,
        const uint16_t cbCustomData,
        const LicenseType licenseType,
        OpenCDMSessionCallbacks* callbacks,
        void* userData)
        : _sessionId()
        , _decryptSession(nullptr)
        , _session(nullptr)
        , _sessionExt(nullptr)
        , _refCount(1)
        , _sink(this)
        , _URL()
        , _callback(callbacks)
        , _userData(userData)
        , _keyStatuses()
        , _error()
        , _errorCode(~0)
        , _sysError(Exchange::OCDM_RESULT::OCDM_SUCCESS)
        , _system(system)
        , _pvtData(nullptr)
    {
        OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
        std::string bufferId;
        Exchange::ISession* realSession = nullptr;

        accessor->CreateSession(system->keySystem(), licenseType, initDataType, pbInitData,
            cbInitData, pbCustomData, cbCustomData, &_sink,
            _sessionId, realSession);

        if (realSession == nullptr) {
            TRACE_L1("Creating a Session failed. %d", __LINE__);
        } else {
            Session(realSession);
            realSession->Release();
            accessor->AddSession(this);
        }
    }

    virtual ~OpenCDMSession();

POP_WARNING()

public:
    static OpenCDMError CreateSession(struct OpenCDMSystem* system,
                            const LicenseType licenseType, const char initDataType[],
                            const uint8_t initData[], const uint16_t initDataLength,
                            const uint8_t CDMData[], const uint16_t CDMDataLength,
                            OpenCDMSessionCallbacks* callbacks, void* userData,
                            struct OpenCDMSession** session);

    void AddRef() { Core::InterlockedIncrement(_refCount); }
    bool Release()
    {
        if (Core::InterlockedDecrement(_refCount) == 0) {

            delete this;

            return (true);
        }
        return (false);
    }
    inline const string& SessionId() const { return (_sessionId); }
    inline string Metadata() const 
    { 
        ASSERT(_session != nullptr);

        return _session->Metadata();
    }
    inline uint32_t Metricdata(uint32_t& bufferSize, uint8_t buffer[])
    {
        ASSERT(_session != nullptr);

        return _session->Metricdata(bufferSize, buffer);
    } 
    inline const string& BufferId() const
    {
        static string EmptyString;

        return (_decryptSession != nullptr ? (*_decryptSession).Name() : EmptyString);
    }
    inline bool IsValid() const { return (_session != nullptr); }
    inline Exchange::ISession::KeyStatus Status(const uint8_t keyIDLength, const uint8_t keyId[]) const
    {
        Exchange::KeyId key(keyId, keyIDLength);

        KeyStatusesMap::const_iterator index = std::find(_keyStatuses.begin(), _keyStatuses.end(), key);

        return index != _keyStatuses.end() ? index->Status() : Exchange::ISession::StatusPending;
    }
    inline bool HasKeyId(const uint8_t keyIDLength, const uint8_t keyID[]) const
    {
        Exchange::KeyId key(keyID, keyIDLength);

        KeyStatusesMap::const_iterator index = std::find(_keyStatuses.begin(), _keyStatuses.end(), key);

        return (index != _keyStatuses.end());
    }
    inline void Close()
    {
        ASSERT(_session != nullptr);

        _session->Close();
    }
    inline void ResetOutputProtection() {
        ASSERT (_session != nullptr);

        _session->ResetOutputProtection();
    }
    inline void SetParameter(const std::string& name, const std::string& value)
    {
        ASSERT (_session != nullptr);

        _session->SetParameter(name, value);
    }
    inline int Remove()
    {

        ASSERT(_session != nullptr);

        return (_session->Remove() == 0);
    }
    inline int Load()
    {

        ASSERT(_session != nullptr);

        return (_session->Load() == 0);
    }
    inline void Update(const uint8_t* pbResponse, const uint16_t cbResponse)
    {

        ASSERT(_session != nullptr);

        _session->Update(pbResponse, cbResponse);
    }
    uint32_t Decrypt(uint8_t* encryptedData, const uint32_t encryptedDataLength,
        const ::SampleInfo* sampleInfo,
        uint32_t initWithLast15,
        const ::MediaProperties* properties)
    {
        uint32_t result = OpenCDMError::ERROR_INVALID_DECRYPT_BUFFER;

        // lazy create decryptbuffer
        if(_decryptSession == nullptr) {
            DecryptSession(_session);
        }

        // prevent unnecesary double atomic access
        DataExchange* decryptSession = _decryptSession;

        if (decryptSession != nullptr) {
            result = decryptSession->Decrypt(encryptedData, encryptedDataLength, 
                sampleInfo,
                initWithLast15,
                properties);
            if(result)
            {
                TRACE_L1("Decrypt() failed with return code: %x", result);
                result = OpenCDMError::ERROR_UNKNOWN;
            }
        }
        return (result);
    }

    void* SessionPrivateData() const
    {
        return _pvtData;
    }

    uint32_t SessionIdExt() const
    {
        ASSERT(_sessionExt && "This method only works on Exchange::ISessionExt implementations.");
        return _sessionExt->SessionIdExt();
    }

    Exchange::OCDM_RESULT SetDrmHeader(const uint8_t drmHeader[],
        uint32_t drmHeaderLength)
    {
        ASSERT(_sessionExt && "This method only works on Exchange::ISessionExt implementations.");
        return _sessionExt->SetDrmHeader(drmHeader, drmHeaderLength);
    }

    Exchange::OCDM_RESULT GetChallengeDataExt(uint8_t* challenge,
        uint16_t& challengeSize,
        uint32_t isLDL)
    {
        ASSERT(_sessionExt && "This method only works on Exchange::ISessionExt implementations.");
        return _sessionExt->GetChallengeDataExt(challenge, challengeSize, isLDL);
    }

    Exchange::OCDM_RESULT CancelChallengeDataExt()
    {
        ASSERT(_sessionExt && "This method only works on Exchange::ISessionExt implementations.");
        return _sessionExt->CancelChallengeDataExt();
    }

    Exchange::OCDM_RESULT StoreLicenseData(const uint8_t licenseData[],
        uint16_t licenseDataSize,
        uint8_t* secureStopId)
    {
        ASSERT(_sessionExt && "This method only works on Exchange::ISessionExt implementations.");
        return _sessionExt->StoreLicenseData(licenseData, licenseDataSize,
            secureStopId);
    }

    Exchange::OCDM_RESULT SelectKeyId(const uint8_t keyLength, const uint8_t keyId[])
    {
        ASSERT(_sessionExt && "This method only works on Exchange::ISessionExt implementations.");
        return _sessionExt->SelectKeyId(keyLength, keyId);
    }

    Exchange::OCDM_RESULT CleanDecryptContext()
    {
        ASSERT(_sessionExt && "This method only works on Exchange::ISessionExt implementations.");
        return _sessionExt->CleanDecryptContext();
    }

public:
    inline uint32_t Error() const
    {
        return (_errorCode);
    }
    inline uint32_t Error(const uint8_t[], uint8_t) const
    {
        return (_sysError);
    }

    bool BelongsTo(OpenCDMSystem* system) { return system == _system; }

protected:
    void Session(Exchange::ISession* session)
    {
        ASSERT((_session == nullptr) ^ (session == nullptr));

        if (session == nullptr) {

            ASSERT (_session != nullptr);

            _session->Release();

            if (_sessionExt != nullptr) {
                _sessionExt->Release();
                _sessionExt = nullptr;
            }
        }

        _session = session;

        if (session != nullptr) {

            _session->AddRef();
            _sessionExt = _session->QueryInterface<Exchange::ISessionExt>();
        }
    }
    void DecryptSession(Exchange::ISession* session)
    {
        if (session == nullptr) {
            delete _decryptSession;
            _decryptSession = nullptr;
        } else {
            std::string bufferid;
            uint32_t result = _session->CreateSessionBuffer(bufferid);

            if( result == 0 ) {
                ASSERT (_decryptSession == nullptr);
                _decryptSession = new DataExchange(bufferid); 
            }
            else if ( result == 1 ) {
                while( _decryptSession == nullptr ) {
                    SleepMs(0);
                }
            }
            else {
                ASSERT (_decryptSession == nullptr);
                TRACE_L1("DecryptSession could not be created!");
            }
        }
    }
   // Event fired when a key message is successfully created.
    void OnKeyMessage(const uint8_t keyMessage[], const uint16_t length, const std::string& URL)
    {
        _URL = URL;
        TRACE_L1("Received URL: [%s]", _URL.c_str());

        if (_callback != nullptr && _callback->process_challenge_callback != nullptr) {
            _callback->process_challenge_callback(this, _userData, _URL.c_str(), keyMessage, length);
        }
    }

    // Event fired when MediaKeySession encounters an error.
    void OnError(const int16_t error, const Exchange::OCDM_RESULT sysError,
        const std::string& errorMessage)
    {
        _errorCode = error;
        _sysError = sysError;

        if (_callback != nullptr && _callback->error_message_callback != nullptr) {
            _callback->error_message_callback(this, _userData, errorMessage.c_str());
        }
    }

    // Event fired on key status update
    void OnKeyStatusUpdate(const uint8_t keyID[], const uint8_t keyIDLength, const Exchange::ISession::KeyStatus status)
    {   
        Exchange::KeyId key(keyID, keyIDLength);

        KeyStatusesMap::iterator index = std::find(_keyStatuses.begin(), _keyStatuses.end(), key);

        if (index == _keyStatuses.end()) {
            key.Status(status);
            _keyStatuses.emplace_back(key);
        }
        else {
            index->Status(status);
        }

        if ((_callback != nullptr) && (_callback->key_update_callback != nullptr) && (status != Exchange::ISession::StatusPending)) {
            _callback->key_update_callback(this, _userData, keyID, keyIDLength);
        } 
    }

    void OnKeyStatusesUpdated() const
    {
        if ((_callback != nullptr) && (_callback->keys_updated_callback)) {
            _callback->keys_updated_callback(this, _userData);
        }
    }

private:
    std::string _sessionId;
    std::atomic<DataExchange*> _decryptSession;
    Exchange::ISession* _session;
    Exchange::ISessionExt* _sessionExt;
    uint32_t _refCount;
    Core::SinkType<Sink> _sink;
    std::string _URL;
    OpenCDMSessionCallbacks* _callback;
    void* _userData; 
    KeyStatusesMap _keyStatuses;
    std::string _error;
    uint32_t _errorCode;
    Exchange::OCDM_RESULT _sysError;
    OpenCDMSystem* _system;
    void* _pvtData;
};

