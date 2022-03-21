find_package(WPEFramework)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(${NAMESPACE}Core REQUIRED)
find_package(CompileSettingsDebug REQUIRED)

add_executable(local_trace_test
    main.cpp
)

target_link_libraries(local_trace_test PRIVATE
    CompileSettingsDebug::CompileSettingsDebug
    ${NAMESPACE}Definitions::${NAMESPACE}Definitions
    ${NAMESPACE}Core::${NAMESPACE}Core
    ${NAMESPACE}LocalTracer::${NAMESPACE}LocalTracer)

install(TARGETS local_trace_test DESTINATION bin)
