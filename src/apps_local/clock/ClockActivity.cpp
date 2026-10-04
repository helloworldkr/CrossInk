#include "ClockActivity.h"

#include <HalClock.h>
#include <HalPowerManager.h>
#include <HalStorage.h>
#include <I18n.h>
#include <Memory.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>

#include "../../CrossPointSettings.h"
#include "../../components/UITheme.h"
#include "../../fontIds.h"
#include "../Shelf.h"
#include "../ui/ToyboxFonts.h"

namespace {

constexpr const char* kConfigDir = "/XTData/clock";
constexpr const char* kConfigFile = "/XTData/clock/config.txt";

const char* kDaysOfWeek[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const char* kDaysShort[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
const char* kMonths[] = {"January",   "February", "March",    "April",
                         "May",       "June",     "July",     "August",
                         "September", "October",  "November", "December"};
const char* kMonthsShort[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

bool rectContains(const Rect& r, int x, int y) {
  return (x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height);
}

void drawCircle(GfxRenderer& renderer, int cx, int cy, int radius, bool state) {
  int x = 0;
  int y = radius;
  int d = 3 - 2 * radius;
  while (y >= x) {
    renderer.drawPixel(cx + x, cy + y, state);
    renderer.drawPixel(cx - x, cy + y, state);
    renderer.drawPixel(cx + x, cy - y, state);
    renderer.drawPixel(cx - x, cy - y, state);
    renderer.drawPixel(cx + y, cy + x, state);
    renderer.drawPixel(cx - y, cy + x, state);
    renderer.drawPixel(cx + y, cy - x, state);
    renderer.drawPixel(cx - y, cy - x, state);
    x++;
    if (d > 0) {
      y--;
      d = d + 4 * (x - y) + 10;
    } else {
      d = d + 4 * x + 6;
    }
  }
}

void fillCircle(GfxRenderer& renderer, int cx, int cy, int radius, bool state) {
  for (int dy = -radius; dy <= radius; dy++) {
    int dxLimit = static_cast<int>(std::sqrt(radius * radius - dy * dy));
    renderer.drawLine(cx - dxLimit, cy + dy, cx + dxLimit, cy + dy, state);
  }
}

}  // namespace

std::unique_ptr<Activity> ClockActivity::create(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return makeUniqueNoThrow<ClockActivity>(renderer, mappedInput);
}

ClockActivity::ClockActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("Clock", renderer, mappedInput) {}

void ClockActivity::loadConfig() {
  char buf[128];
  size_t read = Storage.readFileToBuffer(kConfigFile, buf, sizeof(buf) - 1);
  if (read > 0) {
    buf[read] = '\0';
    int m = 0, u24 = 0, d = 0, ka = 1;
    if (std::sscanf(buf, "mode=%d\nuse24=%d\ndark=%d\nkeepAwake=%d", &m, &u24, &d, &ka) >= 3) {
      mode_ = (m == 1) ? DisplayMode::Digital : DisplayMode::Analog;
      use24Hour_ = (u24 == 1);
      darkMode_ = (d == 1);
      keepAwake_ = (ka == 1);
    }
  }
}

void ClockActivity::saveConfig() {
  Storage.ensureDirectoryExists(kConfigDir);
  HalFile file;
  if (Storage.openFileForWrite("CLOCK", kConfigFile, file)) {
    char buf[128];
    std::snprintf(buf, sizeof(buf), "mode=%d\nuse24=%d\ndark=%d\nkeepAwake=%d\n",
                  (mode_ == DisplayMode::Digital ? 1 : 0), (use24Hour_ ? 1 : 0), (darkMode_ ? 1 : 0),
                  (keepAwake_ ? 1 : 0));
    file.write(buf, std::strlen(buf));
    file.close();
  }
}

ClockActivity::WallTime ClockActivity::getCurrentWallTime() const {
  WallTime wt;

  // 1. Check RTC hardware if available
  uint16_t y = 2026;
  uint8_t mo = 1, d = 1, h = 0, m = 0;
  if (halClock.isAvailable() && halClock.getDateTime(y, mo, d, h, m)) {
    uint8_t offsetQ = SETTINGS.clockUtcOffsetQ;
    if (offsetQ > 104) offsetQ = 48;
    int offsetMinutes = (static_cast<int>(offsetQ) - 48) * 15;

    struct tm tStruct{};
    tStruct.tm_year = (y >= 1900) ? (y - 1900) : (y + 100);
    tStruct.tm_mon = (mo >= 1 && mo <= 12) ? (mo - 1) : 0;
    tStruct.tm_mday = (d >= 1 && d <= 31) ? d : 1;
    tStruct.tm_hour = h % 24;
    tStruct.tm_min = m % 60;
    tStruct.tm_sec = 0;

    time_t utcEpoch = mktime(&tStruct);
    if (utcEpoch != static_cast<time_t>(-1)) {
      time_t localEpoch = utcEpoch + offsetMinutes * 60;
      struct tm localTm{};
      localtime_r(&localEpoch, &localTm);
      wt.year = localTm.tm_year + 1900;
      wt.month = localTm.tm_mon + 1;
      wt.day = localTm.tm_mday;
      wt.weekday = localTm.tm_wday;
      wt.hour = localTm.tm_hour;
      wt.minute = localTm.tm_min;
      wt.second = localTm.tm_sec;
      return wt;
    }
  }

  // 2. Fallback to system time (simulator or network-synced clock)
  time_t nowT = time(nullptr);
  if (nowT > 100000) {
    struct tm localTm{};
    localtime_r(&nowT, &localTm);
    wt.year = localTm.tm_year + 1900;
    wt.month = localTm.tm_mon + 1;
    wt.day = localTm.tm_mday;
    wt.weekday = localTm.tm_wday;
    wt.hour = localTm.tm_hour;
    wt.minute = localTm.tm_min;
    wt.second = localTm.tm_sec;
    return wt;
  }

  return wt;
}

void ClockActivity::onEnter() {
  Activity::onEnter();
  toybox::ensureFonts(renderer);

  loadConfig();
  lastMinute_ = 255;
  lastPollMs_ = millis();
  fullRefreshCycle_ = 0;

  requestUpdate();
}

void ClockActivity::onExit() {
  saveConfig();
  Activity::onExit();
}

void ClockActivity::loop() {
  // 1. Hardware Buttons
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    shelf::leave(renderer, mappedInput);
    return;
  }

  // Confirm: Toggle Analog <-> Digital
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    mode_ = (mode_ == DisplayMode::Analog) ? DisplayMode::Digital : DisplayMode::Analog;
    saveConfig();
    requestUpdate();
    return;
  }

  // PageForward / Right: Toggle Dark Mode (Nightstand Mode)
  if (mappedInput.wasReleased(MappedInputManager::Button::PageForward) ||
      mappedInput.wasReleased(MappedInputManager::Button::Right)) {
    darkMode_ = !darkMode_;
    saveConfig();
    requestUpdate();
    return;
  }

  // PageBack / Left: Toggle 12h / 24h
  if (mappedInput.wasReleased(MappedInputManager::Button::PageBack) ||
      mappedInput.wasReleased(MappedInputManager::Button::Left)) {
    use24Hour_ = !use24Hour_;
    saveConfig();
    requestUpdate();
    return;
  }

  // 2. Touch Screen Interactions
  int tx = 0, ty = 0;
  if (mappedInput.wasScreenTapped(tx, ty)) {
    if (rectContains(exitBtnRect_, tx, ty)) {
      shelf::leave(renderer, mappedInput);
      return;
    }
    if (rectContains(modeBtnRect_, tx, ty) || rectContains(clockFaceRect_, tx, ty)) {
      mode_ = (mode_ == DisplayMode::Analog) ? DisplayMode::Digital : DisplayMode::Analog;
      saveConfig();
      requestUpdate();
      return;
    }
    if (rectContains(formatBtnRect_, tx, ty)) {
      use24Hour_ = !use24Hour_;
      saveConfig();
      requestUpdate();
      return;
    }
    if (rectContains(themeBtnRect_, tx, ty)) {
      darkMode_ = !darkMode_;
      saveConfig();
      requestUpdate();
      return;
    }
  }

  // 3. Periodic Time Check (Update once per minute on wall-clock boundary)
  const unsigned long now = millis();
  if (now - lastPollMs_ >= 1000) {
    lastPollMs_ = now;
    WallTime t = getCurrentWallTime();
    if (t.minute != lastMinute_) {
      lastMinute_ = t.minute;
      fullRefreshCycle_++;
      requestUpdate();
    }
  }
}

void ClockActivity::renderAnalog(const WallTime& t) {
  constexpr int cx = 240;
  constexpr int cy = 340;
  constexpr int radius = 165;

  clockFaceRect_ = Rect{cx - radius, cy - radius, radius * 2, radius * 2};

  // Outer bezel circles
  drawCircle(renderer, cx, cy, radius, true);
  drawCircle(renderer, cx, cy, radius - 1, true);
  drawCircle(renderer, cx, cy, radius - 2, true);

  // Inner subtle minute track circle
  drawCircle(renderer, cx, cy, radius - 8, true);

  // Hour tick marks
  for (int i = 0; i < 12; ++i) {
    const float angle = static_cast<float>(i) * (2.0f * static_cast<float>(M_PI) / 12.0f);
    const float sinA = std::sin(angle);
    const float cosA = std::cos(angle);

    const bool isMajor = (i % 3 == 0);  // 12, 3, 6, 9
    const int tickLen = isMajor ? 20 : 12;
    const int tickWidth = isMajor ? 4 : 2;

    const int x1 = cx + static_cast<int>((radius - 10) * sinA);
    const int y1 = cy - static_cast<int>((radius - 10) * cosA);
    const int x2 = cx + static_cast<int>((radius - 10 - tickLen) * sinA);
    const int y2 = cy - static_cast<int>((radius - 10 - tickLen) * cosA);

    renderer.drawLine(x1, y1, x2, y2, tickWidth, true);
  }

  // Hour numerals (12, 3, 6, 9)
  renderer.drawCenteredText(UI_12_FONT_ID, cy - radius + 34, "12", true, EpdFontFamily::BOLD);
  renderer.drawCenteredText(UI_12_FONT_ID, cy + radius - 58, "6", true, EpdFontFamily::BOLD);
  renderer.drawText(UI_12_FONT_ID, cx + radius - 44, cy - 14, "3", true, EpdFontFamily::BOLD);
  renderer.drawText(UI_12_FONT_ID, cx - radius + 28, cy - 14, "9", true, EpdFontFamily::BOLD);

  // Date Complication Badge (inside dial above 6 o'clock)
  const int badgeW = 92;
  const int badgeH = 26;
  const int badgeX = cx - badgeW / 2;
  const int badgeY = cy + 42;
  renderer.drawRect(badgeX, badgeY, badgeW, badgeH, true);
  char badgeText[24];
  const char* dayName = (t.weekday >= 0 && t.weekday < 7) ? kDaysShort[t.weekday] : "---";
  std::snprintf(badgeText, sizeof(badgeText), "%s %02d", dayName, t.day);
  renderer.drawCenteredText(UI_10_FONT_ID, badgeY + 6, badgeText, true, EpdFontFamily::BOLD);

  // Hour Hand (length ~92px, width 6px, tail 18px)
  const float hAngle = (static_cast<float>(t.hour % 12) + static_cast<float>(t.minute) / 60.0f) *
                       (2.0f * static_cast<float>(M_PI) / 12.0f);
  const int hx = cx + static_cast<int>(92.0f * std::sin(hAngle));
  const int hy = cy - static_cast<int>(92.0f * std::cos(hAngle));
  const int htx = cx - static_cast<int>(18.0f * std::sin(hAngle));
  const int hty = cy + static_cast<int>(18.0f * std::cos(hAngle));
  renderer.drawLine(htx, hty, hx, hy, 6, true);

  // Minute Hand (length ~138px, width 3px, tail 24px)
  const float mAngle = static_cast<float>(t.minute) * (2.0f * static_cast<float>(M_PI) / 60.0f);
  const int mx = cx + static_cast<int>(138.0f * std::sin(mAngle));
  const int my = cy - static_cast<int>(138.0f * std::cos(mAngle));
  const int mtx = cx - static_cast<int>(24.0f * std::sin(mAngle));
  const int mty = cy + static_cast<int>(24.0f * std::cos(mAngle));
  renderer.drawLine(mtx, mty, mx, my, 3, true);

  // Center Pivot Hub
  fillCircle(renderer, cx, cy, 9, true);
  fillCircle(renderer, cx, cy, 3, false);

  // Digital readout below the dial
  char digiStr[32];
  if (use24Hour_) {
    std::snprintf(digiStr, sizeof(digiStr), "%02d:%02d", t.hour, t.minute);
  } else {
    int h12 = t.hour % 12;
    if (h12 == 0) h12 = 12;
    std::snprintf(digiStr, sizeof(digiStr), "%d:%02d %s", h12, t.minute, (t.hour >= 12 ? "PM" : "AM"));
  }
  renderer.drawCenteredText(toybox::kDisplayFontId, cy + radius + 32, digiStr, true);

  // Full date subtitle
  char fullDateStr[64];
  const char* fullDay = (t.weekday >= 0 && t.weekday < 7) ? kDaysOfWeek[t.weekday] : "";
  const char* fullMonth = (t.month >= 1 && t.month <= 12) ? kMonths[t.month - 1] : "";
  std::snprintf(fullDateStr, sizeof(fullDateStr), "%s, %s %d, %d", fullDay, fullMonth, t.day, t.year);
  renderer.drawCenteredText(UI_10_FONT_ID, cy + radius + 84, fullDateStr, true, EpdFontFamily::BOLD);
}

void ClockActivity::renderDigital(const WallTime& t) {
  constexpr int cardX = 24;
  constexpr int cardY = 175;
  constexpr int cardW = 480 - 48;
  constexpr int cardH = 345;

  clockFaceRect_ = Rect{cardX, cardY, cardW, cardH};

  // Card Borders
  renderer.drawRect(cardX, cardY, cardW, cardH, true);
  renderer.drawRect(cardX + 2, cardY + 2, cardW - 4, cardH - 4, true);

  // Time String
  char timeBuf[16];
  if (use24Hour_) {
    std::snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", t.hour, t.minute);
    renderer.drawCenteredText(toybox::kHugeFontId, cardY + 65, timeBuf, true);
    renderer.drawCenteredText(UI_10_FONT_ID, cardY + 185, "- 24-HOUR CLOCK -", true, EpdFontFamily::BOLD);
  } else {
    int h12 = t.hour % 12;
    if (h12 == 0) h12 = 12;
    std::snprintf(timeBuf, sizeof(timeBuf), "%d:%02d", h12, t.minute);
    renderer.drawCenteredText(toybox::kHugeFontId, cardY + 50, timeBuf, true);

    char ampmStr[8];
    std::snprintf(ampmStr, sizeof(ampmStr), "%s", (t.hour >= 12 ? "PM" : "AM"));
    renderer.drawCenteredText(toybox::kDisplayFontId, cardY + 165, ampmStr, true);
  }

  // Hour progress bar
  constexpr int barX = 54;
  constexpr int barY = cardY + 235;
  constexpr int barW = 480 - 108;
  constexpr int barH = 14;
  renderer.drawRect(barX, barY, barW, barH, true);
  const int fillW = (barW - 4) * t.minute / 60;
  if (fillW > 0) {
    renderer.fillRect(barX + 2, barY + 2, fillW, barH - 4, true);
  }
  char progStr[32];
  std::snprintf(progStr, sizeof(progStr), "%d / 60 minutes", t.minute);
  renderer.drawCenteredText(UI_10_FONT_ID, barY + 22, progStr, true);

  // Date Banner below card
  const char* fullDay = (t.weekday >= 0 && t.weekday < 7) ? kDaysOfWeek[t.weekday] : "";
  renderer.drawCenteredText(toybox::kDisplayFontId, 550, fullDay, true);

  char fullDateStr[64];
  const char* fullMonth = (t.month >= 1 && t.month <= 12) ? kMonths[t.month - 1] : "";
  std::snprintf(fullDateStr, sizeof(fullDateStr), "%s %d, %d", fullMonth, t.day, t.year);
  renderer.drawCenteredText(UI_12_FONT_ID, 610, fullDateStr, true, EpdFontFamily::BOLD);
}

void ClockActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const WallTime t = getCurrentWallTime();

