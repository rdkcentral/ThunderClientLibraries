# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2022 Metrological
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

project(GraphicsBufferType LANGUAGES CXX)

cmake_minimum_required(VERSION 3.15)

find_package(Thunder)

project_version(1.0.0)

set(MODULE_NAME Client${PROJECT_NAME})

message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

option(INSTALL_EXAMPLES "Install the examples." OFF)

find_package(${NAMESPACE}Core REQUIRED)

add_library(${MODULE_NAME} INTERFACE)
add_library(${MODULE_NAME}::${MODULE_NAME} ALIAS ${MODULE_NAME})

target_include_directories(${MODULE_NAME} INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}>)

target_compile_features(${MODULE_NAME} INTERFACE cxx_std_11)

install(TARGETS ${MODULE_NAME} EXPORT ${MODULE_NAME}Targets)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/graphicsbuffer/GraphicsBufferType.h 
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/graphicsbuffer COMPONENT ${NAMESPACE}_Development)

include(HeaderOnlyInstall)

HeaderOnlyInstallPackageConfig(TARGET ${MODULE_NAME}
    NAME ${MODULE_NAME}
    VERSION ${PROJECT_VERSION}
    DESCRIPTION "Header only library for creating and using graphics buffers.")

HeaderOnlyInstallCMakeConfig(TARGET ${MODULE_NAME} TREAT_AS_NORMAL)

add_subdirectory(example)
