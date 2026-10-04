#include "SanyamScreens.h"

#include <cstdio>
#include <cstring>
#include <string>

#include "../ui/ToyboxMetrics.h"
#include "../ui/ToyboxText.h"
#include "../ui/ToyboxTokens.h"

namespace sanyamui {

namespace {

inline void addBtn(toybox::Screen& screen, fui::ActionId action, const fui::Rect& rect) {
  fui::ButtonProps b;
  b.action = action;
  b.enabled = true;
  b.styles = fui::flatButtonStyles(0);
  screen.button(b, rect);
}

inline void drawText(toybox::Screen& screen, const fui::Rect& r, const char* str, fui::FontId font,
                     fui::TextAlign align = fui::TextAlign::Left, fui::Color color = fui::Color::Black) {
  fui::TextStyle style;
  style.font = font;
  style.align = align;
  style.color = color;
  style.maxLines = 1;
  screen.target().text(r, str, style);
}

inline void drawMultiLineText(toybox::Screen& screen, const fui::Rect& r, const char* str, fui::FontId font,
                              int maxLines = 4, fui::TextAlign align = fui::TextAlign::Left,
                              fui::Color color = fui::Color::Black) {
  fui::TextStyle style;
  style.font = font;
  style.align = align;
  style.color = color;
  style.maxLines = maxLines;
  const std::string fitted = toybox::fitLines(screen.target(), str, r.width, maxLines, style);
  screen.target().text(r, fitted.c_str(), style);
}

int getCategoryIndices(int tab, int outIndices[sanyam::kTotalItems]) {
  int count = 0;
  for (int i = 0; i < sanyam::kTotalItems; ++i) {
    if (tab == 0) {  // ALL
      outIndices[count++] = i;
    } else if (tab == 1 && sanyam::kItems[i].category == sanyam::Category::Yama) {
      outIndices[count++] = i;
    } else if (tab == 2 && sanyam::kItems[i].category == sanyam::Category::Niyama) {
      outIndices[count++] = i;
    } else if (tab == 3 && sanyam::kItems[i].category == sanyam::Category::Antaraya) {
      outIndices[count++] = i;
    } else if (tab == 4 && sanyam::kItems[i].category == sanyam::Category::Sahabhuva) {
      outIndices[count++] = i;
    }
  }
  return count;
}

}  // namespace

void buildCardView(toybox::Screen& screen, const ::sanyam::Store& store, int itemIndex,
                   const char* displayDate, const char* displayHeader, int dayOffset) {
  constexpr int leftMargin = 16;
  constexpr int rightMargin = 480 - 16;
  constexpr int contentWidth = rightMargin - leftMargin;
  const auto ink = fui::Paint::solid(fui::Color::Black);

  if (itemIndex < 0 || itemIndex >= sanyam::kTotalItems) itemIndex = 0;
  const auto& item = sanyam::kItems[itemIndex];
  const int currentRating = store.getRating(itemIndex);

  int loggedCount = 0, totalItems = 0;
  store.getScore(loggedCount, totalItems);

  // 1. Top Header (y = 12, h = 34)
  // [ < LIST ] button
  const fui::Rect listBtnRect = fui::makeRect(leftMargin, 12, 84, 30);
  screen.target().stroke(listBtnRect, ink, 1, 4);
  drawText(screen, fui::makeRect(leftMargin, 16, 84, 22), "< LIST", toybox::kTileFont, fui::TextAlign::Center);
  addBtn(screen, ActionSwitchToList, listBtnRect);

  // Date Navigation in Middle
  const int dateGroupX = leftMargin + 96;
  const fui::Rect prevDayRect = fui::makeRect(dateGroupX, 12, 30, 30);
  screen.target().stroke(prevDayRect, ink, 1, 4);
  drawText(screen, fui::makeRect(dateGroupX, 16, 30, 22), "<", toybox::kTileFont, fui::TextAlign::Center);
  addBtn(screen, ActionDayPrev, prevDayRect);

  char dateStr[32] = {};
  if (dayOffset == 0) {
    std::snprintf(dateStr, sizeof(dateStr), "TODAY");
  } else {
    std::strncpy(dateStr, displayHeader ? displayHeader : displayDate, sizeof(dateStr) - 1);
  }
  drawText(screen, fui::makeRect(dateGroupX + 36, 16, 110, 22), dateStr, toybox::kTileFont, fui::TextAlign::Center);

  const fui::Rect nextDayRect = fui::makeRect(dateGroupX + 152, 12, 30, 30);
  screen.target().stroke(nextDayRect, ink, 1, 4);
  drawText(screen, fui::makeRect(dateGroupX + 152, 16, 30, 22), ">", toybox::kTileFont, fui::TextAlign::Center);
  addBtn(screen, ActionDayNext, nextDayRect);

  // Progress Pill on Right
  char scoreStr[24];
  std::snprintf(scoreStr, sizeof(scoreStr), "%d/%d", loggedCount, totalItems);
  const fui::Rect scoreRect = fui::makeRect(rightMargin - 70, 12, 70, 30);
  screen.target().stroke(scoreRect, ink, 1, 4);
  drawText(screen, fui::makeRect(rightMargin - 70, 16, 70, 22), scoreStr, toybox::kTileFont, fui::TextAlign::Center);

  // Divider
  screen.target().fill(fui::makeRect(leftMargin, 48, contentWidth, 2), ink);

  // 2. Category Pill & Counter
  const fui::Rect catPillRect = fui::makeRect(leftMargin, 56, 130, 24);
  screen.target().fill(catPillRect, ink, 4);
  drawText(screen, fui::makeRect(leftMargin, 59, 130, 20), item.categoryLabel, toybox::kTileFont,
           fui::TextAlign::Center, fui::Color::White);

  char itemCounterStr[32];
  std::snprintf(itemCounterStr, sizeof(itemCounterStr), "Item %d of %d", itemIndex + 1, sanyam::kTotalItems);
  drawText(screen, fui::makeRect(rightMargin - 120, 59, 120, 20), itemCounterStr, toybox::kTileFont,
           fui::TextAlign::Right);

  // 3. Item Name (Bold)
  drawText(screen, fui::makeRect(leftMargin, 86, contentWidth, 32), item.name, toybox::kUiFont);

  // 4. Benefit Subtitle
  char benefitBuf[160];
  std::snprintf(benefitBuf, sizeof(benefitBuf), "Benefit: %s", item.benefit);
  drawMultiLineText(screen, fui::makeRect(leftMargin, 122, contentWidth, 36), benefitBuf, toybox::kTileFont, 2);

  // 5. Daily Reflection Box
  const int refY = 166;
  const int refH = 144;
  const fui::Rect refBox = fui::makeRect(leftMargin, refY, contentWidth, refH);
  screen.target().stroke(refBox, ink, 2, 6);
  drawText(screen, fui::makeRect(leftMargin + 12, refY + 10, contentWidth - 24, 20), "DAILY CONTEMPLATION:",
           toybox::kTileFont, fui::TextAlign::Left);
  drawMultiLineText(screen, fui::makeRect(leftMargin + 12, refY + 34, contentWidth - 24, refH - 44), item.questions,
                    toybox::kSmallFont, 4);

  // 6. Practice / How to Follow Box
  const int pracY = refY + refH + 12;
  const int pracH = 132;
  const fui::Rect pracBox = fui::makeRect(leftMargin, pracY, contentWidth, pracH);
  screen.target().stroke(pracBox, ink, 1, 6);
  drawText(screen, fui::makeRect(leftMargin + 12, pracY + 10, contentWidth - 24, 20), "PRACTICE WISDOM:",
           toybox::kTileFont, fui::TextAlign::Left);
  drawMultiLineText(screen, fui::makeRect(leftMargin + 12, pracY + 34, contentWidth - 24, pracH - 44),
                    item.howToFollow, toybox::kSmallFont, 4);

  // 7. Rating Choices
  const int rateY = pracY + pracH + 14;
  drawText(screen, fui::makeRect(leftMargin, rateY, contentWidth, 22), "LOG YOUR PRACTICE TODAY:",
           toybox::kTileFont);

  const int optW = (contentWidth - 16) / 3;
  const int optH = 46;
  const int btnRowY = rateY + 26;

  struct OptBtn {
    int val;
    const char* label;
    fui::ActionId action;
  };
  const OptBtn opts[3] = {
      {1, item.opt1, ActionRate1},
      {2, item.opt2, ActionRate2},
      {3, item.opt3, ActionRate3},
  };

  for (int i = 0; i < 3; ++i) {
    const int bx = leftMargin + i * (optW + 8);
    const fui::Rect bRect = fui::makeRect(bx, btnRowY, optW, optH);
    const bool isSelected = (currentRating == opts[i].val);

    if (isSelected) {
      screen.target().fill(bRect, ink, 6);
      drawText(screen, fui::makeRect(bx, btnRowY + 13, optW, 22), opts[i].label, toybox::kTileFont,
               fui::TextAlign::Center, fui::Color::White);
    } else {
      screen.target().stroke(bRect, ink, 1, 6);
      drawText(screen, fui::makeRect(bx, btnRowY + 13, optW, 22), opts[i].label, toybox::kTileFont,
               fui::TextAlign::Center, fui::Color::Black);
    }
    addBtn(screen, opts[i].action, bRect);
  }

  // 8. Bottom Navigation Bar
  const int botY = 742;
  const fui::Rect prevBtnRect = fui::makeRect(leftMargin, botY, 110, 42);
  screen.target().stroke(prevBtnRect, ink, 1, 6);
  drawText(screen, fui::makeRect(leftMargin, botY + 11, 110, 22), "< PREV", toybox::kTileFont,
           fui::TextAlign::Center);
  addBtn(screen, ActionPrevItem, prevBtnRect);

  const int sutraW = 120;
  const int sutraX = (480 - sutraW) / 2;
  const fui::Rect sutraBtnRect = fui::makeRect(sutraX, botY, sutraW, 42);
  screen.target().stroke(sutraBtnRect, ink, 1, 6);
  drawText(screen, fui::makeRect(sutraX, botY + 11, sutraW, 22), "SUTRA WISDOM", toybox::kTileFont,
           fui::TextAlign::Center);
  addBtn(screen, ActionOpenSutra, sutraBtnRect);

  const fui::Rect nextBtnRect = fui::makeRect(rightMargin - 110, botY, 110, 42);
  screen.target().stroke(nextBtnRect, ink, 1, 6);
  drawText(screen, fui::makeRect(rightMargin - 110, botY + 11, 110, 22), "NEXT >", toybox::kTileFont,
           fui::TextAlign::Center);
  addBtn(screen, ActionNextItem, nextBtnRect);
}

void buildListView(toybox::Screen& screen, const ::sanyam::Store& store, int activeTab, int page,
                   const char* displayDate, const char* displayHeader, int dayOffset) {
  constexpr int leftMargin = 16;
  constexpr int rightMargin = 480 - 16;
  constexpr int contentWidth = rightMargin - leftMargin;
  const auto ink = fui::Paint::solid(fui::Color::Black);

  int loggedCount = 0, totalItems = 0;
  store.getScore(loggedCount, totalItems);

  // 1. Top Header
  // [ < EXIT ] button
  const fui::Rect exitBtnRect = fui::makeRect(leftMargin, 12, 80, 30);
  screen.target().stroke(exitBtnRect, ink, 1, 4);
  drawText(screen, fui::makeRect(leftMargin, 16, 80, 22), "< EXIT", toybox::kTileFont, fui::TextAlign::Center);
  addBtn(screen, ActionSwitchToList, exitBtnRect);

  // Date Navigation in Middle
  const int dateGroupX = leftMargin + 92;
  const fui::Rect prevDayRect = fui::makeRect(dateGroupX, 12, 30, 30);
  screen.target().stroke(prevDayRect, ink, 1, 4);
  drawText(screen, fui::makeRect(dateGroupX, 16, 30, 22), "<", toybox::kTileFont, fui::TextAlign::Center);
  addBtn(screen, ActionDayPrev, prevDayRect);

  char dateStr[32] = {};
  if (dayOffset == 0) {
    std::snprintf(dateStr, sizeof(dateStr), "TODAY");
  } else {
    std::strncpy(dateStr, displayHeader ? displayHeader : displayDate, sizeof(dateStr) - 1);
  }
  drawText(screen, fui::makeRect(dateGroupX + 34, 16, 114, 22), dateStr, toybox::kTileFont, fui::TextAlign::Center);

  const fui::Rect nextDayRect = fui::makeRect(dateGroupX + 152, 12, 30, 30);
  screen.target().stroke(nextDayRect, ink, 1, 4);
  drawText(screen, fui::makeRect(dateGroupX + 152, 16, 30, 22), ">", toybox::kTileFont, fui::TextAlign::Center);
  addBtn(screen, ActionDayNext, nextDayRect);

  // [ TODAY ] button on Right
  if (dayOffset != 0) {
    const fui::Rect todayBtnRect = fui::makeRect(rightMargin - 70, 12, 70, 30);
    screen.target().stroke(todayBtnRect, ink, 1, 4);
    drawText(screen, fui::makeRect(rightMargin - 70, 16, 70, 22), "TODAY", toybox::kTileFont, fui::TextAlign::Center);
    addBtn(screen, ActionGoToday, todayBtnRect);
  } else {
    const fui::Rect sutraBtnRect = fui::makeRect(rightMargin - 70, 12, 70, 30);
    screen.target().stroke(sutraBtnRect, ink, 1, 4);
    drawText(screen, fui::makeRect(rightMargin - 70, 16, 70, 22), "SUTRA", toybox::kTileFont, fui::TextAlign::Center);
    addBtn(screen, ActionOpenSutra, sutraBtnRect);
  }

  // Divider
  screen.target().fill(fui::makeRect(leftMargin, 48, contentWidth, 2), ink);

  // 2. Progress Banner
  char statStr[48];
  if (loggedCount == totalItems) {
    std::snprintf(statStr, sizeof(statStr), "ALL %d CONTEMPLATIONS COMPLETED!", totalItems);
  } else {
    std::snprintf(statStr, sizeof(statStr), "%d OF %d CONTEMPLATED TODAY", loggedCount, totalItems);
  }
  drawText(screen, fui::makeRect(leftMargin, 56, contentWidth, 22), statStr, toybox::kTileFont);

  // Progress Bar
  const int barH = 6;
  const fui::Rect barBg = fui::makeRect(leftMargin, 82, contentWidth, barH);
  screen.target().stroke(barBg, ink, 1, 2);
  if (loggedCount > 0 && totalItems > 0) {
    const int fillW = (contentWidth * loggedCount) / totalItems;
    screen.target().fill(fui::makeRect(leftMargin, 82, fillW, barH), ink);
  }

  // 3. Category Filter Tabs (y = 96, h = 34)
  const char* const tabLabels[5] = {"ALL", "YAMA", "NIYAMA", "OBSTACLE", "SYMPTOM"};
  const int tabW = (contentWidth - 16) / 5;
  const int tabY = 96;

  for (int i = 0; i < 5; ++i) {
    const int tx = leftMargin + i * (tabW + 4);
    const fui::Rect tRect = fui::makeRect(tx, tabY, tabW, 30);
    const bool isTabActive = (activeTab == i);

    if (isTabActive) {
      screen.target().fill(tRect, ink, 4);
      drawText(screen, fui::makeRect(tx, tabY + 6, tabW, 20), tabLabels[i], toybox::kTileFont,
               fui::TextAlign::Center, fui::Color::White);
    } else {
      screen.target().stroke(tRect, ink, 1, 4);
      drawText(screen, fui::makeRect(tx, tabY + 6, tabW, 20), tabLabels[i], toybox::kTileFont,
               fui::TextAlign::Center, fui::Color::Black);
    }
    addBtn(screen, ActionTabAll + i, tRect);
  }

  // 4. Item List Cards
  int tabIndices[sanyam::kTotalItems];
  const int tabItemCount = getCategoryIndices(activeTab, tabIndices);

  const int totalPages = (tabItemCount + kItemsPerPage - 1) / kItemsPerPage;
  if (page < 0) page = 0;
  if (page >= totalPages && totalPages > 0) page = totalPages - 1;

  const int startIndex = page * kItemsPerPage;
  const int endIndex = (startIndex + kItemsPerPage < tabItemCount) ? (startIndex + kItemsPerPage) : tabItemCount;

  const int listStartY = 138;
  const int cardH = 136;
  const int cardGap = 10;

  for (int i = startIndex; i < endIndex; ++i) {
    const int globalIdx = tabIndices[i];
    const auto& item = sanyam::kItems[globalIdx];
    const int rating = store.getRating(globalIdx);

    const int cy = listStartY + (i - startIndex) * (cardH + cardGap);
    const fui::Rect cardRect = fui::makeRect(leftMargin, cy, contentWidth, cardH);
    screen.target().stroke(cardRect, ink, 1, 6);

    // Left Category Pill
    const fui::Rect pillRect = fui::makeRect(leftMargin + 10, cy + 10, 96, 22);
    screen.target().fill(pillRect, ink, 4);
    drawText(screen, fui::makeRect(leftMargin + 10, cy + 12, 96, 18), item.categoryLabel, toybox::kTileFont,
             fui::TextAlign::Center, fui::Color::White);

    // Right Rating Badge
    const fui::Rect ratingBadgeRect = fui::makeRect(rightMargin - 110, cy + 10, 100, 24);
    if (rating == 1) {
      screen.target().fill(ratingBadgeRect, ink, 4);
      drawText(screen, fui::makeRect(rightMargin - 110, cy + 13, 100, 20), item.opt1, toybox::kTileFont,
               fui::TextAlign::Center, fui::Color::White);
    } else if (rating == 2) {
      screen.target().fill(ratingBadgeRect, ink, 4);
      drawText(screen, fui::makeRect(rightMargin - 110, cy + 13, 100, 20), item.opt2, toybox::kTileFont,
               fui::TextAlign::Center, fui::Color::White);
    } else if (rating == 3) {
      screen.target().fill(ratingBadgeRect, ink, 4);
      drawText(screen, fui::makeRect(rightMargin - 110, cy + 13, 100, 20), item.opt3, toybox::kTileFont,
               fui::TextAlign::Center, fui::Color::White);
    } else {
      screen.target().stroke(ratingBadgeRect, ink, 1, 4);
      drawText(screen, fui::makeRect(rightMargin - 110, cy + 13, 100, 20), "[ UNLOGGED ]", toybox::kTileFont,
               fui::TextAlign::Center, fui::Color::Black);
    }

    // Title
    drawText(screen, fui::makeRect(leftMargin + 10, cy + 38, contentWidth - 20, 26), item.name, toybox::kUiFont);

    // Question snippet
    drawMultiLineText(screen, fui::makeRect(leftMargin + 10, cy + 68, contentWidth - 20, cardH - 74),
                      item.questions, toybox::kSmallFont, 2);

    addBtn(screen, ActionItemCardBase + globalIdx, cardRect);
  }

  // 5. Bottom Pagination Bar (y = 736, h = 42)
  const int botY = 736;
  const fui::Rect prevPageRect = fui::makeRect(leftMargin, botY, 120, 42);
  screen.target().stroke(prevPageRect, ink, 1, 6);
  drawText(screen, fui::makeRect(leftMargin, botY + 11, 120, 22), "< PREV PAGE", toybox::kTileFont,
           fui::TextAlign::Center);
  addBtn(screen, ActionPagePrev, prevPageRect);

  char pageStr[32];
  std::snprintf(pageStr, sizeof(pageStr), "Page %d of %d", page + 1, totalPages > 0 ? totalPages : 1);
  drawText(screen, fui::makeRect(leftMargin + 130, botY + 11, contentWidth - 260, 22), pageStr, toybox::kTileFont,
           fui::TextAlign::Center);

  const fui::Rect nextPageRect = fui::makeRect(rightMargin - 120, botY, 120, 42);
  screen.target().stroke(nextPageRect, ink, 1, 6);
  drawText(screen, fui::makeRect(rightMargin - 120, botY + 11, 120, 22), "NEXT PAGE >", toybox::kTileFont,
           fui::TextAlign::Center);
  addBtn(screen, ActionPageNext, nextPageRect);
}

void buildSutraView(toybox::Screen& screen, int sutraIndex) {
  constexpr int leftMargin = 20;
  constexpr int rightMargin = 480 - 20;
  constexpr int contentWidth = rightMargin - leftMargin;
  const auto ink = fui::Paint::solid(fui::Color::Black);

  if (sutraIndex < 0 || sutraIndex >= sanyam::kTotalQuotes) sutraIndex = 0;

  // Outer and Inner ornamental border
  screen.target().stroke(fui::makeRect(10, 10, 460, 780), ink, 3, 10);
  screen.target().stroke(fui::makeRect(16, 16, 448, 768), ink, 1, 8);

  // Header
  const fui::Rect badge = fui::makeRect((480 - 260) / 2, 40, 260, 36);
  screen.target().fill(badge, ink, 6);
  drawText(screen, fui::makeRect((480 - 260) / 2, 46, 260, 24), "PATANJALI YOGA SUTRA", toybox::kUiFont,
           fui::TextAlign::Center, fui::Color::White);

  char sutraCountStr[32];
  std::snprintf(sutraCountStr, sizeof(sutraCountStr), "SUTRA WISDOM %d OF %d", sutraIndex + 1, sanyam::kTotalQuotes);
  drawText(screen, fui::makeRect(leftMargin, 90, contentWidth, 24), sutraCountStr, toybox::kTileFont,
           fui::TextAlign::Center);

  // Quote poster body
  const fui::Rect quoteBox = fui::makeRect(leftMargin + 10, 160, contentWidth - 20, 380);
  screen.target().stroke(quoteBox, ink, 2, 8);

  drawText(screen, fui::makeRect(leftMargin + 20, 180, contentWidth - 40, 24), "SUTRA TEACHING:", toybox::kTileFont,
           fui::TextAlign::Left);

  drawMultiLineText(screen, fui::makeRect(leftMargin + 20, 220, contentWidth - 40, 300),
                    sanyam::kPatanjaliQuotes[sutraIndex], toybox::kUiFont, 8, fui::TextAlign::Left);

  // Bottom Navigation
  const int botNavY = 640;
  const fui::Rect prevBtn = fui::makeRect(leftMargin + 20, botNavY, 100, 42);
  screen.target().stroke(prevBtn, ink, 1, 6);
  drawText(screen, fui::makeRect(leftMargin + 20, botNavY + 11, 100, 22), "< PREV", toybox::kTileFont,
           fui::TextAlign::Center);
  addBtn(screen, ActionPrevSutra, prevBtn);

  const fui::Rect nextBtn = fui::makeRect(rightMargin - 120, botNavY, 100, 42);
  screen.target().stroke(nextBtn, ink, 1, 6);
  drawText(screen, fui::makeRect(rightMargin - 120, botNavY + 11, 100, 22), "NEXT >", toybox::kTileFont,
           fui::TextAlign::Center);
  addBtn(screen, ActionNextSutra, nextBtn);

  const int closeY = 702;
  const fui::Rect closeBtn = fui::makeRect((480 - 240) / 2, closeY, 240, 44);
  screen.target().fill(closeBtn, ink, 6);
  drawText(screen, fui::makeRect((480 - 240) / 2, closeY + 12, 240, 24), "BACK TO CONTEMPLATION",
           toybox::kTileFont, fui::TextAlign::Center, fui::Color::White);
  addBtn(screen, ActionCloseSutra, closeBtn);
}

}  // namespace sanyamui
