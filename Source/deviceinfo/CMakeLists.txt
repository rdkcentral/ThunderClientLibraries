# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2021 Metrological
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

project(deviceinfo)

cmake_minimum_required(VERSION 3.3)

find_package(WPEFramework)

project_version(1.0.0)

set(TARGET ${PROJECT_NAME})

message("Setup ${TARGET} v${PROJECT_VERSION}")

find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}Tracing REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(${NAMESPACE}COM REQUIRED)

set(PUBLIC_HEADERS "deviceinfo.h")

add_library(${TARGET}  SHARED
    Module.cpp
    DeviceInfo.cpp
)

target_link_libraries(${TARGET}
        PUBLIC
          ${NAMESPACE}Core::${NAMESPACE}Core
          ${NAMESPACE}Tracing::${NAMESPACE}Tracing
          ${NAMESPACE}COM::${NAMESPACE}COM
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

target_include_directories( ${TARGET}
        PUBLIC
          $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
          $<INSTALL_INTERFACE:include>
        )

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/deviceinfo COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}/deviceinfo # headers
)

InstallCMakeConfig(
        TARGETS ${TARGET})

InstallPackageConfig(
        TARGETS ${TARGET}
        DESCRIPTION "communications channel abstraction to get the device properties")

add_subdirectory(device_info)
