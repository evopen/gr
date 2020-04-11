set(TEST_TARGET ${PROJECT_NAME}_test)
set(BENCH_TARGET ${PROJECT_NAME}_bench)

add_executable(${TEST_TARGET} "offline_test.cpp" "library_test.cpp")


include_directories(${SOURCE_DIR})

find_package(GTest CONFIG REQUIRED)
target_link_libraries(${TEST_TARGET} PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)

find_package(glm CONFIG REQUIRED)
target_link_libraries(${TEST_TARGET} PRIVATE glm)

find_package(OpenCV CONFIG REQUIRED)
target_link_libraries(${TEST_TARGET} PRIVATE ${OpenCV_LIBS})

add_executable(${BENCH_TARGET} "offline_bench.cpp")

find_package(benchmark CONFIG REQUIRED)
target_link_libraries(${BENCH_TARGET} PRIVATE benchmark::benchmark benchmark::benchmark_main)

include(GoogleTest)
gtest_discover_tests(${TEST_TARGET})