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

option(BUILD_NETFLIX_VAULT_GENERATOR "Build Netflix Vault generator" OFF)

find_package(${NAMESPACE}Core REQUIRED)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../../../../Source)

set(CMAKE_CXX_STANDARD 11)

add_executable(cgimptests
        Module.cpp
        ImplementationTests.cpp
        Helpers.cpp
        Test.c
    )

find_package(OpenSSL)

include_directories(${CMAKE_CURRENT_LIST_DIR}/../../../cryptography)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../../../cryptography/implementation)
include_directories($<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}>)

set_target_properties(cgimptests PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
    )

target_link_libraries(cgimptests
        PRIVATE
        ${NAMESPACE}Cryptography
        ${NAMESPACE}Core::${NAMESPACE}Core
        ssl
        crypto
    )


add_executable(cgfacetests
        InterfaceTests.cpp
        Helpers.cpp
        Test.c
    )

find_package(OpenSSL)

include_directories(${CMAKE_CURRENT_LIST_DIR}/../../../cryptography)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../../../cryptography/implementation)
include_directories($<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}>)

set_target_properties(cgfacetests PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
    )

target_link_libraries(cgfacetests
        PRIVATE
        ${NAMESPACE}Cryptography
        ${NAMESPACE}Core::${NAMESPACE}Core
        ssl
        crypto
    )

add_executable(cgnfsecuritytests
        NetflixSecurityTests.cpp
        Helpers.cpp
        Test.c
    )

include_directories(${CMAKE_CURRENT_LIST_DIR}/../../../cryptography)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../../../cryptography/implementation)
include_directories($<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}>)

set_target_properties(cgnfsecuritytests PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
    )

target_link_libraries(cgnfsecuritytests
        PRIVATE
        ${NAMESPACE}Cryptography
        ${NAMESPACE}Core::${NAMESPACE}Core
        ssl
        crypto
    )

install(TARGETS cgimptests DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Test)
install(TARGETS cgfacetests DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Test)
install(TARGETS cgnfsecuritytests DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Test)

if (BUILD_NETFLIX_VAULT_GENERATOR)
   add_subdirectory(NetflixVaultGenerator)
endif (BUILD_NETFLIX_VAULT_GENERATOR)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../implementation/Icrypto-nexus/cmake")

find_package(NXCLIENT REQUIRED)

add_executable(cobalttests
        CobaltInterfaceTests.cpp
        Helpers.cpp
        Test.c
    )

set_target_properties(cobalttests PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
    )

target_compile_options(cobalttests PUBLIC -DVALIDATE_VALUES -DTEST_PLATFORM_APOLLOV1PLUS)
target_link_libraries(cobalttests
        PRIVATE
        ${NAMESPACE}Cryptography
        ssl
        crypto
        utility
        NXCLIENT::NXCLIENT
       -Wl,--no-undefined
    )

install(TARGETS cobalttests DESTINATION bin)


if(${CRYPTOGRAPHY_IMPLEMENTATION} STREQUAL "OpenSSL")
   add_definitions(-DOpenSSL)
elseif (${CRYPTOGRAPHY_IMPLEMENTATION} STREQUAL "SecApi")
   add_definitions(-DSecApi)
endif()
