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

#include "IPCSecurityToken.h"
#include "securityagent.h"

using namespace Thunder;

static string GetEndPoint()
{
    TCHAR* value = ::getenv(_T("SECURITYAGENT_PATH"));

    return (value == nullptr ?
        #ifdef __WINDOWS__
        _T("127.0.0.1:63000")
        #else
        _T("/tmp/SecurityAgent/token")
        #endif
        : value);
}

extern "C" {

/*
 * GetToken - function to obtain a token from the SecurityAgent
 *
 * Parameters
 *  MaxIdLength - holds the maximal uint8_t length of the token
 *  Id          - Buffer holds the data to tokenize on its way in, and returns in the same buffer the token.
 *
 * Return value
 *  < 0 - failure, absolute value returned is the length required to store the token
 *  > 0 - success, char length of the returned token
 *
 * Post-condition; return value 0 should not occur
 *
 */
int GetToken(unsigned short maxLength, unsigned short inLength, unsigned char buffer[])
{
    auto engine = Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create();
    auto client = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId(GetEndPoint().c_str()), Core::ProxyType<Core::IIPCServer>(engine));

    int result = -1;

    if ((client.IsValid() == true) && (client->IsOpen() == false)) {
        PluginHost::IAuthenticate* securityAgentInterface = client->Open<PluginHost::IAuthenticate>("SecurityAgent");
        
        if (securityAgentInterface != nullptr) {
            std::string token;
            uint32_t error = securityAgentInterface->CreateToken(inLength, buffer, token);

            if (error == Core::ERROR_NONE) {
                result = static_cast<uint32_t>(token.length());

                if (result <= maxLength) {
                    std::copy(std::begin(token), std::end(token), buffer);
                } else {
                    TRACE_L1(_T("Received token is too long [%d]."), result);
                    result = -result;
                }
            } else {
                result = error;
                result = -result;
            }

            securityAgentInterface->Release();
        }

        client.Release();
    } else {
        TRACE_L1(_T("Could not open link. error=%d"), result);
    }

    return (result);
}

void securityagent_dispose() {
    Core::Singleton::Dispose();
}

}
