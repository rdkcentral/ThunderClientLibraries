# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2025 Metrological
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
# limitations under the License

find_package(Thunder REQUIRED)
find_package(${NAMESPACE}Core CONFIG REQUIRED)
find_package(${NAMESPACE}Messaging CONFIG REQUIRED)
find_package(${NAMESPACE}LocalTracer CONFIG REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_executable(client-compositor-render
    Module.cpp
    Main.cpp
)

target_link_libraries(client-compositor-render
    PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging
        ${NAMESPACE}LocalTracer::${NAMESPACE}LocalTracer
        CompileSettingsDebug::CompileSettingsDebug
        ClientCompositorRenderCommon
)

set_target_properties(client-compositor-render PROPERTIES
    CXX_STANDARD ${CXX_STD}
    CXX_STANDARD_REQUIRED YES
)

if(INSTALL_CLIENT_COMPOSITOR_RENDER_TEST_APP)
install(TARGETS client-compositor-render 
    DESTINATION ${CMAKE_INSTALL_BINDIR} 
    COMPONENT ${NAMESPACE}_Test)
endif()