  // 1. Top Header Bar (y = 12, h = 34)
  constexpr int leftMargin = 16;
  constexpr int rightMargin = 480 - 16;

  renderer.drawText(UI_10_FONT_ID, leftMargin, 16, "DESK CLOCK", true, EpdFontFamily::BOLD);

  // Battery status on top-right
  const uint16_t battPct = powerManager.getBatteryPercentage();
  char battStr[16];
  std::snprintf(battStr, sizeof(battStr), "%u%%", battPct);
  const int battTextW = renderer.getTextWidth(UI_10_FONT_ID, battStr);

  constexpr int iconW = 22;
  constexpr int iconH = 12;
  const int iconX = rightMargin - iconW;
  const int iconY = 17;
  renderer.drawRect(iconX, iconY, iconW, iconH, true);
  renderer.drawRect(iconX + iconW, iconY + 3, 2, iconH - 6, true);  // positive terminal
  const int fillW = (iconW - 4) * battPct / 100;
  if (fillW > 0) {
    renderer.fillRect(iconX + 2, iconY + 2, fillW, iconH - 4, true);
  }
  renderer.drawText(UI_10_FONT_ID, iconX - battTextW - 6, 16, battStr, true);

  // Header Divider
  renderer.drawLine(leftMargin, 44, rightMargin, 44, true);

