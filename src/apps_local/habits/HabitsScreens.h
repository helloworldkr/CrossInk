#pragma once

#include <cstdint>

#include "../ui/ToyboxScreen.h"
#include "HabitsCore.h"

namespace fui = freeink::ui;

namespace habitsui {

// Daily view actions
inline constexpr fui::ActionId ActionHabitTap0 = 1;
inline constexpr fui::ActionId ActionHabitTap1 = 2;
inline constexpr fui::ActionId ActionHabitTap2 = 3;
inline constexpr fui::ActionId ActionDayPrev = 4;
inline constexpr fui::ActionId ActionDayNext = 5;
inline constexpr fui::ActionId ActionGoToday = 6;

// Navigation tabs
inline constexpr fui::ActionId ActionTabDaily = 7;
inline constexpr fui::ActionId ActionTabWeek = 8;
inline constexpr fui::ActionId ActionTabManage = 9;
inline constexpr fui::ActionId ActionTabCompleted = 17;

// Week view day columns
inline constexpr fui::ActionId ActionWeekDay0 = 10;
inline constexpr fui::ActionId ActionWeekDay1 = 11;
inline constexpr fui::ActionId ActionWeekDay2 = 12;
inline constexpr fui::ActionId ActionWeekDay3 = 13;
inline constexpr fui::ActionId ActionWeekDay4 = 14;
inline constexpr fui::ActionId ActionWeekDay5 = 15;
inline constexpr fui::ActionId ActionWeekDay6 = 16;

// Manage actions
inline constexpr fui::ActionId ActionManageAdd0 = 20;
inline constexpr fui::ActionId ActionManageAdd1 = 21;
inline constexpr fui::ActionId ActionManageAdd2 = 22;
inline constexpr fui::ActionId ActionManageRename0 = 23;
inline constexpr fui::ActionId ActionManageRename1 = 24;
inline constexpr fui::ActionId ActionManageRename2 = 25;
inline constexpr fui::ActionId ActionManageRemove0 = 26;
inline constexpr fui::ActionId ActionManageRemove1 = 27;
inline constexpr fui::ActionId ActionManageRemove2 = 28;
inline constexpr fui::ActionId ActionManageGoal0 = 40;
inline constexpr fui::ActionId ActionManageGoal1 = 41;
inline constexpr fui::ActionId ActionManageGoal2 = 42;
inline constexpr fui::ActionId ActionHabitComplete0 = 45;
inline constexpr fui::ActionId ActionHabitComplete1 = 46;
inline constexpr fui::ActionId ActionHabitComplete2 = 47;

// Completed list actions
inline constexpr fui::ActionId ActionCompletedRemove0 = 70;

// Presets
inline constexpr fui::ActionId ActionPreset0 = 30;  // READ
inline constexpr fui::ActionId ActionPreset1 = 31;  // WALK
inline constexpr fui::ActionId ActionPreset2 = 32;  // MEDITATE
inline constexpr fui::ActionId ActionPreset3 = 33;  // WORKOUT
inline constexpr fui::ActionId ActionPreset4 = 34;  // WATER
inline constexpr fui::ActionId ActionPreset5 = 35;  // JOURNAL
inline constexpr fui::ActionId ActionPresetCustom = 36;

void buildDaily(toybox::Screen& screen, const ::habits::Store& store, const char* displayDate,
                const char* displayHeader, int dayOffset);
void buildWeek(toybox::Screen& screen, const ::habits::Store& store, const char* todayDate);
void buildManage(toybox::Screen& screen, const ::habits::Store& store);
void buildCompleted(toybox::Screen& screen, const ::habits::Store& store);

}  // namespace habitsui

