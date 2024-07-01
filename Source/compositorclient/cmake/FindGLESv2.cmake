# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 Metrological
#
# Licensed under the Apache License, Version 2.0 (the License);
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an AS IS BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# - Try to Find GLESv2
# Once done, this will define
#
#  GLESV2_FOUND - system has EGL installed.
#  GLESV2_INCLUDE_DIRS - directories which contain the EGL headers.
#  GLESV2_LIBRARIES - libraries required to link against EGL.
#  GLESV2_DEFINITIONS - Compiler switches required for using EGL.
#

if(GLESv2_FIND_QUIETLY)
    set(_GLESV2_MODE QUIET)
elseif(GLESv2_FIND_REQUIRED)
    set(_GLESV2_MODE REQUIRED)
endif()

find_package(PkgConfig)
pkg_check_modules(glesv2 ${_GLESV2_MODE} IMPORTED_TARGET GLOBAL glesv2)

if (glesv2_FOUND)
   add_library(GLESv2::GLESv2 ALIAS PkgConfig::glesv2)
endif()
