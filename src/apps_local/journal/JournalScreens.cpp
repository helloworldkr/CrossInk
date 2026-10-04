#include "JournalScreens.h"

#include <GfxRenderer.h>
#include <cstdio>
#include <cstring>
#include <string>

#include "../../components/themes/BaseTheme.h"
#include "../ui/ToyboxMetrics.h"
#include "../ui/ToyboxText.h"
#include "../ui/ToyboxTheme.h"

namespace journalui {

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
                              int maxLines = 2, fui::TextAlign align = fui::TextAlign::Left,
                              fui::Color color = fui::Color::Black) {
  fui::TextStyle style;
  style.font = font;
  style.align = align;
  style.color = color;
  style.maxLines = maxLines;
  const std::string fitted = toybox::fitLines(screen.target(), str, r.width, maxLines, style);
  screen.target().text(r, fitted.c_str(), style);
}

void drawCommonHeader(toybox::Screen& screen, const char* title, const char* statStr, int contentWidth, int leftMargin,
                      int rightMargin, int& outAfterHeaderY) {
  constexpr int topMargin = 14;
  constexpr int headerHeight = 34;
  const auto ink = fui::Paint::solid(fui::Color::Black);

  drawText(screen, fui::makeRect(leftMargin, topMargin, contentWidth - 160, headerHeight), title, toybox::kUiFont);
  if (statStr && statStr[0] != '\0') {
    drawText(screen, fui::makeRect(rightMargin - 160, topMargin, 160, headerHeight), statStr, toybox::kTileFont,
             fui::TextAlign::Right);
  }

  const int ruleY = topMargin + headerHeight + 2;
  screen.target().fill(fui::makeRect(leftMargin, ruleY, contentWidth, 2), ink);
  outAfterHeaderY = ruleY + 8;
}

void drawTabs(toybox::Screen& screen, int activeTab, int startY, int contentWidth, int leftMargin) {
  constexpr int tabH = 32;
  const int tabW = (contentWidth - 12) / 3;
  const auto ink = fui::Paint::solid(fui::Color::Black);

  struct TabDef {
    const char* label;
    fui::ActionId action;
  };
  const TabDef tabs[3] = {
      {"DAILY", ActionTabDaily},
      {"HISTORY", ActionTabHistory},
      {"QUESTIONS", ActionTabQuestions},
  };

  for (int i = 0; i < 3; ++i) {
    const fui::Rect r = fui::makeRect(leftMargin + i * (tabW + 6), startY, tabW, tabH);
    if (activeTab == i) {
      screen.target().fill(r, ink, 4);
      drawText(screen, fui::makeRect(r.x, r.y + 6, r.width, 20), tabs[i].label, toybox::kSmallFont,
               fui::TextAlign::Center, fui::Color::White);
    } else {
      screen.target().stroke(r, ink, 1, 4);
      drawText(screen, fui::makeRect(r.x, r.y + 6, r.width, 20), tabs[i].label, toybox::kSmallFont,
               fui::TextAlign::Center, fui::Color::Black);
    }
    addBtn(screen, tabs[i].action, r);
  }
}

void drawPagination(toybox::Screen& screen, int curPage, int totalPages, int deviceHeight, int contentWidth,
                    int leftMargin, int rightMargin) {
  if (totalPages <= 1) return;
  const auto ink = fui::Paint::solid(fui::Color::Black);
  const int pageBarY = deviceHeight - 52;
  constexpr int pageBarH = 38;

  if (curPage > 0) {
    const fui::Rect pPrev = fui::makeRect(leftMargin, pageBarY, 110, pageBarH);
    screen.target().stroke(pPrev, ink, 1, 4);
    drawText(screen, fui::makeRect(pPrev.x, pPrev.y + 7, pPrev.width, 24), "^ PREV", toybox::kTileFont,
             fui::TextAlign::Center);
    addBtn(screen, ActionPagePrev, pPrev);
  }

  char pageBuf[32];
  std::snprintf(pageBuf, sizeof(pageBuf), "Page %d of %d", curPage + 1, totalPages);
  drawText(screen, fui::makeRect(leftMargin + 120, pageBarY + 8, contentWidth - 240, 24), pageBuf, toybox::kTileFont,
           fui::TextAlign::Center);

  if (curPage < totalPages - 1) {
    const fui::Rect pNext = fui::makeRect(rightMargin - 110, pageBarY, 110, pageBarH);
    screen.target().stroke(pNext, ink, 1, 4);
    drawText(screen, fui::makeRect(pNext.x, pNext.y + 7, pNext.width, 24), "NEXT v", toybox::kTileFont,
             fui::TextAlign::Center);
    addBtn(screen, ActionPageNext, pNext);
  }
}

}  // namespace

