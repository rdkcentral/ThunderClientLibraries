project(deviceidentification)
cmake_minimum_required(VERSION 3.3)

find_package(WPEFramework)

set(TARGET ${NAMESPACE}${PROJECT_NAME})

message("Setup ${TARGET} v${PROJECT_VERSION}")

find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(${NAMESPACE}Core REQUIRED)
if(PROTOCOLS)
    find_package(${NAMESPACE}Protocols REQUIRED)
else()
    find_package(${NAMESPACE}COM REQUIRED)
endif()

set(PUBLIC_HEADERS "include/deviceidentification.h")

add_library(${TARGET}  SHARED 
    DeviceIdentification.cpp
)

target_link_libraries(${TARGET}
        PUBLIC
          ${NAMESPACE}Core::${NAMESPACE}Core
          CompileSettingsDebug::CompileSettingsDebug
        )

if(PROTOCOLS)
target_link_libraries(${TARGET}
        PUBLIC
          ${NAMESPACE}Protocols::${NAMESPACE}Protocols
        )
else()
target_link_libraries(${TARGET}
        PUBLIC
          ${NAMESPACE}COM::${NAMESPACE}COM
        )
endif()

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        VERSION ${PROJECT_VERSION}
        VERSION ${PROJECT_VERSION_MAJOR}
        )

target_include_directories( ${TARGET}
        PUBLIC
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
          $<INSTALL_INTERFACE:include>
        )

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/deviceidentification COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}/deviceidentification # headers
)

InstallCMakeConfig(
        TARGETS ${TARGET})

InstallPackageConfig(
        TARGETS ${TARGET} 
        DESCRIPTION "communications channel abstraction to get the device properties")
        add_subdirectory(device_identification)
