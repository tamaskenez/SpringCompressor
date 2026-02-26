file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.h *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

add_executable(testing ${sources})
target_compile_definitions(testing
    PRIVATE
        TESTING_OUTPUT_DIR=\"${TESTING_OUTPUT_DIR}\"
)

target_link_libraries(testing
    PRIVATE
        SpringCompressorEngine
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(testing)