  // 2. Main Clock Area
  if (mode_ == DisplayMode::Analog) {
    renderAnalog(t);
  } else {
    renderDigital(t);
  }

  // 3. Bottom Control Toolbar (y = 724, h = 48)
  constexpr int btnY = 724;
  constexpr int btnH = 48;
  constexpr int btnW = 105;
  constexpr int btnGap = 8;

  modeBtnRect_ = Rect{leftMargin, btnY, btnW, btnH};
  renderer.drawRect(modeBtnRect_.x, modeBtnRect_.y, modeBtnRect_.width, modeBtnRect_.height, true);
  renderer.drawCenteredText(UI_10_FONT_ID, btnY + 8, "MODE", true, EpdFontFamily::BOLD);
  renderer.drawCenteredText(UI_10_FONT_ID, btnY + 26, (mode_ == DisplayMode::Analog ? "Analog" : "Digital"), true);

  formatBtnRect_ = Rect{leftMargin + btnW + btnGap, btnY, btnW, btnH};
  renderer.drawRect(formatBtnRect_.x, formatBtnRect_.y, formatBtnRect_.width, formatBtnRect_.height, true);
  renderer.drawCenteredText(UI_10_FONT_ID, btnY + 8, "TIME", true, EpdFontFamily::BOLD);
  renderer.drawCenteredText(UI_10_FONT_ID, btnY + 26, (use24Hour_ ? "24-Hour" : "12-Hour"), true);

