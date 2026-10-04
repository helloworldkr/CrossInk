#include "BreatheActivity.h"

#include <GfxRenderer.h>
#include <Memory.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

#include "../../components/themes/BaseTheme.h"
#include "../Shelf.h"
#include "../ui/ToyboxFonts.h"
#include "../ui/ToyboxMetrics.h"
#include "../ui/ToyboxText.h"
#include "../ui/ToyboxTheme.h"

namespace fui = freeink::ui;

namespace breathe {

SessionEngine::SessionEngine() {
  configure(Technique::Equal, 8);
}

void SessionEngine::configure(Technique technique, int breathCount) {
  technique_ = technique;
  totalBreaths_ = std::clamp(breathCount, 1, 30);
  currentBreath_ = 1;
  currentPhase_ = Phase::Inhale;
  secondsRemainingInPhase_ = phaseDuration();
  totalElapsedSeconds_ = 0;
  paused_ = false;
  finished_ = false;
}

void SessionEngine::start() {
  currentBreath_ = 1;
  currentPhase_ = Phase::Inhale;
  secondsRemainingInPhase_ = phaseDuration();
  totalElapsedSeconds_ = 0;
  paused_ = false;
  finished_ = false;
}

void SessionEngine::pause() {
  paused_ = true;
}

void SessionEngine::resume() {
  paused_ = false;
}

void SessionEngine::stop() {
  finished_ = true;
  paused_ = false;
}

bool SessionEngine::tick() {
  if (finished_ || paused_) return false;

  ++totalElapsedSeconds_;
  --secondsRemainingInPhase_;

  if (secondsRemainingInPhase_ <= 0) {
    advanceToNextPhase();
  }

  return !finished_;
}

void SessionEngine::advanceToNextPhase() {
  switch (technique_) {
    case Technique::Equal:
      if (currentPhase_ == Phase::Inhale) {
        currentPhase_ = Phase::Exhale;
      } else {
        ++currentBreath_;
        if (currentBreath_ > totalBreaths_) {
          finished_ = true;
          return;
        }
        currentPhase_ = Phase::Inhale;
      }
      break;

    case Technique::Relax478:
      if (currentPhase_ == Phase::Inhale) {
        currentPhase_ = Phase::HoldIn;
      } else if (currentPhase_ == Phase::HoldIn) {
        currentPhase_ = Phase::Exhale;
      } else {
        ++currentBreath_;
        if (currentBreath_ > totalBreaths_) {
          finished_ = true;
          return;
        }
        currentPhase_ = Phase::Inhale;
      }
      break;

    case Technique::Box:
      if (currentPhase_ == Phase::Inhale) {
        currentPhase_ = Phase::HoldIn;
      } else if (currentPhase_ == Phase::HoldIn) {
        currentPhase_ = Phase::Exhale;
      } else if (currentPhase_ == Phase::Exhale) {
        currentPhase_ = Phase::HoldOut;
      } else {
        ++currentBreath_;
        if (currentBreath_ > totalBreaths_) {
          finished_ = true;
          return;
        }
        currentPhase_ = Phase::Inhale;
      }
      break;
  }

  secondsRemainingInPhase_ = phaseDuration();
}

int SessionEngine::phaseDuration() const {
  if (technique_ == Technique::Relax478) {
    if (currentPhase_ == Phase::HoldIn) return 7;
    if (currentPhase_ == Phase::Exhale) return 8;
  }
  return 4;
}

float SessionEngine::phaseProgress() const {
  const int duration = phaseDuration();
  if (duration <= 0) return 1.0f;
  const int elapsed = duration - secondsRemainingInPhase_;
  float prog = static_cast<float>(elapsed) / static_cast<float>(duration);
  return std::clamp(prog, 0.0f, 1.0f);
}

float SessionEngine::sessionProgress() const {
  const int totalEst = totalEstimatedSeconds();
  if (totalEst <= 0) return 1.0f;
  float prog = static_cast<float>(totalElapsedSeconds_) / static_cast<float>(totalEst);
  return std::clamp(prog, 0.0f, 1.0f);
}

const char* SessionEngine::phaseLabel() const {
  static const char* const kLabels[4] = {"BREATHE IN", "HOLD", "BREATHE OUT", "HOLD"};
  const size_t idx = static_cast<size_t>(currentPhase_);
  return idx < 4 ? kLabels[idx] : "BREATHE";
}

const char* SessionEngine::mindfulCue() const {
  static const char* const kCues[4] = {
      "Inhale deep through nose...",
      "Stillness, soften shoulders...",
      "Exhale slow, release tension...",
      "Empty lungs, rest in calm...",
  };
  const size_t idx = static_cast<size_t>(currentPhase_);
  return idx < 4 ? kCues[idx] : "Follow breath rhythm.";
}

const char* SessionEngine::techniqueName(Technique t) {
  static const char* const kNames[3] = {
      "EQUAL BREATHING",
      "4-7-8 BREATHING",
      "BOX BREATHING",
  };
  const size_t idx = static_cast<size_t>(t);
  return idx < 3 ? kNames[idx] : "BREATHING";
}

const char* SessionEngine::techniqueSubtitle(Technique t) {
  static const char* const kSubtitles[3] = {
      "4s IN - 4s OUT (8s cycle)",
      "4s IN - 7s HOLD - 8s OUT (19s)",
      "4s IN - 4s HOLD - 4s OUT - 4s HOLD (16s)",
  };
  const size_t idx = static_cast<size_t>(t);
  return idx < 3 ? kSubtitles[idx] : "";
}

const char* SessionEngine::techniqueDescription(Technique t) {
  static const char* const kDescs[3] = {
      "Balanced calm - Sama Vritti",
      "Deep nervous system reset",
      "Focus and stress control",
  };
  const size_t idx = static_cast<size_t>(t);
  return idx < 3 ? kDescs[idx] : "";
}

int SessionEngine::cycleDuration(Technique t) {
  static const uint8_t kDurations[3] = {8, 19, 16};
  const size_t idx = static_cast<size_t>(t);
  return idx < 3 ? kDurations[idx] : 8;
}

void SessionEngine::formatTime(int totalSeconds, char* out, size_t outSize) {
  if (!out || outSize == 0) return;
  const int mins = totalSeconds / 60;
  const int secs = totalSeconds % 60;
  if (mins > 0) {
    std::snprintf(out, outSize, "%dm %02ds", mins, secs);
  } else {
    std::snprintf(out, outSize, "%ds", secs);
  }
}

}  // namespace breathe

