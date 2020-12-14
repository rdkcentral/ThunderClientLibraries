find_package(${NAMESPACE}Protocols REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

include_directories(include)
add_executable(reconnection main.cpp)

set_target_properties(reconnection PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        )     

target_link_libraries(reconnection 
        PRIVATE
        ${NAMESPACE}Protocols::${NAMESPACE}Protocols
        CompileSettingsDebug::CompileSettingsDebug
    )

target_link_libraries(reconnection
    PUBLIC
      ${NAMESPACE}Core::${NAMESPACE}Core
      CompileSettingsDebug::CompileSettingsDebug
    )

if(PROTOCOLS)
target_link_libraries(reconnection
    PUBLIC
      ${NAMESPACE}Protocols::${NAMESPACE}Protocols
    )
else()
target_link_libraries(reconnection
    PUBLIC
      ${NAMESPACE}COM::${NAMESPACE}COM
    )
endif()

install(TARGETS reconnection DESTINATION bin)