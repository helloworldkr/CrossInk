#pragma once

#include <cstdint>

#include "util/FrontlightSchedule.h"

// Daily dark mode schedule utilities. Reuses the time-of-day math and boundary
// rules from FrontlightSchedule while isolating dark-mode policy.
namespace DarkModeSchedule {

constexpr uint16_t kMinutesPerDay = FrontlightSchedule::kMinutesPerDay;
constexpr uint16_t kUnsetTimeOfDay = FrontlightSchedule::kUnsetTimeOfDay;

constexpr bool isTimeOfDayValid(const uint16_t timeOfDay) {
  return FrontlightSchedule::isTimeOfDayValid(timeOfDay);
}

constexpr bool hasCompleteWindow(const bool enabled, const uint16_t startTimeOfDay, const uint16_t endTimeOfDay) {
  return FrontlightSchedule::hasCompleteWindow(enabled, startTimeOfDay, endTimeOfDay);
}

constexpr bool containsTimeOfDay(const uint16_t startTimeOfDay, const uint16_t endTimeOfDay,
                                 const uint16_t currentTimeOfDay) {
  return FrontlightSchedule::containsTimeOfDay(startTimeOfDay, endTimeOfDay, currentTimeOfDay);
}

constexpr uint16_t localTimeOfDay(const uint8_t utcHour, const uint8_t utcMinute,
                                  const uint8_t utcOffsetQuarterHoursBiased) {
  return FrontlightSchedule::localTimeOfDay(utcHour, utcMinute, utcOffsetQuarterHoursBiased);
}

}  // namespace DarkModeSchedule
