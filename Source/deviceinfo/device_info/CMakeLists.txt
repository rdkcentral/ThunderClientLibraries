add_executable(device_info main.c)

target_include_directories(device_info 
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../include>
        $<INSTALL_INTERFACE:include/deviceinfo.h> 
)

target_link_libraries(device_info
    PRIVATE
        deviceinfo)

install(TARGETS device_info 
    DESTINATION bin)
