# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2024 RDK Management
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

cmake_minimum_required(VERSION 3.8)
project(L2Test)

set(CMAKE_CXX_STANDARD 11)

find_package(Thunder)
find_package(ClientOCDM REQUIRED)
find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}COM REQUIRED)
find_package(${NAMESPACE}Messaging REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(GTest REQUIRED)
include(CTest)

file(GLOB TESTS tests/*.cpp)

add_executable(${PROJECT_NAME}
	gtest_main.cpp
        ${TESTS}
        )

include_directories(../../Source/ocdm)
link_directories(../../Source/ocdm)

target_link_libraries(${PROJECT_NAME}
	${NAMESPACE}Core::${NAMESPACE}Core
	${NAMESPACE}COM::${NAMESPACE}COM
	${NAMESPACE}Messaging::${NAMESPACE}Messaging
        CompileSettingsDebug::CompileSettingsDebug
	ClientOCDM::ClientOCDM
	GTest::GTest
        )

gtest_add_tests(TARGET ${PROJECT_NAME})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
