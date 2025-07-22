# If not stated otherwise in this file or this component's license file the
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

find_package(${NAMESPACE}Core CONFIG REQUIRED)
find_package(${NAMESPACE}COM CONFIG REQUIRED)
find_package(${NAMESPACE}PrivilegedRequest REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(${NAMESPACE}Messaging CONFIG REQUIRED)

find_package(gbm REQUIRED)
find_package(libdrm REQUIRED)
find_package(EGL REQUIRED)

add_library(${PLUGIN_COMPOSITOR_IMPLEMENTATION} OBJECT Implementation.cpp)

target_link_libraries(${PLUGIN_COMPOSITOR_IMPLEMENTATION}
    PRIVATE
        common::include
        ${NAMESPACE}Core::${NAMESPACE}Core
        ClientVirtualInput::ClientVirtualInput
        ${NAMESPACE}COM::${NAMESPACE}COM
        ${NAMESPACE}PrivilegedRequest::${NAMESPACE}PrivilegedRequest
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging
        ClientGraphicsBufferType::ClientGraphicsBufferType
        CompileSettingsDebug::CompileSettingsDebug
        libdrm::libdrm
        gbm::gbm
        EGL::EGL
)

set_target_properties(${PLUGIN_COMPOSITOR_IMPLEMENTATION} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_compile_definitions(${PLUGIN_COMPOSITOR_IMPLEMENTATION}
    PUBLIC
        EGL_NO_X11
    )