void buildDaily(toybox::Screen& screen, const ::journal::Store& store, const char* displayDate,
                const char* displayHeader, int dayOffset, int page) {
  toybox::absoluteChrome(screen);
  const fui::DeviceContext& device = screen.device();
  const auto ink = fui::Paint::solid(fui::Color::Black);

  const int leftMargin = toybox::kMargin;
  const int rightMargin = device.width - toybox::kMargin;
  const int contentWidth = rightMargin - leftMargin;

  int answered = 0, total = 0;
  store.getScore(answered, total);
  char scoreBuf[32];
  if (total == 0) {
    std::strcpy(scoreBuf, "NO QUESTIONS");
  } else if (answered == total) {
    std::snprintf(scoreBuf, sizeof(scoreBuf), "%d/%d ALL DONE", answered, total);
  } else {
    std::snprintf(scoreBuf, sizeof(scoreBuf), "%d / %d LOGGED", answered, total);
  }

  int navY = 0;
  drawCommonHeader(screen, "JOURNAL", scoreBuf, contentWidth, leftMargin, rightMargin, navY);

  constexpr int navH = 38;
  const fui::Rect prevRect = fui::makeRect(leftMargin, navY, 40, navH);
  screen.target().stroke(prevRect, ink, 1, 4);
  drawText(screen, fui::makeRect(prevRect.x, prevRect.y + 4, prevRect.width, 26), "<", toybox::kUiFont,
           fui::TextAlign::Center);
  addBtn(screen, ActionDayPrev, prevRect);

  if (dayOffset < 0) {
    const fui::Rect nextRect = fui::makeRect(rightMargin - 40, navY, 40, navH);
    screen.target().stroke(nextRect, ink, 1, 4);
    drawText(screen, fui::makeRect(nextRect.x, nextRect.y + 4, nextRect.width, 26), ">", toybox::kUiFont,
             fui::TextAlign::Center);
    addBtn(screen, ActionDayNext, nextRect);

    const fui::Rect todayShortcutRect = fui::makeRect(rightMargin - 40 - 74, navY + 3, 68, navH - 6);
    screen.target().fill(todayShortcutRect, ink, 4);
    drawText(screen, fui::makeRect(todayShortcutRect.x, todayShortcutRect.y + 6, todayShortcutRect.width, 18), "TODAY",
             toybox::kSmallFont, fui::TextAlign::Center, fui::Color::White);
    addBtn(screen, ActionGoToday, todayShortcutRect);
  }

  const int dateTitleX = leftMargin + 48;
  const int dateTitleW = (dayOffset < 0) ? (contentWidth - 48 - 120) : (contentWidth - 48);
  drawText(screen, fui::makeRect(dateTitleX, navY + 7, dateTitleW, 24), displayHeader, toybox::kTileFont);

  const int tabY = navY + navH + 8;
  drawTabs(screen, 0, tabY, contentWidth, leftMargin);

  const int questionCount = store.questionCount();
  const int totalPages = (questionCount + kQuestionsPerPage - 1) / kQuestionsPerPage;
  const int curPage = (page < 0) ? 0 : (page >= totalPages ? (totalPages > 0 ? totalPages - 1 : 0) : page);
  const int startIndex = curPage * kQuestionsPerPage;
  const int endIndex = (startIndex + kQuestionsPerPage < questionCount) ? startIndex + kQuestionsPerPage : questionCount;

  int currentY = tabY + 32 + 12;
  constexpr int cardH = 145;
  constexpr int cardGap = 12;

  if (questionCount == 0) {
    const fui::Rect emptyRect = fui::makeRect(leftMargin, currentY + 40, contentWidth, 100);
    screen.target().stroke(emptyRect, ink, 1, 6);
    drawText(screen, fui::makeRect(emptyRect.x, emptyRect.y + 35, emptyRect.width, 24), "No questions configured.",
             toybox::kTileFont, fui::TextAlign::Center);
  } else {
    for (int i = startIndex; i < endIndex; ++i) {
      const auto* q = store.questionAt(i);
      if (!q) continue;

      const char* ans = store.getAnswerAt(i);
      const bool hasAnswer = (ans && ans[0] != '\0');

      const fui::Rect cardBox = fui::makeRect(leftMargin, currentY, contentWidth, cardH);
      screen.target().stroke(cardBox, ink, hasAnswer ? 2 : 1, 6);

      char qHeaderBuf[32];
      std::snprintf(qHeaderBuf, sizeof(qHeaderBuf), "QUESTION %d", i + 1);
      drawText(screen, fui::makeRect(cardBox.x + 14, cardBox.y + 10, 120, 20), qHeaderBuf, toybox::kSmallFont,
               fui::TextAlign::Left);

      if (hasAnswer) {
        const fui::Rect badgeRect = fui::makeRect(cardBox.right() - 84, cardBox.y + 8, 70, 20);
        screen.target().fill(badgeRect, ink, 3);
        drawText(screen, fui::makeRect(badgeRect.x, badgeRect.y + 2, badgeRect.width, 16), "ANSWERED",
                 toybox::kSmallFont, fui::TextAlign::Center, fui::Color::White);
      }

      drawMultiLineText(screen, fui::makeRect(cardBox.x + 14, cardBox.y + 32, contentWidth - 28, 44), q->text,
                        toybox::kTileFont, 2);

      screen.target().fill(fui::makeRect(cardBox.x + 14, cardBox.y + 78, contentWidth - 28, 1), ink);

      if (hasAnswer) {
        char ansPrefixed[journal::kAnswerMax + 8];
        std::snprintf(ansPrefixed, sizeof(ansPrefixed), "> %s", ans);
        drawMultiLineText(screen, fui::makeRect(cardBox.x + 14, cardBox.y + 86, contentWidth - 28, 48), ansPrefixed,
                          toybox::kTileFont, 2, fui::TextAlign::Left);
      } else {
        drawText(screen, fui::makeRect(cardBox.x + 14, cardBox.y + 96, contentWidth - 28, 24),
                 "+ Tap here to write your answer...", toybox::kTileFont, fui::TextAlign::Left);
      }

      addBtn(screen, ActionQuestionCardBase + i, cardBox);
      currentY += cardH + cardGap;
    }
  }

  drawPagination(screen, curPage, totalPages, device.height, contentWidth, leftMargin, rightMargin);
}

