#pragma once

#include <memory>

#include "../../activities/Activity.h"
#include "../ui/ToyboxScreen.h"
#include "HabitsCore.h"
#include "HabitsScreens.h"

class HabitsActivity final : public Activity {
 public:
  HabitsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Habits", renderer, mappedInput) {}
  ~HabitsActivity() override = default;

  static std::unique_ptr<Activity> create(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  enum class View : uint8_t { Daily, Week, Manage, Completed };

  void openDaily();
  void openWeek();
  void openManage();
  void openCompleted();
  void updateDisplayDate();
  void openKeyboardForSlot(int slotIndex, const char* initialText = nullptr);
  void openTargetStreakForSlot(int slotIndex, std::string habitName, int currentTarget = 21, bool isNewCreation = true);
  void confirmCompleteHabit(int slotIndex);


  habits::Store store_;
  View view_ = View::Daily;
  int dayOffset_ = 0;

  char todayDate_[habits::kDateLen] = {};
  char displayDate_[habits::kDateLen] = {};
  char displayHeader_[32] = {};

  toybox::Interactions interactions_;
  bool interactionsReady_ = false;
};
