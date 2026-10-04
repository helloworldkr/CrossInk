#include "PomodoroActivity.h"

#include <HalStorage.h>
#include <I18n.h>
#include <Memory.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>

#include "../../components/UITheme.h"
#include "../../fontIds.h"
#include "../Shelf.h"
#include "../ui/ToyboxFonts.h"

namespace {

constexpr const char* kStatsFile = "/XTData/pomodoro/stats.txt";

// Draw a circular progress ring. Progress 0.0–1.0, starts from 12 o'clock clockwise.
void drawProgressRing(GfxRenderer& renderer, int cx, int cy, int radius, int thickness, float progress) {
  const int outerR = radius;
  const int innerR = radius - thickness;
  const int outerSq = outerR * outerR;
  const int innerSq = innerR * innerR;

  for (int dy = -outerR; dy <= outerR; dy++) {
    for (int dx = -outerR; dx <= outerR; dx++) {
      const int distSq = dx * dx + dy * dy;
      if (distSq > outerSq || distSq < innerSq) continue;

      float angle = atan2f(static_cast<float>(dx), static_cast<float>(-dy));
      if (angle < 0) angle += 2.0f * M_PI;
      const float progressAngle = progress * 2.0f * M_PI;

      if (angle <= progressAngle) {
        renderer.drawPixel(cx + dx, cy + dy, true);
      } else {
        const int distSqInner1 = (innerR + 1) * (innerR + 1);
        const int distSqOuter1 = (outerR - 1) * (outerR - 1);
        if (distSq >= distSqOuter1 || distSq <= distSqInner1) {
          renderer.drawPixel(cx + dx, cy + dy, true);
        }
      }
    }
  }
}

bool rectContains(const Rect& r, int x, int y) {
  return (x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height);
}

}  // namespace

std::unique_ptr<Activity> PomodoroActivity::create(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return makeUniqueNoThrow<PomodoroActivity>(renderer, mappedInput);
}

PomodoroActivity::PomodoroActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("Pomodoro", renderer, mappedInput) {}

void PomodoroActivity::loadStats() {
  totalCompletedToday_ = 0;
  char buffer[64];
  size_t read = Storage.readFileToBuffer(kStatsFile, buffer, sizeof(buffer) - 1);
  if (read > 0) {
    buffer[read] = '\0';
    int count = 0;
    if (std::sscanf(buffer, "%d", &count) == 1 && count >= 0) {
      totalCompletedToday_ = count;
    }
  }
}

void PomodoroActivity::saveStats() {
  Storage.ensureDirectoryExists("/XTData/pomodoro");
  HalFile file;
  if (Storage.openFileForWrite("POMODORO", kStatsFile, file)) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d\n", totalCompletedToday_);
    file.write(buf, std::strlen(buf));
    file.close();
  }
}

uint32_t PomodoroActivity::getRemainingMs() const {
  if (state_ == State::IDLE) return totalDurationMs_;
  if (state_ == State::PAUSED) {
    if (pausedElapsedMs_ >= totalDurationMs_) return 0;
    return totalDurationMs_ - pausedElapsedMs_;
  }

  unsigned long elapsed = pausedElapsedMs_ + (millis() - timerStartMs_);
  if (elapsed >= totalDurationMs_) return 0;
  return totalDurationMs_ - elapsed;
}

void PomodoroActivity::startTimer(State newState, uint32_t durationMs) {
  state_ = newState;
  totalDurationMs_ = durationMs;
  pausedElapsedMs_ = 0;
  timerStartMs_ = millis();
  lastTickMs_ = millis();
  requestUpdate();
}

void PomodoroActivity::advanceState() {
  if (state_ == State::FOCUS || (state_ == State::PAUSED && pausedFrom_ == State::FOCUS)) {
    completedSessions_++;
    totalCompletedToday_++;
    saveStats();

    if (completedSessions_ >= kSessionsBeforeLongBreak) {
      completedSessions_ = 0;
      startTimer(State::LONG_BREAK, longBreakDurationMs_);
    } else {
      startTimer(State::SHORT_BREAK, shortBreakDurationMs_);
    }
  } else {
    // After break, return to focus session
    startTimer(State::FOCUS, focusDurationMs_);
  }
}

const char* PomodoroActivity::getStateLabel() const {
  switch (state_) {
    case State::IDLE:
      return "READY TO FOCUS";
    case State::FOCUS:
      return "FOCUS SESSION";
    case State::SHORT_BREAK:
      return "SHORT BREAK";
    case State::LONG_BREAK:
      return "LONG BREAK";
    case State::PAUSED:
      return "PAUSED";
  }
  return "";
}

