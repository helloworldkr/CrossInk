#include "HabitsScreens.h"

#include <GfxRenderer.h>
#include <cstdio>
#include <cstring>

#include "../../components/themes/BaseTheme.h"  // Rect
#include "../ui/ToyboxMetrics.h"
#include "../ui/ToyboxText.h"
#include "../ui/ToyboxTheme.h"

namespace habitsui {

namespace {

static inline void addBtn(toybox::Screen& screen, fui::ActionId action, const fui::Rect& rect) {
  fui::ButtonProps b;
  b.action = action;
  b.enabled = true;
  b.styles = fui::flatButtonStyles(0);
  screen.button(b, rect);
}

static inline void drawText(toybox::Screen& screen, const fui::Rect& r, const char* str, fui::FontId font,
                            fui::TextAlign align = fui::TextAlign::Left, fui::Color color = fui::Color::Black) {
  fui::TextStyle style;
  style.font = font;
  style.align = align;
  style.color = color;
  style.maxLines = 1;
  screen.target().text(r, str, style);
}

}  // namespace

void buildDaily(toybox::Screen& screen, const ::habits::Store& store, const char* displayDate,
                const char* displayHeader, int dayOffset) {
  toybox::absoluteChrome(screen);
  const fui::DeviceContext& device = screen.device();
  const auto ink = fui::Paint::solid(fui::Color::Black);

  const int leftMargin = toybox::kMargin;
  const int rightMargin = device.width - toybox::kMargin;
  const int contentWidth = rightMargin - leftMargin;

  // --- 1. App Header: HABITS                2 / 3 DONE ---
  constexpr int topMargin = 14;
  constexpr int headerHeight = 34;

  drawText(screen, fui::makeRect(leftMargin, topMargin, contentWidth - 130, headerHeight), "HABITS", toybox::kUiFont);

  int completed = 0, total = 0;
  store.getTodayScore(displayDate, completed, total);
  char scoreBuf[32];
  if (total == 0) {
    std::strcpy(scoreBuf, "NO HABITS");
  } else {
    std::snprintf(scoreBuf, sizeof(scoreBuf), "%d / %d %s", completed, total,
                  (completed == total) ? "ALL DONE" : "DONE");
  }
  drawText(screen, fui::makeRect(rightMargin - 140, topMargin, 140, headerHeight), scoreBuf, toybox::kTileFont,
           fui::TextAlign::Right);

  // Header separator rule
  const int ruleY = topMargin + headerHeight + 2;
  screen.target().fill(fui::makeRect(leftMargin, ruleY, contentWidth, 2), ink);

  // --- 2. Date Navigator Bar ---
  // [ < ]  TODAY, OCT 4           [ TODAY ] [ > ]
  const int navY = ruleY + 8;
  constexpr int navH = 42;

  // Prev day button [ < ]
  const fui::Rect prevRect = fui::makeRect(leftMargin, navY, 42, navH);
  screen.target().stroke(prevRect, ink, 1, 4);
  drawText(screen, fui::makeRect(prevRect.x, prevRect.y + 6, prevRect.width, 28), "<", toybox::kUiFont,
           fui::TextAlign::Center);
  addBtn(screen, ActionDayPrev, prevRect);

  // Next day button [ > ] and [ TODAY ] shortcut (only if dayOffset < 0)
  if (dayOffset < 0) {
    const fui::Rect nextRect = fui::makeRect(rightMargin - 42, navY, 42, navH);
    screen.target().stroke(nextRect, ink, 1, 4);
    drawText(screen, fui::makeRect(nextRect.x, nextRect.y + 6, nextRect.width, 28), ">", toybox::kUiFont,
             fui::TextAlign::Center);
    addBtn(screen, ActionDayNext, nextRect);

    // [ TODAY ] shortcut button
    const fui::Rect todayShortcutRect = fui::makeRect(rightMargin - 42 - 76, navY + 4, 70, navH - 8);
    screen.target().fill(todayShortcutRect, ink, 4);
    drawText(screen, fui::makeRect(todayShortcutRect.x, todayShortcutRect.y + 8, todayShortcutRect.width, 18), "TODAY",
             toybox::kSmallFont, fui::TextAlign::Center, fui::Color::White);
    addBtn(screen, ActionGoToday, todayShortcutRect);
  }

  // Date Title in middle
  const int dateTitleX = leftMargin + 50;
  const int dateTitleW = (dayOffset < 0) ? (contentWidth - 50 - 124) : (contentWidth - 50);
  drawText(screen, fui::makeRect(dateTitleX, navY + 10, dateTitleW, 24), displayHeader, toybox::kTileFont);

  // --- 3. Habit Cards (Up to 3) ---
  int currentY = navY + navH + 12;
  constexpr int cardHeight = 136;
  constexpr int cardGap = 12;

  for (int i = 0; i < habits::kMaxHabits; ++i) {
    const auto* h = store.habitAt(i);
    const fui::Rect cardBox = fui::makeRect(leftMargin, currentY, contentWidth, cardHeight);

    if (h && h->name[0] != '\0') {
      const habits::DailyRecord rec = store.getRecord(h->id, displayDate);

      // Card border
      screen.target().stroke(cardBox, ink, rec.completed ? 2 : 1, 6);

      // Left column: Habit name, streak, total, status
      drawText(screen, fui::makeRect(cardBox.x + 18, cardBox.y + 16, cardBox.width - 120, 28), h->name,
               toybox::kUiFont);

      char streakBuf[64];
      std::snprintf(streakBuf, sizeof(streakBuf), "STREAK: %d DAYS", h->streak);
      drawText(screen, fui::makeRect(cardBox.x + 18, cardBox.y + 48, cardBox.width - 120, 22), streakBuf,
               toybox::kTileFont);

      char subBuf[64];
      std::snprintf(subBuf, sizeof(subBuf), "BEST: %d DAYS  ·  TOTAL: %d DAYS", h->bestStreak, h->totalCompleted);
      drawText(screen, fui::makeRect(cardBox.x + 18, cardBox.y + 74, cardBox.width - 120, 20), subBuf,
               toybox::kSmallFont, fui::TextAlign::Left, fui::Color::DarkGray);

      const char* hint = rec.completed ? "DONE FOR THIS DAY" : "TAP TO MARK YES";
      drawText(screen, fui::makeRect(cardBox.x + 18, cardBox.y + 102, cardBox.width - 120, 20), hint,
               toybox::kSmallFont, fui::TextAlign::Left, rec.completed ? fui::Color::Black : fui::Color::DarkGray);

      // Right column: Big 64x64 Yes/No Checkbox
      constexpr int boxSize = 64;
      const fui::Rect checkRect = fui::makeRect(cardBox.x + cardBox.width - boxSize - 18,
                                                cardBox.y + (cardHeight - boxSize) / 2, boxSize, boxSize);

      if (rec.completed) {
        screen.target().fill(checkRect, ink, 6);
        drawText(screen, fui::makeRect(checkRect.x, checkRect.y + 18, checkRect.width, 28), "YES", toybox::kUiFont,
                 fui::TextAlign::Center, fui::Color::White);
      } else {
        screen.target().stroke(checkRect, ink, 2, 6);
        drawText(screen, fui::makeRect(checkRect.x, checkRect.y + 20, checkRect.width, 24), "NO", toybox::kTileFont,
                 fui::TextAlign::Center, fui::Color::DarkGray);
      }

      addBtn(screen, static_cast<fui::ActionId>(ActionHabitTap0 + i), cardBox);

    } else {
      // Empty Slot Card
      screen.target().stroke(cardBox, ink, 1, 6);

      char slotTitle[48];
      std::snprintf(slotTitle, sizeof(slotTitle), "+ ADD HABIT (SLOT %d)", i + 1);
      drawText(screen, fui::makeRect(cardBox.x, cardBox.y + 36, cardBox.width, 28), slotTitle, toybox::kUiFont,
               fui::TextAlign::Center);

      drawText(screen, fui::makeRect(cardBox.x, cardBox.y + 70, cardBox.width, 22),
               "Tap here to choose a habit or create one", toybox::kTileFont, fui::TextAlign::Center,
               fui::Color::DarkGray);

      addBtn(screen, static_cast<fui::ActionId>(ActionManageAdd0 + i), cardBox);
    }

    currentY += cardHeight + cardGap;
  }

  // --- 4. Bottom Navigation Tabs ---
  constexpr int footerHeight = 44;
  const int footerY = device.height - footerHeight - 12;
  constexpr int tabGap = 12;
  const int tabWidth = (contentWidth - tabGap * 2) / 3;

  int tabX = leftMargin;

  // Tab 1: TODAY / LOG (active)
  const fui::Rect tab1Rect = fui::makeRect(tabX, footerY, tabWidth, footerHeight);
  screen.target().fill(tab1Rect, ink, 4);
  drawText(screen, fui::makeRect(tab1Rect.x, tab1Rect.y + 11, tab1Rect.width, 22), (dayOffset == 0) ? "TODAY" : "LOG",
           toybox::kTileFont, fui::TextAlign::Center, fui::Color::White);
  addBtn(screen, ActionTabDaily, tab1Rect);
  tabX += tabWidth + tabGap;

  // Tab 2: WEEK
  const fui::Rect tab2Rect = fui::makeRect(tabX, footerY, tabWidth, footerHeight);
  screen.target().stroke(tab2Rect, ink, 1, 4);
  drawText(screen, fui::makeRect(tab2Rect.x, tab2Rect.y + 11, tab2Rect.width, 22), "WEEK", toybox::kTileFont,
           fui::TextAlign::Center);
  addBtn(screen, ActionTabWeek, tab2Rect);
  tabX += tabWidth + tabGap;

  // Tab 3: MANAGE
  const fui::Rect tab3Rect = fui::makeRect(tabX, footerY, contentWidth - (tabX - leftMargin), footerHeight);
  screen.target().stroke(tab3Rect, ink, 1, 4);
  drawText(screen, fui::makeRect(tab3Rect.x, tab3Rect.y + 11, tab3Rect.width, 22), "MANAGE", toybox::kTileFont,
           fui::TextAlign::Center);
  addBtn(screen, ActionTabManage, tab3Rect);
}

void buildWeek(toybox::Screen& screen, const ::habits::Store& store, const char* todayDate) {
  toybox::absoluteChrome(screen);
  const fui::DeviceContext& device = screen.device();
  const auto ink = fui::Paint::solid(fui::Color::Black);

  constexpr int topMargin = 14;
  const int leftMargin = toybox::kMargin;
  const int contentWidth = device.width - 2 * toybox::kMargin;

  // Header Title
  drawText(screen, fui::makeRect(leftMargin, topMargin, contentWidth, 32), "THIS WEEK", toybox::kUiFont,
           fui::TextAlign::Center);

  // Subtitle hint
  drawText(screen, fui::makeRect(leftMargin, topMargin + 32, contentWidth, 22), "TAP ANY DAY COLUMN TO OPEN AND LOG",
           toybox::kSmallFont, fui::TextAlign::Center, fui::Color::DarkGray);

  // Separator
  screen.target().fill(fui::makeRect(leftMargin, topMargin + 56, contentWidth, 2), ink);

  char weekDates[7][12] = {};
  habits::Store::getWeekDays(todayDate, weekDates);
  static const char* kDays[7] = {"M", "T", "W", "T", "F", "S", "S"};

  // Column metrics
  constexpr int labelColW = 114;
  constexpr int tallyColW = 54;
  const int daysAreaW = contentWidth - labelColW - tallyColW;
  const int dayColW = daysAreaW / 7;
  const int gridStartY = topMargin + 66;

  // Days header row: M  T  W  T  F  S  S
  for (int col = 0; col < 7; ++col) {
    const int colX = leftMargin + labelColW + col * dayColW;
    const fui::Rect dayBox = fui::makeRect(colX + 2, gridStartY, dayColW - 4, 40);

    if (std::strcmp(weekDates[col], todayDate) == 0) {
      screen.target().stroke(dayBox, ink, 2, 4);
    }

    drawText(screen, fui::makeRect(colX, gridStartY + 2, dayColW, 18), kDays[col], toybox::kTileFont,
             fui::TextAlign::Center);

    if (weekDates[col][8] != '\0') {
      drawText(screen, fui::makeRect(colX, gridStartY + 22, dayColW, 16), weekDates[col] + 8, toybox::kSmallFont,
               fui::TextAlign::Center, fui::Color::DarkGray);
    }

    addBtn(screen, static_cast<fui::ActionId>(ActionWeekDay0 + col), fui::makeRect(colX, gridStartY, dayColW, 300));
  }

  // Header for Tally
  drawText(screen, fui::makeRect(leftMargin + labelColW + daysAreaW, gridStartY + 10, tallyColW, 24), "TOTAL",
           toybox::kTileFont, fui::TextAlign::Center);

  // Habit rows in grid
  constexpr int rowHeight = 74;
  int rowY = gridStartY + 50;

  for (int r = 0; r < habits::kMaxHabits; ++r) {
    const auto* h = store.habitAt(r);
    screen.target().fill(fui::makeRect(leftMargin, rowY, contentWidth, 1), ink);

    if (h && h->name[0] != '\0') {
      // Habit Name on left
      drawText(screen, fui::makeRect(leftMargin + 4, rowY + 24, labelColW - 8, 28), h->name, toybox::kTileFont);

      // 7 checkmarks
      int completedDays = 0;
      for (int col = 0; col < 7; ++col) {
        const int colX = leftMargin + labelColW + col * dayColW;
        const int cellX = colX + (dayColW - 26) / 2;
        const int cellY = rowY + 22;
        const fui::Rect cellBox = fui::makeRect(cellX, cellY, 26, 26);
        const habits::DailyRecord rec = store.getRecord(h->id, weekDates[col]);
        if (rec.completed) ++completedDays;

        if (rec.completed) {
          screen.target().fill(cellBox, ink, 4);
          drawText(screen, cellBox, "X", toybox::kTileFont, fui::TextAlign::Center, fui::Color::White);
        } else {
          screen.target().stroke(cellBox, ink, 1, 4);
        }
      }

      // Tally on right (e.g. "6 / 7")
      char tallyBuf[16];
      std::snprintf(tallyBuf, sizeof(tallyBuf), "%d / 7", completedDays);
      drawText(screen, fui::makeRect(leftMargin + labelColW + daysAreaW, rowY + 24, tallyColW, 28), tallyBuf,
               toybox::kTileFont, fui::TextAlign::Center);
    } else {
      // Empty habit slot
      char emptySlotBuf[32];
      std::snprintf(emptySlotBuf, sizeof(emptySlotBuf), "Slot %d (Empty)", r + 1);
      drawText(screen, fui::makeRect(leftMargin + 4, rowY + 24, labelColW - 8, 28), emptySlotBuf, toybox::kSmallFont,
               fui::TextAlign::Left, fui::Color::DarkGray);
    }

    rowY += rowHeight;
  }

  // Bottom door: [ < BACK TO TODAY ]
  constexpr int footerHeight = 44;
  const int footerY = device.height - footerHeight - 12;
  const fui::Rect backRect = fui::makeRect(leftMargin, footerY, contentWidth, footerHeight);
  screen.target().fill(backRect, ink, 4);
  drawText(screen, fui::makeRect(backRect.x, backRect.y + 11, backRect.width, 22), "< BACK TO TODAY", toybox::kTileFont,
           fui::TextAlign::Center, fui::Color::White);
  addBtn(screen, ActionTabDaily, backRect);
}

void buildManage(toybox::Screen& screen, const ::habits::Store& store) {
  toybox::absoluteChrome(screen);
  const fui::DeviceContext& device = screen.device();
  const auto ink = fui::Paint::solid(fui::Color::Black);

  const int leftMargin = toybox::kMargin;
  const int rightMargin = device.width - toybox::kMargin;
  const int contentWidth = rightMargin - leftMargin;

  // Header: MANAGE HABITS
  constexpr int topMargin = 14;
  constexpr int headerHeight = 34;

  drawText(screen, fui::makeRect(leftMargin, topMargin, contentWidth - 110, headerHeight), "MANAGE HABITS",
           toybox::kUiFont);

  char countBuf[32];
  std::snprintf(countBuf, sizeof(countBuf), "%d / %d ACTIVE", store.activeHabitCount(), habits::kMaxHabits);
  drawText(screen, fui::makeRect(rightMargin - 110, topMargin, 110, headerHeight), countBuf, toybox::kTileFont,
           fui::TextAlign::Right);

  const int ruleY = topMargin + headerHeight + 2;
  screen.target().fill(fui::makeRect(leftMargin, ruleY, contentWidth, 2), ink);

  // Subtitle
  drawText(screen, fui::makeRect(leftMargin, ruleY + 8, contentWidth, 22), "CONFIGURE YOUR 3 HABITS (KEEP IT SIMPLE):",
           toybox::kTileFont, fui::TextAlign::Left, fui::Color::DarkGray);

  // 3 Slot Cards
  int currentY = ruleY + 36;
  constexpr int slotH = 82;
  constexpr int slotGap = 8;

  for (int i = 0; i < habits::kMaxHabits; ++i) {
    const auto* h = store.habitAt(i);
    const fui::Rect box = fui::makeRect(leftMargin, currentY, contentWidth, slotH);
    screen.target().stroke(box, ink, 1, 6);

    if (h && h->name[0] != '\0') {
      // Habit Name
      char nameBuf[48];
      std::snprintf(nameBuf, sizeof(nameBuf), "%d. %s", i + 1, h->name);
      drawText(screen, fui::makeRect(box.x + 14, box.y + 10, box.width - 190, 28), nameBuf, toybox::kUiFont);

      char statsBuf[48];
      std::snprintf(statsBuf, sizeof(statsBuf), "Streak: %d days  ·  Total: %d", h->streak, h->totalCompleted);
      drawText(screen, fui::makeRect(box.x + 14, box.y + 46, box.width - 190, 20), statsBuf, toybox::kSmallFont,
               fui::TextAlign::Left, fui::Color::DarkGray);

      // Rename button [ RENAME ]
      const fui::Rect renRect = fui::makeRect(box.x + box.width - 180, box.y + 20, 82, 42);
      screen.target().stroke(renRect, ink, 1, 4);
      drawText(screen, fui::makeRect(renRect.x, renRect.y + 11, renRect.width, 20), "RENAME", toybox::kTileFont,
               fui::TextAlign::Center);
      addBtn(screen, static_cast<fui::ActionId>(ActionManageRename0 + i), renRect);

      // Remove button [ REMOVE ]
      const fui::Rect remRect = fui::makeRect(box.x + box.width - 90, box.y + 20, 82, 42);
      screen.target().stroke(remRect, ink, 1, 4);
      drawText(screen, fui::makeRect(remRect.x, remRect.y + 11, remRect.width, 20), "REMOVE", toybox::kTileFont,
               fui::TextAlign::Center);
      addBtn(screen, static_cast<fui::ActionId>(ActionManageRemove0 + i), remRect);

    } else {
      // Empty slot
      char emptyBuf[48];
      std::snprintf(emptyBuf, sizeof(emptyBuf), "Slot %d: (Empty)", i + 1);
      drawText(screen, fui::makeRect(box.x + 14, box.y + 28, box.width - 160, 26), emptyBuf, toybox::kTileFont,
               fui::TextAlign::Left, fui::Color::DarkGray);

      // Add button [ + ADD ]
      const fui::Rect addRect = fui::makeRect(box.x + box.width - 110, box.y + 20, 98, 42);
      screen.target().fill(addRect, ink, 4);
      drawText(screen, fui::makeRect(addRect.x, addRect.y + 11, addRect.width, 20), "+ ADD", toybox::kTileFont,
               fui::TextAlign::Center, fui::Color::White);
      addBtn(screen, static_cast<fui::ActionId>(ActionManageAdd0 + i), addRect);
    }

    currentY += slotH + slotGap;
  }

  // Quick 1-tap Presets
  const int presetStartY = currentY + 18;
  drawText(screen, fui::makeRect(leftMargin, presetStartY, contentWidth, 22), "QUICK 1-TAP PRESETS:", toybox::kTileFont);
  drawText(screen, fui::makeRect(leftMargin, presetStartY + 26, contentWidth, 20),
           "Tap any preset below to instantly fill an empty slot:", toybox::kSmallFont, fui::TextAlign::Left,
           fui::Color::DarkGray);

  constexpr int pBtnH = 40;
  constexpr int pGap = 8;
  const int pBtnW = (contentWidth - pGap * 2) / 3;

  static const char* const kPresetNames[6] = {"READ", "WALK", "MEDITATE", "WORKOUT", "WATER", "JOURNAL"};
  const int row1Y = presetStartY + 52;
  const int row2Y = row1Y + pBtnH + pGap;

  for (int p = 0; p < 6; ++p) {
    const int col = p % 3;
    const int y = (p < 3) ? row1Y : row2Y;
    const fui::Rect pRect = fui::makeRect(leftMargin + col * (pBtnW + pGap), y, pBtnW, pBtnH);
    screen.target().stroke(pRect, ink, 1, 4);
    drawText(screen, fui::makeRect(pRect.x, pRect.y + 10, pRect.width, 20), kPresetNames[p], toybox::kTileFont,
             fui::TextAlign::Center);
    addBtn(screen, static_cast<fui::ActionId>(ActionPreset0 + p), pRect);
  }

  // Or Custom Type
  const int customY = row2Y + pBtnH + 12;
  const fui::Rect customRect = fui::makeRect(leftMargin, customY, contentWidth, 40);
  screen.target().stroke(customRect, ink, 1, 4);
  drawText(screen, fui::makeRect(customRect.x, customRect.y + 10, customRect.width, 20), "+ TYPE CUSTOM HABIT NAME",
           toybox::kTileFont, fui::TextAlign::Center);
  addBtn(screen, ActionPresetCustom, customRect);

  // Bottom door [ < BACK TO TRACKER ]
  constexpr int footerHeight = 44;
  const int footerY = device.height - footerHeight - 12;
  const fui::Rect backRect = fui::makeRect(leftMargin, footerY, contentWidth, footerHeight);
  screen.target().fill(backRect, ink, 4);
  drawText(screen, fui::makeRect(backRect.x, backRect.y + 11, backRect.width, 22), "< BACK TO TRACKER", toybox::kTileFont,
           fui::TextAlign::Center, fui::Color::White);
  addBtn(screen, ActionTabDaily, backRect);
}

}  // namespace habitsui
