option(BUILD_CRYPTOGRAPHY_TESTS "Build cryptography tests" OFF)

if (BUILD_CRYPTOGRAPHY_TESTS)
    add_subdirectory(cryptography)
endif()
