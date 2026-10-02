add_executable(DarkModeScheduleTest
  DarkModeScheduleTest.cpp
)

target_include_directories(DarkModeScheduleTest PRIVATE
  ${REPO_ROOT}/src/util
)

target_link_libraries(DarkModeScheduleTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

gtest_discover_tests(DarkModeScheduleTest)
