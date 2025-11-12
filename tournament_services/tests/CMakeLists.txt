project(tournament_tests)

set(TEST_SOURCES
        ../../tournament_consumer/tests/MatchDelegateTest.cpp
)

set(SOURCES ${TEST_SOURCES})
include_directories(../include)

find_package(GTest CONFIG REQUIRED)



add_executable(${PROJECT_NAME}_runner
    ${TEST_SOURCES}
)

target_link_libraries(${PROJECT_NAME}_runner PRIVATE
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        GTest::gmock_main
        tournament_common)

add_test(AllTestsInMain ${PROJECT_NAME}_runner)