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

set(TARGET ${PROJECT_NAME})

message("Setup ${TARGET} v${PROJECT_VERSION}")

set(PLUGIN_COMPOSITOR_IMPLEMENTATION "None" CACHE STRING "Defines which implementation is used")

find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(${NAMESPACE}Core REQUIRED)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if("${PLUGIN_COMPOSITOR_IMPLEMENTATION}" STREQUAL "Nexus")
    if (COMPOSITORCLIENT_IMPLEMENTATION_PATH)
        add_subdirectory(${COMPOSITORCLIENT_IMPLEMENTATION_PATH})
    else()
        include(GetExternalCode)
        set(COMPOSITORCLIENT_IMPLEMENTATION_VERSION "master" CACHE STRING "compositorclient implementation version: commit_id_or_tag_or_branch_name")
        set(COMPOSITORCLIENT_IMPLEMENTATION_REPOSITORY "https://code.rdkcentral.com/r/soc/broadcom/components/rdkcentral/thunder/compositorclient" CACHE STRING "compositorclient implementation repository")
        GetExternalCode(
            GIT_REPOSITORY ${COMPOSITORCLIENT_IMPLEMENTATION_REPOSITORY}
            GIT_VERSION ${COMPOSITORCLIENT_IMPLEMENTATION_VERSION}
            SOURCE_DIR "Nexus"
        )
        add_subdirectory(Nexus)
    endif()
else()
    add_subdirectory (${PLUGIN_COMPOSITOR_IMPLEMENTATION})
endif()

add_library(${TARGET}  $<TARGET_OBJECTS:${PLUGIN_COMPOSITOR_IMPLEMENTATION}> Module.cpp) 

target_link_libraries(${TARGET}
    PRIVATE
        ${PLUGIN_COMPOSITOR_IMPLEMENTATION}
        ${NAMESPACE}Core::${NAMESPACE}Core
        CompileSettingsDebug::CompileSettingsDebug
    )

set(PUBLIC_HEADERS "../include/compositor/Client.h")

set_target_properties(${TARGET} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    FRAMEWORK FALSE
    PUBLIC_HEADER "${PUBLIC_HEADERS}"
    LINK_WHAT_YOU_USE TRUE
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

install(
    TARGETS ${TARGET} EXPORT ${TARGET}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Runtime NAMELINK_COMPONENT ${NAMESPACE}_Development
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/compositor COMPONENT ${NAMESPACE}_Development
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}
)

target_include_directories(${TARGET} INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}>)

add_library(${TARGET}::${TARGET} ALIAS ${TARGET})

InstallPackageConfig(
        TARGETS ${TARGET}
        DESCRIPTION "A ${PLUGIN_COMPOSITOR_IMPLEMENTATION} compositor client implemetation wrapper.")

InstallCMakeConfig(TARGETS ${TARGET})
