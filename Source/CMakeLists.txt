# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2020 Metrological
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

if(BLUETOOTHAUDIOSINK)
    add_subdirectory(bluetoothaudiosink)
endif()

if(BLUETOOTHAUDIOSOURCE)
    add_subdirectory(bluetoothaudiosource)
endif()

if(GRAPHICSBUFFER)
    add_subdirectory(graphicsbuffer)
endif()

if(COMPOSITORCLIENT)
    add_subdirectory(compositorclient)
endif()

if(DEVICEINFO)
    add_subdirectory(deviceinfo)
endif()

if(DISPLAYINFO)
    add_subdirectory(displayinfo)
endif()

if(SECURITYAGENT)
    add_subdirectory(securityagent)
endif()

if(PLAYERINFO)
    add_subdirectory(playerinfo)
endif()

if(PROVISIONPROXY)
    add_subdirectory(provisionproxy)
endif()

if(VIRTUALINPUT)
    add_subdirectory(virtualinput)
endif()

if(CDMI)
    add_subdirectory(ocdm)
endif()

if(CRYPTOGRAPHY)
    add_subdirectory(cryptography)
endif()
