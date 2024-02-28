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

// As this is an archive, it will not be the actal name that is used since the actual namee will be cdeclared (using the
// defined name there) in the module that will turn this into a application/SO. However, we need to please Thunder
// that a module name is defined before we can include the Thunder unit libraries. That is what we doe here :-)
// NOTE: Do not declare the MODULE_NAME here as you will get a clash with the using application/SO.
#define MODULE_NAME ProvisionProxy

#include <core/core.h>
#include <com/com.h>
#include <interfaces/IProvisioning.h>
#include <provision/DRMInfo.h>

#include "IPCProvision.h"
MODULE_NAME_ARCHIVE_DECLARATION

using namespace WPEFramework;

static string GetEndPoint()
{
    TCHAR* value = ::getenv(_T("PROVISION_PATH"));

#ifdef __WINDOWS__
    return (value == nullptr ? _T("127.0.0.1:7777") : value);
#else
    return (value == nullptr ? _T("/tmp/provision") : value);
#endif
}

extern "C" {
/*
 * GetDeviceId - function to obtain the unique Device ID
 *
 * Parameters
 *  MaxIdLength - holds the maximal char length of the Id parameter
 *  Id          - Buffer holds the unique Device ID upon succession, empty in case of failure
 *
 * Return value
 *  < 0 - failure, absolute value returned is the length required to store the Id
 *  > 0 - success, char length of the returned Id
 *
 * Post-condition; return value 0 should not occur
 *
 */
int GetDeviceId(unsigned short MaxIdLength, char Id[])
{
    auto engine = Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create();
    auto client = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId(GetEndPoint().c_str()), Core::ProxyType<Core::IIPCServer>(engine));

    int result = -1;

    if ((client.IsValid() == true) && (client->IsOpen() == false)) {
        Exchange::IProvisioning* provisioningInterface = client->Open<Exchange::IProvisioning>("Provisioning");
        if (provisioningInterface != nullptr) {
            string deviceId;
            uint32_t error = provisioningInterface->DeviceId(deviceId);
            if (error == Core::ERROR_NONE) {
                TRACE_L1(_T("Received deviceId '%s'."), deviceId.c_str());
                result = static_cast<int>(deviceId.size());
                if (result <= MaxIdLength) {
                    std::copy(deviceId.begin(), deviceId.end(), Id);
                } else {
                    TRACE_L1(_T("Received deviceId is too long [%d]."), result);
                    result = -result;
                }

            } else {
                result = error;
                result = -result;
            }

            provisioningInterface->Release();
        }
        client.Release();
    } else {
        TRACE_L1(_T("Could not open link. error=%d"), result);
    }

    return result;
}

/*
 * GetDRMId - function to obtain the DRM Id in the clear
 *
 * Parameters
 *  MaxIdLength - holds the maximal char length of the Id parameter
 *  Id          - Buffer holds the DRM ID upon succession, empty in case of failure
 *  label       - Name of the DRMId requested (if not supplied, label == playready)
 *
 * Return value
 *  < 0 - failure, absolute value returned is the length required to store the Id
 *  > 0 - success, char length of the returned Id
 *
 * Post-condition; return value 0 should not occur
 *
 */

int GetDRMId(const char label[], const unsigned short maxIdLength, char outId[])
{
    auto engine = Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create();
    auto client = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId(GetEndPoint().c_str()), Core::ProxyType<Core::IIPCServer>(engine));

    int result = -1;

    if ((client.IsValid() == true) && (client->IsOpen() == false)) {
        Exchange::IProvisioning* provisioningInterface = client->Open<Exchange::IProvisioning>("Provisioning");
        if (provisioningInterface != nullptr) {
            uint8_t buffer[10 * 1024]; //should fit ;)
            uint16_t size = sizeof(buffer);

            uint32_t error = provisioningInterface->DRMId(label, size, buffer);

            if (error == Core::ERROR_NONE) {

                // This is a huge encrypted blob, convert it to an uencrypted required info
                result = ClearBlob(size, reinterpret_cast<const char*>(buffer) , maxIdLength, outId);
                if (result > 0) {
                    TRACE_L1(_T("Received Provision Info for '%s' with length [%d]."), label, result);
                } else {
                    TRACE_L1(_T("Provisioning for %s too big. Length: %d - %d."), label, -result, maxIdLength);
                }

            } else {
                TRACE_L1(_T("Failed to extract %s provisioning. Error code %d."), label, error);
            }

            provisioningInterface->Release();
        }
        client.Release();
    } else {
        TRACE_L1(_T("Could not open link. error=%d"), result);
    }

    return result;
}

void provisioningproxy_dispose()
{
    Core::SingletonType<Provisioning>::Dispose();
}
} // extern "C"
