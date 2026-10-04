#include "HabitsActivity.h"

#include <Memory.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "../../activities/ActivityResult.h"
#include "../../activities/util/ConfirmationActivity.h"
#include "../../activities/util/KeyboardEntryActivity.h"
#include "../Shelf.h"
#include "../ui/ToyboxFonts.h"
#include "../ui/ToyboxTheme.h"

std::unique_ptr<Activity> HabitsActivity::create(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return makeUniqueNoThrow<HabitsActivity>(renderer, mappedInput);
}

void HabitsActivity::onEnter() {
  Activity::onEnter();
  toybox::ensureFonts(renderer);
  store_.load();
  habits::Store::getTodayDate(todayDate_, sizeof(todayDate_), nullptr, 0);
  dayOffset_ = 0;
  updateDisplayDate();
  store_.recalculateStreaks(todayDate_);
  openDaily();
}

void HabitsActivity::onExit() {
  store_.save();
  Activity::onExit();
}

void HabitsActivity::updateDisplayDate() {
  habits::Store::formatDisplayDate(todayDate_, dayOffset_, displayDate_, sizeof(displayDate_),
                                   displayHeader_, sizeof(displayHeader_));
}

void HabitsActivity::openDaily() {
  view_ = View::Daily;
  interactionsReady_ = false;
  requestUpdate();
}

void HabitsActivity::openWeek() {
  view_ = View::Week;
  interactionsReady_ = false;
  requestUpdate();
}

void HabitsActivity::openManage() {
  view_ = View::Manage;
  interactionsReady_ = false;
  requestUpdate();
}

void HabitsActivity::openCompleted() {
  view_ = View::Completed;
  interactionsReady_ = false;
  requestUpdate();
}

void HabitsActivity::confirmCompleteHabit(int slotIndex) {
  const auto* h = store_.habitAt(slotIndex);
  if (!h || h->name[0] == '\0') return;

  char heading[64];
  std::snprintf(heading, sizeof(heading), "Complete %s?", h->name);

  char body[200];
  const int target = (h->targetStreak > 0) ? h->targetStreak : 21;
  if (h->streak >= target) {
    std::snprintf(body, sizeof(body),
                  "Target goal reached (%d / %d days)!\nGraduate and archive '%s' to completed habits?\nThis will celebrate your achievement and free up the slot.",
                  h->streak, target, h->name);
  } else {
    std::snprintf(body, sizeof(body),
                  "Current streak: %d / %d days.\nArchive '%s' to completed habits now?\nThis will save your progress and free up the slot.",
                  h->streak, target, h->name);
  }

  auto confirm = makeUniqueNoThrow<ConfirmationActivity>(renderer, mappedInput, heading, body);
  if (!confirm) return;

  startActivityForResult(std::move(confirm), [this, slotIndex](const ActivityResult& result) {
    if (result.isCancelled) {
      requestUpdate();
      return;
    }
    store_.completeHabit(slotIndex, todayDate_);
    store_.recalculateStreaks(todayDate_);
    openCompleted();
  });
}


void HabitsActivity::openKeyboardForSlot(int slotIndex, const char* initialText) {
  auto keyboard = makeUniqueNoThrow<KeyboardEntryActivity>(
      renderer, mappedInput, "Habit Name", initialText ? initialText : "",
      habits::kNameMax - 1, InputType::Text, 1);
  if (!keyboard) return;

  startActivityForResult(std::move(keyboard), [this, slotIndex](const ActivityResult& result) {
    if (result.isCancelled) {
      requestUpdate();
      return;
    }
    const auto* keyboardResult = std::get_if<KeyboardResult>(&result.data);
    if (!keyboardResult || keyboardResult->text.empty()) {
      requestUpdate();
      return;
    }

    const auto* h = store_.habitAt(slotIndex);
    if (!h || h->name[0] == '\0') {
      openTargetStreakForSlot(slotIndex, keyboardResult->text, 21, true);
    } else {
      store_.renameHabit(slotIndex, keyboardResult->text.c_str());
      store_.recalculateStreaks(todayDate_);
      openManage();
    }
  });
}