void PomodoroActivity::onEnter() {
  Activity::onEnter();
  toybox::ensureFonts(renderer);

  state_ = State::IDLE;
  totalDurationMs_ = focusDurationMs_;
  completedSessions_ = 0;
  pausedElapsedMs_ = 0;
  selectedField_ = IdleField::FOCUS;

  loadStats();
  requestUpdate();
}

void PomodoroActivity::onExit() {
  saveStats();
  Activity::onExit();
}

void PomodoroActivity::loop() {
  // 1. Hardware Buttons
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    if (state_ == State::FOCUS || state_ == State::SHORT_BREAK || state_ == State::LONG_BREAK) {
      pausedFrom_ = state_;
      pausedElapsedMs_ += (millis() - timerStartMs_);
      state_ = State::PAUSED;
      requestUpdate();
      return;
    }
    if (state_ == State::PAUSED) {
      state_ = State::IDLE;
      totalDurationMs_ = focusDurationMs_;
      pausedElapsedMs_ = 0;
      requestUpdate();
      return;
    }
    shelf::leave(renderer, mappedInput);
    return;
  }

  // Confirm Button (Start in IDLE, Pause/Resume in active)
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    switch (state_) {
      case State::IDLE:
        startTimer(State::FOCUS, focusDurationMs_);
        return;
      case State::FOCUS:
      case State::SHORT_BREAK:
      case State::LONG_BREAK:
        pausedFrom_ = state_;
        pausedElapsedMs_ += (millis() - timerStartMs_);
        state_ = State::PAUSED;
        requestUpdate();
        return;
      case State::PAUSED:
        state_ = pausedFrom_;
        timerStartMs_ = millis();
        lastTickMs_ = millis();
        requestUpdate();
        return;
    }
  }

  // Right / PageForward (Skip session)
  if (mappedInput.wasReleased(MappedInputManager::Button::Right) ||
      mappedInput.wasReleased(MappedInputManager::Button::PageForward)) {
    if (state_ == State::IDLE) {
      startTimer(State::FOCUS, focusDurationMs_);
      return;
    }
    advanceState();
    return;
  }

  // Left / PageBack (Reset to IDLE)
  if (mappedInput.wasReleased(MappedInputManager::Button::Left) ||
      mappedInput.wasReleased(MappedInputManager::Button::PageBack)) {
    if (state_ != State::IDLE) {
      state_ = State::IDLE;
      totalDurationMs_ = focusDurationMs_;
      pausedElapsedMs_ = 0;
      requestUpdate();
      return;
    }
  }

  // Up/Down in IDLE adjusts selected duration
  if (state_ == State::IDLE) {
    if (mappedInput.wasReleased(MappedInputManager::Button::Up)) {
      if (focusDurationMs_ < 60 * 60 * 1000) {
        focusDurationMs_ += 5 * 60 * 1000;
        totalDurationMs_ = focusDurationMs_;
        requestUpdate();
      }
      return;
    }
    if (mappedInput.wasReleased(MappedInputManager::Button::Down)) {
      if (focusDurationMs_ > 5 * 60 * 1000) {
        focusDurationMs_ -= 5 * 60 * 1000;
        totalDurationMs_ = focusDurationMs_;
        requestUpdate();
      }
      return;
    }
  }

  // 2. Timer Expiry & Tick Handling
  if (state_ == State::FOCUS || state_ == State::SHORT_BREAK || state_ == State::LONG_BREAK) {
    if (getRemainingMs() == 0) {
      advanceState();
      return;
    }

    const unsigned long now = millis();
    if (now - lastTickMs_ >= 1000) {
      lastTickMs_ = now;
      requestUpdate();
    }
  }

  // 3. Touch Screen Interactions
  int tx = 0, ty = 0;
  if (!mappedInput.wasScreenTapped(tx, ty)) return;

  if (state_ == State::IDLE) {
    if (rectContains(exitBtnRect_, tx, ty)) {
      shelf::leave(renderer, mappedInput);
      return;
    }
    if (rectContains(startBtnRect_, tx, ty)) {
      startTimer(State::FOCUS, focusDurationMs_);
      return;
    }
    // Steppers: Focus
    if (rectContains(focusMinusRect_, tx, ty)) {
      if (focusDurationMs_ > 5 * 60 * 1000) {
        focusDurationMs_ -= 5 * 60 * 1000;
        totalDurationMs_ = focusDurationMs_;
        requestUpdate();
      }
      return;
    }
    if (rectContains(focusPlusRect_, tx, ty)) {
      if (focusDurationMs_ < 60 * 60 * 1000) {
        focusDurationMs_ += 5 * 60 * 1000;
        totalDurationMs_ = focusDurationMs_;
        requestUpdate();
      }
      return;
    }
    // Steppers: Short Break
    if (rectContains(shortMinusRect_, tx, ty)) {
      if (shortBreakDurationMs_ > 1 * 60 * 1000) {
        shortBreakDurationMs_ -= 1 * 60 * 1000;
        requestUpdate();
      }
      return;
    }
    if (rectContains(shortPlusRect_, tx, ty)) {
      if (shortBreakDurationMs_ < 30 * 60 * 1000) {
        shortBreakDurationMs_ += 1 * 60 * 1000;
        requestUpdate();
      }
      return;
    }
    // Steppers: Long Break
    if (rectContains(longMinusRect_, tx, ty)) {
      if (longBreakDurationMs_ > 5 * 60 * 1000) {
        longBreakDurationMs_ -= 5 * 60 * 1000;
        requestUpdate();
      }
      return;
    }
    if (rectContains(longPlusRect_, tx, ty)) {
      if (longBreakDurationMs_ < 60 * 60 * 1000) {
        longBreakDurationMs_ += 5 * 60 * 1000;
        requestUpdate();
      }
      return;
    }
  } else {
    // Active / Paused view buttons
    if (rectContains(resetBtnRect_, tx, ty)) {
      state_ = State::IDLE;
      totalDurationMs_ = focusDurationMs_;
      pausedElapsedMs_ = 0;
      requestUpdate();
      return;
    }
    if (rectContains(pauseResumeBtnRect_, tx, ty)) {
      if (state_ == State::PAUSED) {
        state_ = pausedFrom_;
        timerStartMs_ = millis();
        lastTickMs_ = millis();
      } else {
        pausedFrom_ = state_;
        pausedElapsedMs_ += (millis() - timerStartMs_);
        state_ = State::PAUSED;
      }
      requestUpdate();
      return;
    }
    if (rectContains(skipBtnRect_, tx, ty)) {
      advanceState();
      return;
    }
  }
}

