# If not stated otherwise in this file or this component's license file the
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

cmake_minimum_required(VERSION 3.7)

# Be compatible even if a newer CMake version is available
cmake_policy(VERSION 3.7...3.12)

if(libdrm_FIND_QUIETLY)
    set(_libdrm_MODE QUIET)
elseif(libdrm_FIND_REQUIRED)
    set(_libdrm_MODE REQUIRED)
endif()

find_package(PkgConfig)
if(${PKG_CONFIG_FOUND})

    # Just check if the libdrm.pc exist, and create the PkgConfig::libdrm target
    # No version requirement (yet)
    pkg_check_modules(libdrm ${_libdrm_MODE} IMPORTED_TARGET libdrm)

    find_library(libdrm_ACTUAL_LIBRARY NAMES drm
        HINTS ${libdrm_LIBRARY_DIRS} )

    include(FindPackageHandleStandardArgs)

    # Sets the FOUND variable to TRUE if all required variables are present and set
    find_package_handle_standard_args(
        libdrm
        REQUIRED_VARS
            libdrm_INCLUDE_DIRS
            libdrm_CFLAGS
            libdrm_LDFLAGS
            libdrm_LIBRARIES
            libdrm_ACTUAL_LIBRARY
        VERSION_VAR
            libdrm_VERSION
    )
    mark_as_advanced(libdrm_INCLUDE_DIRS libdrm_LIBRARIES)
    set(libdrm_FOUND ${libdrm_FOUND})

    if(libdrm_FOUND AND NOT TARGET libdrm::libdrm)
        add_library(libdrm::libdrm UNKNOWN IMPORTED)
        set_target_properties(libdrm::libdrm PROPERTIES
            IMPORTED_LOCATION "${libdrm_ACTUAL_LIBRARY}"
            INTERFACE_LINK_LIBRARIES "${libdrm_LIBRARIES}"
            INTERFACE_COMPILE_OPTIONS "${libdrm_CFLAGS}"
            INTERFACE_INCLUDE_DIRECTORIES "${libdrm_INCLUDE_DIRS}"
            )
    else()
        message(STATUS "Some required variable(s) is (are) not found / set! Does libdrm.pc exist?")
    endif()
else()
    message(STATUS "Unable to locate PkgConfig")
endif()
