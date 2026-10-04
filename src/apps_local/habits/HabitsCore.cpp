#include "HabitsCore.h"

#include <HalStorage.h>
#include <Logging.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>

namespace habits {

namespace {

constexpr const char* kDataDir = "/XTData";
constexpr const char* kFilePath = "/XTData/habits.dat";
constexpr const char* kPartPath = "/XTData/habits.dat.part";
constexpr size_t kMaxFileBufferSize = 8 * 1024;

inline void writeStr(HalFile& file, const char* str) {
  file.write(str, std::strlen(str));
}

bool parseDate(const char* str, struct tm& outTm) {
  std::memset(&outTm, 0, sizeof(struct tm));
  if (!str || std::strlen(str) < 10 || str[4] != '-' || str[7] != '-') {
    return false;
  }
  const int y = (str[0] - '0') * 1000 + (str[1] - '0') * 100 + (str[2] - '0') * 10 + (str[3] - '0');
  const int m = (str[5] - '0') * 10 + (str[6] - '0');
  const int d = (str[8] - '0') * 10 + (str[9] - '0');
  outTm.tm_year = y - 1900;
  outTm.tm_mon = m - 1;
  outTm.tm_mday = d;
  outTm.tm_isdst = -1;
  return true;
}

static const char* const kDays[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const char* const kMonths[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

void formatDate(const struct tm& inTm, char* out, size_t outLen) {
  std::snprintf(out, outLen, "%04d-%02d-%02d", inTm.tm_year + 1900, inTm.tm_mon + 1, inTm.tm_mday);
}

}  // namespace

Store::Store() {
  initDefaults();
}

void Store::initDefaults() {
  recordCount_ = 0;
  completedCount_ = 0;
  for (int i = 0; i < kMaxCompletedHabits; ++i) {
    completed_[i] = CompletedHabit{};
  }
  for (int i = 0; i < kMaxHabits; ++i) {
    habits_[i] = Habit{};
    habits_[i].targetStreak = 21;
  }
  habits_[0].id = 1;
  std::strncpy(habits_[0].name, "READ", kNameMax - 1);
  habits_[0].targetStreak = 21;
  habits_[1].id = 2;
  std::strncpy(habits_[1].name, "WALK", kNameMax - 1);
  habits_[1].targetStreak = 21;
  habits_[2].id = 3;
  std::strncpy(habits_[2].name, "MEDITATE", kNameMax - 1);
  habits_[2].targetStreak = 21;
}

Habit* Store::habitAt(int index) {
  if (index < 0 || index >= kMaxHabits) return nullptr;
  return &habits_[index];
}

const Habit* Store::habitAt(int index) const {
  if (index < 0 || index >= kMaxHabits) return nullptr;
  return &habits_[index];
}

Habit* Store::habitById(uint32_t id) {
  for (int i = 0; i < kMaxHabits; ++i) {
    if (habits_[i].id == id) return &habits_[i];
  }
  return nullptr;
}

const Habit* Store::habitById(uint32_t id) const {
  for (int i = 0; i < kMaxHabits; ++i) {
    if (habits_[i].id == id) return &habits_[i];
  }
  return nullptr;
}

const CompletedHabit* Store::completedHabitAt(int index) const {
  if (index < 0 || index >= completedCount_) return nullptr;
  return &completed_[index];
}

bool Store::completeHabit(int index, const char* completionDate) {
  if (index < 0 || index >= kMaxHabits || habits_[index].name[0] == '\0') return false;

  // Shift completed list down to insert newest at index 0
  if (completedCount_ >= kMaxCompletedHabits) {
    completedCount_ = kMaxCompletedHabits - 1;
  }
  for (int i = completedCount_; i > 0; --i) {
    completed_[i] = completed_[i - 1];
  }

  CompletedHabit& ch = completed_[0];
  std::strncpy(ch.name, habits_[index].name, kNameMax - 1);
  ch.name[kNameMax - 1] = '\0';
  ch.targetStreak = (habits_[index].targetStreak > 0) ? habits_[index].targetStreak : 21;
  ch.finalStreak = habits_[index].streak;
  ch.totalDays = habits_[index].totalCompleted;
  if (completionDate && completionDate[0] != '\0') {
    std::strncpy(ch.completionDate, completionDate, kDateLen - 1);
    ch.completionDate[kDateLen - 1] = '\0';
  } else {
    ch.completionDate[0] = '\0';
  }
  ++completedCount_;

  // Remove habit from active slots (frees up the slot)
  removeHabit(index);
  return true;
}

bool Store::removeCompletedHabit(int index) {
  if (index < 0 || index >= completedCount_) return false;
  for (int i = index; i < completedCount_ - 1; ++i) {
    completed_[i] = completed_[i + 1];
  }
  completed_[completedCount_ - 1] = CompletedHabit{};
  --completedCount_;
  save();
  return true;
}

void Store::clearCompletedHabits() {
  completedCount_ = 0;
  for (int i = 0; i < kMaxCompletedHabits; ++i) {
    completed_[i] = CompletedHabit{};
  }
  save();
}


DailyRecord Store::getRecord(uint32_t habitId, const char* date) const {
  for (int i = 0; i < recordCount_; ++i) {
    if (records_[i].habitId == habitId && std::strcmp(records_[i].date, date) == 0) {
      return records_[i];
    }
  }
  DailyRecord empty;
  std::strncpy(empty.date, date, kDateLen - 1);
  empty.habitId = habitId;
  empty.count = 0;
  empty.completed = false;
  return empty;
}

DailyRecord* Store::findOrCreateRecord(uint32_t habitId, const char* date) {
  for (int i = 0; i < recordCount_; ++i) {
    if (records_[i].habitId == habitId && std::strcmp(records_[i].date, date) == 0) {
      return &records_[i];
    }
  }
  if (recordCount_ >= kMaxRecords) {
    for (int i = 0; i < kMaxRecords - 1; ++i) {
      records_[i] = records_[i + 1];
    }
    recordCount_ = kMaxRecords - 1;
  }
  DailyRecord& rec = records_[recordCount_++];
  std::strncpy(rec.date, date, kDateLen - 1);
  rec.habitId = habitId;
  rec.count = 0;
  rec.completed = false;
  return &rec;
}

void Store::toggleBinary(uint32_t habitId, const char* date) {
  DailyRecord* r = findOrCreateRecord(habitId, date);
  r->completed = !r->completed;
  r->count = r->completed ? 1 : 0;
  save();
}

void Store::recalculateStreaks(const char* todayDate) {
  for (int i = 0; i < kMaxHabits; ++i) {
    Habit& h = habits_[i];
    int currentStreak = 0;
    int total = 0;

    for (int r = 0; r < recordCount_; ++r) {
      if (records_[r].habitId == h.id && records_[r].completed) {
        ++total;
      }
    }
    h.totalCompleted = total;

    DailyRecord todayRec = getRecord(h.id, todayDate);
    if (todayRec.completed) {
      currentStreak = 1;
    }

    char checkDate[kDateLen] = {};
    for (int dayOffset = -1; dayOffset >= -60; --dayOffset) {
      shiftDate(todayDate, dayOffset, checkDate, sizeof(checkDate));
      DailyRecord prevRec = getRecord(h.id, checkDate);
      if (prevRec.completed) {
        ++currentStreak;
      } else {
        break;
      }
    }

    h.streak = currentStreak;
    if (h.streak > h.bestStreak) {
      h.bestStreak = h.streak;
    }
  }
}

void Store::getTodayScore(const char* todayDate, int& completed, int& total) const {
  completed = 0;
  total = 0;
  for (int i = 0; i < kMaxHabits; ++i) {
    if (habits_[i].name[0] == '\0') continue;
    ++total;
    DailyRecord rec = getRecord(habits_[i].id, todayDate);
    if (rec.completed) ++completed;
  }
}

int Store::weekCompletedDays(uint32_t habitId, const char weekDates[7][12]) const {
  int count = 0;
  for (int i = 0; i < 7; ++i) {
    DailyRecord rec = getRecord(habitId, weekDates[i]);
    if (rec.completed) ++count;
  }
  return count;
}

bool Store::load() {
  char* buffer = new (std::nothrow) char[kMaxFileBufferSize];
  if (!buffer) return false;

  size_t readBytes = Storage.readFileToBuffer(kFilePath, buffer, kMaxFileBufferSize);
  if (readBytes == 0) {
    delete[] buffer;
    initDefaults();
    save();
    return true;
  }

  recordCount_ = 0;
  completedCount_ = 0;
  int habitSlot = 0;
  int section = 0;  // 0 = HABIT, 1 = RECORDS, 2 = COMPLETED

  char* line = buffer;
  char* nextLine = nullptr;
  while (line && *line) {
    nextLine = std::strchr(line, '\n');
    if (nextLine) {
      *nextLine = '\0';
      ++nextLine;
    }
    size_t len = std::strlen(line);
    if (len > 0 && line[len - 1] == '\r') line[len - 1] = '\0';

    if (line[0] == '#' || line[0] == '\0') {
      line = nextLine;
      continue;
    }

    if (std::strcmp(line, "[RECORDS]") == 0) {
      section = 1;
      line = nextLine;
      continue;
    }

    if (std::strcmp(line, "[COMPLETED]") == 0) {
      section = 2;
      line = nextLine;
      continue;
    }

    if (std::strcmp(line, "[HABIT]") == 0) {
      section = 0;
      line = nextLine;
      continue;
    }

    if (section == 0) {
      char* eq = std::strchr(line, '=');
      if (eq) {
        *eq = '\0';
        const char* key = line;
        const char* val = eq + 1;
        if (habitSlot < kMaxHabits) {
          Habit& h = habits_[habitSlot];
          if (std::strcmp(key, "id") == 0) h.id = static_cast<uint32_t>(std::atoi(val));
          else if (std::strcmp(key, "name") == 0) std::strncpy(h.name, val, kNameMax - 1);
          else if (std::strcmp(key, "target_streak") == 0) h.targetStreak = std::atoi(val);
          else if (std::strcmp(key, "best_streak") == 0) {
            h.bestStreak = std::atoi(val);
            if (h.targetStreak <= 0) h.targetStreak = 21;
            ++habitSlot;
          }
        }
      }
    } else if (section == 1) {
      char* p = line;
      char* dStr = strsep(&p, " ");
      char* idStr = strsep(&p, " ");
      char* cntStr = strsep(&p, " ");
      char* compStr = strsep(&p, " ");
      if (dStr && idStr && cntStr && compStr && recordCount_ < kMaxRecords) {
        DailyRecord& r = records_[recordCount_++];
        std::strncpy(r.date, dStr, kDateLen - 1);
        r.habitId = static_cast<uint32_t>(std::atoi(idStr));
        r.count = std::atoi(cntStr);
        r.completed = (std::atoi(compStr) != 0);
      }
    } else if (section == 2) {
      char* p = line;
      char* dStr = strsep(&p, " ");
      char* targetStr = strsep(&p, " ");
      char* finalStr = strsep(&p, " ");
      char* totalStr = strsep(&p, " ");
      if (dStr && targetStr && finalStr && totalStr && p && completedCount_ < kMaxCompletedHabits) {
        CompletedHabit& ch = completed_[completedCount_++];
        if (std::strcmp(dStr, "-") != 0) {
          std::strncpy(ch.completionDate, dStr, kDateLen - 1);
        } else {
          ch.completionDate[0] = '\0';
        }
        ch.targetStreak = std::atoi(targetStr);
        ch.finalStreak = std::atoi(finalStr);
        ch.totalDays = std::atoi(totalStr);
        std::strncpy(ch.name, p, kNameMax - 1);
        ch.name[kNameMax - 1] = '\0';
      }
    }
    line = nextLine;
  }

  delete[] buffer;
  return true;
}

bool Store::save() {
  Storage.ensureDirectoryExists(kDataDir);

  HalFile file;
  if (!Storage.openFileForWrite("HABITS", kPartPath, file)) return false;

  char line[96];
  writeStr(file, "# HABITS CONFIG v2\n");

  for (int i = 0; i < kMaxHabits; ++i) {
    const Habit& h = habits_[i];
    if (h.name[0] == '\0') continue;
    writeStr(file, "[HABIT]\n");
    std::snprintf(line, sizeof(line), "id=%u\nname=%s\ntarget_streak=%d\nbest_streak=%d\n\n",
                  static_cast<unsigned int>(h.id), h.name, h.targetStreak > 0 ? h.targetStreak : 21, h.bestStreak);
    writeStr(file, line);
  }

  writeStr(file, "[RECORDS]\n");
  for (int i = 0; i < recordCount_; ++i) {
    const auto& r = records_[i];
    std::snprintf(line, sizeof(line), "%s %u %d %d\n", r.date, static_cast<unsigned int>(r.habitId), r.count, r.completed ? 1 : 0);
    writeStr(file, line);
  }

  writeStr(file, "[COMPLETED]\n");
  for (int i = 0; i < completedCount_; ++i) {
    const auto& c = completed_[i];
    std::snprintf(line, sizeof(line), "%s %d %d %d %s\n",
                  c.completionDate[0] ? c.completionDate : "-",
                  c.targetStreak, c.finalStreak, c.totalDays, c.name);
    writeStr(file, line);
  }


  file.close();
  Storage.remove(kFilePath);
  if (!Storage.rename(kPartPath, kFilePath)) {
    Storage.remove(kPartPath);
    return false;
  }
  return true;
}

void Store::getTodayDate(char* ymdBuf, size_t ymdLen, char* headerBuf, size_t headerLen) {
  std::time_t now = std::time(nullptr);
  struct tm tm {};
  localtime_r(&now, &tm);

  if (ymdBuf && ymdLen >= kDateLen) formatDate(tm, ymdBuf, ymdLen);

  if (headerBuf && headerLen >= 32) {
    const char* dayName = (tm.tm_wday >= 0 && tm.tm_wday < 7) ? kDays[tm.tm_wday] : "";
    const char* monName = (tm.tm_mon >= 0 && tm.tm_mon < 12) ? kMonths[tm.tm_mon] : "";
    std::snprintf(headerBuf, headerLen, "%s, %s %d", dayName, monName, tm.tm_mday);
  }
}

void Store::shiftDate(const char* baseDate, int daysDelta, char* outDate, size_t outLen) {
  struct tm tm {};
  if (!parseDate(baseDate, tm)) {
    std::strncpy(outDate, baseDate, outLen - 1);
    return;
  }
  tm.tm_mday += daysDelta;
  std::mktime(&tm);
  formatDate(tm, outDate, outLen);
}

void Store::getWeekDays(const char* anchorDate, char weekDates[7][12]) {
  struct tm tm {};
  if (!parseDate(anchorDate, tm)) return;
  std::mktime(&tm);
  int daysSinceMonday = (tm.tm_wday + 6) % 7;
  for (int i = 0; i < 7; ++i) {
    shiftDate(anchorDate, i - daysSinceMonday, weekDates[i], 12);
  }
}

bool Store::addHabit(const char* name, int targetStreak) {
  for (int i = 0; i < kMaxHabits; ++i) {
    if (habits_[i].name[0] == '\0') {
      return addHabitAt(i, name, targetStreak);
    }
  }
  return false;
}

bool Store::addHabitAt(int index, const char* name, int targetStreak) {
  if (index < 0 || index >= kMaxHabits || !name || name[0] == '\0') return false;
  habits_[index] = Habit{};
  habits_[index].id = static_cast<uint32_t>(index + 1);
  std::strncpy(habits_[index].name, name, kNameMax - 1);
  habits_[index].name[kNameMax - 1] = '\0';
  habits_[index].targetStreak = (targetStreak > 0) ? targetStreak : 21;
  save();
  return true;
}

bool Store::setTargetStreak(int index, int targetStreak) {
  if (index < 0 || index >= kMaxHabits) return false;
  habits_[index].targetStreak = (targetStreak > 0) ? targetStreak : 21;
  save();
  return true;
}

bool Store::removeHabit(int index) {
  if (index < 0 || index >= kMaxHabits) return false;
  uint32_t removedId = habits_[index].id;
  std::memset(&habits_[index], 0, sizeof(Habit));
  int writeIdx = 0;
  for (int r = 0; r < recordCount_; ++r) {
    if (records_[r].habitId != removedId) {
      if (writeIdx != r) {
        records_[writeIdx] = records_[r];
      }
      ++writeIdx;
    }
  }
  recordCount_ = writeIdx;
  save();
  return true;
}

bool Store::renameHabit(int index, const char* newName) {
  if (index < 0 || index >= kMaxHabits || !newName || newName[0] == '\0') return false;
  std::strncpy(habits_[index].name, newName, kNameMax - 1);
  habits_[index].name[kNameMax - 1] = '\0';
  save();
  return true;
}

int Store::activeHabitCount() const {
  int count = 0;
  for (int i = 0; i < kMaxHabits; ++i) {
    if (habits_[i].name[0] != '\0') ++count;
  }
  return count;
}

void Store::formatDisplayDate(const char* baseDate, int dayOffset, char* outYmd, size_t ymdLen, char* outHeader, size_t headerLen) {
  char ymd[kDateLen] = {};
  if (dayOffset == 0) {
    std::strncpy(ymd, baseDate, sizeof(ymd) - 1);
  } else {
    shiftDate(baseDate, dayOffset, ymd, sizeof(ymd));
  }
  if (outYmd && ymdLen >= kDateLen) {
    std::strncpy(outYmd, ymd, ymdLen - 1);
    outYmd[ymdLen - 1] = '\0';
  }

  if (outHeader && headerLen > 0) {
    struct tm tm {};
    if (parseDate(ymd, tm)) {
      std::mktime(&tm);
      const char* dayName = (tm.tm_wday >= 0 && tm.tm_wday < 7) ? kDays[tm.tm_wday] : "";
      const char* monName = (tm.tm_mon >= 0 && tm.tm_mon < 12) ? kMonths[tm.tm_mon] : "";

      if (dayOffset == 0) {
        std::snprintf(outHeader, headerLen, "TODAY, %s %d", monName, tm.tm_mday);
      } else if (dayOffset == -1) {
        std::snprintf(outHeader, headerLen, "YESTERDAY, %s %d", monName, tm.tm_mday);
      } else {
        std::snprintf(outHeader, headerLen, "%s, %s %d", dayName, monName, tm.tm_mday);
      }
    } else {
      std::strncpy(outHeader, ymd, headerLen - 1);
      outHeader[headerLen - 1] = '\0';
    }
  }
}

}  // namespace habits
