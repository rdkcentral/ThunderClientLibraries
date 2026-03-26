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

find_package(PNG REQUIRED)
find_package(EGL REQUIRED)
find_package(GLESv2 REQUIRED)
find_package(Thunder REQUIRED)
find_package(${NAMESPACE}Core CONFIG REQUIRED)
find_package(${NAMESPACE}Messaging CONFIG REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_subdirectory(Fonts)

add_library(ClientCompositorRenderCommon STATIC
    Module.cpp
    TextureBounce.cpp
    TextureLoader.cpp
    Renderer.cpp
    TextRender.cpp
)

set_target_properties(ClientCompositorRenderCommon PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(ClientCompositorRenderCommon
    PUBLIC
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging
        ClientCompositor::ClientCompositor
        ClientVirtualInput::ClientVirtualInput # this hack is needed because the test app cannot look up the library in the staging dir yet... 
        CompileSettingsDebug::CompileSettingsDebug
        EGL::EGL
        GLESv2::GLESv2
        PNG::PNG
        ArialFont
)

target_include_directories(ClientCompositorRenderCommon PUBLIC
          $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>)


set_target_properties(ClientCompositorRenderCommon PROPERTIES
    CXX_STANDARD ${CXX_STD}
    CXX_STANDARD_REQUIRED YES
)

target_compile_definitions(ClientCompositorRenderCommon PUBLIC 
    NAMESPACE=${NAMESPACE}    
)

if(INSTALL_CLIENT_COMPOSITOR_RENDER_TEST_APP OR INSTALL_CLIENT_COMPOSITOR_RENDER_TEST_PLUGIN)
install(FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/ml-tv-color-small.png 
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${NAMESPACE}/ClientCompositorRender 
    COMPONENT ${NAMESPACE}_Test)
endif()