void HabitsActivity::openTargetStreakForSlot(int slotIndex, std::string habitName, int currentTarget, bool isNewCreation) {
  char defaultTargetStr[16];
  std::snprintf(defaultTargetStr, sizeof(defaultTargetStr), "%d", currentTarget > 0 ? currentTarget : 21);

  char titleBuf[48];
  std::snprintf(titleBuf, sizeof(titleBuf), "TARGET: %s", habitName.c_str());

  auto keyboard = makeUniqueNoThrow<KeyboardEntryActivity>(
      renderer, mappedInput, titleBuf, defaultTargetStr, 4, InputType::Text, 1,
      "Enter your target streak in days (e.g. 7, 14, 21, 30, 60, 100):");
  if (!keyboard) return;

  startActivityForResult(std::move(keyboard), [this, slotIndex, habitName, currentTarget, isNewCreation](const ActivityResult& result) {
    int target = currentTarget > 0 ? currentTarget : 21;
    if (!result.isCancelled) {
      const auto* keyboardResult = std::get_if<KeyboardResult>(&result.data);
      if (keyboardResult && !keyboardResult->text.empty()) {
        const int parsed = std::atoi(keyboardResult->text.c_str());
        if (parsed > 0) target = parsed;
      }
    }
    if (isNewCreation) {
      store_.addHabitAt(slotIndex, habitName.c_str(), target);
    } else {
      store_.setTargetStreak(slotIndex, target);
    }
    store_.recalculateStreaks(todayDate_);
    openManage();
  });
}

