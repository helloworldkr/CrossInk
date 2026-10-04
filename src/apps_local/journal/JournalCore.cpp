#include "JournalCore.h"

#include <HalStorage.h>
#include <Logging.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

namespace journal {

namespace {

constexpr const char* kDataDir = "/XTData/journal";
constexpr const char* kQuestionsPath = "/XTData/journal/questions.dat";
constexpr const char* kQuestionsPart = "/XTData/journal/questions.dat.part";
constexpr const char* kHistoryPath = "/XTData/journal/history.idx";
constexpr const char* kHistoryPart = "/XTData/journal/history.idx.part";
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
  initDefaultQuestions();
}

void Store::initDefaultQuestions() {
  static const char* const kDefaults[] = {
      "What is one thing I want to complete today to feel accomplished?",
      "Am I doing deep work?",
      "What do I want to improve the most, and how can I improve that by 1%?",
      "What don't I want to repeat in my life?",
      "Am I out of my comfort zone? Did I do something intentionally uncomfortable today?",
      "If I had to finish what I did in half the time, what would I have done differently?",
  };
  constexpr int count = static_cast<int>(sizeof(kDefaults) / sizeof(kDefaults[0]));

  questionCount_ = 0;
  nextQuestionId_ = 1;
  for (int i = 0; i < count && i < kMaxQuestions; ++i) {
    questions_[i].id = nextQuestionId_++;
    std::strncpy(questions_[i].text, kDefaults[i], kQuestionMax - 1);
    questions_[i].text[kQuestionMax - 1] = '\0';
    questionCount_++;
  }
}

void Store::resetQuestionsToDefaults() {
  initDefaultQuestions();
  saveQuestions();
}

const Question* Store::questionAt(int index) const {
  if (index < 0 || index >= questionCount_) return nullptr;
  return &questions_[index];
}

bool Store::addQuestion(const char* text) {
  if (!text || text[0] == '\0') return false;
  if (questionCount_ >= kMaxQuestions) return false;

  Question& q = questions_[questionCount_++];
  q.id = nextQuestionId_++;
  std::strncpy(q.text, text, kQuestionMax - 1);
  q.text[kQuestionMax - 1] = '\0';

  saveQuestions();
  return true;
}

bool Store::removeQuestion(int index) {
  if (index < 0 || index >= questionCount_) return false;

  for (int i = index; i < questionCount_ - 1; ++i) {
    questions_[i] = questions_[i + 1];
  }
  questionCount_--;
  saveQuestions();
  return true;
}

bool Store::loadQuestions() {
  char* buffer = new (std::nothrow) char[kMaxFileBufferSize];
  if (!buffer) return false;

  size_t readBytes = Storage.readFileToBuffer(kQuestionsPath, buffer, kMaxFileBufferSize);
  if (readBytes == 0) {
    delete[] buffer;
    initDefaultQuestions();
    saveQuestions();
    return true;
  }

  questionCount_ = 0;
  nextQuestionId_ = 1;

  char* line = buffer;
  char* nextLine = nullptr;
  Question currentQ{};
  bool hasQ = false;

  while (line && *line) {
    char* end = std::strchr(line, '\n');
    if (end) {
      *end = '\0';
      nextLine = end + 1;
      if (end > line && *(end - 1) == '\r') *(end - 1) = '\0';
    } else {
      nextLine = nullptr;
    }

    if (std::strcmp(line, "[QUESTION]") == 0) {
      if (hasQ && questionCount_ < kMaxQuestions) {
        questions_[questionCount_++] = currentQ;
        if (currentQ.id >= nextQuestionId_) nextQuestionId_ = currentQ.id + 1;
      }
      currentQ = Question{};
      hasQ = true;
    } else if (hasQ) {
      if (std::strncmp(line, "id=", 3) == 0) {
        currentQ.id = static_cast<uint32_t>(std::atoi(line + 3));
      } else if (std::strncmp(line, "text=", 5) == 0) {
        std::strncpy(currentQ.text, line + 5, kQuestionMax - 1);
        currentQ.text[kQuestionMax - 1] = '\0';
      }
    }

    line = nextLine;
  }

  if (hasQ && questionCount_ < kMaxQuestions) {
    questions_[questionCount_++] = currentQ;
    if (currentQ.id >= nextQuestionId_) nextQuestionId_ = currentQ.id + 1;
  }

  delete[] buffer;
  if (questionCount_ == 0) {
    initDefaultQuestions();
    saveQuestions();
  } else {
    bool upgraded = false;
    for (int i = 0; i < questionCount_; ++i) {
      if (std::strstr(questions_[i].text, "one thing i want completed") != nullptr) {
        std::strncpy(questions_[i].text, "What is one thing I want to complete today to feel accomplished?", kQuestionMax - 1);
        upgraded = true;
      } else if (std::strcmp(questions_[i].text, "Am i doing deep work") == 0) {
        std::strncpy(questions_[i].text, "Am I doing deep work?", kQuestionMax - 1);
        upgraded = true;
      } else if (std::strstr(questions_[i].text, "what do i want to improve") != nullptr) {
        std::strncpy(questions_[i].text, "What do I want to improve the most, and how can I improve that by 1%?", kQuestionMax - 1);
        upgraded = true;
      } else if (std::strstr(questions_[i].text, "What i dont want") != nullptr || std::strstr(questions_[i].text, "what i dont want") != nullptr) {
        std::strncpy(questions_[i].text, "What don't I want to repeat in my life?", kQuestionMax - 1);
        upgraded = true;
      } else if (std::strstr(questions_[i].text, "Am i out of my comfort zone") != nullptr) {
        std::strncpy(questions_[i].text, "Am I out of my comfort zone? Did I do something intentionally uncomfortable today?", kQuestionMax - 1);
        upgraded = true;
      } else if (std::strstr(questions_[i].text, "if i have to finish") != nullptr) {
        std::strncpy(questions_[i].text, "If I had to finish what I did in half the time, what would I have done differently?", kQuestionMax - 1);
        upgraded = true;
      }
    }
    if (upgraded) {
      saveQuestions();
    }
  }
  return true;
}

bool Store::saveQuestions() {
  Storage.ensureDirectoryExists(kDataDir);

  HalFile file;
  if (!Storage.openFileForWrite("JOURNAL", kQuestionsPart, file)) return false;

  char line[kQuestionMax + 32];
  writeStr(file, "# JOURNAL QUESTIONS v1\n");

  for (int i = 0; i < questionCount_; ++i) {
    const Question& q = questions_[i];
    writeStr(file, "[QUESTION]\n");
    std::snprintf(line, sizeof(line), "id=%u\ntext=%s\n\n", static_cast<unsigned int>(q.id), q.text);
    writeStr(file, line);
  }

  file.close();
  Storage.remove(kQuestionsPath);
  if (!Storage.rename(kQuestionsPart, kQuestionsPath)) {
    Storage.remove(kQuestionsPart);
    return false;
  }
  return true;
}

bool Store::loadDay(const char* date) {
  if (!date || date[0] == '\0') return false;

  std::strncpy(currentDay_.date, date, kDateLen - 1);
  currentDay_.date[kDateLen - 1] = '\0';
  currentDay_.entryCount = 0;
  for (int i = 0; i < kMaxQuestions; ++i) currentDay_.entries[i] = Entry{};

  char filePath[64];
  std::snprintf(filePath, sizeof(filePath), "%s/%s.dat", kDataDir, date);

  char* buffer = new (std::nothrow) char[kMaxFileBufferSize];
  if (!buffer) return false;

  size_t readBytes = Storage.readFileToBuffer(filePath, buffer, kMaxFileBufferSize);
  if (readBytes == 0) {
    delete[] buffer;
    return true;  // Empty day is fine
  }

  char* line = buffer;
  char* nextLine = nullptr;
  Entry currentEntry{};
  bool hasEntry = false;

  while (line && *line) {
    char* end = std::strchr(line, '\n');
    if (end) {
      *end = '\0';
      nextLine = end + 1;
      if (end > line && *(end - 1) == '\r') *(end - 1) = '\0';
    } else {
      nextLine = nullptr;
    }

    if (std::strcmp(line, "[ENTRY]") == 0) {
      if (hasEntry && currentDay_.entryCount < kMaxQuestions) {
        currentDay_.entries[currentDay_.entryCount++] = currentEntry;
      }
      currentEntry = Entry{};
      hasEntry = true;
    } else if (hasEntry) {
      if (std::strncmp(line, "id=", 3) == 0) {
        currentEntry.questionId = static_cast<uint32_t>(std::atoi(line + 3));
      } else if (std::strncmp(line, "q=", 2) == 0) {
        std::strncpy(currentEntry.questionText, line + 2, kQuestionMax - 1);
        currentEntry.questionText[kQuestionMax - 1] = '\0';
      } else if (std::strncmp(line, "a=", 2) == 0) {
        std::strncpy(currentEntry.answer, line + 2, kAnswerMax - 1);
        currentEntry.answer[kAnswerMax - 1] = '\0';
      }
    }

    line = nextLine;
  }

  if (hasEntry && currentDay_.entryCount < kMaxQuestions) {
    currentDay_.entries[currentDay_.entryCount++] = currentEntry;
  }

  delete[] buffer;
  return true;
}

bool Store::saveDay() {
  if (currentDay_.date[0] == '\0') return false;

  Storage.ensureDirectoryExists(kDataDir);

  char filePath[64];
  char partPath[64];
  std::snprintf(filePath, sizeof(filePath), "%s/%s.dat", kDataDir, currentDay_.date);
  std::snprintf(partPath, sizeof(partPath), "%s/%s.dat.part", kDataDir, currentDay_.date);

  HalFile file;
  if (!Storage.openFileForWrite("JOURNAL", partPath, file)) return false;

  char line[kAnswerMax + kQuestionMax + 32];
  std::snprintf(line, sizeof(line), "# JOURNAL %s\n", currentDay_.date);
  writeStr(file, line);

  int answered = 0;
  for (int i = 0; i < currentDay_.entryCount; ++i) {
    const Entry& e = currentDay_.entries[i];
    if (e.answer[0] != '\0') answered++;

    writeStr(file, "[ENTRY]\n");
    std::snprintf(line, sizeof(line), "id=%u\nq=%s\na=%s\n\n", static_cast<unsigned int>(e.questionId),
                  e.questionText, e.answer);
    writeStr(file, line);
  }

  file.close();
  Storage.remove(filePath);
  if (!Storage.rename(partPath, filePath)) {
    Storage.remove(partPath);
    return false;
  }

  updateHistoryIndex(currentDay_.date, answered, questionCount_);
  return true;
}

const char* Store::getAnswer(uint32_t questionId) const {
  for (int i = 0; i < currentDay_.entryCount; ++i) {
    if (currentDay_.entries[i].questionId == questionId) {
      return currentDay_.entries[i].answer;
    }
  }
  return "";
}

const char* Store::getAnswerAt(int questionIndex) const {
  const auto* q = questionAt(questionIndex);
  if (!q) return "";
  return getAnswer(q->id);
}

bool Store::setAnswer(uint32_t questionId, const char* questionText, const char* answer) {
  for (int i = 0; i < currentDay_.entryCount; ++i) {
    if (currentDay_.entries[i].questionId == questionId) {
      std::strncpy(currentDay_.entries[i].answer, answer ? answer : "", kAnswerMax - 1);
      currentDay_.entries[i].answer[kAnswerMax - 1] = '\0';
      if (questionText && questionText[0] != '\0') {
        std::strncpy(currentDay_.entries[i].questionText, questionText, kQuestionMax - 1);
        currentDay_.entries[i].questionText[kQuestionMax - 1] = '\0';
      }
      return saveDay();
    }
  }

  if (currentDay_.entryCount >= kMaxQuestions) return false;

  Entry& e = currentDay_.entries[currentDay_.entryCount++];
  e.questionId = questionId;
  std::strncpy(e.questionText, questionText ? questionText : "", kQuestionMax - 1);
  e.questionText[kQuestionMax - 1] = '\0';
  std::strncpy(e.answer, answer ? answer : "", kAnswerMax - 1);
  e.answer[kAnswerMax - 1] = '\0';

  return saveDay();
}

bool Store::setAnswerAt(int questionIndex, const char* answer) {
  const auto* q = questionAt(questionIndex);
  if (!q) return false;
  return setAnswer(q->id, q->text, answer);
}

void Store::getScore(int& answered, int& total) const {
  total = questionCount_;
  answered = 0;
  for (int i = 0; i < questionCount_; ++i) {
    const char* ans = getAnswerAt(i);
    if (ans && ans[0] != '\0') {
      answered++;
    }
  }
}

void Store::updateHistoryIndex(const char* date, int answered, int total) {
  if (!date || date[0] == '\0') return;

  loadHistory();

  int foundIdx = -1;
  for (int i = 0; i < historyCount_; ++i) {
    if (std::strcmp(history_[i].date, date) == 0) {
      foundIdx = i;
      break;
    }
  }

  if (answered == 0 && foundIdx >= 0) {
    // If all answers cleared, remove date from history
    for (int i = foundIdx; i < historyCount_ - 1; ++i) {
      history_[i] = history_[i + 1];
    }
    historyCount_--;
  } else if (foundIdx >= 0) {
    history_[foundIdx].answered = answered;
    history_[foundIdx].total = total;
  } else if (answered > 0 && historyCount_ < kMaxHistory) {
    // Insert into sorted position (descending date)
    int insertIdx = historyCount_;
    for (int i = 0; i < historyCount_; ++i) {
      if (std::strcmp(date, history_[i].date) > 0) {
        insertIdx = i;
        break;
      }
    }
    for (int i = historyCount_; i > insertIdx; --i) {
      history_[i] = history_[i - 1];
    }
    std::strncpy(history_[insertIdx].date, date, kDateLen - 1);
    history_[insertIdx].date[kDateLen - 1] = '\0';
    history_[insertIdx].answered = answered;
    history_[insertIdx].total = total;
    historyCount_++;
  }

  // Save history index
  HalFile file;
  if (!Storage.openFileForWrite("JOURNAL", kHistoryPart, file)) return;

  char line[64];
  for (int i = 0; i < historyCount_; ++i) {
    std::snprintf(line, sizeof(line), "%s %d %d\n", history_[i].date, history_[i].answered, history_[i].total);
    writeStr(file, line);
  }
  file.close();

  Storage.remove(kHistoryPath);
  if (!Storage.rename(kHistoryPart, kHistoryPath)) {
    Storage.remove(kHistoryPart);
  }
}

bool Store::loadHistory() {
  historyCount_ = 0;

  char* buffer = new (std::nothrow) char[kMaxFileBufferSize];
  if (!buffer) return false;

  size_t readBytes = Storage.readFileToBuffer(kHistoryPath, buffer, kMaxFileBufferSize);
  if (readBytes == 0) {
    delete[] buffer;
    return true;
  }

  char* line = buffer;
  char* nextLine = nullptr;

  while (line && *line && historyCount_ < kMaxHistory) {
    char* end = std::strchr(line, '\n');
    if (end) {
      *end = '\0';
      nextLine = end + 1;
      if (end > line && *(end - 1) == '\r') *(end - 1) = '\0';
    } else {
      nextLine = nullptr;
    }

    if (line[0] != '#' && line[0] != '\0') {
      char dStr[16] = {};
      int ans = 0, tot = 0;
      if (std::sscanf(line, "%15s %d %d", dStr, &ans, &tot) == 3) {
        HistoryItem& item = history_[historyCount_++];
        std::strncpy(item.date, dStr, kDateLen - 1);
        item.date[kDateLen - 1] = '\0';
        item.answered = ans;
        item.total = tot;
      }
    }

    line = nextLine;
  }

  delete[] buffer;
  return true;
}

const HistoryItem* Store::historyAt(int index) const {
  if (index < 0 || index >= historyCount_) return nullptr;
  return &history_[index];
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
      std::time_t t = std::mktime(&tm);
      localtime_r(&t, &tm);
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
      std::snprintf(outHeader, headerLen, "%s", ymd);
    }
  }
}

}  // namespace journal
