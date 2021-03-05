# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 RDK Management
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

project(compositorclient)

cmake_minimum_required(VERSION 3.3)

find_package(WPEFramework)

project_version(1.0.0)

set(TARGET ${PROJECT_NAME})

message("Setup ${TARGET} v${PROJECT_VERSION}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(PLUGIN_COMPOSITOR_IMPLEMENTATION "Defines which implementation is used." "None")

find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(${NAMESPACE}Core REQUIRED)

if("${PLUGIN_COMPOSITOR_IMPLEMENTATION}" STREQUAL "Nexus")
    if (COMPOSITORCLIENT_IMPLEMENTATION_PATH)
        add_subdirectory(${COMPOSITORCLIENT_IMPLEMENTATION_PATH})
    else()
        include(GetExternalCode)
        set(COMPOSITORCLIENT_IMPLEMENTATION_VERSION "master" CACHE STRING "compositorclient implementation version")
        set(COMPOSITORCLIENT_IMPLEMENTATION_REPOSITORY "https://code.rdkcentral.com/r/soc/broadcom/components/rdkcentral/thunder/compositorclient" CACHE STRING "compositorclient implementation repository")
        GetExternalCode(
            GIT_REPOSITORY ${COMPOSITORCLIENT_IMPLEMENTATION_REPOSITORY}
            GIT_TAG ${COMPOSITORCLIENT_IMPLEMENTATION_VERSION}
            SOURCE_DIR "Nexus"
        )
        add_subdirectory(Nexus)
    endif()
else()
    add_subdirectory (${PLUGIN_COMPOSITOR_IMPLEMENTATION})
endif()

install(
	FILES Client.h
        DESTINATION include/${NAMESPACE}/compositor
)
