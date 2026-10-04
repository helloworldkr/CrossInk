#pragma once

#include <cstdint>

#include "../ui/ToyboxScreen.h"
#include "JournalCore.h"

namespace fui = freeink::ui;

namespace journalui {

// Navigation tabs
inline constexpr fui::ActionId ActionTabDaily = 1;
inline constexpr fui::ActionId ActionTabHistory = 2;
inline constexpr fui::ActionId ActionTabQuestions = 3;

// Date navigation
inline constexpr fui::ActionId ActionDayPrev = 4;
inline constexpr fui::ActionId ActionDayNext = 5;
inline constexpr fui::ActionId ActionGoToday = 6;

// Paging
inline constexpr fui::ActionId ActionPagePrev = 7;
inline constexpr fui::ActionId ActionPageNext = 8;

// Questions management
inline constexpr fui::ActionId ActionAddQuestion = 9;
inline constexpr fui::ActionId ActionResetDefaults = 10;

// Dynamic action bases
inline constexpr fui::ActionId ActionQuestionCardBase = 20;   // 20..35
inline constexpr fui::ActionId ActionRemoveQuestionBase = 40;  // 40..55
inline constexpr fui::ActionId ActionHistorySelectBase = 60;   // 60..79

constexpr int kQuestionsPerPage = 3;
constexpr int kManageQuestionsPerPage = 4;
constexpr int kHistoryPerPage = 5;

void buildDaily(toybox::Screen& screen, const ::journal::Store& store, const char* displayDate,
                const char* displayHeader, int dayOffset, int page);

void buildQuestions(toybox::Screen& screen, const ::journal::Store& store, int page);

void buildHistory(toybox::Screen& screen, const ::journal::Store& store, int page);

}  // namespace journalui