void HabitsActivity::loop() {
  // 1. Physical Hardware Buttons
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    switch (view_) {
      case View::Daily:
        if (dayOffset_ != 0) {
          dayOffset_ = 0;
          updateDisplayDate();
          requestUpdate();
          return;
        }
        shelf::leave(renderer, mappedInput);
        return;
      case View::Week:
      case View::Manage:
      case View::Completed:
        openDaily();
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

  switch (action.action) {
    case habitsui::ActionHabitTap0:
    case habitsui::ActionHabitTap1:
    case habitsui::ActionHabitTap2: {
      const int idx = action.action - habitsui::ActionHabitTap0;
      const auto* h = store_.habitAt(idx);
      if (h && h->name[0] != '\0') {
        store_.toggleBinary(h->id, displayDate_);
        store_.recalculateStreaks(todayDate_);
        requestUpdate();
      }
      return;
    }

    case habitsui::ActionDayPrev:
      --dayOffset_;
      updateDisplayDate();
      requestUpdate();
      return;

    case habitsui::ActionDayNext:
      if (dayOffset_ < 0) {
        ++dayOffset_;
        updateDisplayDate();
        requestUpdate();
      }
      return;

    case habitsui::ActionGoToday:
      dayOffset_ = 0;
      updateDisplayDate();
      requestUpdate();
      return;

    case habitsui::ActionTabDaily:
      openDaily();
      return;

    case habitsui::ActionTabWeek:
      openWeek();
      return;

    case habitsui::ActionTabManage:
      openManage();
      return;

    case habitsui::ActionTabCompleted:
      openCompleted();
      return;

    case habitsui::ActionHabitComplete0:
    case habitsui::ActionHabitComplete1:
    case habitsui::ActionHabitComplete2: {
      const int slot = action.action - habitsui::ActionHabitComplete0;
      confirmCompleteHabit(slot);
      return;
    }

    case habitsui::ActionWeekDay0:
    case habitsui::ActionWeekDay1:
    case habitsui::ActionWeekDay2:
    case habitsui::ActionWeekDay3:
    case habitsui::ActionWeekDay4:
    case habitsui::ActionWeekDay5:
    case habitsui::ActionWeekDay6: {
      char weekDates[7][12] = {};
      habits::Store::getWeekDays(todayDate_, weekDates);
      int todayCol = 0;
      for (int c = 0; c < 7; ++c) {
        if (std::strcmp(weekDates[c], todayDate_) == 0) todayCol = c;
      }
      const int clickedCol = action.action - habitsui::ActionWeekDay0;
      int offset = clickedCol - todayCol;
      if (offset > 0) offset = 0;
      dayOffset_ = offset;
      updateDisplayDate();
      openDaily();
      return;
    }

    case habitsui::ActionManageAdd0:
    case habitsui::ActionManageAdd1:
    case habitsui::ActionManageAdd2: {
      const int slot = action.action - habitsui::ActionManageAdd0;
      openKeyboardForSlot(slot);
      return;
    }

    case habitsui::ActionManageRename0:
    case habitsui::ActionManageRename1:
    case habitsui::ActionManageRename2: {
      const int slot = action.action - habitsui::ActionManageRename0;
      const auto* h = store_.habitAt(slot);
      openKeyboardForSlot(slot, h ? h->name : nullptr);
      return;
    }

    case habitsui::ActionManageRemove0:
    case habitsui::ActionManageRemove1:
    case habitsui::ActionManageRemove2: {
      const int slot = action.action - habitsui::ActionManageRemove0;
      store_.removeHabit(slot);
      store_.recalculateStreaks(todayDate_);
      requestUpdate();
      return;
    }

    case habitsui::ActionManageGoal0:
    case habitsui::ActionManageGoal1:
    case habitsui::ActionManageGoal2: {
      const int slot = action.action - habitsui::ActionManageGoal0;
      const auto* h = store_.habitAt(slot);
      if (h && h->name[0] != '\0') {
        openTargetStreakForSlot(slot, h->name, h->targetStreak, false);
      }
      return;
    }

    case habitsui::ActionPreset0:
    case habitsui::ActionPreset1:
    case habitsui::ActionPreset2:
    case habitsui::ActionPreset3:
    case habitsui::ActionPreset4:
    case habitsui::ActionPreset5: {
      static const char* kPresets[6] = {"READ", "WALK", "MEDITATE", "WORKOUT", "WATER", "JOURNAL"};
      const int pIdx = action.action - habitsui::ActionPreset0;
      if (pIdx >= 0 && pIdx < 6) {
        int emptySlot = -1;
        for (int i = 0; i < habits::kMaxHabits; ++i) {
          const auto* h = store_.habitAt(i);
          if (!h || h->name[0] == '\0') {
            emptySlot = i;
            break;
          }
        }
        if (emptySlot >= 0) {
          openTargetStreakForSlot(emptySlot, kPresets[pIdx], 21, true);
          return;
        }
      }
      return;
    }

    case habitsui::ActionPresetCustom: {
      for (int i = 0; i < habits::kMaxHabits; ++i) {
        const auto* h = store_.habitAt(i);
        if (!h || h->name[0] == '\0') {
          openKeyboardForSlot(i);
          return;
        }
      }
      return;
    }

    default:
      if (action.action >= habitsui::ActionCompletedRemove0 &&
          action.action < habitsui::ActionCompletedRemove0 + habits::kMaxCompletedHabits) {
        const int cIdx = action.action - habitsui::ActionCompletedRemove0;
        store_.removeCompletedHabit(cIdx);
        requestUpdate();
        return;
      }
      return;
  }
}

void HabitsActivity::render(RenderLock&&) {
  renderer.clearScreen();
  fui::GfxRendererTarget target = toybox::makeTarget(renderer);
  const fui::InputSnapshot noInput{};
  interactionsReady_ = false;
  toybox::Frame frame(target, target.deviceContext(), noInput, interactions_);
  toybox::Screen screen(frame);

  switch (view_) {
    case View::Daily:
      habitsui::buildDaily(screen, store_, displayDate_, displayHeader_, dayOffset_);
      break;

    case View::Week:
      habitsui::buildWeek(screen, store_, todayDate_);
      break;

    case View::Manage:
      habitsui::buildManage(screen, store_);
      break;

    case View::Completed:
      habitsui::buildCompleted(screen, store_);
      break;
  }

  interactionsReady_ = true;
  toybox::reportOverflow(interactions_, "Habits");
  renderer.displayBuffer();
}

