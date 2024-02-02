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

set(TARGET implementation)

find_package(OpenSSL REQUIRED)

option(USE_PROVISIONING "Load Netflix data from a provisioning label" OFF)

# FIXME: As of OpenSSL 3.0 the low level low-level key exchange and object 
#        creation functions are deprecated. We should mirgrate to use 
#        the high-level EVP_PKEY API.  
#
#        More info: 
#        https://www.openssl.org/docs/man3.0/man7/migration_guide.html#Deprecated-low-level-object-creation
#        https://jira.rdkcentral.com/jira/projects/METROL/issues/METROL-506
#        
#        For now force deprecated declarations to be a warning.
add_compile_options( 
    -Wno-deprecated-declarations)

add_library(${TARGET} STATIC
    Vault.cpp
    Hash.cpp
    Cipher.cpp
    DiffieHellman.cpp
    Derive.cpp
)

target_link_libraries(${TARGET}
    PUBLIC
        ${NAMESPACE}Core::${NAMESPACE}Core
        OpenSSL::SSL
        OpenSSL::Crypto
)

if(USE_PROVISIONING)
    message(STATUS "Build with provisioning support")

    find_package(libprovision REQUIRED)

    target_compile_definitions(${TARGET} PRIVATE
        USE_PROVISIONING)

    target_link_libraries(${TARGET}
        PUBLIC
            libprovision::libprovision
    )
endif()

target_include_directories(${TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../..>)
