
# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2025 Metrological
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

option(BUILD_CLIENT_COMPOSITOR_GBM_UTIL "Build the GBM basic test" ON)
option(BUILD_CLIENT_COMPOSITOR_RENDER_TEST "Build the renderer compositor client test" ON)
option(BUILD_COMPOSITORCLIENT_TEST "Build Compositor Client legacy test" OFF)


if(BUILD_CLIENT_COMPOSITOR_GBM_UTIL)
add_subdirectory(gbm_buffer_test)
endif()

if(BUILD_CLIENT_COMPOSITOR_RENDER_TEST)
add_subdirectory(client-renderer)
endif()

if(BUILD_COMPOSITORCLIENT_TEST)
add_subdirectory(legacy-test)
endif()