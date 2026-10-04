#pragma once

#include <cstdint>

#include "../ui/ToyboxScreen.h"
#include "SanyamCore.h"

namespace fui = freeink::ui;

namespace sanyamui {

// Navigation / Tabs
inline constexpr fui::ActionId ActionTabAll = 1;
inline constexpr fui::ActionId ActionTabYama = 2;
inline constexpr fui::ActionId ActionTabNiyama = 3;
inline constexpr fui::ActionId ActionTabObstacle = 4;
inline constexpr fui::ActionId ActionTabSymptom = 5;

// Date navigation
inline constexpr fui::ActionId ActionDayPrev = 6;
inline constexpr fui::ActionId ActionDayNext = 7;
inline constexpr fui::ActionId ActionGoToday = 8;

// Views
inline constexpr fui::ActionId ActionSwitchToList = 9;
inline constexpr fui::ActionId ActionOpenSutra = 10;
inline constexpr fui::ActionId ActionCloseSutra = 11;
inline constexpr fui::ActionId ActionNextSutra = 12;
inline constexpr fui::ActionId ActionPrevSutra = 13;

// Item navigation in Card view
inline constexpr fui::ActionId ActionPrevItem = 14;
inline constexpr fui::ActionId ActionNextItem = 15;

// Rating choices in Card view
inline constexpr fui::ActionId ActionRate1 = 20;
inline constexpr fui::ActionId ActionRate2 = 21;
inline constexpr fui::ActionId ActionRate3 = 22;

// Pagination in List view
inline constexpr fui::ActionId ActionPagePrev = 30;
inline constexpr fui::ActionId ActionPageNext = 31;

// Item tap in List view
inline constexpr fui::ActionId ActionItemCardBase = 50;  // 50 .. 73

constexpr int kItemsPerPage = 4;

void buildCardView(toybox::Screen& screen, const ::sanyam::Store& store, int itemIndex,
                   const char* displayDate, const char* displayHeader, int dayOffset);

void buildListView(toybox::Screen& screen, const ::sanyam::Store& store, int activeTab, int page,
                   const char* displayDate, const char* displayHeader, int dayOffset);

void buildSutraView(toybox::Screen& screen, int sutraIndex);

}  // namespace sanyamui
