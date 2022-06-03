project(LocalTracer LANGUAGES CXX)
cmake_minimum_required(VERSION 3.7)

find_package(WPEFramework)

project_version(1.0.0)

set(MODULE_NAME ${NAMESPACE}LocalTracer)

message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

option(EXAMPLE_LOCALTRACER "Include an example app." OFF)

find_package(${NAMESPACE}Definitions REQUIRED)
find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}Messaging REQUIRED)

add_library(${MODULE_NAME} INTERFACE)

target_link_libraries(${MODULE_NAME} INTERFACE
    ${NAMESPACE}Definitions::${NAMESPACE}Definitions
    ${NAMESPACE}Core::${NAMESPACE}Core
    ${NAMESPACE}Messaging::${NAMESPACE}Messaging)

target_include_directories(${MODULE_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    $<INSTALL_INTERFACE:include/${NAMESPACE}>)

target_compile_features(${MODULE_NAME} INTERFACE cxx_std_11)

install(TARGETS ${MODULE_NAME} EXPORT ${MODULE_NAME}Targets)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/localtracer/localtracer.h
        DESTINATION include/${NAMESPACE}/localtracer)

# fixme: current cmake functions don't like installation of interface libraries.
include(cmake/HeaderOnly.cmake)

HeaderOnlyInstallPackageConfig(TARGET ${MODULE_NAME}
    NAME ${MODULE_NAME}
    VERSION ${PROJECT_VERSION}
    DESCRIPTION "Header only library to locally print traces coming from Messaging without the need of running Thunder/WPEFramework.")

HeaderOnlyInstallCMakeConfig(TARGET ${MODULE_NAME} TREAT_AS_NORMAL)

if(EXAMPLE_LOCALTRACER)
    add_subdirectory(example)
endif()