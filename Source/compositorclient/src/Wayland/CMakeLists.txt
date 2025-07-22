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

if(NOT PLUGIN_COMPOSITOR_SUB_IMPLEMENTATION)
    set(PLUGIN_COMPOSITOR_SUB_IMPLEMENTATION "Westeros")
endif()

if ((NOT "${PLUGIN_COMPOSITOR_SUB_IMPLEMENTATION}" STREQUAL "Weston") AND
    (NOT "${PLUGIN_COMPOSITOR_SUB_IMPLEMENTATION}" STREQUAL "Westeros"))
message( FATAL_ERROR "Invalid Wayland implementation, set PLUGIN_COMPOSITOR_SUB_IMPLEMENTATION (currently: [${PLUGIN_COMPOSITOR_SUB_IMPLEMENTATION}]) to \"Weston\" or \"Westeros\"" )
endif ()

find_package(WaylandClient REQUIRED)
find_package(EGL REQUIRED)
find_package(WaylandEGL REQUIRED)
find_package(${NAMESPACE}Messaging REQUIRED)

find_package(NXCLIENT)

add_library(${PLUGIN_COMPOSITOR_IMPLEMENTATION} OBJECT ${PLUGIN_COMPOSITOR_SUB_IMPLEMENTATION}.cpp)

target_link_libraries(${PLUGIN_COMPOSITOR_IMPLEMENTATION}
    PRIVATE
        common::include
        CompileSettingsDebug::CompileSettingsDebug
    PUBLIC
        WaylandEGL::WaylandEGL
        ${EGL_LIBRARIES}
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging
        WaylandClient::WaylandClient)

set_target_properties(${PLUGIN_COMPOSITOR_IMPLEMENTATION} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
)

if ("${PLUGIN_COMPOSITOR_SUB_IMPLEMENTATION}" STREQUAL "Westeros")
    find_package(${PLUGIN_COMPOSITOR_SUB_IMPLEMENTATION}Client REQUIRED)

    target_link_libraries(${PLUGIN_COMPOSITOR_IMPLEMENTATION}
        PUBLIC
            ${PLUGIN_COMPOSITOR_SUB_IMPLEMENTATION}Client::${PLUGIN_COMPOSITOR_SUB_IMPLEMENTATION}Client)
endif()

if ("${PLUGIN_COMPOSITOR_SUB_IMPLEMENTATION}" STREQUAL "Weston")
    target_sources(${PLUGIN_COMPOSITOR_IMPLEMENTATION} PRIVATE xdg-shell-client-protocol.c)

    find_package(GLESv2 REQUIRED)
    if (GLESV2_FOUND)
        target_link_libraries(${PLUGIN_COMPOSITOR_IMPLEMENTATION}
            PRIVATE
                GLESv2::GLESv2
        )
    endif (GLESV2_FOUND)
endif ()

if (NXCLIENT_FOUND)
    target_link_libraries(${PLUGIN_COMPOSITOR_IMPLEMENTATION}
        PRIVATE
            NXCLIENT::NXCLIENT)
endif ()

find_package(BCM_HOST QUIET)
if(BCM_HOST_FOUND)
    target_link_libraries(${PLUGIN_COMPOSITOR_IMPLEMENTATION}
        PRIVATE
            BCM_HOST::BCM_HOST)
    target_compile_definitions(${PLUGIN_COMPOSITOR_IMPLEMENTATION}
        PRIVATE
            BCM_HOST=ON)
endif(BCM_HOST_FOUND)

if(EGL_FOUND)
    add_definitions(${EGL_CFLAGS})
endif(EGL_FOUND)

if (PLUGIN_COMPOSITOR_NXSERVER)
    if (V3D_DRM_DISABLE)
        target_compile_definitions(${PLUGIN_COMPOSITOR_IMPLEMENTATION}
            PRIVATE
                V3D_DRM_DISABLE=1)
    endif ()
endif ()

target_include_directories(${PLUGIN_COMPOSITOR_IMPLEMENTATION}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}>
    PRIVATE
        ${EGL_INCLUDE_DIRS}
)

target_compile_definitions(${PLUGIN_COMPOSITOR_IMPLEMENTATION}
    PRIVATE
        ${EGL_DEFINITIONS})

set_target_properties(${PLUGIN_COMPOSITOR_IMPLEMENTATION} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        DEFINE_SYMBOL THUNDER_WAYLAND_EGL_CLIENT
)

install(
	FILES Implementation.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/compositorclient
)