void buildQuestions(toybox::Screen& screen, const ::journal::Store& store, int page) {
  toybox::absoluteChrome(screen);
  const fui::DeviceContext& device = screen.device();
  const auto ink = fui::Paint::solid(fui::Color::Black);

  const int leftMargin = toybox::kMargin;
  const int rightMargin = device.width - toybox::kMargin;
  const int contentWidth = rightMargin - leftMargin;

  char qCountBuf[32];
  std::snprintf(qCountBuf, sizeof(qCountBuf), "%d QUESTIONS", store.questionCount());

  int tabY = 0;
  drawCommonHeader(screen, "MANAGE QUESTIONS", qCountBuf, contentWidth, leftMargin, rightMargin, tabY);
  drawTabs(screen, 2, tabY, contentWidth, leftMargin);

  const int questionCount = store.questionCount();
  const int totalPages = (questionCount + kManageQuestionsPerPage - 1) / kManageQuestionsPerPage;
  const int curPage = (page < 0) ? 0 : (page >= totalPages ? (totalPages > 0 ? totalPages - 1 : 0) : page);
  const int startIndex = curPage * kManageQuestionsPerPage;
  const int endIndex =
      (startIndex + kManageQuestionsPerPage < questionCount) ? startIndex + kManageQuestionsPerPage : questionCount;

  int currentY = tabY + 32 + 12;
  constexpr int rowH = 88;
  constexpr int rowGap = 10;

  for (int i = startIndex; i < endIndex; ++i) {
    const auto* q = store.questionAt(i);
    if (!q) continue;

    const fui::Rect rowBox = fui::makeRect(leftMargin, currentY, contentWidth, rowH);
    screen.target().stroke(rowBox, ink, 1, 6);

    char numBuf[16];
    std::snprintf(numBuf, sizeof(numBuf), "%d.", i + 1);
    drawText(screen, fui::makeRect(rowBox.x + 12, rowBox.y + 10, 28, 24), numBuf, toybox::kTileFont);

    drawMultiLineText(screen, fui::makeRect(rowBox.x + 42, rowBox.y + 10, contentWidth - 145, 68), q->text,
                      toybox::kTileFont, 3);

    const fui::Rect removeBtnRect = fui::makeRect(rowBox.right() - 86, rowBox.y + 24, 76, 38);
    screen.target().stroke(removeBtnRect, ink, 1, 4);
    drawText(screen, fui::makeRect(removeBtnRect.x, removeBtnRect.y + 8, removeBtnRect.width, 20), "REMOVE",
             toybox::kSmallFont, fui::TextAlign::Center);
    addBtn(screen, ActionRemoveQuestionBase + i, removeBtnRect);

    currentY += rowH + rowGap;
  }

  const int actionY = device.height - 110;
  constexpr int btnH = 42;

  const fui::Rect addBtnRect = fui::makeRect(leftMargin, actionY, contentWidth, btnH);
  screen.target().fill(addBtnRect, ink, 4);
  drawText(screen, fui::makeRect(addBtnRect.x, addBtnRect.y + 10, addBtnRect.width, 24), "+ ADD NEW QUESTION",
           toybox::kTileFont, fui::TextAlign::Center, fui::Color::White);
  addBtn(screen, ActionAddQuestion, addBtnRect);

  const fui::Rect resetBtnRect = fui::makeRect(leftMargin, actionY + btnH + 10, contentWidth, 36);
  screen.target().stroke(resetBtnRect, ink, 1, 4);
  drawText(screen, fui::makeRect(resetBtnRect.x, resetBtnRect.y + 8, resetBtnRect.width, 20),
           "RESET TO DEFAULT 6 QUESTIONS", toybox::kSmallFont, fui::TextAlign::Center);
  addBtn(screen, ActionResetDefaults, resetBtnRect);
}

