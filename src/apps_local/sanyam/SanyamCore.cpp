#include "SanyamCore.h"

#include <HalStorage.h>
#include <Logging.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

namespace sanyam {

namespace {

constexpr const char* kDataDir = "/XTData/sanyam";
constexpr size_t kMaxFileBufferSize = 4 * 1024;

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

int findItemIndexById(const char* id) {
  if (!id) return -1;
  for (int i = 0; i < kTotalItems; ++i) {
    if (std::strcmp(kItems[i].id, id) == 0) return i;
  }
  return -1;
}

}  // namespace

Store::Store() {
  std::memset(&currentDay_, 0, sizeof(currentDay_));
}

bool Store::loadDay(const char* date) {
  if (!date || std::strlen(date) == 0) return false;

  if (dirty_ && currentDay_.date[0] != '\0') {
    saveDay();
  }

  std::memset(&currentDay_, 0, sizeof(currentDay_));
  std::strncpy(currentDay_.date, date, kDateLen - 1);
  currentDay_.date[kDateLen - 1] = '\0';
  dirty_ = false;

  char filePath[64];
  std::snprintf(filePath, sizeof(filePath), "%s/%s.txt", kDataDir, date);

  char* buffer = new (std::nothrow) char[kMaxFileBufferSize];
  if (!buffer) return false;

  const size_t bytesRead = Storage.readFileToBuffer(filePath, buffer, kMaxFileBufferSize);
  if (bytesRead == 0) {
    delete[] buffer;
    return true;  // Clean empty record for new date
  }

  char* line = buffer;
  while (line && *line) {
    char* end = std::strchr(line, '\n');
    if (end) {
      *end = '\0';
      if (end > line && *(end - 1) == '\r') *(end - 1) = '\0';
    }

    if (line[0] != '#' && line[0] != '\0') {
      char* eq = std::strchr(line, '=');
      if (eq) {
        *eq = '\0';
        const char* key = line;
        const int val = std::atoi(eq + 1);
        const int idx = findItemIndexById(key);
        if (idx >= 0 && idx < kTotalItems) {
          if (val >= 0 && val <= 3) {
            currentDay_.ratings[idx] = val;
          }
        }
      }
    }

    line = end ? (end + 1) : nullptr;
  }

  delete[] buffer;
  dirty_ = false;
  return true;
}

bool Store::saveDay() {
  if (currentDay_.date[0] == '\0') return false;

  Storage.ensureDirectoryExists(kDataDir);

  char filePath[64];
  char partPath[64];
  std::snprintf(filePath, sizeof(filePath), "%s/%s.txt", kDataDir, currentDay_.date);
  std::snprintf(partPath, sizeof(partPath), "%s/%s.txt.part", kDataDir, currentDay_.date);

  HalFile file;
  if (!Storage.openFileForWrite("SANYAM", partPath, file)) {
    LOG_ERR("Sanyam", "Failed to create sanyam log part file: %s", partPath);
    return false;
  }

  writeStr(file, "# Patanjali Sanyam Daily Log\n");
  char line[64];
  for (int i = 0; i < kTotalItems; ++i) {
    if (currentDay_.ratings[i] > 0) {
      std::snprintf(line, sizeof(line), "%s=%d\n", kItems[i].id, currentDay_.ratings[i]);
      writeStr(file, line);
    }
  }

  file.close();
  Storage.remove(filePath);
  if (!Storage.rename(partPath, filePath)) {
    Storage.remove(partPath);
    return false;
  }

  dirty_ = false;
  return true;
}

int Store::getRating(int index) const {
  if (index < 0 || index >= kTotalItems) return 0;
  return currentDay_.ratings[index];
}

bool Store::setRating(int index, int rating) {
  if (index < 0 || index >= kTotalItems) return false;
  if (rating < 0 || rating > 3) return false;
  if (currentDay_.ratings[index] != rating) {
    currentDay_.ratings[index] = rating;
    dirty_ = true;
    saveDay();
  }
  return true;
}

int Store::cycleRating(int index) {
  if (index < 0 || index >= kTotalItems) return 0;
  // Cycle: 0 -> 1 -> 2 -> 3 -> 1
  int nextRating = (currentDay_.ratings[index] % 3) + 1;
  setRating(index, nextRating);
  return nextRating;
}

void Store::getScore(int& logged, int& total) const {
  logged = 0;
  total = kTotalItems;
  for (int i = 0; i < kTotalItems; ++i) {
    if (currentDay_.ratings[i] > 0) logged++;
  }
}

void Store::getCategoryScore(Category cat, int& logged, int& total) const {
  logged = 0;
  total = 0;
  for (int i = 0; i < kTotalItems; ++i) {
    if (kItems[i].category == cat) {
      total++;
      if (currentDay_.ratings[i] > 0) logged++;
    }
  }
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
  if (!baseDate || outLen < kDateLen) return;
  struct tm tm {};
  if (!parseDate(baseDate, tm)) return;

  std::time_t t = std::mktime(&tm);
  t += (daysDelta * 86400);
  struct tm shiftedTm {};
  localtime_r(&t, &shiftedTm);

  formatDate(shiftedTm, outDate, outLen);
}

void Store::formatDisplayDate(const char* baseDate, int dayOffset, char* outYmd, size_t ymdLen, char* outHeader,
                              size_t headerLen) {
  char ymd[kDateLen];
  shiftDate(baseDate, dayOffset, ymd, sizeof(ymd));

  if (outYmd && ymdLen >= kDateLen) {
    std::strncpy(outYmd, ymd, ymdLen - 1);
    outYmd[ymdLen - 1] = '\0';
  }

  if (outHeader && headerLen >= 32) {
    struct tm tm {};
    if (parseDate(ymd, tm)) {
      const char* dayName = (tm.tm_wday >= 0 && tm.tm_wday < 7) ? kDays[tm.tm_wday] : "";
      const char* monName = (tm.tm_mon >= 0 && tm.tm_mon < 12) ? kMonths[tm.tm_mon] : "";
      std::snprintf(outHeader, headerLen, "%s, %s %d", dayName, monName, tm.tm_mday);
    } else {
      std::snprintf(outHeader, headerLen, "%s", ymd);
    }
  }
}

}  // namespace sanyam
