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

find_package(WPEFramework)

find_package(${NAMESPACE}Definitions REQUIRED)
find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}PrivilegedRequest REQUIRED)
find_package(CompileSettingsDebug REQUIRED)

add_executable(cb-test main.cpp)

target_link_libraries(cb-test PRIVATE
    CompileSettingsDebug::CompileSettingsDebug
    ${NAMESPACE}Definitions::${NAMESPACE}Definitions
    ${NAMESPACE}Core::${NAMESPACE}Core
    ${NAMESPACE}PrivilegedRequest::${NAMESPACE}PrivilegedRequest
    ${NAMESPACE}CompositorBufferType::${NAMESPACE}CompositorBufferType)

if(INSTALL_EXAMPLES)
    install(TARGETS cb-test DESTINATION bin)
endif()



