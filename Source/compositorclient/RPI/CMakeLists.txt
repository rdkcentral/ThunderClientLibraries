# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 RDK Management
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

set(TARGET compositorclient)

find_package(EGL REQUIRED)

find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}COM REQUIRED)

set(PUBLIC_HEADERS
        "include/traits.h"
        )

if(VC6)
    find_package(GBM REQUIRED)
    find_package(LibDRM REQUIRED)
    find_package(EGL REQUIRED)
    find_package(GLESv2 REQUIRED)

    if(NOT GBM_FOUND)
        message(FATAL_ERROR "The GBM library and its headers cannot be found.")
    endif()

    if(NOT LIBDRM_FOUND)
        message(FATAL_ERROR "The libDRM library and its headers cannot be found.")
    endif()

    if(NOT EGL_FOUND)
        message(FATAL_ERROR "The EGL library and its headers cannot be found.")
    endif()

    if(NOT GLESV2_FOUND)
        message(FATAL_ERROR "The GLESv2 library and its headers cannot be found.")
    endif()

    add_library(${TARGET} SHARED Headless.cpp Implementation.cpp)
    target_compile_definitions(${TARGET} PRIVATE VC6)
else()
    add_library(${TARGET} SHARED Implementation.cpp)
endif()

target_link_libraries(${TARGET}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}VirtualInput::${NAMESPACE}VirtualInput
        ${NAMESPACE}COM::${NAMESPACE}COM
        ${EGL_LIBRARIES}
)

if(VC6)
    target_link_libraries(${TARGET}
        PUBLIC
          CompileSettingsDebug::CompileSettingsDebug
        PRIVATE
            libdrm::libdrm
            libgbm::libgbm
            EGL::EGL
            GLESV2::GLESV2
    )
endif()

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}"
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_include_directories(${TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${EGL_INCLUDE_DIRS})

target_compile_definitions(${TARGET}
    PRIVATE
        ${EGL_DEFINITIONS})

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/compositorclient COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}/compositorclient # headers
)

InstallPackageConfig(
        TARGETS ${TARGET}
        DESCRIPTION "A compositor implementation wrapper for RPI and key handling.")

InstallCMakeConfig(TARGETS ${TARGET})