namespace {

enum : fui::ActionId {
  ActionTechEqual = 10,
  ActionTech478,
  ActionTechBox,
  ActionCount4,
  ActionCount8,
  ActionCount12,
  ActionCount16,
  ActionCountMinus,
  ActionCountPlus,
  ActionStart,
  ActionPauseResume,
  ActionStop,
  ActionRestart,
  ActionFinish,
};

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

static inline void drawCircle(toybox::Screen& screen, int cx, int cy, int radius, int lineWidth = 1) {
  if (radius <= 0) return;
  const auto ink = fui::Paint::solid(fui::Color::Black);
  const int size = radius * 2;
  const fui::Rect r = fui::makeRect(cx - radius, cy - radius, size, size);
  screen.target().stroke(r, ink, lineWidth, radius);
}

void buildSetup(toybox::Screen& screen, breathe::Technique selectedTech, int selectedBreaths) {
  toybox::absoluteChrome(screen);
  const fui::DeviceContext& device = screen.device();
  const auto ink = fui::Paint::solid(fui::Color::Black);

  const int leftMargin = toybox::kMargin;
  const int rightMargin = device.width - toybox::kMargin;
  const int contentWidth = rightMargin - leftMargin;

  // Header Bar
  constexpr int topMargin = 14;
  constexpr int headerHeight = 34;

  drawText(screen, fui::makeRect(leftMargin, topMargin, contentWidth - 120, headerHeight), "BREATHE", toybox::kUiFont);
  drawText(screen, fui::makeRect(rightMargin - 120, topMargin, 120, headerHeight), "RELAXATION", toybox::kTileFont, fui::TextAlign::Right);

  const int ruleY = topMargin + headerHeight + 2;
  screen.target().fill(fui::makeRect(leftMargin, ruleY, contentWidth, 2), ink);

  // Technique Selection
  drawText(screen, fui::makeRect(leftMargin, ruleY + 8, contentWidth, 22), "SELECT BREATHING TECHNIQUE:", toybox::kTileFont, fui::TextAlign::Left, fui::Color::DarkGray);

  constexpr int cardH = 92;
  constexpr int cardGap = 8;
  int currentY = ruleY + 34;

  for (int i = 0; i < 3; ++i) {
    const auto tech = static_cast<breathe::Technique>(i);
    const bool isSelected = (selectedTech == tech);
    const fui::Rect box = fui::makeRect(leftMargin, currentY, contentWidth, cardH);

    screen.target().stroke(box, ink, isSelected ? 2 : 1, 6);

    drawText(screen, fui::makeRect(box.x + 16, box.y + 12, box.width - 130, 22),
             breathe::SessionEngine::techniqueName(tech), toybox::kTileFont);
    drawText(screen, fui::makeRect(box.x + 16, box.y + 40, box.width - 130, 18),
             breathe::SessionEngine::techniqueSubtitle(tech), toybox::kSmallFont);
    drawText(screen, fui::makeRect(box.x + 16, box.y + 64, box.width - 130, 16),
             breathe::SessionEngine::techniqueDescription(tech), toybox::kSmallFont, fui::TextAlign::Left, fui::Color::DarkGray);

    const fui::Rect badge = fui::makeRect(box.x + box.width - 104, box.y + 32, 92, 28);
    if (isSelected) {
      screen.target().fill(badge, ink, 4);
      drawText(screen, fui::makeRect(badge.x, badge.y + 6, badge.width, 16), "SELECTED", toybox::kSmallFont, fui::TextAlign::Center, fui::Color::White);
    } else {
      screen.target().stroke(badge, ink, 1, 4);
      drawText(screen, fui::makeRect(badge.x, badge.y + 6, badge.width, 16), "SELECT", toybox::kSmallFont, fui::TextAlign::Center);
    }

    addBtn(screen, static_cast<fui::ActionId>(ActionTechEqual + i), box);
    currentY += cardH + cardGap;
  }

  // Breath Count & Estimated Time
  currentY += 6;
  drawText(screen, fui::makeRect(leftMargin, currentY, contentWidth, 22), "NUMBER OF BREATHS:", toybox::kTileFont, fui::TextAlign::Left, fui::Color::DarkGray);

  currentY += 28;
  constexpr int countBtnH = 38;
  constexpr int countGap = 8;
  const int countBtnW = (contentWidth - countGap * 3) / 4;

  for (int p = 0; p < 4; ++p) {
    const int count = (p + 1) * 4;
    const fui::Rect pBox = fui::makeRect(leftMargin + p * (countBtnW + countGap), currentY, countBtnW, countBtnH);
    const bool isMatch = (selectedBreaths == count);

    if (isMatch) {
      screen.target().fill(pBox, ink, 4);
    } else {
      screen.target().stroke(pBox, ink, 1, 4);
    }

    char numStr[8];
    std::snprintf(numStr, sizeof(numStr), "%d", count);
    drawText(screen, fui::makeRect(pBox.x, pBox.y + 10, pBox.width, 20), numStr, toybox::kTileFont,
             fui::TextAlign::Center, isMatch ? fui::Color::White : fui::Color::Black);

    addBtn(screen, static_cast<fui::ActionId>(ActionCount4 + p), pBox);
  }

  // Stepper Row
  currentY += countBtnH + 12;
  const fui::Rect minusBox = fui::makeRect(leftMargin, currentY, 52, 42);
  screen.target().stroke(minusBox, ink, 1, 4);
  drawText(screen, fui::makeRect(minusBox.x, minusBox.y + 8, minusBox.width, 24), "-", toybox::kUiFont, fui::TextAlign::Center);
  addBtn(screen, ActionCountMinus, minusBox);

  const fui::Rect plusBox = fui::makeRect(rightMargin - 52, currentY, 52, 42);
  screen.target().stroke(plusBox, ink, 1, 4);
  drawText(screen, fui::makeRect(plusBox.x, plusBox.y + 8, plusBox.width, 24), "+", toybox::kUiFont, fui::TextAlign::Center);
  addBtn(screen, ActionCountPlus, plusBox);

  char breathsStr[32];
  std::snprintf(breathsStr, sizeof(breathsStr), "%d BREATHS", selectedBreaths);
  drawText(screen, fui::makeRect(leftMargin + 60, currentY + 10, contentWidth - 120, 24), breathsStr, toybox::kUiFont, fui::TextAlign::Center);

  // Estimated Duration Banner
  currentY += 56;
  const int totalSeconds = breathe::SessionEngine::cycleDuration(selectedTech) * selectedBreaths;
  char timeStr[32];
  breathe::SessionEngine::formatTime(totalSeconds, timeStr, sizeof(timeStr));

  const fui::Rect estBox = fui::makeRect(leftMargin, currentY, contentWidth, 54);
  screen.target().stroke(estBox, ink, 1, 6);

  char estBanner[64];
  std::snprintf(estBanner, sizeof(estBanner), "ESTIMATED SESSION TIME: %s", timeStr);
  drawText(screen, fui::makeRect(estBox.x, estBox.y + 16, estBox.width, 24), estBanner, toybox::kTileFont, fui::TextAlign::Center);

  // Start Button
  constexpr int footerH = 48;
  const int footerY = device.height - footerH - 16;
  const fui::Rect startBox = fui::makeRect(leftMargin, footerY, contentWidth, footerH);

  screen.target().fill(startBox, ink, 6);
  drawText(screen, fui::makeRect(startBox.x, startBox.y + 12, startBox.width, 26), "START BREATHING", toybox::kUiFont, fui::TextAlign::Center, fui::Color::White);
  addBtn(screen, ActionStart, startBox);
}

void buildActive(toybox::Screen& screen, const breathe::SessionEngine& engine) {
  toybox::absoluteChrome(screen);
  const fui::DeviceContext& device = screen.device();
  const auto ink = fui::Paint::solid(fui::Color::Black);

  const int leftMargin = toybox::kMargin;
  const int rightMargin = device.width - toybox::kMargin;
  const int contentWidth = rightMargin - leftMargin;

  // Top Header
  constexpr int topMargin = 14;
  constexpr int headerHeight = 34;

  drawText(screen, fui::makeRect(leftMargin, topMargin, contentWidth - 140, headerHeight),
           breathe::SessionEngine::techniqueName(engine.technique()), toybox::kUiFont);

  char countHeader[32];
  std::snprintf(countHeader, sizeof(countHeader), "BREATH %d / %d", engine.currentBreath(), engine.totalBreaths());
  drawText(screen, fui::makeRect(rightMargin - 150, topMargin + 4, 150, 24), countHeader, toybox::kTileFont, fui::TextAlign::Right);

  const int ruleY = topMargin + headerHeight + 2;
  screen.target().fill(fui::makeRect(leftMargin, ruleY, contentWidth, 2), ink);

  // Visual Breathing Mandala
  constexpr int cx = 240;
  constexpr int cy = 295;
  constexpr int baseR = 40;
  constexpr int maxR = 120;

  const float pProg = engine.phaseProgress();
  int currentRadius = baseR;
  if (engine.currentPhase() == breathe::Phase::Inhale) {
    currentRadius = baseR + static_cast<int>((maxR - baseR) * pProg);
  } else if (engine.currentPhase() == breathe::Phase::Exhale) {
    currentRadius = maxR - static_cast<int>((maxR - baseR) * pProg);
  } else {
    currentRadius = (engine.currentPhase() == breathe::Phase::HoldIn) ? maxR : baseR;
  }

  // Box Breathing perimeter frame & traveling marker
  if (engine.technique() == breathe::Technique::Box) {
    constexpr int boxSize = 250;
    const fui::Rect boxFrame = fui::makeRect(cx - boxSize / 2, cy - boxSize / 2, boxSize, boxSize);
    screen.target().stroke(boxFrame, ink, 1, 16);

    int dotX = cx;
    int dotY = cy;
    constexpr int half = boxSize / 2;

    switch (engine.currentPhase()) {
      case breathe::Phase::Inhale:
        dotX = (cx - half) + static_cast<int>(boxSize * pProg);
        dotY = cy - half;
        break;
      case breathe::Phase::HoldIn:
        dotX = cx + half;
        dotY = (cy - half) + static_cast<int>(boxSize * pProg);
        break;
      case breathe::Phase::Exhale:
        dotX = (cx + half) - static_cast<int>(boxSize * pProg);
        dotY = cy + half;
        break;
      case breathe::Phase::HoldOut:
        dotX = cx - half;
        dotY = (cy + half) - static_cast<int>(boxSize * pProg);
        break;
    }

    constexpr int markerR = 7;
    screen.target().fill(fui::makeRect(dotX - markerR, dotY - markerR, markerR * 2, markerR * 2), ink, markerR);
  }

  // Concentric Rings
  drawCircle(screen, cx, cy, currentRadius, 2);
  if (currentRadius > baseR + 25) {
    drawCircle(screen, cx, cy, currentRadius - 25, 1);
  }
  if (currentRadius > baseR + 50) {
    drawCircle(screen, cx, cy, currentRadius - 50, 1);
  }
  drawCircle(screen, cx, cy, baseR, 1);

  // If Hold: draw 4 calm orbital marks around the halo
  if (engine.currentPhase() == breathe::Phase::HoldIn) {
    constexpr int dotR = 4;
    const int d = currentRadius + 8;
    const int offsets[4][2] = {{0, -d}, {d, 0}, {0, d}, {-d, 0}};
    for (int k = 0; k < 4; ++k) {
      screen.target().fill(fui::makeRect(cx + offsets[k][0] - dotR, cy + offsets[k][1] - dotR, dotR * 2, dotR * 2), ink, dotR);
    }
  }

  // Center Text Block inside the mandala (clean white knockout pill for legibility)
  const fui::Rect textPill = fui::makeRect(cx - 95, cy - 30, 190, 60);
  screen.target().fill(textPill, fui::Paint::solid(fui::Color::White), 8);
  screen.target().stroke(textPill, ink, 1, 8);

  const fui::Rect labelBox = fui::makeRect(cx - 90, cy - 24, 180, 24);
  drawText(screen, labelBox, engine.phaseLabel(), toybox::kTileFont, fui::TextAlign::Center);

  char secStr[16];
  std::snprintf(secStr, sizeof(secStr), "%d s", engine.secondsRemainingInPhase());
  drawText(screen, fui::makeRect(cx - 50, cy + 4, 100, 22), secStr, toybox::kTileFont, fui::TextAlign::Center);

  // Session Progress & Guidance
  constexpr int infoStartY = 480;

  const fui::Rect barBox = fui::makeRect(leftMargin + 30, infoStartY, contentWidth - 60, 10);
  screen.target().stroke(barBox, ink, 1, 3);
  const int fillW = static_cast<int>((contentWidth - 60) * engine.sessionProgress());
  if (fillW > 2) {
    screen.target().fill(fui::makeRect(barBox.x, barBox.y, fillW, barBox.height), ink, 3);
  }

  char elapsedStr[16];
  char totalEstStr[16];
  breathe::SessionEngine::formatTime(engine.totalElapsedSeconds(), elapsedStr, sizeof(elapsedStr));
  breathe::SessionEngine::formatTime(engine.totalEstimatedSeconds(), totalEstStr, sizeof(totalEstStr));

  char timeProgressBuf[48];
  std::snprintf(timeProgressBuf, sizeof(timeProgressBuf), "Elapsed: %s  /  Total: ~%s", elapsedStr, totalEstStr);
  drawText(screen, fui::makeRect(leftMargin, infoStartY + 18, contentWidth, 20), timeProgressBuf, toybox::kSmallFont, fui::TextAlign::Center);

  drawText(screen, fui::makeRect(leftMargin + 20, infoStartY + 50, contentWidth - 40, 24), engine.mindfulCue(), toybox::kTileFont, fui::TextAlign::Center, fui::Color::DarkGray);

  if (engine.isPaused()) {
    const fui::Rect pauseNotice = fui::makeRect(leftMargin + 60, infoStartY + 86, contentWidth - 120, 32);
    screen.target().fill(pauseNotice, ink, 4);
    drawText(screen, fui::makeRect(pauseNotice.x, pauseNotice.y + 6, pauseNotice.width, 20), "SESSION PAUSED", toybox::kTileFont, fui::TextAlign::Center, fui::Color::White);
  }

  // Bottom Controls
  constexpr int footerH = 46;
  const int footerY = device.height - footerH - 16;
  constexpr int gap = 16;
  const int btnW = (contentWidth - gap) / 2;

  const fui::Rect pauseBox = fui::makeRect(leftMargin, footerY, btnW, footerH);
  screen.target().stroke(pauseBox, ink, 1, 4);
  drawText(screen, fui::makeRect(pauseBox.x, pauseBox.y + 12, pauseBox.width, 22), engine.isPaused() ? "RESUME" : "PAUSE", toybox::kTileFont, fui::TextAlign::Center);
  addBtn(screen, ActionPauseResume, pauseBox);

  const fui::Rect stopBox = fui::makeRect(leftMargin + btnW + gap, footerY, btnW, footerH);
  screen.target().stroke(stopBox, ink, 1, 4);
  drawText(screen, fui::makeRect(stopBox.x, stopBox.y + 12, stopBox.width, 22), "STOP", toybox::kTileFont, fui::TextAlign::Center);
  addBtn(screen, ActionStop, stopBox);
}

void buildComplete(toybox::Screen& screen, const breathe::SessionEngine& engine) {
  toybox::absoluteChrome(screen);
  const fui::DeviceContext& device = screen.device();
  const auto ink = fui::Paint::solid(fui::Color::Black);

  const int leftMargin = toybox::kMargin;
  const int rightMargin = device.width - toybox::kMargin;
  const int contentWidth = rightMargin - leftMargin;

  constexpr int topMargin = 14;
  constexpr int headerHeight = 34;

  drawText(screen, fui::makeRect(leftMargin, topMargin, contentWidth, headerHeight), "SESSION COMPLETE", toybox::kUiFont, fui::TextAlign::Center);

  const int ruleY = topMargin + headerHeight + 2;
  screen.target().fill(fui::makeRect(leftMargin, ruleY, contentWidth, 2), ink);

  constexpr int cx = 240;
  constexpr int cy = 160;
  drawCircle(screen, cx, cy, 54, 2);
  drawCircle(screen, cx, cy, 38, 1);
  drawCircle(screen, cx, cy, 22, 1);

  drawText(screen, fui::makeRect(leftMargin, cy + 70, contentWidth, 26), "TAKE A MOMENT TO NOTICE HOW YOU FEEL", toybox::kUiFont, fui::TextAlign::Center);

  const fui::Rect card = fui::makeRect(leftMargin, cy + 115, contentWidth, 140);
  screen.target().stroke(card, ink, 1, 6);

  drawText(screen, fui::makeRect(card.x + 20, card.y + 18, card.width - 40, 24), breathe::SessionEngine::techniqueName(engine.technique()), toybox::kUiFont);

  char breathsStr[48];
  std::snprintf(breathsStr, sizeof(breathsStr), "COMPLETED: %d MINDFUL BREATHS", engine.totalBreaths());
  drawText(screen, fui::makeRect(card.x + 20, card.y + 48, card.width - 40, 20), breathsStr, toybox::kTileFont);

  char timeStr[32];
  breathe::SessionEngine::formatTime(engine.totalElapsedSeconds(), timeStr, sizeof(timeStr));
  char durStr[64];
  std::snprintf(durStr, sizeof(durStr), "TOTAL DURATION: %s", timeStr);
  drawText(screen, fui::makeRect(card.x + 20, card.y + 74, card.width - 40, 20), durStr, toybox::kTileFont);

  drawText(screen, fui::makeRect(card.x + 20, card.y + 104, card.width - 40, 20), "Carry this calm presence with you.", toybox::kSmallFont, fui::TextAlign::Left, fui::Color::DarkGray);

  constexpr int footerH = 46;
  const int footerY = device.height - footerH - 16;
  constexpr int gap = 16;
  const int btnW = (contentWidth - gap) / 2;

  const fui::Rect restartBox = fui::makeRect(leftMargin, footerY, btnW, footerH);
  screen.target().stroke(restartBox, ink, 1, 4);
  drawText(screen, fui::makeRect(restartBox.x, restartBox.y + 12, restartBox.width, 22), "BREATHE AGAIN", toybox::kTileFont, fui::TextAlign::Center);
  addBtn(screen, ActionRestart, restartBox);

  const fui::Rect finishBox = fui::makeRect(leftMargin + btnW + gap, footerY, btnW, footerH);
  screen.target().fill(finishBox, ink, 4);
  drawText(screen, fui::makeRect(finishBox.x, finishBox.y + 12, finishBox.width, 22), "DONE", toybox::kTileFont, fui::TextAlign::Center, fui::Color::White);
  addBtn(screen, ActionFinish, finishBox);
}

}  // namespace

