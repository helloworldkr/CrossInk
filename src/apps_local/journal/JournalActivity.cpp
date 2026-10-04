#include "JournalActivity.h"

#include <Memory.h>
#include <cstdio>
#include <cstring>
#include <ctime>

#include "../../activities/ActivityResult.h"
#include "../../activities/util/KeyboardEntryActivity.h"
#include "../Shelf.h"
#include "../ui/ToyboxFonts.h"
#include "../ui/ToyboxTheme.h"

namespace {

bool parseDateHelper(const char* str, struct tm& outTm) {
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

int computeDayOffset(const char* todayYmd, const char* targetYmd) {
  struct tm t1{}, t2{};
  if (!parseDateHelper(todayYmd, t1) || !parseDateHelper(targetYmd, t2)) return 0;
  const std::time_t s1 = std::mktime(&t1);
  const std::time_t s2 = std::mktime(&t2);
  return static_cast<int>((s2 - s1) / 86400);
}

}  // namespace

std::unique_ptr<Activity> JournalActivity::create(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return makeUniqueNoThrow<JournalActivity>(renderer, mappedInput);
}

void JournalActivity::onEnter() {
  Activity::onEnter();
  toybox::ensureFonts(renderer);

  store_.loadQuestions();
  store_.loadHistory();

  journal::Store::getTodayDate(todayDate_, sizeof(todayDate_), nullptr, 0);
  dayOffset_ = 0;
  dailyPage_ = 0;
  questionsPage_ = 0;
  historyPage_ = 0;

  updateDisplayDate();
  store_.loadDay(displayDate_);

  openDaily();
}

void JournalActivity::onExit() {
  store_.saveDay();
  Activity::onExit();
}

void JournalActivity::updateDisplayDate() {
  journal::Store::formatDisplayDate(todayDate_, dayOffset_, displayDate_, sizeof(displayDate_), displayHeader_,
                                    sizeof(displayHeader_));
  store_.loadDay(displayDate_);
}

void JournalActivity::openDaily() {
  view_ = View::Daily;
  interactionsReady_ = false;
  requestUpdate();
}

void JournalActivity::openQuestions() {
  view_ = View::Questions;
  interactionsReady_ = false;
  requestUpdate();
}

void JournalActivity::openHistory() {
  view_ = View::History;
  store_.loadHistory();
  interactionsReady_ = false;
  requestUpdate();
}

void JournalActivity::openKeyboardForAnswer(int questionIndex) {
  const auto* q = store_.questionAt(questionIndex);
  if (!q) return;

  char titleBuf[48];
  std::snprintf(titleBuf, sizeof(titleBuf), "REFLECTION (%d/%d)", questionIndex + 1, store_.questionCount());

  const bool hasNext = (questionIndex + 1 < store_.questionCount());
  const char* existingAnswer = store_.getAnswerAt(questionIndex);
  auto keyboard = makeUniqueNoThrow<KeyboardEntryActivity>(renderer, mappedInput, titleBuf,
                                                           existingAnswer ? existingAnswer : "",
                                                           journal::kAnswerMax - 1, InputType::Text, 0,
                                                           q->text, hasNext);
  if (!keyboard) return;

  startActivityForResult(std::move(keyboard), [this, questionIndex](const ActivityResult& result) {
    if (result.isCancelled) {
      requestUpdate();
      return;
    }
    const auto* keyboardResult = std::get_if<KeyboardResult>(&result.data);
    if (!keyboardResult) {
      requestUpdate();
      return;
    }
    store_.setAnswerAt(questionIndex, keyboardResult->text.c_str());
    if (keyboardResult->goToNext && questionIndex + 1 < store_.questionCount()) {
      dailyPage_ = (questionIndex + 1) / journalui::kQuestionsPerPage;
      openKeyboardForAnswer(questionIndex + 1);
    } else {
      dailyPage_ = questionIndex / journalui::kQuestionsPerPage;
      requestUpdate();
    }
  });
}

void JournalActivity::openKeyboardForNewQuestion() {
  auto keyboard = makeUniqueNoThrow<KeyboardEntryActivity>(renderer, mappedInput, "NEW QUESTION", "",
                                                           journal::kQuestionMax - 1, InputType::Text, 1,
                                                           "Enter your new reflection prompt to be shown daily:");
  if (!keyboard) return;

  startActivityForResult(std::move(keyboard), [this](const ActivityResult& result) {
    if (result.isCancelled) {
      requestUpdate();
      return;
    }
    const auto* keyboardResult = std::get_if<KeyboardResult>(&result.data);
    if (!keyboardResult || keyboardResult->text.empty()) {
      requestUpdate();
      return;
    }
    store_.addQuestion(keyboardResult->text.c_str());
    requestUpdate();
  });
}

void JournalActivity::loop() {
  // 1. Hardware Buttons
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    if (view_ != View::Daily) {
      openDaily();
      return;
    }
    if (dayOffset_ != 0) {
      dayOffset_ = 0;
      dailyPage_ = 0;
      updateDisplayDate();
      requestUpdate();
      return;
    }
    shelf::leave(renderer, mappedInput);
    return;
  }

