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

fui::TextStyle styleWith(const fui::FontId font, const fui::TextAlign align = fui::TextAlign::Left,
                         const fui::Color color = fui::Color::Black) {
  fui::TextStyle style;
  style.font = font;
  style.align = align;
  style.color = color;
  style.maxLines = 1;
  return style;
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

  screen.target().text(fui::makeRect(leftMargin, topMargin, contentWidth - 130, headerHeight), "HABITS",
                       styleWith(toybox::kUiFont, fui::TextAlign::Left));

  int completed = 0, total = 0;
  store.getTodayScore(displayDate, completed, total);
  char scoreBuf[32];
  if (total == 0) {
    std::snprintf(scoreBuf, sizeof(scoreBuf), "NO HABITS");
  } else if (completed == total) {
    std::snprintf(scoreBuf, sizeof(scoreBuf), "%d / %d ALL DONE", completed, total);
  } else {
    std::snprintf(scoreBuf, sizeof(scoreBuf), "%d / %d DONE", completed, total);
  }
  screen.target().text(fui::makeRect(rightMargin - 140, topMargin, 140, headerHeight), scoreBuf,
                       styleWith(toybox::kTileFont, fui::TextAlign::Right));

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
  screen.target().text(fui::makeRect(prevRect.x, prevRect.y + 6, prevRect.width, 28), "<",
                       styleWith(toybox::kUiFont, fui::TextAlign::Center));
  fui::ButtonProps prevBtn;
  prevBtn.action = ActionDayPrev;
  prevBtn.enabled = true;
  prevBtn.styles = fui::flatButtonStyles(0);
  screen.button(prevBtn, prevRect);

  // Next day button [ > ] and [ TODAY ] shortcut (only if dayOffset < 0)
  if (dayOffset < 0) {
    const fui::Rect nextRect = fui::makeRect(rightMargin - 42, navY, 42, navH);
    screen.target().stroke(nextRect, ink, 1, 4);
    screen.target().text(fui::makeRect(nextRect.x, nextRect.y + 6, nextRect.width, 28), ">",
                         styleWith(toybox::kUiFont, fui::TextAlign::Center));
    fui::ButtonProps nextBtn;
    nextBtn.action = ActionDayNext;
    nextBtn.enabled = true;
    nextBtn.styles = fui::flatButtonStyles(0);
    screen.button(nextBtn, nextRect);

    // [ TODAY ] shortcut button
    const fui::Rect todayShortcutRect = fui::makeRect(rightMargin - 42 - 76, navY + 4, 70, navH - 8);
    screen.target().fill(todayShortcutRect, ink, 4);
    screen.target().text(fui::makeRect(todayShortcutRect.x, todayShortcutRect.y + 8, todayShortcutRect.width, 18),
                         "TODAY", styleWith(toybox::kSmallFont, fui::TextAlign::Center, fui::Color::White));
    fui::ButtonProps todayScBtn;
    todayScBtn.action = ActionGoToday;
    todayScBtn.enabled = true;
    todayScBtn.styles = fui::flatButtonStyles(0);
    screen.button(todayScBtn, todayShortcutRect);
  }

  // Date Title in middle
  const int dateTitleX = leftMargin + 50;
  const int dateTitleW = (dayOffset < 0) ? (contentWidth - 50 - 124) : (contentWidth - 50);
  screen.target().text(fui::makeRect(dateTitleX, navY + 10, dateTitleW, 24), displayHeader,
                       styleWith(toybox::kTileFont, fui::TextAlign::Left));

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
      screen.target().text(fui::makeRect(cardBox.x + 18, cardBox.y + 16, cardBox.width - 120, 28), h->name,
                           styleWith(toybox::kUiFont, fui::TextAlign::Left));

      char streakBuf[64];
      std::snprintf(streakBuf, sizeof(streakBuf), "STREAK: %d DAYS", h->streak);
      screen.target().text(fui::makeRect(cardBox.x + 18, cardBox.y + 48, cardBox.width - 120, 22), streakBuf,
                           styleWith(toybox::kTileFont, fui::TextAlign::Left));

      char subBuf[64];
      std::snprintf(subBuf, sizeof(subBuf), "BEST: %d DAYS  ·  TOTAL: %d DAYS", h->bestStreak, h->totalCompleted);
      screen.target().text(fui::makeRect(cardBox.x + 18, cardBox.y + 74, cardBox.width - 120, 20), subBuf,
                           styleWith(toybox::kSmallFont, fui::TextAlign::Left, fui::Color::DarkGray));

      const char* hint = rec.completed ? "DONE FOR THIS DAY" : "TAP TO MARK YES";
      screen.target().text(fui::makeRect(cardBox.x + 18, cardBox.y + 102, cardBox.width - 120, 20), hint,
                           styleWith(toybox::kSmallFont, fui::TextAlign::Left,
                                     rec.completed ? fui::Color::Black : fui::Color::DarkGray));

      // Right column: Big 64x64 Yes/No Checkbox
      constexpr int boxSize = 64;
      const fui::Rect checkRect = fui::makeRect(cardBox.x + cardBox.width - boxSize - 18,
                                                cardBox.y + (cardHeight - boxSize) / 2, boxSize, boxSize);

      if (rec.completed) {
        // Checked: Inverted solid black box with bold white "YES"
        screen.target().fill(checkRect, ink, 6);
        screen.target().text(fui::makeRect(checkRect.x, checkRect.y + 18, checkRect.width, 28), "YES",
                             styleWith(toybox::kUiFont, fui::TextAlign::Center, fui::Color::White));
      } else {
        // Unchecked: Stroked box with subtle "NO"
        screen.target().stroke(checkRect, ink, 2, 6);
        screen.target().text(fui::makeRect(checkRect.x, checkRect.y + 20, checkRect.width, 24), "NO",
                             styleWith(toybox::kTileFont, fui::TextAlign::Center, fui::Color::DarkGray));
      }

      // Tap entire card to toggle YES / NO
      fui::ButtonProps hitBtn;
      hitBtn.action = static_cast<fui::ActionId>(ActionHabitTap0 + i);
      hitBtn.enabled = true;
      hitBtn.styles = fui::flatButtonStyles(0);
      screen.button(hitBtn, cardBox);

    } else {
      // Empty Slot Card
      screen.target().stroke(cardBox, ink, 1, 6);

      char slotTitle[48];
      std::snprintf(slotTitle, sizeof(slotTitle), "+ ADD HABIT (SLOT %d)", i + 1);
      screen.target().text(fui::makeRect(cardBox.x, cardBox.y + 36, cardBox.width, 28), slotTitle,
                           styleWith(toybox::kUiFont, fui::TextAlign::Center));

      screen.target().text(fui::makeRect(cardBox.x, cardBox.y + 70, cardBox.width, 22),
                           "Tap here to choose a habit or create one",
                           styleWith(toybox::kTileFont, fui::TextAlign::Center, fui::Color::DarkGray));

      fui::ButtonProps addBtn;
      addBtn.action = static_cast<fui::ActionId>(ActionManageAdd0 + i);
      addBtn.enabled = true;
      addBtn.styles = fui::flatButtonStyles(0);
      screen.button(addBtn, cardBox);
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
  screen.target().text(fui::makeRect(tab1Rect.x, tab1Rect.y + 11, tab1Rect.width, 22),
                       (dayOffset == 0) ? "TODAY" : "LOG",
                       styleWith(toybox::kTileFont, fui::TextAlign::Center, fui::Color::White));
  fui::ButtonProps dailyBtn;
  dailyBtn.action = ActionTabDaily;
  dailyBtn.enabled = true;
  dailyBtn.styles = fui::flatButtonStyles(0);
  screen.button(dailyBtn, tab1Rect);
  tabX += tabWidth + tabGap;

  // Tab 2: WEEK
  const fui::Rect tab2Rect = fui::makeRect(tabX, footerY, tabWidth, footerHeight);
  screen.target().stroke(tab2Rect, ink, 1, 4);
  screen.target().text(fui::makeRect(tab2Rect.x, tab2Rect.y + 11, tab2Rect.width, 22), "WEEK",
                       styleWith(toybox::kTileFont, fui::TextAlign::Center));
  fui::ButtonProps weekBtn;
  weekBtn.action = ActionTabWeek;
  weekBtn.enabled = true;
  weekBtn.styles = fui::flatButtonStyles(0);
  screen.button(weekBtn, tab2Rect);
  tabX += tabWidth + tabGap;

  // Tab 3: MANAGE
  const fui::Rect tab3Rect = fui::makeRect(tabX, footerY, contentWidth - (tabX - leftMargin), footerHeight);
  screen.target().stroke(tab3Rect, ink, 1, 4);
  screen.target().text(fui::makeRect(tab3Rect.x, tab3Rect.y + 11, tab3Rect.width, 22), "MANAGE",
                       styleWith(toybox::kTileFont, fui::TextAlign::Center));
  fui::ButtonProps manageBtn;
  manageBtn.action = ActionTabManage;
  manageBtn.enabled = true;
  manageBtn.styles = fui::flatButtonStyles(0);
  screen.button(manageBtn, tab3Rect);
}

