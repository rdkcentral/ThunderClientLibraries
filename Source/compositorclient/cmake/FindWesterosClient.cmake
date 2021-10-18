# - Try to find Westeros Client library.
# Once done this will define
#  WESTEROS_CLIENT_FOUND - System has westeros client
#  WESTEROS_CLIENT_INCLUDE_DIRS - The westeros client include directories
#  WESTEROS_CLIENT_LIBRARIES    - The libraries needed to use westeros client
#
#  WesterosClient::WesterosClient, the westeros compositor
#
# Copyright (C) 2015 Metrological.
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

if(WesterosClient_FIND_QUIETLY)
    set(_WESTEROS_CLIENT_MODE QUIET)
elseif(WesterosClient_FIND_REQUIRED)
    set(_WESTEROS_CLIENT_MODE REQUIRED)
endif()

find_package(PkgConfig)
pkg_check_modules(PC_WESTEROS ${_WESTEROS_CLIENT_MODE} westeros-compositor)

find_library(WESTEROS_CLIENT_LIB NAMES westeros_compositor
        HINTS ${PC_WESTEROS_LIBDIR} ${PC_WESTEROS_LIBRARY_DIRS}
)
find_library(WESTEROS_CLIENT_LIB_BUFFER_CLIENT NAMES westeros_simplebuffer_client
        HINTS ${PC_WESTEROS_LIBDIR} ${PC_WESTEROS_LIBRARY_DIRS}
        )
find_library(WESTEROS_CLIENT_LIB_SHELL_CLIENT NAMES westeros_simpleshell_client
        HINTS ${PC_WESTEROS_LIBDIR} ${PC_WESTEROS_LIBRARY_DIRS}
        )
find_path(WESTEROS_CLIENT_INCLUDE_DIRS NAMES westeros-compositor.h
        HINTS ${PC_WESTEROS_INCLUDEDIR} ${PC_WESTEROS_INCLUDE_DIRS}
        )

set(WESTEROS_CLIENT_LIBRARIES ${PC_WESTEROS_LIBRARIES} "${WESTEROS_CLIENT_LIB_SERVER}" "${WESTEROS_CLIENT_LIB_SERVER}" "${WESTEROS_CLIENT_LIB_CLIENT}" "${WESTEROS_CLIENT_LIB_CLIENT}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WesterosClient DEFAULT_MSG PC_WESTEROS_FOUND WESTEROS_CLIENT_INCLUDE_DIRS WESTEROS_CLIENT_LIBRARIES)
mark_as_advanced(WESTEROS_CLIENT_INCLUDE_DIRS WESTEROS_CLIENT_LIBRARIES)
set(WESTEROS_CLIENT_FOUND ${WesterosClient_FOUND})

if(WesterosClient_FOUND AND NOT TARGET WesterosClient::WesterosClient)
    set(WESTEROS_CLIENT_LIB_CLIENT_LINK_LIBRARIES "${WESTEROS_CLIENT_LIB}" "${WESTEROS_CLIENT_LIB_SHELL_CLIENT}" "${WESTEROS_CLIENT_LIB_BUFFER_CLIENT}")
    add_library(WesterosClient::WesterosClient UNKNOWN IMPORTED)
    set_target_properties(WesterosClient::WesterosClient PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${WESTEROS_CLIENT_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${WESTEROS_CLIENT_INCLUDE_DIRS}"
            INTERFACE_COMPILE_OPTIONS "${WESTEROS_CLIENT_CFLAGS_OTHER}"
            INTERFACE_LINK_LIBRARIES "${WESTEROS_CLIENT_LIB_CLIENT_LINK_LIBRARIES}"
            )
endif()
