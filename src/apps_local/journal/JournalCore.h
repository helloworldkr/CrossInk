#pragma once

#include <cstddef>
#include <cstdint>

namespace journal {

constexpr int kMaxQuestions = 16;
constexpr int kQuestionMax = 128;
constexpr int kAnswerMax = 256;
constexpr size_t kDateLen = 12;  // "YYYY-MM-DD" + null
constexpr int kMaxHistory = 100;

struct Question {
  uint32_t id = 0;
  char text[kQuestionMax] = {};
};

struct Entry {
  uint32_t questionId = 0;
  char questionText[kQuestionMax] = {};
  char answer[kAnswerMax] = {};
};

struct DayRecord {
  char date[kDateLen] = {};
  int entryCount = 0;
  Entry entries[kMaxQuestions] = {};
};

struct HistoryItem {
  char date[kDateLen] = {};
  int answered = 0;
  int total = 0;
};

class Store {
 public:
  Store();

  bool loadQuestions();
  bool saveQuestions();
  void resetQuestionsToDefaults();

  int questionCount() const { return questionCount_; }
  const Question* questionAt(int index) const;
  bool addQuestion(const char* text);
  bool removeQuestion(int index);

  bool loadDay(const char* date);
  bool saveDay();

  const DayRecord& currentDay() const { return currentDay_; }
  const char* getAnswer(uint32_t questionId) const;
  const char* getAnswerAt(int questionIndex) const;
  bool setAnswer(uint32_t questionId, const char* questionText, const char* answer);
  bool setAnswerAt(int questionIndex, const char* answer);

  void getScore(int& answered, int& total) const;

  bool loadHistory();
  int historyCount() const { return historyCount_; }
  const HistoryItem* historyAt(int index) const;

  static void getTodayDate(char* ymdBuf, size_t ymdLen, char* headerBuf, size_t headerLen);
  static void shiftDate(const char* baseDate, int daysDelta, char* outDate, size_t outLen);
  static void formatDisplayDate(const char* baseDate, int dayOffset, char* outYmd, size_t ymdLen, char* outHeader,
                                size_t headerLen);

 private:
  Question questions_[kMaxQuestions];
  int questionCount_ = 0;
  uint32_t nextQuestionId_ = 1;

  DayRecord currentDay_;

  HistoryItem history_[kMaxHistory];
  int historyCount_ = 0;

  void initDefaultQuestions();
  void updateHistoryIndex(const char* date, int answered, int total);
};

}  // namespace journal
