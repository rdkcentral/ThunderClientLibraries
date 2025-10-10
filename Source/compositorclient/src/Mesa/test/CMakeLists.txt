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

find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(gbm REQUIRED)
find_package(libdrm REQUIRED)
find_package(EGL REQUIRED)

add_executable(gbm_buffer_test gbm_buffer_test.cpp)

# Set target properties
set_target_properties(gbm_buffer_test PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    OUTPUT_NAME gbm_buffer_test
)

target_link_libraries(gbm_buffer_test
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        libdrm::libdrm
        gbm::gbm
        EGL::EGL
)

install(TARGETS gbm_buffer_test DESTINATION ${CMAKE_INSTALL_BINDIR}/${NAMESPACE}Tests COMPONENT ${NAMESPACE}_Test)

