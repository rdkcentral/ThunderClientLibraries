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

project(ocdm)

cmake_minimum_required(VERSION 3.3 )

find_package(WPEFramework)

project_version(1.0.0)

set(TARGET ${PROJECT_NAME})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

message("Setup ${TARGET} v${PROJECT_VERSION}")


find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}COM REQUIRED)

find_package(GSTREAMER REQUIRED)
find_package(GSTREAMER_BASE REQUIRED)

option(CDMI_ADAPTER_IMPLEMENTATION "Defines which implementation is used." "None")


add_library(${TARGET} SHARED
        open_cdm.cpp
        open_cdm_ext.cpp
        )

set(PUBLIC_HEADERS
        open_cdm.h
        adapter/open_cdm_adapter.h
        open_cdm_ext.h
        Module.h
        )

target_link_libraries(${TARGET}
        PUBLIC
          ${NAMESPACE}Core::${NAMESPACE}Core
          ${NAMESPACE}COM::${NAMESPACE}COM
        )

target_include_directories( ${TARGET}
        PUBLIC
          $<INSTALL_INTERFACE:include/opencdm>
        )

target_include_directories( ${TARGET}
        PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/adapter>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
        )

if("${CDMI_ADAPTER_IMPLEMENTATION}" STREQUAL "broadcom-svp")
    if (OCDM_IMPLEMENTATION_PATH)
        target_sources(${TARGET} PRIVATE ${OCDM_IMPLEMENTATION_PATH}/open_cdm_adapter.cpp)
    else()
        include(GetExternalCode)
        set(OCDM_IMPLEMENTATION_VERSION "master" CACHE STRING "ocdmadapter implementation version")
        set(OCDM_IMPLEMENTATION_REPOSITORY "https://code.rdkcentral.com/r/soc/broadcom/components/rdkcentral/thunder/ocdmadapter" CACHE STRING "ocdmadapter implementation repository")
        GetExternalCode(
            GIT_REPOSITORY ${OCDM_IMPLEMENTATION_REPOSITORY}
            GIT_TAG ${OCDM_IMPLEMENTATION_VERSION}
            SOURCE_DIR "adapter/broadcom-svp"
        )
        target_sources(${TARGET} PRIVATE adapter/broadcom-svp/open_cdm_adapter.cpp)
    endif()
else()
    target_sources(${TARGET} PRIVATE adapter/${CDMI_ADAPTER_IMPLEMENTATION}/open_cdm_adapter.cpp)
endif()

target_link_libraries(${TARGET}
        PRIVATE
        ${GSTREAMER_LIBRARIES}
        ${GSTREAMER_BASE_LIBRARIES}
        CompileSettingsDebug::CompileSettingsDebug
        )

target_include_directories( ${TARGET}
        PRIVATE
        ${GSTREAMER_INCLUDES}
        ${GSTREAMER_BASE_INCLUDES}
        )

if (${CDMI_ADAPTER_IMPLEMENTATION} STREQUAL "broadcom-svp")
    find_package(NEXUS REQUIRED)
    target_link_libraries(${TARGET}
            PRIVATE
            NEXUS::NEXUS
            -lbrcmsvpmeta
            )
endif()

if (${CDMI_ADAPTER_IMPLEMENTATION} STREQUAL "broadcom-svp-secbuf")
    target_link_libraries(${TARGET}
            PRIVATE
            -lb_secbuf
            -lgstsvpext
            )
endif()

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        )


install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/ocdm COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}/ocdm   # default include path
)

#FIXME: This needs to become a sysmlink.
install(
        FILES ${PUBLIC_HEADERS}
        DESTINATION include/opencdm
)

InstallCMakeConfig(
        TARGETS ${TARGET})

InstallPackageConfig(
        TARGETS ${TARGET} 
        DESCRIPTION "OCDM library")
