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

project(ocdmtest)

set(TARGET ${PROJECT_NAME})

cmake_minimum_required(VERSION 3.15)

find_package(${NAMESPACE}Core REQUIRED)

if(NOT TARGET ocdm::ocdm)
    find_package(ocdm REQUIRED)
endif()

find_package(CompileSettingsDebug CONFIG REQUIRED)

add_executable(${PROJECT_NAME}
    main.cpp
)

target_link_libraries(${TARGET}
   PRIVATE 
        ${NAMESPACE}Core::${NAMESPACE}Core
        CompileSettingsDebug::CompileSettingsDebug
        ocdm::ocdm
)

if(INSTALL_TESTS)
    install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
