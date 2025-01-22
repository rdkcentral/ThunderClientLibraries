#
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

cmake_minimum_required(VERSION 3.3)
project(ClientLibraries)

find_package(WPEFramework)

option(BLUETOOTHAUDIOSINK
        "Include the bluetoothaudiosink library." OFF)
option(BLUETOOTHAUDIOSOURCE
        "Include the bluetoothaudiosource library." OFF)
option(PROTOCOLS
        "Include the protocols library." ON)
option(COMPOSITORCLIENT
        "Include a graphics backend abstraction for external applications." OFF)
option(DEVICEINFO
        "Include the deviceinfo COMRPC abstraction library." OFF)
option(DISPLAYINFO
        "Include the displayinfo COMRPC abstraction library." OFF)
option(SECURITYAGENT
        "Include the securityagent library." OFF)
option(PLAYERINFO
        "Include the playerinfo COMRPC abstraction library." OFF)
option(PROVISIONPROXY
        "Include the provisionproxy library." OFF)
option(CDMI
        "Include OpenCDM interface." OFF)
option(CRYPTOGRAPHY
        "Include the cryptography library." OFF)
# TODO: enable conditionally from recipe file
option(POWERMANAGER
        "Include the powermanager COMRPC abstraction library." ON)

option(INSTALL_TESTS "Install the test applications" OFF)

if (BUILD_REFERENCE)
    add_definitions (-DBUILD_REFERENCE=${BUILD_REFERENCE})
endif()

add_subdirectory(Source)
add_subdirectory(Tests)

