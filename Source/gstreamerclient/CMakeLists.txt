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

project(gstreamerclient)

cmake_minimum_required(VERSION 3.3)

find_package(WPEFramework)

project_version(1.0.0)

set(TARGET ${PROJECT_NAME})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

message("Setup ${TARGET} v${PROJECT_VERSION}")

set(GSTREAMERCLIENT_IMPLEMENTATION "None" CACHE STRING "Defines which implementation is used.")

# Legacy option support
if(GSTREAMER_CLIENT_IMPLEMENTATION)
	set(GSTREAMERCLIENT_IMPLEMENTATION "${GSTREAMER_CLIENT_IMPLEMENTATION}" CACHE STRING "Defines which implementation is used." FORCE)
	unset(GSTREAMER_CLIENT_IMPLEMENTATION CACHE)
endif()

find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}COM REQUIRED)

if (NOT "${GSTREAMERCLIENT_IMPLEMENTATION}" STREQUAL "None")

	message("GstreamerClient impl. for ${GSTREAMERCLIENT_IMPLEMENTATION}")
	find_package(GSTREAMER REQUIRED)
	find_package(GSTREAMER_BASE REQUIRED)

    if("${GSTREAMERCLIENT_IMPLEMENTATION}" STREQUAL "Nexus")
        if (GSTREAMERCLIENT_IMPLEMENTATION_PATH)
            add_subdirectory(${GSTREAMERCLIENT_IMPLEMENTATION_PATH})
        else()
            include(GetExternalCode)
            set(GSTREAMERCLIENT_IMPLEMENTATION_VERSION "master" CACHE STRING "gstreamerclient implementation version")
            set(GSTREAMERCLIENT_IMPLEMENTATION_REPOSITORY "https://code.rdkcentral.com/r/soc/broadcom/components/rdkcentral/thunder/gstreamerclient" CACHE STRING "gstreamerclient implementation repository")
            GetExternalCode(
                GIT_REPOSITORY ${GSTREAMERCLIENT_IMPLEMENTATION_REPOSITORY}
                GIT_TAG ${GSTREAMERCLIENT_IMPLEMENTATION_VERSION}
                SOURCE_DIR "Nexus"
            )
            add_subdirectory(Nexus)
        endif()
    else()
        add_subdirectory (${GSTREAMERCLIENT_IMPLEMENTATION})
    endif()
endif()

install(
	FILES gstreamerclient.h
        DESTINATION include/${NAMESPACE}/gstreamerclient/
)
