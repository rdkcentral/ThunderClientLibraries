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

cmake_minimum_required(VERSION 3.15)

find_package(Thunder)

project(VirtualInput)

project_version(1.0.0)

set(TARGET Client${PROJECT_NAME})

message("Setup ${TARGET} v${PROJECT_VERSION}")

option(VIRTUALINPUT_TOOLS "Build VirtualInput tools" OFF)

find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(${NAMESPACE}Core REQUIRED)

add_library(${TARGET} 
        Module.cpp
        virtualinput.cpp
        )

set(PUBLIC_HEADERS
        virtualinput.h
        Module.h
        )

target_link_libraries(${TARGET}
        PRIVATE
          ${NAMESPACE}Core::${NAMESPACE}Core
          CompileSettingsDebug::CompileSettingsDebug
        )

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        )

target_include_directories(${TARGET}
        PUBLIC
          $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../>
          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}>
          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/virtualinput>
        )

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Development
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Runtime NAMELINK_COMPONENT ${NAMESPACE}_Development
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Runtime
        FRAMEWORK DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Runtime
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/virtualinput COMPONENT ${NAMESPACE}_Development
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/virtualinput  
)

InstallCMakeConfig(
        TARGETS ${TARGET})

InstallPackageConfig(
        TARGETS ${TARGET}
        DESCRIPTION "An abstraction layer for input virtualisation using an IPC message channel.")

if (VIRTUALINPUT_TOOLS)
    add_subdirectory(tools)
endif()
