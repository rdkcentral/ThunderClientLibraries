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
option(BUILD_CRYPTOGRAPHY_TESTS "Build cryptography test" OFF)
option(BUILD_CRYPTOGRAPHY_RPC_TESTS "Build cryptography rpc test" OFF)

if (BUILD_CRYPTOGRAPHY_TESTS)
    add_subdirectory(cryptography_test)
endif()

if (BUILD_CRYPTOGRAPHY_RPC_TESTS)
    add_subdirectory(rpc_cryptography_test)
endif()
