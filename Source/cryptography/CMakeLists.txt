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

cmake_minimum_required(VERSION 3.15)

find_package(Thunder)

project(Cryptography)

project_version(1.0.0)

set(TARGET Client${PROJECT_NAME})

message("Setup ${TARGET} v${PROJECT_VERSION}")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# remove the -as-needed flag because it can lead to losing dependencies when we're mixing static and shared libs
string(REPLACE "-Wl,--as-needed" "" CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")

if(NOT APPLE)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-as-needed")
endif()

option(INCLUDE_SOFTWARE_CRYPTOGRAPHY_LIBRARY "Include explicitly a software based cryptography library" OFF)

find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}COM REQUIRED)

add_subdirectory(implementation)

add_library(${TARGET} 
    Module.cpp
    Cryptography.cpp
    NetflixSecurity.cpp
)

target_link_libraries(${TARGET}
    PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}COM::${NAMESPACE}COM
        CompileSettingsDebug::CompileSettingsDebug
)

if(NOT APPLE)
    target_link_libraries(${TARGET}
        PRIVATE
            -Wl,--whole-archive implementation -Wl,--no-whole-archive
    )
else()
    target_link_libraries(${TARGET}
        PRIVATE
            implementation
    )
endif()

target_include_directories(${TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/implementation>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/cryptography>
)

set(PUBLIC_HEADERS
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/Module.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/cryptography.h>
)

set_target_properties(${TARGET} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    FRAMEWORK FALSE
    PUBLIC_HEADER "${PUBLIC_HEADERS}"
    LINK_WHAT_YOU_USE TRUE
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

install(
    TARGETS ${TARGET}  EXPORT ${TARGET}Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Development
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Runtime NAMELINK_COMPONENT ${NAMESPACE}_Development
    FRAMEWORK DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Runtime 
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/cryptography COMPONENT ${NAMESPACE}_Development
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/cryptography
)

InstallPackageConfig(
    TARGETS ${TARGET}
    DESCRIPTION "A crypto wrapper library."
)

InstallCMakeConfig(
    TARGETS ${TARGET}
)

if(INCLUDE_SOFTWARE_CRYPTOGRAPHY_LIBRARY)
    find_package(OpenSSL REQUIRED)

    add_library(${TARGET}Software 
        Module.cpp
        Cryptography.cpp
        NetflixSecurity.cpp
        implementation/OpenSSL/Vault.cpp
        implementation/OpenSSL/Hash.cpp
        implementation/OpenSSL/Cipher.cpp
        implementation/OpenSSL/DiffieHellman.cpp
        implementation/OpenSSL/Derive.cpp
    )

    target_link_libraries(${TARGET}Software 
        PRIVATE
            ${NAMESPACE}Core::${NAMESPACE}Core
            ${NAMESPACE}COM::${NAMESPACE}COM
            OpenSSL::SSL
            OpenSSL::Crypto
    )

    target_include_directories(${TARGET}Software 
        PRIVATE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/implementation>
        PUBLIC
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/cryptography>
    )

    set_target_properties(${TARGET}Software PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        LINK_WHAT_YOU_USE TRUE
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )

    install(
        TARGETS ${TARGET}Software EXPORT ${TARGET}SoftwareTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Development
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Runtime NAMELINK_COMPONENT ${NAMESPACE}_Development
        FRAMEWORK DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT libs
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/cryptography
    )

    InstallPackageConfig(
        TARGETS ${TARGET}Software
        DESCRIPTION "A software crypto wrapper library."
    )

    InstallCMakeConfig(
        TARGETS ${TARGET}Software 
    )
endif(INCLUDE_SOFTWARE_CRYPTOGRAPHY_LIBRARY)
add_subdirectory(tests)
