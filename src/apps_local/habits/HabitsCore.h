#pragma once

#include <cstddef>
#include <cstdint>

namespace habits {

constexpr int kMaxHabits = 3;  // Exactly 3 active habits
constexpr int kMaxRecords = 120; // 40 days of history for 3 habits
constexpr size_t kNameMax = 32;
constexpr size_t kDateLen = 12;  // "YYYY-MM-DD" + null

constexpr int kMaxCompletedHabits = 20;

struct Habit {
  uint32_t id = 0;
  char name[kNameMax] = {};
  int streak = 0;
  int bestStreak = 0;
  int totalCompleted = 0;
  int targetStreak = 21;
};

struct CompletedHabit {
  char name[kNameMax] = {};
  int targetStreak = 21;
  int finalStreak = 0;
  int totalDays = 0;
  char completionDate[kDateLen] = {};
};

struct DailyRecord {
  char date[kDateLen] = {};
  uint32_t habitId = 0;
  int count = 0;
  bool completed = false;
};

class Store {
 public:
  Store();

  bool load();
  bool save();

  int count() const { return kMaxHabits; }
  Habit* habitAt(int index);
  const Habit* habitAt(int index) const;
  Habit* habitById(uint32_t id);
  const Habit* habitById(uint32_t id) const;

  int completedHabitCount() const { return completedCount_; }
  const CompletedHabit* completedHabitAt(int index) const;
  bool completeHabit(int index, const char* completionDate);
  bool removeCompletedHabit(int index);
  void clearCompletedHabits();

  DailyRecord getRecord(uint32_t habitId, const char* date) const;
  void toggleBinary(uint32_t habitId, const char* date);

  void recalculateStreaks(const char* todayDate);
  void getTodayScore(const char* todayDate, int& completed, int& total) const;
  int weekCompletedDays(uint32_t habitId, const char weekDates[7][12]) const;

  bool addHabit(const char* name, int targetStreak = 21);
  bool addHabitAt(int index, const char* name, int targetStreak = 21);
  bool setTargetStreak(int index, int targetStreak);
  bool removeHabit(int index);
  bool renameHabit(int index, const char* newName);
  int activeHabitCount() const;

  static void getTodayDate(char* ymdBuf, size_t ymdLen, char* headerBuf, size_t headerLen);
  static void getWeekDays(const char* anchorDate, char weekDates[7][12]);
  static void shiftDate(const char* baseDate, int daysDelta, char* outDate, size_t outLen);
  static void formatDisplayDate(const char* baseDate, int dayOffset, char* outYmd, size_t ymdLen, char* outHeader, size_t headerLen);

 private:
  Habit habits_[kMaxHabits];
  CompletedHabit completed_[kMaxCompletedHabits];
  int completedCount_ = 0;
  DailyRecord records_[kMaxRecords];
  int recordCount_ = 0;

  void initDefaults();
  DailyRecord* findOrCreateRecord(uint32_t habitId, const char* date);
};

}  // namespace habits

