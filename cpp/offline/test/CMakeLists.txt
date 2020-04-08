set(MAIN_TARGET ${PROJECT_NAME}_test)

add_executable(${MAIN_TARGET} "offline_test.cpp")

find_package(GTest CONFIG REQUIRED)
target_link_libraries(${MAIN_TARGET} PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)

include(GoogleTest)
gtest_discover_tests(${MAIN_TARGET})