  // Physical page/direction buttons
  if (mappedInput.wasReleased(MappedInputManager::Button::PageForward) ||
      mappedInput.wasReleased(MappedInputManager::Button::Down)) {
    if (view_ == View::Daily) {
      const int totalPages =
          (store_.questionCount() + journalui::kQuestionsPerPage - 1) / journalui::kQuestionsPerPage;
      if (dailyPage_ < totalPages - 1) {
        dailyPage_++;
        requestUpdate();
        return;
      }
    }
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::PageBack) ||
      mappedInput.wasReleased(MappedInputManager::Button::Up)) {
    if (view_ == View::Daily && dailyPage_ > 0) {
      dailyPage_--;
      requestUpdate();
      return;
    }
  }

  // 2. Touch Screen Interactions
  int x = 0;
  int y = 0;
  if (!mappedInput.wasScreenTapped(x, y) || !interactionsReady_) return;

  fui::InputSnapshot input{};
  input.touchReleased = true;
  input.touchX = static_cast<int16_t>(x);
  input.touchY = static_cast<int16_t>(y);
  const fui::ActionEvent action = interactions_.route(input);

  // Top Tabs
  if (action.action == journalui::ActionTabDaily) {
    openDaily();
    return;
  }
  if (action.action == journalui::ActionTabHistory) {
    openHistory();
    return;
  }
  if (action.action == journalui::ActionTabQuestions) {
    openQuestions();
    return;
  }

  // Date Navigation
  if (action.action == journalui::ActionDayPrev) {
    --dayOffset_;
    dailyPage_ = 0;
    updateDisplayDate();
    requestUpdate();
    return;
  }
  if (action.action == journalui::ActionDayNext) {
    if (dayOffset_ < 0) {
      ++dayOffset_;
      dailyPage_ = 0;
      updateDisplayDate();
      requestUpdate();
    }
    return;
  }
  if (action.action == journalui::ActionGoToday) {
    dayOffset_ = 0;
    dailyPage_ = 0;
    updateDisplayDate();
    requestUpdate();
    return;
  }

  // Paging
  if (action.action == journalui::ActionPagePrev) {
    int* p = (view_ == View::Daily) ? &dailyPage_ : (view_ == View::History) ? &historyPage_ : &questionsPage_;
    if (*p > 0) {
      (*p)--;
      requestUpdate();
    }
    return;
  }
  if (action.action == journalui::ActionPageNext) {
    const int count = (view_ == View::Daily)     ? store_.questionCount()
                      : (view_ == View::History) ? store_.historyCount()
                                                 : store_.questionCount();
    const int perPage = (view_ == View::Daily)     ? journalui::kQuestionsPerPage
                        : (view_ == View::History) ? journalui::kHistoryPerPage
                                                   : journalui::kManageQuestionsPerPage;
    const int totalPages = (count + perPage - 1) / perPage;
    int* p = (view_ == View::Daily) ? &dailyPage_ : (view_ == View::History) ? &historyPage_ : &questionsPage_;
    if (*p < totalPages - 1) {
      (*p)++;
      requestUpdate();
    }
    return;
  }

  // Question Card tapped -> Open Answer Editor
  if (action.action >= journalui::ActionQuestionCardBase &&
      action.action < journalui::ActionQuestionCardBase + journal::kMaxQuestions) {
    const int idx = action.action - journalui::ActionQuestionCardBase;
    openKeyboardForAnswer(idx);
    return;
  }

  // Questions Management
  if (action.action == journalui::ActionAddQuestion) {
    openKeyboardForNewQuestion();
    return;
  }
  if (action.action == journalui::ActionResetDefaults) {
    store_.resetQuestionsToDefaults();
    dailyPage_ = 0;
    questionsPage_ = 0;
    requestUpdate();
    return;
  }
  if (action.action >= journalui::ActionRemoveQuestionBase &&
      action.action < journalui::ActionRemoveQuestionBase + journal::kMaxQuestions) {
    const int idx = action.action - journalui::ActionRemoveQuestionBase;
    store_.removeQuestion(idx);
    requestUpdate();
    return;
  }

  // History: Select Date
  if (action.action >= journalui::ActionHistorySelectBase &&
      action.action < journalui::ActionHistorySelectBase + journal::kMaxHistory) {
    const int idx = action.action - journalui::ActionHistorySelectBase;
    const auto* item = store_.historyAt(idx);
    if (item) {
      dayOffset_ = computeDayOffset(todayDate_, item->date);
      dailyPage_ = 0;
      updateDisplayDate();
      openDaily();
    }
    return;
  }
}

void JournalActivity::render(RenderLock&&) {
  renderer.clearScreen();
  fui::GfxRendererTarget target = toybox::makeTarget(renderer);
  const fui::InputSnapshot noInput{};
  interactionsReady_ = false;
  toybox::Frame frame(target, target.deviceContext(), noInput, interactions_);
  toybox::Screen screen(frame);

  switch (view_) {
    case View::Daily:
      journalui::buildDaily(screen, store_, displayDate_, displayHeader_, dayOffset_, dailyPage_);
      break;

    case View::Questions:
      journalui::buildQuestions(screen, store_, questionsPage_);
      break;

    case View::History:
      journalui::buildHistory(screen, store_, historyPage_);
      break;
  }

  interactionsReady_ = true;
  toybox::reportOverflow(interactions_, "Journal");
  renderer.displayBuffer();
}
