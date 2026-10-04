#pragma once

#include <cstddef>
#include <cstdint>

#include "SanyamData.h"

namespace sanyam {

constexpr size_t kDateLen = 12;  // "YYYY-MM-DD" + null

struct DayRecord {
  char date[kDateLen] = {};
  int ratings[kTotalItems] = {0};  // 0 = unlogged, 1 = opt1, 2 = opt2, 3 = opt3
};

class Store {
 public:
  Store();

  bool loadDay(const char* date);
  bool saveDay();

  const DayRecord& currentDay() const { return currentDay_; }

  int getRating(int index) const;
  bool setRating(int index, int rating);
  int cycleRating(int index);

  void getScore(int& logged, int& total) const;
  void getCategoryScore(Category cat, int& logged, int& total) const;

  static void getTodayDate(char* ymdBuf, size_t ymdLen, char* headerBuf, size_t headerLen);
  static void shiftDate(const char* baseDate, int daysDelta, char* outDate, size_t outLen);
  static void formatDisplayDate(const char* baseDate, int dayOffset, char* outYmd, size_t ymdLen, char* outHeader,
                                size_t headerLen);

 private:
  DayRecord currentDay_;
  bool dirty_ = false;
};

}  // namespace sanyam