void PomodoroActivity::render(RenderLock&&) {
  renderer.clearScreen();

  constexpr int leftMargin = 16;
  constexpr int rightMargin = 480 - 16;
  constexpr int contentWidth = rightMargin - leftMargin;

  // 1. Top Header Bar (y = 12, h = 34)
  if (state_ == State::IDLE) {
    exitBtnRect_ = Rect{leftMargin, 12, 80, 32};
    renderer.drawRect(exitBtnRect_.x, exitBtnRect_.y, exitBtnRect_.width, exitBtnRect_.height, true);
    renderer.drawCenteredText(UI_10_FONT_ID, 18, "< EXIT", true, EpdFontFamily::BOLD);

    renderer.drawCenteredText(UI_12_FONT_ID, 14, "POMODORO TIMER", true, EpdFontFamily::BOLD);
  } else {
    resetBtnRect_ = Rect{leftMargin, 12, 86, 32};
    renderer.drawRect(resetBtnRect_.x, resetBtnRect_.y, resetBtnRect_.width, resetBtnRect_.height, true);
    renderer.drawText(UI_10_FONT_ID, leftMargin + 10, 18, "< RESET", true, EpdFontFamily::BOLD);

    // Mode Banner in Header
    const char* label = getStateLabel();
    renderer.drawCenteredText(UI_12_FONT_ID, 14, label, true, EpdFontFamily::BOLD);

    // Session Indicator on Right
    char sessStr[24];
    std::snprintf(sessStr, sizeof(sessStr), "%d/4", completedSessions_ + 1);
    renderer.drawText(UI_10_FONT_ID, rightMargin - 40, 18, sessStr, true, EpdFontFamily::BOLD);
  }

  // Header Divider
  renderer.drawLine(leftMargin, 50, rightMargin, 50, true);

  if (state_ == State::IDLE) {
    // ---------------- IDLE CONFIGURATION VIEW ----------------
    // Top Hero Circle Preview
    constexpr int heroCx = 240;
    constexpr int heroCy = 125;
    constexpr int heroR = 48;
    drawProgressRing(renderer, heroCx, heroCy, heroR, 4, 1.0f);

    char heroMinStr[16];
    std::snprintf(heroMinStr, sizeof(heroMinStr), "%02u:00", focusDurationMs_ / 60000);
    renderer.drawCenteredText(UI_12_FONT_ID, heroCy - 10, heroMinStr, true, EpdFontFamily::BOLD);
    renderer.drawCenteredText(UI_10_FONT_ID, heroCy + 56, "Customize session lengths below", true);

    // Steppers Section
    auto drawStepper = [&](int y, const char* title, uint32_t ms, Rect& minusRect, Rect& plusRect) {
      renderer.drawRect(leftMargin, y, contentWidth, 68, true);
      renderer.drawText(UI_10_FONT_ID, leftMargin + 14, y + 8, title, true, EpdFontFamily::BOLD);

      minusRect = Rect{leftMargin + 14, y + 30, 70, 30};
      renderer.fillRect(minusRect.x, minusRect.y, minusRect.width, minusRect.height, true);
      renderer.drawCenteredText(UI_10_FONT_ID, y + 36, "-", false, EpdFontFamily::BOLD);

      char valBuf[32];
      std::snprintf(valBuf, sizeof(valBuf), "%u Minutes", ms / 60000);
      renderer.drawCenteredText(UI_10_FONT_ID, y + 36, valBuf, true, EpdFontFamily::BOLD);

      plusRect = Rect{rightMargin - 84, y + 30, 70, 30};
      renderer.fillRect(plusRect.x, plusRect.y, plusRect.width, plusRect.height, true);
      renderer.drawCenteredText(UI_10_FONT_ID, y + 36, "+", false, EpdFontFamily::BOLD);
    };

    drawStepper(204, "FOCUS TIME", focusDurationMs_, focusMinusRect_, focusPlusRect_);
    drawStepper(284, "SHORT BREAK", shortBreakDurationMs_, shortMinusRect_, shortPlusRect_);
    drawStepper(364, "LONG BREAK (AFTER 4 SESSIONS)", longBreakDurationMs_, longMinusRect_, longPlusRect_);

    // Big Start Button
    startBtnRect_ = Rect{leftMargin, 452, contentWidth, 54};
    renderer.fillRect(startBtnRect_.x, startBtnRect_.y, startBtnRect_.width, startBtnRect_.height, true);
    char startLabel[48];
    std::snprintf(startLabel, sizeof(startLabel), "START FOCUS (%u min)", focusDurationMs_ / 60000);
    renderer.drawCenteredText(UI_12_FONT_ID, startBtnRect_.y + 16, startLabel, false, EpdFontFamily::BOLD);

    // Daily Productivity Box
    renderer.drawRect(leftMargin, 530, contentWidth, 120, true);
    renderer.drawText(UI_10_FONT_ID, leftMargin + 14, 542, "TODAY'S PRODUCTIVITY", true, EpdFontFamily::BOLD);
    renderer.drawLine(leftMargin + 14, 564, rightMargin - 14, 564, true);

    char statsLine1[64];
    std::snprintf(statsLine1, sizeof(statsLine1), "Completed Pomodoros: %d sessions", totalCompletedToday_);
    renderer.drawText(UI_10_FONT_ID, leftMargin + 14, 576, statsLine1, true);

    char statsLine2[64];
    const unsigned long focusedMinutes = totalCompletedToday_ * (focusDurationMs_ / 60000);
    std::snprintf(statsLine2, sizeof(statsLine2), "Total Focused Time: ~%lu minutes", focusedMinutes);
    renderer.drawText(UI_10_FONT_ID, leftMargin + 14, 604, statsLine2, true);

    renderer.drawCenteredText(UI_10_FONT_ID, 740, "Press [Confirm] or Tap button to Start", true);
  } else {
    // ---------------- ACTIVE / PAUSED TIMER VIEW ----------------
    // Circular Progress Ring
    constexpr int ringCx = 240;
    constexpr int ringCy = 270;
    constexpr int ringRadius = 120;
    constexpr int ringThickness = 12;

    const uint32_t remaining = getRemainingMs();
    float progress = 0.0f;
    if (totalDurationMs_ > 0) {
      progress = static_cast<float>(totalDurationMs_ - remaining) / static_cast<float>(totalDurationMs_);
      if (progress < 0.0f) progress = 0.0f;
      if (progress > 1.0f) progress = 1.0f;
    }

    drawProgressRing(renderer, ringCx, ringCy, ringRadius, ringThickness, progress);

    // Text Inside Progress Ring
    renderer.drawCenteredText(UI_10_FONT_ID, ringCy - 55, getStateLabel(), true, EpdFontFamily::BOLD);

    const int minutes = remaining / 60000;
    const int seconds = (remaining % 60000) / 1000;
    char timeStr[16];
    std::snprintf(timeStr, sizeof(timeStr), "%02d:%02d", minutes, seconds);
    renderer.drawCenteredText(UI_12_FONT_ID, ringCy - 15, timeStr, true, EpdFontFamily::BOLD);

    if (state_ == State::PAUSED) {
      renderer.drawCenteredText(UI_10_FONT_ID, ringCy + 32, "- PAUSED -", true);
    } else {
      renderer.drawCenteredText(UI_10_FONT_ID, ringCy + 32, "Remaining", true);
    }

    // 4 Session Progress Indicators (under ring)
    constexpr int dotBoxW = 44;
    constexpr int dotBoxH = 28;
    constexpr int dotSpacing = 16;
    constexpr int totalDotsW = kSessionsBeforeLongBreak * dotBoxW + (kSessionsBeforeLongBreak - 1) * dotSpacing;
    const int dotStartX = (480 - totalDotsW) / 2;
    const int dotY = ringCy + ringRadius + 24;

    for (int i = 0; i < kSessionsBeforeLongBreak; ++i) {
      const int dx = dotStartX + i * (dotBoxW + dotSpacing);
      char dotLbl[8];
      std::snprintf(dotLbl, sizeof(dotLbl), "%d", i + 1);

      if (i < completedSessions_) {
        // Finished session in cycle
        renderer.fillRect(dx, dotY, dotBoxW, dotBoxH, true);
        renderer.drawCenteredText(UI_10_FONT_ID, dotY + 6, dotLbl, false, EpdFontFamily::BOLD);
      } else if (i == completedSessions_ && state_ == State::FOCUS) {
        // Active session
        renderer.drawRect(dx, dotY, dotBoxW, dotBoxH, true);
        renderer.drawRect(dx + 2, dotY + 2, dotBoxW - 4, dotBoxH - 4, true);
        renderer.drawCenteredText(UI_10_FONT_ID, dotY + 6, dotLbl, true, EpdFontFamily::BOLD);
      } else {
        // Upcoming session
        renderer.drawRect(dx, dotY, dotBoxW, dotBoxH, true);
        renderer.drawCenteredText(UI_10_FONT_ID, dotY + 6, dotLbl, true);
      }
    }

    // Control Buttons (Pause / Resume & Skip)
    const int btnY = dotY + dotBoxH + 34;
    pauseResumeBtnRect_ = Rect{leftMargin, btnY, 216, 52};
    if (state_ == State::PAUSED) {
      renderer.fillRect(pauseResumeBtnRect_.x, pauseResumeBtnRect_.y, pauseResumeBtnRect_.width,
                        pauseResumeBtnRect_.height, true);
      renderer.drawCenteredText(UI_12_FONT_ID, btnY + 15, "RESUME", false, EpdFontFamily::BOLD);
    } else {
      renderer.drawRect(pauseResumeBtnRect_.x, pauseResumeBtnRect_.y, pauseResumeBtnRect_.width,
                        pauseResumeBtnRect_.height, true);
      renderer.drawCenteredText(UI_12_FONT_ID, btnY + 15, "PAUSE", true, EpdFontFamily::BOLD);
    }

    skipBtnRect_ = Rect{rightMargin - 216, btnY, 216, 52};
    renderer.drawRect(skipBtnRect_.x, skipBtnRect_.y, skipBtnRect_.width, skipBtnRect_.height, true);
    renderer.drawCenteredText(UI_12_FONT_ID, btnY + 15, "SKIP >>", true, EpdFontFamily::BOLD);

    // Tip Box
    const int tipY = btnY + 68;
    renderer.drawRect(leftMargin, tipY, contentWidth, 76, true);
    renderer.drawText(UI_10_FONT_ID, leftMargin + 14, tipY + 8, "FOCUS TIP:", true, EpdFontFamily::BOLD);

    if (state_ == State::FOCUS) {
      renderer.drawText(UI_10_FONT_ID, leftMargin + 14, tipY + 34,
                        "Protect your attention. Silence interruptions.", true);
    } else if (state_ == State::SHORT_BREAK) {
      renderer.drawText(UI_10_FONT_ID, leftMargin + 14, tipY + 34,
                        "Rest your eyes, stretch your back, drink water.", true);
    } else if (state_ == State::LONG_BREAK) {
      renderer.drawText(UI_10_FONT_ID, leftMargin + 14, tipY + 34,
                        "Great job completing a 4-session cycle! Step away.", true);
    } else {
      renderer.drawText(UI_10_FONT_ID, leftMargin + 14, tipY + 34,
                        "Paused. Take a breath and resume when ready.", true);
    }

    renderer.drawCenteredText(UI_10_FONT_ID, 740, "Hardware: [Confirm] Pause/Resume   [Right] Skip", true);
  }

  renderer.displayBuffer();
}
