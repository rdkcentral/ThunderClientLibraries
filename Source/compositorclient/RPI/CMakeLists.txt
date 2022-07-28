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

find_package(EGL REQUIRED)

if(VC6)
    find_package(GBM REQUIRED)
    find_package(LibDRM REQUIRED)

    if(NOT GBM_FOUND)
        message(FATAL_ERROR "The GBM library and its headers cannot be found.")
    endif()

    if(NOT LIBDRM_FOUND)
        message(FATAL_ERROR "The libDRM library and its headers cannot be found.")
    endif()

    target_sources(${TARGET}
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/ModeSet.cpp
            ${CMAKE_CURRENT_LIST_DIR}/Implementation.cpp)
    target_compile_definitions(${TARGET} PRIVATE VC6)
else()
    target_sources(${TARGET}
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/Implementation.cpp)
endif()

target_link_libraries(${TARGET}
    PRIVATE
        ${EGL_LIBRARIES}
)

if(VC6)
    target_link_libraries(${TARGET}
        PRIVATE
            LibDRM::LibDRM
    )
endif()

target_include_directories(${TARGET}
    PRIVATE
        ${EGL_INCLUDE_DIRS})

target_compile_definitions(${TARGET}
    PRIVATE
        ${EGL_DEFINITIONS})
