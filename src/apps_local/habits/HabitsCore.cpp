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

void formatDate(const struct tm& inTm, char* out, size_t outLen) {
  std::snprintf(out, outLen, "%04d-%02d-%02d", inTm.tm_year + 1900, inTm.tm_mon + 1, inTm.tm_mday);
}

}  // namespace

Store::Store() {
  initDefaults();
}

void Store::initDefaults() {
  habits_[0].id = 1;
  std::strncpy(habits_[0].name, "READ", kNameMax - 1);
  habits_[0].type = HabitType::Count;
  habits_[0].target = 10;
  std::strncpy(habits_[0].unit, "pages", kUnitMax - 1);
  habits_[0].increment = 1;
  habits_[0].minimum = 2;
  std::strncpy(habits_[0].cue, "After dinner", kTextMax - 1);
  std::strncpy(habits_[0].identity, "I am a reader", kTextMax - 1);

  habits_[1].id = 2;
  std::strncpy(habits_[1].name, "WALK", kNameMax - 1);
  habits_[1].type = HabitType::Count;
  habits_[1].target = 20;
  std::strncpy(habits_[1].unit, "min", kUnitMax - 1);
  habits_[1].increment = 5;
  habits_[1].minimum = 10;
  std::strncpy(habits_[1].cue, "Morning break", kTextMax - 1);
  std::strncpy(habits_[1].identity, "I am active", kTextMax - 1);

  habits_[2].id = 3;
  std::strncpy(habits_[2].name, "MEDITATE", kNameMax - 1);
  habits_[2].type = HabitType::Count;
  habits_[2].target = 10;
  std::strncpy(habits_[2].unit, "min", kUnitMax - 1);
  habits_[2].increment = 5;
  habits_[2].minimum = 5;
  std::strncpy(habits_[2].cue, "Before sleep", kTextMax - 1);
  std::strncpy(habits_[2].identity, "I am mindful", kTextMax - 1);
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
  empty.minimumReached = false;
  return empty;
}

DailyRecord* Store::findOrCreateRecord(uint32_t habitId, const char* date) {
  for (int i = 0; i < recordCount_; ++i) {
    if (records_[i].habitId == habitId && std::strcmp(records_[i].date, date) == 0) {
      return &records_[i];
    }
  }
  if (recordCount_ >= kMaxRecords) {
    // Evict oldest record by shifting
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
  rec.minimumReached = false;
  return &rec;
}

void Store::setRecord(uint32_t habitId, const char* date, int count) {
  const Habit* h = habitById(habitId);
  if (!h) return;

  DailyRecord* r = findOrCreateRecord(habitId, date);
  r->count = std::max(0, count);
  r->completed = (r->count >= h->target);
  r->minimumReached = (h->minimum > 0 && r->count >= h->minimum);
  save();
}

void Store::increment(uint32_t habitId, const char* date) {
  toggleBinary(habitId, date);
}

void Store::decrement(uint32_t habitId, const char* date) {
  toggleBinary(habitId, date);
}

void Store::toggleBinary(uint32_t habitId, const char* date) {
  DailyRecord* r = findOrCreateRecord(habitId, date);
  r->completed = !r->completed;
  r->count = r->completed ? 1 : 0;
  r->minimumReached = r->completed;
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
  int habitSlot = 0;
  bool inRecords = false;

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
      inRecords = true;
      line = nextLine;
      continue;
    }

    if (!inRecords) {
      if (std::strcmp(line, "[HABIT]") == 0) {
        line = nextLine;
        continue;
      }
      char* eq = std::strchr(line, '=');
      if (eq) {
        *eq = '\0';
        const char* key = line;
        const char* val = eq + 1;
        if (habitSlot < kMaxHabits) {
          Habit& h = habits_[habitSlot];
          if (std::strcmp(key, "id") == 0) h.id = static_cast<uint32_t>(std::atoi(val));
          else if (std::strcmp(key, "name") == 0) std::strncpy(h.name, val, kNameMax - 1);
          else if (std::strcmp(key, "type") == 0) h.type = (std::strcmp(val, "binary") == 0) ? HabitType::Binary : HabitType::Count;
          else if (std::strcmp(key, "target") == 0) h.target = std::atoi(val);
          else if (std::strcmp(key, "unit") == 0) std::strncpy(h.unit, val, kUnitMax - 1);
          else if (std::strcmp(key, "increment") == 0) h.increment = std::max(1, std::atoi(val));
          else if (std::strcmp(key, "minimum") == 0) h.minimum = std::atoi(val);
          else if (std::strcmp(key, "best_streak") == 0) h.bestStreak = std::atoi(val);
          else if (std::strcmp(key, "cue") == 0) std::strncpy(h.cue, val, kTextMax - 1);
          else if (std::strcmp(key, "identity") == 0) {
            std::strncpy(h.identity, val, kTextMax - 1);
            ++habitSlot;
          }
        }
      }
    } else {
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
        r.minimumReached = p ? (std::atoi(p) != 0) : false;
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

  char line[128];
  writeStr(file, "# HABITS CONFIG v1\n");

  for (int i = 0; i < kMaxHabits; ++i) {
    const Habit& h = habits_[i];
    writeStr(file, "[HABIT]\n");
    std::snprintf(line, sizeof(line), "id=%u\nname=%s\ntype=%s\ntarget=%d\nunit=%s\nincrement=%d\nminimum=%d\nbest_streak=%d\ncue=%s\nidentity=%s\n\n",
                  h.id, h.name, h.type == HabitType::Binary ? "binary" : "count", h.target, h.unit, h.increment, h.minimum, h.bestStreak, h.cue, h.identity);
    writeStr(file, line);
  }

  writeStr(file, "[RECORDS]\n# date habitId count completed minReached\n");
  for (int i = 0; i < recordCount_; ++i) {
    const auto& r = records_[i];
    std::snprintf(line, sizeof(line), "%s %u %d %d %d\n", r.date, r.habitId, r.count, r.completed ? 1 : 0, r.minimumReached ? 1 : 0);
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
    static const char* kDays[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    static const char* kMonths[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
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

bool Store::addHabit(const char* name) {
  for (int i = 0; i < kMaxHabits; ++i) {
    if (habits_[i].name[0] == '\0') {
      return addHabitAt(i, name);
    }
  }
  return false;
}

bool Store::addHabitAt(int index, const char* name) {
  if (index < 0 || index >= kMaxHabits || !name || name[0] == '\0') return false;
  habits_[index].id = static_cast<uint32_t>(index + 1);
  std::strncpy(habits_[index].name, name, kNameMax - 1);
  habits_[index].name[kNameMax - 1] = '\0';
  habits_[index].type = HabitType::Binary;
  habits_[index].target = 1;
  habits_[index].increment = 1;
  habits_[index].streak = 0;
  habits_[index].bestStreak = 0;
  habits_[index].totalCompleted = 0;
  habits_[index].cue[0] = '\0';
  habits_[index].identity[0] = '\0';
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
      static const char* kDays[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
      static const char* kMonths[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
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
