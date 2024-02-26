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
#include "open_cdm_impl.h"

class SessionPrivate {
    private:
        typedef uint32_t (*ConstructSessionPrivate)(struct OpenCDMSession*, void*&);
        typedef uint32_t (*DestructSessionPrivate)(struct OpenCDMSession*, void*&);

    public:
        SessionPrivate(const SessionPrivate&) = delete;
        SessionPrivate& operator= (const SessionPrivate&) = delete;

        SessionPrivate()
            : _constructSessionPvt(nullptr)
            , _destructSessionPvt(nullptr) {
                Load();
        }
        ~SessionPrivate() = default;

        OpenCDMError Construct(struct OpenCDMSession* session, void* &pvtData)
        {
            OpenCDMError result = OpenCDMError::ERROR_METHOD_NOT_IMPLEMENTED;
            if(_constructSessionPvt != nullptr) {
                result = ( _constructSessionPvt(session, pvtData) == 0 ? OpenCDMError::ERROR_NONE : OpenCDMError::ERROR_UNKNOWN);
            }
            return result;
        }

        OpenCDMError Destruct(struct OpenCDMSession* session, void* &pvtData)
        {
            OpenCDMError result = OpenCDMError::ERROR_METHOD_NOT_IMPLEMENTED;
            if(_destructSessionPvt != nullptr) {
                result = ( _destructSessionPvt(session, pvtData) == 0 ? OpenCDMError::ERROR_NONE : OpenCDMError::ERROR_UNKNOWN);
            }
            return result;
        }

    private:
        void Load() {
            const TCHAR *name = nullptr;
            Core::Library library(name);
            if (library.IsLoaded() == true) {
                _constructSessionPvt = reinterpret_cast<ConstructSessionPrivate>(library.LoadFunction(_T("opencdm_construct_session_private")));
                _destructSessionPvt = reinterpret_cast<DestructSessionPrivate>(library.LoadFunction(_T("opencdm_destruct_session_private")));
            }

        }

    private:
        ConstructSessionPrivate _constructSessionPvt;
        DestructSessionPrivate _destructSessionPvt;
        bool _bLoaded;
};

static SessionPrivate SessionPvt;

OpenCDMError OpenCDMSession::CreateSession(struct OpenCDMSystem* system,
                            const LicenseType licenseType, const char initDataType[],
                            const uint8_t initData[], const uint16_t initDataLength,
                            const uint8_t CDMData[], const uint16_t CDMDataLength,
                            OpenCDMSessionCallbacks* callbacks, void* userData,
                            struct OpenCDMSession** session)
{
    OpenCDMError result(ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        *session = new OpenCDMSession(system, std::string(initDataType),
                            initData, initDataLength, CDMData,
                            CDMDataLength, licenseType, callbacks, userData);
        result = (*session != nullptr ? OpenCDMError::ERROR_NONE
                                      : OpenCDMError::ERROR_INVALID_SESSION);
        if(result == OpenCDMError::ERROR_NONE) {
            void *pvtData = nullptr;
            (*session)->_pvtData = nullptr;
            OpenCDMError pvt_result(OpenCDMError::ERROR_METHOD_NOT_IMPLEMENTED);
            pvt_result = SessionPvt.Construct(*session, pvtData);
            if(pvt_result == OpenCDMError::ERROR_NONE) {
                (*session)->_pvtData = pvtData;
            }
            else if(pvt_result != OpenCDMError::ERROR_METHOD_NOT_IMPLEMENTED)
            {
                //Method is available but resulted in internal error. Not good for decryption.
                result = OpenCDMError::ERROR_INVALID_SESSION;
                (*session)->Release();
                *session = nullptr;
            }
        }
    }
    return result;
}

OpenCDMSession::~OpenCDMSession()
{
    OpenCDMAccessor* system = OpenCDMAccessor::Instance();

    SessionPvt.Destruct(this, _pvtData);

    system->RemoveSession(_sessionId);

    if (IsValid()) {
        _session->Revoke(&_sink);
    }

    if (_session != nullptr) {
        Session(nullptr);
    }
    if (_decryptSession != nullptr) {
        DecryptSession(nullptr);
    }

    TRACE_L1("Destructed the Session Client side: %p", this);
}