  themeBtnRect_ = Rect{leftMargin + (btnW + btnGap) * 2, btnY, btnW, btnH};
  renderer.drawRect(themeBtnRect_.x, themeBtnRect_.y, themeBtnRect_.width, themeBtnRect_.height, true);
  renderer.drawCenteredText(UI_10_FONT_ID, btnY + 8, "THEME", true, EpdFontFamily::BOLD);
  renderer.drawCenteredText(UI_10_FONT_ID, btnY + 26, (darkMode_ ? "Dark" : "Light"), true);

  exitBtnRect_ = Rect{leftMargin + (btnW + btnGap) * 3, btnY, btnW, btnH};
  renderer.fillRect(exitBtnRect_.x, exitBtnRect_.y, exitBtnRect_.width, exitBtnRect_.height, true);
  renderer.drawCenteredText(UI_10_FONT_ID, btnY + 16, "EXIT", false, EpdFontFamily::BOLD);

  // 4. Invert screen if Dark Mode (Nightstand Mode) is active
  if (darkMode_) {
    renderer.invertScreen();
  }

  // Every 30 minutes, do a full clean refresh to clear e-ink ghosting
  if (fullRefreshCycle_ >= 30) {
    fullRefreshCycle_ = 0;
    renderer.displayBuffer(HalDisplay::RefreshMode::FULL_REFRESH);
  } else {
    renderer.displayBuffer(HalDisplay::RefreshMode::FAST_REFRESH);
  }
}