void buildHistory(toybox::Screen& screen, const ::journal::Store& store, int page) {
  toybox::absoluteChrome(screen);
  const fui::DeviceContext& device = screen.device();
  const auto ink = fui::Paint::solid(fui::Color::Black);

  const int leftMargin = toybox::kMargin;
  const int rightMargin = device.width - toybox::kMargin;
  const int contentWidth = rightMargin - leftMargin;

  char hCountBuf[32];
  std::snprintf(hCountBuf, sizeof(hCountBuf), "%d LOGGED DAYS", store.historyCount());

  int tabY = 0;
  drawCommonHeader(screen, "PAST ENTRIES", hCountBuf, contentWidth, leftMargin, rightMargin, tabY);
  drawTabs(screen, 1, tabY, contentWidth, leftMargin);

  const int historyCount = store.historyCount();
  const int totalPages = (historyCount + kHistoryPerPage - 1) / kHistoryPerPage;
  const int curPage = (page < 0) ? 0 : (page >= totalPages ? (totalPages > 0 ? totalPages - 1 : 0) : page);
  const int startIndex = curPage * kHistoryPerPage;
  const int endIndex = (startIndex + kHistoryPerPage < historyCount) ? startIndex + kHistoryPerPage : historyCount;

  int currentY = tabY + 32 + 12;
  constexpr int itemH = 76;
  constexpr int itemGap = 10;

  if (historyCount == 0) {
    const fui::Rect emptyRect = fui::makeRect(leftMargin, currentY + 60, contentWidth, 110);
    screen.target().stroke(emptyRect, ink, 1, 6);
    drawText(screen, fui::makeRect(emptyRect.x, emptyRect.y + 30, emptyRect.width, 24), "No past journal entries yet.",
             toybox::kTileFont, fui::TextAlign::Center);
    drawText(screen, fui::makeRect(emptyRect.x, emptyRect.y + 60, emptyRect.width, 20),
             "Answer today's questions to create your first entry!", toybox::kSmallFont, fui::TextAlign::Center);
  } else {
    for (int i = startIndex; i < endIndex; ++i) {
      const auto* item = store.historyAt(i);
      if (!item) continue;

      const fui::Rect itemBox = fui::makeRect(leftMargin, currentY, contentWidth, itemH);
      screen.target().stroke(itemBox, ink, 1, 6);

      drawText(screen, fui::makeRect(itemBox.x + 16, itemBox.y + 14, 200, 24), item->date, toybox::kTileFont);

      char statsBuf[48];
      std::snprintf(statsBuf, sizeof(statsBuf), "%d / %d questions answered", item->answered, item->total);
      drawText(screen, fui::makeRect(itemBox.x + 16, itemBox.y + 42, 240, 20), statsBuf, toybox::kSmallFont);

      const fui::Rect viewBtn = fui::makeRect(itemBox.right() - 86, itemBox.y + 18, 72, 38);
      screen.target().fill(viewBtn, ink, 4);
      drawText(screen, fui::makeRect(viewBtn.x, viewBtn.y + 9, viewBtn.width, 20), "VIEW >", toybox::kSmallFont,
               fui::TextAlign::Center, fui::Color::White);

      addBtn(screen, ActionHistorySelectBase + i, itemBox);
      currentY += itemH + itemGap;
    }
  }

  drawPagination(screen, curPage, totalPages, device.height, contentWidth, leftMargin, rightMargin);
}

}  // namespace journalui
