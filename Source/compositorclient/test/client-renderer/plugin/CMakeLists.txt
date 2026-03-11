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
# limitations under the License

find_package(Thunder REQUIRED)
find_package(${NAMESPACE}Core CONFIG REQUIRED)
find_package(${NAMESPACE}Messaging CONFIG REQUIRED)
find_package(${NAMESPACE}Plugins CONFIG REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

set(PLUGIN_COMPOSITORCLIENT_PLUGIN_STARTMODE "Activated" CACHE STRING "Automatically start ClientCompositorRender plugin")
set(PLUGIN_COMPOSITORCLIENT_PLUGIN_RESUMED "true" CACHE STRING "Set ClientCompositorRender plugin resume state")
set(PLUGIN_COMPOSITORCLIENT_PLUGIN_MODE "Local" CACHE STRING "Controls if the plugin should run in its own process, in process or remote.")
set(PLUGIN_COMPOSITORCLIENT_PLUGIN_INSTANCES 1 CACHE STRING "Controls how many instances will be installed")

add_library(ClientCompositorRender
    Module.cpp
    ClientCompositorRender.cpp
    ClientCompositorRenderImplementation.cpp
)

set_target_properties(ClientCompositorRender PROPERTIES
    CXX_STANDARD ${CXX_STD}
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(ClientCompositorRender
    PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        CompileSettingsDebug::CompileSettingsDebug
        ClientCompositorRenderCommon
)


if(INSTALL_CLIENT_COMPOSITOR_RENDER_TEST_PLUGIN)
install(TARGETS ClientCompositorRender
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/${STORAGE_DIRECTORY}/plugins COMPONENT ${NAMESPACE}_Runtime)

    # Cap instances at 16 and warn if exceeded
    if(PLUGIN_COMPOSITORCLIENT_PLUGIN_INSTANCES GREATER 16)
        message(WARNING "PLUGIN_COMPOSITORCLIENT_PLUGIN_INSTANCES is ${PLUGIN_COMPOSITORCLIENT_PLUGIN_INSTANCES}, capping at 16")
        set(PLUGIN_COMPOSITORCLIENT_PLUGIN_INSTANCES 16)
    endif()

    # Determine grid layout based on instance count
    if(PLUGIN_COMPOSITORCLIENT_PLUGIN_INSTANCES EQUAL 1)
        set(GRID_COLS 1)
        set(GRID_ROWS 1)
        set(CELL_WIDTH 100)
        set(CELL_HEIGHT 100)
    elseif(PLUGIN_COMPOSITORCLIENT_PLUGIN_INSTANCES LESS_EQUAL 4)
        set(GRID_COLS 2)
        set(GRID_ROWS 2)
        set(CELL_WIDTH 50)
        set(CELL_HEIGHT 50)
    else()
        set(GRID_COLS 4)
        set(GRID_ROWS 4)
        set(CELL_WIDTH 25)
        set(CELL_HEIGHT 25)
    endif()

    foreach(Index RANGE 1 ${PLUGIN_COMPOSITORCLIENT_PLUGIN_INSTANCES})
        # Calculate grid position (0-based index for math)
        math(EXPR ZeroIndex "${Index} - 1")
        math(EXPR Col "${ZeroIndex} % ${GRID_COLS}")
        math(EXPR Row "${ZeroIndex} / ${GRID_COLS}")
        
        # Calculate percentage positions
        math(EXPR PLUGIN_COMPOSITORCLIENT_PLUGIN_INSTANCE_X "${Col} * ${CELL_WIDTH}")
        math(EXPR PLUGIN_COMPOSITORCLIENT_PLUGIN_INSTANCE_Y "${Row} * ${CELL_HEIGHT}")
        set(PLUGIN_COMPOSITORCLIENT_PLUGIN_INSTANCE_WIDTH ${CELL_WIDTH})
        set(PLUGIN_COMPOSITORCLIENT_PLUGIN_INSTANCE_HEIGHT ${CELL_HEIGHT})
        
        write_config(
            PLUGINS ClientCompositorRender
            CLASSNAME ClientCompositorRender
            LOCATOR libClientCompositorRender${CMAKE_SHARED_LIBRARY_SUFFIX}
            INSTALL_NAME ClientCompositorRender${Index}.json
        )
    endforeach()
endif()