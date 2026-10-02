#include <gtest/gtest.h>

#include "DarkModeSchedule.h"

namespace {
constexpr uint16_t timeOfDay(const uint8_t hour, const uint8_t minute = 0) {
  return static_cast<uint16_t>(hour * 60 + minute);
}
}  // namespace

TEST(DarkModeSchedule, DisabledOrIncompleteWindowIsInactive) {
  EXPECT_FALSE(DarkModeSchedule::hasCompleteWindow(false, timeOfDay(21), timeOfDay(7)));
  EXPECT_FALSE(DarkModeSchedule::hasCompleteWindow(true, DarkModeSchedule::kUnsetTimeOfDay, timeOfDay(7)));
  EXPECT_FALSE(DarkModeSchedule::hasCompleteWindow(true, timeOfDay(21), DarkModeSchedule::kUnsetTimeOfDay));
}

TEST(DarkModeSchedule, SameEndpointIsAnEmptyWindow) {
  EXPECT_FALSE(DarkModeSchedule::hasCompleteWindow(true, timeOfDay(21), timeOfDay(21)));
  EXPECT_FALSE(DarkModeSchedule::containsTimeOfDay(timeOfDay(21), timeOfDay(21), timeOfDay(21)));
}

TEST(DarkModeSchedule, OvernightWindowWrapsMidnight) {
  // 9:00 PM (21:00) to 7:00 AM (07:00)
  EXPECT_TRUE(DarkModeSchedule::containsTimeOfDay(timeOfDay(21), timeOfDay(7), timeOfDay(21)));
  EXPECT_TRUE(DarkModeSchedule::containsTimeOfDay(timeOfDay(21), timeOfDay(7), timeOfDay(23, 30)));
  EXPECT_TRUE(DarkModeSchedule::containsTimeOfDay(timeOfDay(21), timeOfDay(7), timeOfDay(0)));
  EXPECT_TRUE(DarkModeSchedule::containsTimeOfDay(timeOfDay(21), timeOfDay(7), timeOfDay(6, 59)));
  EXPECT_FALSE(DarkModeSchedule::containsTimeOfDay(timeOfDay(21), timeOfDay(7), timeOfDay(7)));
  EXPECT_FALSE(DarkModeSchedule::containsTimeOfDay(timeOfDay(21), timeOfDay(7), timeOfDay(12)));
  EXPECT_FALSE(DarkModeSchedule::containsTimeOfDay(timeOfDay(21), timeOfDay(7), timeOfDay(20, 59)));
}

TEST(DarkModeSchedule, DaytimeWindowWithinSameDay) {
  // 8:00 AM (08:00) to 6:00 PM (18:00)
  EXPECT_TRUE(DarkModeSchedule::containsTimeOfDay(timeOfDay(8), timeOfDay(18), timeOfDay(8)));
  EXPECT_TRUE(DarkModeSchedule::containsTimeOfDay(timeOfDay(8), timeOfDay(18), timeOfDay(12)));
  EXPECT_TRUE(DarkModeSchedule::containsTimeOfDay(timeOfDay(8), timeOfDay(18), timeOfDay(17, 59)));
  EXPECT_FALSE(DarkModeSchedule::containsTimeOfDay(timeOfDay(8), timeOfDay(18), timeOfDay(18)));
  EXPECT_FALSE(DarkModeSchedule::containsTimeOfDay(timeOfDay(8), timeOfDay(18), timeOfDay(7, 59)));
  EXPECT_FALSE(DarkModeSchedule::containsTimeOfDay(timeOfDay(8), timeOfDay(18), timeOfDay(23)));
}

TEST(DarkModeSchedule, LocalTimeOfDayAppliesQuarterHourOffset) {
  EXPECT_EQ(DarkModeSchedule::localTimeOfDay(23, 45, 52), timeOfDay(0, 45));  // UTC+1
  EXPECT_EQ(DarkModeSchedule::localTimeOfDay(0, 15, 44), timeOfDay(23, 15));  // UTC-1
  EXPECT_EQ(DarkModeSchedule::localTimeOfDay(10, 0, 71), timeOfDay(15, 45));  // Nepal UTC+5:45 (48 + 23 = 71)
}
