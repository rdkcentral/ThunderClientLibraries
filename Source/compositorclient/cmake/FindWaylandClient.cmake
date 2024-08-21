# - Try to find Wayland.
# Once done, this will define
#
#  WAYLANDCLIENT_FOUND - system has Wayland.
#  WAYLANDCLIENT_INCLUDE_DIRS - the Wayland include directories
#  WAYLANDCLIENT_LIBRARIES - link these to use Wayland.
#
#  WaylandClient::WaylandClient, the wayland client library
#
# Copyright (C) 2014 Igalia S.L.
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

if(WaylandClient_FIND_QUIETLY)
    set(_WAYLANDCLIENT_MODE QUIET)
elseif(WaylandClient_FIND_REQUIRED)
    set(_WAYLANDCLIENT_MODE REQUIRED)
endif()

find_package(PkgConfig)
pkg_check_modules(wayland-client ${_WAYLANDCLIENT_MODE} IMPORTED_TARGET GLOBAL wayland-client>=1.2)

if (wayland-client_FOUND)
   add_library(WaylandClient::WaylandClient ALIAS PkgConfig::wayland-client)
endif()