set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(player_info main.c)

target_link_libraries(player_info PRIVATE Threads::Threads)


target_include_directories(player_info 
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include/playerinfo.h> 
)

target_link_libraries(player_info
    PRIVATE
        playerinfo)

install(TARGETS player_info 
    DESTINATION bin)