void buildWeek(toybox::Screen& screen, const ::habits::Store& store, const char* todayDate) {
  toybox::absoluteChrome(screen);
  const fui::DeviceContext& device = screen.device();
  const auto ink = fui::Paint::solid(fui::Color::Black);

  constexpr int topMargin = 14;
  const int leftMargin = toybox::kMargin;
  const int contentWidth = device.width - 2 * toybox::kMargin;

  // Header Title
  screen.target().text(fui::makeRect(leftMargin, topMargin, contentWidth, 32), "THIS WEEK",
                       styleWith(toybox::kUiFont, fui::TextAlign::Center));

  // Subtitle hint
  screen.target().text(fui::makeRect(leftMargin, topMargin + 32, contentWidth, 22),
                       "TAP ANY DAY COLUMN TO OPEN AND LOG",
                       styleWith(toybox::kSmallFont, fui::TextAlign::Center, fui::Color::DarkGray));

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

    screen.target().text(fui::makeRect(colX, gridStartY + 2, dayColW, 18), kDays[col],
                         styleWith(toybox::kTileFont, fui::TextAlign::Center));

    if (weekDates[col][8] != '\0') {
      screen.target().text(fui::makeRect(colX, gridStartY + 22, dayColW, 16), weekDates[col] + 8,
                           styleWith(toybox::kSmallFont, fui::TextAlign::Center, fui::Color::DarkGray));
    }

    // Register button for day column to tap and jump to that day
    fui::ButtonProps dayBtn;
    dayBtn.action = static_cast<fui::ActionId>(ActionWeekDay0 + col);
    dayBtn.enabled = true;
    dayBtn.styles = fui::flatButtonStyles(0);
    screen.button(dayBtn, fui::makeRect(colX, gridStartY, dayColW, 300));
  }

  // Header for Tally
  screen.target().text(fui::makeRect(leftMargin + labelColW + daysAreaW, gridStartY + 10, tallyColW, 24), "TOTAL",
                       styleWith(toybox::kTileFont, fui::TextAlign::Center));

  // Habit rows in grid
  constexpr int rowHeight = 74;
  int rowY = gridStartY + 50;

  for (int r = 0; r < habits::kMaxHabits; ++r) {
    const auto* h = store.habitAt(r);
    screen.target().fill(fui::makeRect(leftMargin, rowY, contentWidth, 1), ink);

    if (h && h->name[0] != '\0') {
      // Habit Name on left
      screen.target().text(fui::makeRect(leftMargin + 4, rowY + 24, labelColW - 8, 28), h->name,
                           styleWith(toybox::kTileFont, fui::TextAlign::Left));

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
          screen.target().text(cellBox, "X", styleWith(toybox::kTileFont, fui::TextAlign::Center, fui::Color::White));
        } else {
          screen.target().stroke(cellBox, ink, 1, 4);
        }
      }

      // Tally on right (e.g. "6 / 7")
      char tallyBuf[16];
      std::snprintf(tallyBuf, sizeof(tallyBuf), "%d / 7", completedDays);
      screen.target().text(fui::makeRect(leftMargin + labelColW + daysAreaW, rowY + 24, tallyColW, 28), tallyBuf,
                           styleWith(toybox::kTileFont, fui::TextAlign::Center));
    } else {
      // Empty habit slot
      char emptySlotBuf[32];
      std::snprintf(emptySlotBuf, sizeof(emptySlotBuf), "Slot %d (Empty)", r + 1);
      screen.target().text(fui::makeRect(leftMargin + 4, rowY + 24, labelColW - 8, 28), emptySlotBuf,
                           styleWith(toybox::kSmallFont, fui::TextAlign::Left, fui::Color::DarkGray));
    }

    rowY += rowHeight;
  }

  // Bottom door: [ < BACK TO TODAY ]
  constexpr int footerHeight = 44;
  const int footerY = device.height - footerHeight - 12;
  const fui::Rect backRect = fui::makeRect(leftMargin, footerY, contentWidth, footerHeight);
  screen.target().fill(backRect, ink, 4);
  screen.target().text(fui::makeRect(backRect.x, backRect.y + 11, backRect.width, 22), "< BACK TO TODAY",
                       styleWith(toybox::kTileFont, fui::TextAlign::Center, fui::Color::White));

  fui::ButtonProps backBtn;
  backBtn.action = ActionTabDaily;
  backBtn.enabled = true;
  backBtn.styles = fui::flatButtonStyles(0);
  screen.button(backBtn, backRect);
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

  screen.target().text(fui::makeRect(leftMargin, topMargin, contentWidth - 110, headerHeight), "MANAGE HABITS",
                       styleWith(toybox::kUiFont, fui::TextAlign::Left));

  char countBuf[32];
  std::snprintf(countBuf, sizeof(countBuf), "%d / %d ACTIVE", store.activeHabitCount(), habits::kMaxHabits);
  screen.target().text(fui::makeRect(rightMargin - 110, topMargin, 110, headerHeight), countBuf,
                       styleWith(toybox::kTileFont, fui::TextAlign::Right));

  const int ruleY = topMargin + headerHeight + 2;
  screen.target().fill(fui::makeRect(leftMargin, ruleY, contentWidth, 2), ink);

  // Subtitle
  screen.target().text(fui::makeRect(leftMargin, ruleY + 8, contentWidth, 22),
                       "CONFIGURE YOUR 3 HABITS (KEEP IT SIMPLE):",
                       styleWith(toybox::kTileFont, fui::TextAlign::Left, fui::Color::DarkGray));

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
      screen.target().text(fui::makeRect(box.x + 14, box.y + 10, box.width - 190, 28), nameBuf,
                           styleWith(toybox::kUiFont, fui::TextAlign::Left));

      char statsBuf[48];
      std::snprintf(statsBuf, sizeof(statsBuf), "Streak: %d days  ·  Total: %d", h->streak, h->totalCompleted);
      screen.target().text(fui::makeRect(box.x + 14, box.y + 46, box.width - 190, 20), statsBuf,
                           styleWith(toybox::kSmallFont, fui::TextAlign::Left, fui::Color::DarkGray));

      // Rename button [ RENAME ]
      const fui::Rect renRect = fui::makeRect(box.x + box.width - 180, box.y + 20, 82, 42);
      screen.target().stroke(renRect, ink, 1, 4);
      screen.target().text(fui::makeRect(renRect.x, renRect.y + 11, renRect.width, 20), "RENAME",
                           styleWith(toybox::kTileFont, fui::TextAlign::Center));
      fui::ButtonProps renBtn;
      renBtn.action = static_cast<fui::ActionId>(ActionManageRename0 + i);
      renBtn.enabled = true;
      renBtn.styles = fui::flatButtonStyles(0);
      screen.button(renBtn, renRect);

      // Remove button [ REMOVE ]
      const fui::Rect remRect = fui::makeRect(box.x + box.width - 90, box.y + 20, 82, 42);
      screen.target().stroke(remRect, ink, 1, 4);
      screen.target().text(fui::makeRect(remRect.x, remRect.y + 11, remRect.width, 20), "REMOVE",
                           styleWith(toybox::kTileFont, fui::TextAlign::Center));
      fui::ButtonProps remBtn;
      remBtn.action = static_cast<fui::ActionId>(ActionManageRemove0 + i);
      remBtn.enabled = true;
      remBtn.styles = fui::flatButtonStyles(0);
      screen.button(remBtn, remRect);

    } else {
      // Empty slot
      char emptyBuf[48];
      std::snprintf(emptyBuf, sizeof(emptyBuf), "Slot %d: (Empty)", i + 1);
      screen.target().text(fui::makeRect(box.x + 14, box.y + 28, box.width - 160, 26), emptyBuf,
                           styleWith(toybox::kTileFont, fui::TextAlign::Left, fui::Color::DarkGray));

      // Add button [ + ADD ]
      const fui::Rect addRect = fui::makeRect(box.x + box.width - 110, box.y + 20, 98, 42);
      screen.target().fill(addRect, ink, 4);
      screen.target().text(fui::makeRect(addRect.x, addRect.y + 11, addRect.width, 20), "+ ADD",
                           styleWith(toybox::kTileFont, fui::TextAlign::Center, fui::Color::White));
      fui::ButtonProps addBtn;
      addBtn.action = static_cast<fui::ActionId>(ActionManageAdd0 + i);
      addBtn.enabled = true;
      addBtn.styles = fui::flatButtonStyles(0);
      screen.button(addBtn, addRect);
    }

    currentY += slotH + slotGap;
  }

  // Quick 1-tap Presets
  const int presetStartY = currentY + 18;
  screen.target().text(fui::makeRect(leftMargin, presetStartY, contentWidth, 22),
                       "QUICK 1-TAP PRESETS:",
                       styleWith(toybox::kTileFont, fui::TextAlign::Left));

  screen.target().text(fui::makeRect(leftMargin, presetStartY + 26, contentWidth, 20),
                       "Tap any preset below to instantly fill an empty slot:",
                       styleWith(toybox::kSmallFont, fui::TextAlign::Left, fui::Color::DarkGray));

  constexpr int pBtnH = 40;
  constexpr int pGap = 8;
  const int pBtnW = (contentWidth - pGap * 2) / 3;

  static const char* kPresetNames[6] = {"READ", "WALK", "MEDITATE", "WORKOUT", "WATER", "JOURNAL"};
  const int row1Y = presetStartY + 52;
  for (int p = 0; p < 3; ++p) {
    const fui::Rect pRect = fui::makeRect(leftMargin + p * (pBtnW + pGap), row1Y, pBtnW, pBtnH);
    screen.target().stroke(pRect, ink, 1, 4);
    screen.target().text(fui::makeRect(pRect.x, pRect.y + 10, pRect.width, 20), kPresetNames[p],
                         styleWith(toybox::kTileFont, fui::TextAlign::Center));
    fui::ButtonProps pb;
    pb.action = static_cast<fui::ActionId>(ActionPreset0 + p);
    pb.enabled = true;
    pb.styles = fui::flatButtonStyles(0);
    screen.button(pb, pRect);
  }

  const int row2Y = row1Y + pBtnH + pGap;
  for (int p = 3; p < 6; ++p) {
    const fui::Rect pRect = fui::makeRect(leftMargin + (p - 3) * (pBtnW + pGap), row2Y, pBtnW, pBtnH);
    screen.target().stroke(pRect, ink, 1, 4);
    screen.target().text(fui::makeRect(pRect.x, pRect.y + 10, pRect.width, 20), kPresetNames[p],
                         styleWith(toybox::kTileFont, fui::TextAlign::Center));
    fui::ButtonProps pb;
    pb.action = static_cast<fui::ActionId>(ActionPreset0 + p);
    pb.enabled = true;
    pb.styles = fui::flatButtonStyles(0);
    screen.button(pb, pRect);
  }

  // Or Custom Type
  const int customY = row2Y + pBtnH + 12;
  const fui::Rect customRect = fui::makeRect(leftMargin, customY, contentWidth, 40);
  screen.target().stroke(customRect, ink, 1, 4);
  screen.target().text(fui::makeRect(customRect.x, customRect.y + 10, customRect.width, 20),
                       "+ TYPE CUSTOM HABIT NAME",
                       styleWith(toybox::kTileFont, fui::TextAlign::Center));
  fui::ButtonProps customBtn;
  customBtn.action = ActionPresetCustom;
  customBtn.enabled = true;
  customBtn.styles = fui::flatButtonStyles(0);
  screen.button(customBtn, customRect);

  // Bottom door [ < BACK TO TRACKER ]
  constexpr int footerHeight = 44;
  const int footerY = device.height - footerHeight - 12;
  const fui::Rect backRect = fui::makeRect(leftMargin, footerY, contentWidth, footerHeight);
  screen.target().fill(backRect, ink, 4);
  screen.target().text(fui::makeRect(backRect.x, backRect.y + 11, backRect.width, 22), "< BACK TO TRACKER",
                       styleWith(toybox::kTileFont, fui::TextAlign::Center, fui::Color::White));

  fui::ButtonProps backBtn;
  backBtn.action = ActionTabDaily;
  backBtn.enabled = true;
  backBtn.styles = fui::flatButtonStyles(0);
  screen.button(backBtn, backRect);
}

}  // namespace habitsui