std::unique_ptr<Activity> BreatheActivity::create(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return makeUniqueNoThrow<BreatheActivity>(renderer, mappedInput);
}

void BreatheActivity::onEnter() {
  Activity::onEnter();
  toybox::ensureFonts(renderer);
  openSetup();
}

void BreatheActivity::onExit() {
  engine_.stop();
  Activity::onExit();
}

void BreatheActivity::openSetup() {
  screenState_ = ScreenState::Setup;
  interactionsReady_ = false;
  requestUpdate();
}

void BreatheActivity::startBreathing() {
  engine_.configure(selectedTech_, selectedBreaths_);
  engine_.start();
  lastTickMs_ = millis();
  screenState_ = ScreenState::Active;
  interactionsReady_ = false;
  requestUpdate();
}

void BreatheActivity::completeSession() {
  screenState_ = ScreenState::Complete;
  interactionsReady_ = false;
  requestUpdate();
}

void BreatheActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    switch (screenState_) {
      case ScreenState::Setup:
        shelf::leave(renderer, mappedInput);
        return;
      case ScreenState::Active:
        engine_.stop();
        openSetup();
        return;
      case ScreenState::Complete:
        openSetup();
        return;
    }
  }

  if (screenState_ == ScreenState::Active && !engine_.isPaused()) {
    const uint32_t now = millis();
    if (now - lastTickMs_ >= 1000) {
      lastTickMs_ = now;
      if (engine_.tick()) {
        interactionsReady_ = false;
        requestUpdate();
      } else {
        completeSession();
      }
    }
  }

  int x = 0;
  int y = 0;
  if (!mappedInput.wasScreenTapped(x, y) || !interactionsReady_) return;

  fui::InputSnapshot input{};
  input.touchReleased = true;
  input.touchX = static_cast<int16_t>(x);
  input.touchY = static_cast<int16_t>(y);
  const fui::ActionEvent action = interactions_.route(input);

  if (action.action >= ActionTechEqual && action.action <= ActionTechBox) {
    selectedTech_ = static_cast<breathe::Technique>(action.action - ActionTechEqual);
    requestUpdate();
    return;
  }
  if (action.action >= ActionCount4 && action.action <= ActionCount16) {
    selectedBreaths_ = 4 * (action.action - ActionCount4 + 1);
    requestUpdate();
    return;
  }

  switch (action.action) {
    case ActionCountMinus:
      if (selectedBreaths_ > 1) {
        --selectedBreaths_;
        requestUpdate();
      }
      return;
    case ActionCountPlus:
      if (selectedBreaths_ < 30) {
        ++selectedBreaths_;
        requestUpdate();
      }
      return;
    case ActionStart:
      startBreathing();
      return;
    case ActionPauseResume:
      if (engine_.isPaused()) {
        engine_.resume();
        lastTickMs_ = millis();
      } else {
        engine_.pause();
      }
      requestUpdate();
      return;
    case ActionStop:
      engine_.stop();
      openSetup();
      return;
    case ActionRestart:
      startBreathing();
      return;
    case ActionFinish:
      openSetup();
      return;
    default:
      return;
  }
}

void BreatheActivity::render(RenderLock&&) {
  renderer.clearScreen();
  fui::GfxRendererTarget target = toybox::makeTarget(renderer);
  const fui::InputSnapshot noInput{};
  interactionsReady_ = false;
  toybox::Frame frame(target, target.deviceContext(), noInput, interactions_);
  toybox::Screen screen(frame);

  switch (screenState_) {
    case ScreenState::Setup:
      buildSetup(screen, selectedTech_, selectedBreaths_);
      break;
    case ScreenState::Active:
      buildActive(screen, engine_);
      break;
    case ScreenState::Complete:
      buildComplete(screen, engine_);
      break;
  }

  interactionsReady_ = true;
  toybox::reportOverflow(interactions_, "Breathe");
  renderer.displayBuffer();
}
