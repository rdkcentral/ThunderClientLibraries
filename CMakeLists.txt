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

cmake_minimum_required(VERSION 3.3)

project(WPEFramework)

set(VERSION_MAJOR 2)
set(VERSION_MINOR 1)
set(VERSION_PATCH 11)

set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
set(NAMESPACE ${PROJECT_NAME} CACHE STRING "Namespace of the project")

message(STATUS "Setting up ${PROJECT_NAME} v${VERSION}")

if (BUILD_REFERENCE)
    add_definitions (-DBUILD_REFERENCE=${BUILD_REFERENCE})
endif()

add_subdirectory(Source)
