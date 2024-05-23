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

#
# virtual input monitor
#

add_executable(vk_monitor vk_monitor.c)

target_link_libraries(vk_monitor
        PRIVATE
            ${NAMESPACE}VirtualInput::${NAMESPACE}VirtualInput)


install(
        TARGETS vk_monitor EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT libs      # static lib
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT libs      # shared lib
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT libs      # binaries
        FRAMEWORK DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/${MODULE} COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${MODULE}      # headers
)
