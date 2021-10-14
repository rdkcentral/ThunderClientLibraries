# - Try to find gstreamer_base.
# Once done, this will define
#
#  GSTREAMER_BASE_FOUND - the gstreamer_base is available
#  GSTREAMER_BASE::GSTREAMER_BASE - The gstreamer_base library and all its dependecies
#
# Copyright (C) 2019 Metrological B.V
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND ITS CONTRIBUTORS ``AS
# IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ITS
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

if(GSTREAMER_BASE_FIND_QUIETLY)
    set(_GSTREAMER_BASE_MODE QUIET)
elseif(GSTREAMER_BASE_FIND_REQUIRED)
    set(_GSTREAMER_BASE_MODE REQUIRED)
endif()

find_package(PkgConfig)
pkg_check_modules(PC_GSTREAMER_BASE ${_GSTREAMER_BASE_MODE} gstreamer-base-1.0)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GSTREAMER_BASE DEFAULT_MSG PC_GSTREAMER_BASE_FOUND PC_GSTREAMER_BASE_INCLUDE_DIRS PC_GSTREAMER_BASE_LIBRARIES)
mark_as_advanced(PC_GSTREAMER_BASE_INCLUDE_DIRS PC_GSTREAMER_BASE_LIBRARIES PC_GSTREAMER_BASE_LIBRARY_DIRS)

foreach(libraryName ${PC_GSTREAMER_BASE_LIBRARIES})
find_library(${libraryName}_GSTREAMER_BASE_SINGLE_LIB ${libraryName} HINTS ${PC_GSTREAMER_BASE_LIBRARY_DIRS})
list(APPEND ALL_GSTREAMER_BASE_LIBS "${${libraryName}_GSTREAMER_BASE_SINGLE_LIB}")
endforeach(libraryName)

if(GSTREAMER_BASE_FOUND)
    set(GSTREAMER_BASE_LIBRARIES ${ALL_GSTREAMER_BASE_LIBS})
    set(GSTREAMER_BASE_INCLUDES ${PC_GSTREAMER_BASE_INCLUDE_DIRS})

    if(NOT TARGET GSTREAMER_BASE::GSTREAMER_BASE)
        add_library(GSTREAMER_BASE::GSTREAMER_BASE UNKNOWN IMPORTED)

        set_target_properties(GSTREAMER_BASE::GSTREAMER_BASE
                PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${PC_GSTREAMER_BASE_LIBRARY_DIRS}"
                INTERFACE_COMPILE_DEFINITIONS "GSTREAMER"
                INTERFACE_COMPILE_OPTIONS "${PC_GSTREAMER_BASE_CFLAGS_OTHER}"
                INTERFACE_INCLUDE_DIRECTORIES "${PC_GSTREAMER_BASE_INCLUDE_DIRS}"
                INTERFACE_LINK_LIBRARIES "${ALL_GSTREAMER_BASE_LIBS}"
                )
    endif()
endif()
