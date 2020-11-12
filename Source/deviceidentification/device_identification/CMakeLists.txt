add_executable(device_identification main.c)

target_include_directories(device_identification 
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include/deviceidentification.h> 
)

target_link_libraries(device_identification
    PRIVATE
        deviceidentification)

install(TARGETS device_identification 
    DESTINATION bin)