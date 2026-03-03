# tests/CMakeLists.txt
# 测试配置

find_package(GTest CONFIG REQUIRED)

# 测试可执行文件
add_executable(mr_tests
    common/test_math.cpp
    common/test_network.cpp
)

target_link_libraries(mr_tests
    PRIVATE
        mr_common
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(mr_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
)

mr_set_compiler_warnings(mr_tests)

# 发现测试
include(GoogleTest)
gtest_discover_tests(mr_tests)

# 输出测试可执行文件位置
set_target_properties(mr_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
