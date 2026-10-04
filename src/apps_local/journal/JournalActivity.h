#pragma once

#include <memory>

#include "../../activities/Activity.h"
#include "../ui/ToyboxScreen.h"
#include "JournalCore.h"
#include "JournalScreens.h"

class JournalActivity final : public Activity {
 public:
  JournalActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Journal", renderer, mappedInput) {}
  ~JournalActivity() override = default;

  static std::unique_ptr<Activity> create(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  enum class View : uint8_t { Daily, Questions, History };

  void openDaily();
  void openQuestions();
  void openHistory();

  void updateDisplayDate();
  void openKeyboardForAnswer(int questionIndex);
  void openKeyboardForNewQuestion();

  journal::Store store_;
  View view_ = View::Daily;
  int dayOffset_ = 0;
  int dailyPage_ = 0;
  int questionsPage_ = 0;
  int historyPage_ = 0;

  char todayDate_[journal::kDateLen] = {};
  char displayDate_[journal::kDateLen] = {};
  char displayHeader_[32] = {};

  toybox::Interactions interactions_;
  bool interactionsReady_ = false;
};
