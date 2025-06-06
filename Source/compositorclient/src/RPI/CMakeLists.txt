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

find_package(EGL REQUIRED)

find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}COM REQUIRED)

add_library(${PLUGIN_COMPOSITOR_IMPLEMENTATION} OBJECT Implementation.cpp)

option(VC6 "Enable VC6 support" OFF)
option(COMPOSITOR_SERVER_PLUGIN "Enable support for compositor server" OFF)

if(COMPOSITOR_SERVER_PLUGIN)
    target_definitions(${PLUGIN_COMPOSITOR_IMPLEMENTATION} PRIVATE COMPOSITORSERVERPLUGIN)
endif()

if(VC6)
    find_package(gbm REQUIRED)
    find_package(libdrm REQUIRED)
    
    target_sources(${PLUGIN_COMPOSITOR_IMPLEMENTATION} PRIVATE ModeSet.cpp)
    target_compile_definitions(${PLUGIN_COMPOSITOR_IMPLEMENTATION} PRIVATE VC6)
endif()

target_link_libraries(${PLUGIN_COMPOSITOR_IMPLEMENTATION}
    PRIVATE
        common::include
        CompileSettingsDebug::CompileSettingsDebug
    PUBLIC
        ${NAMESPACE}Core::${NAMESPACE}Core
        ClientVirtualInput::ClientVirtualInput
        ${NAMESPACE}COM::${NAMESPACE}COM
        EGL::EGL
)

if(VC6)
    target_link_libraries(${PLUGIN_COMPOSITOR_IMPLEMENTATION}
        PUBLIC
            libdrm::libdrm
    )
endif()

set_target_properties(${PLUGIN_COMPOSITOR_IMPLEMENTATION} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_compile_options (${PLUGIN_COMPOSITOR_IMPLEMENTATION} PRIVATE -Wno-psabi)

target_include_directories(${PLUGIN_COMPOSITOR_IMPLEMENTATION}
    PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>)
