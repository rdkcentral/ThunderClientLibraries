# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2023 Metrological
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

project(BluetoothAudioSource)

project_version(1.0.0)

set(TARGET Client${PROJECT_NAME})

message("Setup ${TARGET} ${PROJECT_VERSION}")

find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}COM REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

option(BLUETOOTHAUDIOSOURCE_EXAMPLERECORDER "Build audio recorder" OFF)
set(PUBLIC_HEADERS "include/bluetoothaudiosource.h")

add_library(${TARGET} SHARED
    Module.cpp
    BluetoothAudioSource.cpp
)

target_link_libraries(${TARGET}
   PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}COM::${NAMESPACE}COM
        CompileSettingsDebug::CompileSettingsDebug
)

set_target_properties(${TARGET} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    FRAMEWORK FALSE
    PUBLIC_HEADER "${PUBLIC_HEADERS}"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_include_directories(${TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

install(
    TARGETS ${TARGET} EXPORT ${TARGET}Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT libs
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT libs
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT libs
    FRAMEWORK DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT libs
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/bluetoothaudiosource COMPONENT devel
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/bluetoothaudiosource
)

InstallCMakeConfig(
    TARGETS ${TARGET}
)

InstallPackageConfig(
    TARGETS ${TARGET}
    DESCRIPTION "Bluetooth audio capture abstraction"
)

if(BLUETOOTHAUDIOSOURCE_EXAMPLERECORDER)
    add_subdirectory(btaudiorecorder)
endif()
