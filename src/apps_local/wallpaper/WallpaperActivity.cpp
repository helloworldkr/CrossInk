#include "WallpaperActivity.h"

#include <Bitmap.h>
#include <FsHelpers.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <Logging.h>
#include <Memory.h>

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "../../CrossPointSettings.h"
#include "../../CrossPointState.h"
#include "../../activities/util/KeyboardEntryActivity.h"
#include "../../components/themes/BaseTheme.h"
#include "../../util/ScreenshotUtil.h"
#include "../Shelf.h"
#include "../ui/ToyboxFonts.h"
#include "../ui/ToyboxIcons.h"
#include "../ui/ToyboxMetrics.h"
#include "../ui/ToyboxText.h"
#include "../ui/ToyboxTheme.h"
#include "WallpaperCommon.h"

namespace fui = freeink::ui;

namespace {

static inline void addBtn(toybox::Screen& screen, fui::ActionId action, const fui::Rect& rect) {
  screen.frame().hit(rect, action);
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

static void drawPillBtn(toybox::Screen& screen, const fui::Paint& ink, const fui::Rect& r,
                        const char* label, fui::ActionId action, bool filled = false,
                        fui::FontId font = toybox::kTileFont,
                        fui::Color textColor = fui::Color::Black) {
  if (filled) {
    screen.target().fill(r, ink, 4);
    textColor = fui::Color::White;
  } else {
    screen.target().stroke(r, ink, 1, 4);
  }
  const int textY = r.y + (r.height - 20) / 2;
  drawText(screen, fui::makeRect(r.x, textY, r.width, 20), label, font, fui::TextAlign::Center, textColor);
  addBtn(screen, action, r);
}

static void renderPager(toybox::Screen& screen, const fui::Paint& ink, int leftMargin, int rightMargin,
                        int contentWidth, int contentY, int currentPage, int totalPages) {
  if (totalPages <= 1) return;
  drawPillBtn(screen, ink, fui::makeRect(leftMargin, contentY, 80, 36), "< PREV", 107);
  char pBuf[32];
  std::snprintf(pBuf, sizeof(pBuf), "%d / %d", currentPage + 1, totalPages);
  drawText(screen, fui::makeRect(leftMargin + 90, contentY + 8, contentWidth - 180, 20),
           pBuf, toybox::kTileFont, fui::TextAlign::Center);
  drawPillBtn(screen, ink, fui::makeRect(rightMargin - 80, contentY, 80, 36), "NEXT >", 108);
}

static void renderWrappedText(toybox::Screen& screen, const std::string& text, size_t maxCharsPerLine,
                              int startX, int startY, int width, int lineH, int maxLines,
                              fui::FontId font, fui::TextAlign align = fui::TextAlign::Center,
                              fui::Color color = fui::Color::Black) {
  size_t start = 0;
  int lineCount = 0;
  char lineBuf[96];
  while (start < text.size() && lineCount < maxLines) {
    while (start < text.size() && text[start] == ' ') ++start;
    if (start >= text.size()) break;
    size_t end = start + maxCharsPerLine;
    if (end >= text.size()) {
      size_t take = text.size() - start;
      size_t cp = std::min(take, sizeof(lineBuf) - 1);
      std::memcpy(lineBuf, text.data() + start, cp);
      lineBuf[cp] = '\0';
      drawText(screen, fui::makeRect(startX, startY + lineCount * lineH, width, lineH), lineBuf, font, align, color);
      break;
    }
    size_t space = text.rfind(' ', end);
    if (space != std::string::npos && space > start) {
      size_t take = space - start;
      size_t cp = std::min(take, sizeof(lineBuf) - 1);
      std::memcpy(lineBuf, text.data() + start, cp);
      lineBuf[cp] = '\0';
      start = space + 1;
    } else {
      size_t take = maxCharsPerLine;
      size_t cp = std::min(take, sizeof(lineBuf) - 1);
      std::memcpy(lineBuf, text.data() + start, cp);
      lineBuf[cp] = '\0';
      start += maxCharsPerLine;
    }
    drawText(screen, fui::makeRect(startX, startY + lineCount * lineH, width, lineH), lineBuf, font, align, color);
    ++lineCount;
  }
}

enum : fui::ActionId {
  ActionTabShuffle = 10,
  ActionTabImages = 11,
  ActionTabQuotes = 12,
  ActionTabSettings = 13,
  ActionPickRandom = 14,
  ActionFullscreen = 15,
  ActionToggleAutoShuffle = 16,
  ActionSetPinnedWallpaper = 17,
  ActionAddQuote = 18,
  ActionCreateSample = 19,
  ActionToggleInvertImages = 20,
  ActionToggleInvertQuotes = 21,
  ActionSetScheduleEverySleep = 22,
  ActionSetScheduleEveryHour = 23,
  ActionSetScheduleEveryDay = 24,
  ActionItemView0 = 30,
  ActionItemSet0 = 50,
  ActionItemDel0 = 70,
  ActionPagePrev = 107,
  ActionPageNext = 108,
};

}  // namespace

std::unique_ptr<Activity> WallpaperActivity::create(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return makeUniqueNoThrow<WallpaperActivity>(renderer, mappedInput);
}

void WallpaperActivity::onEnter() {
  Activity::onEnter();
  toybox::ensureFonts(renderer);
  wallpaper::ensureStorageDirs();
  settings_ = wallpaper::loadSettings();
  wallpaper::loadQuotesFromSd(quotes_);
  wallpaper::scanBmpImages(images_);
  if (quotes_.empty() && images_.empty()) {
    pickRandom();
  }
  interactionsReady_ = false;
  requestUpdate();
}

void WallpaperActivity::pickRandom() {
  const bool hasQuotes = !quotes_.empty();
  const bool hasImages = !images_.empty();
  if (!hasQuotes && !hasImages) return;

  notificationMsg_ = nullptr;

  bool chooseImage = false;
  if (hasQuotes && hasImages) {
    chooseImage = (random(2) == 1);
  } else {
    chooseImage = hasImages;
  }

  if (chooseImage) {
    activeType_ = wallpaper::WallpaperType::Image;
    selectedImageIdx_ = static_cast<int>(random(static_cast<long>(images_.size())));
  } else {
    activeType_ = wallpaper::WallpaperType::Quote;
    selectedQuoteIdx_ = static_cast<int>(random(static_cast<long>(quotes_.size())));
  }
}

void WallpaperActivity::toggleAutoShuffle() {
  settings_.autoShuffle = !settings_.autoShuffle;
  wallpaper::saveSettings(settings_);
  if (settings_.autoShuffle) {
    SETTINGS.sleepScreen = CrossPointSettings::SLEEP_SCREEN_MODE::CUSTOM;
    APP_STATE.preferredSleepFolderPath = "/XTData/Wallpaper";
    APP_STATE.favoriteSleepImagePath = "";
    SETTINGS.saveToFile();
    APP_STATE.saveToFile();
    notificationMsg_ = "AUTO-SHUFFLE ON SLEEP: ACTIVE";
  } else {
    notificationMsg_ = "AUTO-SHUFFLE DISABLED";
  }
  requestUpdate();
}

void WallpaperActivity::toggleInvertImages() {
  settings_.invertImages = !settings_.invertImages;
  wallpaper::saveSettings(settings_);
  notificationMsg_ = settings_.invertImages ? "INVERT IMAGES: ON (NEGATIVE)" : "INVERT IMAGES: OFF (NORMAL)";
  requestUpdate();
}

void WallpaperActivity::toggleInvertQuotes() {
  settings_.invertQuotes = !settings_.invertQuotes;
  wallpaper::saveSettings(settings_);
  notificationMsg_ = settings_.invertQuotes ? "INVERT QUOTES: ON (DARK MODE)" : "INVERT QUOTES: OFF (LIGHT MODE)";
  requestUpdate();
}

void WallpaperActivity::setSchedule(wallpaper::Schedule sched) {
  settings_.schedule = sched;
  wallpaper::saveSettings(settings_);
  switch (sched) {
    case wallpaper::Schedule::EverySleep:
      notificationMsg_ = "SCHEDULE: EVERY SLEEP";
      break;
    case wallpaper::Schedule::EveryHour:
      notificationMsg_ = "SCHEDULE: EVERY 1 HOUR";
      break;
    case wallpaper::Schedule::EveryDay:
      notificationMsg_ = "SCHEDULE: EVERYDAY";
      break;
  }
  requestUpdate();
}

void WallpaperActivity::setAsPinnedWallpaper() {
  wallpaper::ensureStorageDirs();
  settings_.autoShuffle = false;
  wallpaper::saveSettings(settings_);
  if (activeType_ == wallpaper::WallpaperType::Image) {
    if (selectedImageIdx_ >= 0 && selectedImageIdx_ < static_cast<int>(images_.size())) {
      if (settings_.invertImages) {
        wallpaper::renderBmpImage(renderer, "/XTData/Wallpaper/" + images_[selectedImageIdx_].filename, true);
        ScreenshotUtil::saveFramebufferAsBmp("/XTData/Wallpaper/_active_wallpaper.bmp",
                                            renderer.getFrameBuffer(),
                                            renderer.getDisplayWidth(),
                                            renderer.getDisplayHeight());
        APP_STATE.favoriteSleepImagePath = "/XTData/Wallpaper/_active_wallpaper.bmp";
      } else {
        APP_STATE.favoriteSleepImagePath = "/XTData/Wallpaper/" + images_[selectedImageIdx_].filename;
      }
      APP_STATE.preferredSleepFolderPath = "/XTData/Wallpaper";
      SETTINGS.sleepScreen = CrossPointSettings::SLEEP_SCREEN_MODE::CUSTOM;
      SETTINGS.saveToFile();
      APP_STATE.saveToFile();
      notificationMsg_ = "PINNED AS STATIC SLEEP WALLPAPER";
    }
  } else {
    if (selectedQuoteIdx_ >= 0 && selectedQuoteIdx_ < static_cast<int>(quotes_.size())) {
      wallpaper::renderQuotePoster(renderer, quotes_[selectedQuoteIdx_], settings_.invertQuotes);
      ScreenshotUtil::saveFramebufferAsBmp("/XTData/Wallpaper/_active_wallpaper.bmp",
                                          renderer.getFrameBuffer(),
                                          renderer.getDisplayWidth(),
                                          renderer.getDisplayHeight());
      APP_STATE.favoriteSleepImagePath = "/XTData/Wallpaper/_active_wallpaper.bmp";
      APP_STATE.preferredSleepFolderPath = "/XTData/Wallpaper";
      SETTINGS.sleepScreen = CrossPointSettings::SLEEP_SCREEN_MODE::CUSTOM;
      SETTINGS.saveToFile();
      APP_STATE.saveToFile();
      notificationMsg_ = "PINNED AS STATIC SLEEP WALLPAPER";
    }
  }
  requestUpdate();
}

__attribute__((noinline)) void WallpaperActivity::createSampleWallpaper() {
  wallpaper::ensureStorageDirs();
  if (!quotes_.empty()) {
    wallpaper::renderQuotePoster(renderer, quotes_[0], settings_.invertQuotes);
  }
  ScreenshotUtil::saveFramebufferAsBmp("/XTData/Wallpaper/sample_wallpaper.bmp",
                                      renderer.getFrameBuffer(),
                                      renderer.getDisplayWidth(),
                                      renderer.getDisplayHeight());
  wallpaper::scanBmpImages(images_);
  selectedImageIdx_ = 0;
  activeType_ = wallpaper::WallpaperType::Image;
  notificationMsg_ = "SAMPLE BMP CREATED";
  requestUpdate();
}

void WallpaperActivity::openKeyboardForNewQuote() {
  auto keyboard = makeUniqueNoThrow<KeyboardEntryActivity>(renderer, mappedInput, "ADD QUOTE", "", 120);
  if (!keyboard) return;
  startActivityForResult(std::move(keyboard), [this](const ActivityResult& result) {
    const auto* keyboardResult = std::get_if<KeyboardResult>(&result.data);
    if (!keyboardResult || keyboardResult->text.empty()) return;
    quotes_.push_back(wallpaper::parseQuote(keyboardResult->text));
    wallpaper::saveQuotesToSd(quotes_);
    selectedQuoteIdx_ = static_cast<int>(quotes_.size()) - 1;
    activeType_ = wallpaper::WallpaperType::Quote;
    notificationMsg_ = "QUOTE ADDED";
    requestUpdate();
  });
}

void WallpaperActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    if (isFullscreen_) {
      isFullscreen_ = false;
      interactionsReady_ = false;
      requestUpdate();
      return;
    }
    shelf::leave(renderer, mappedInput);
    return;
  }

  int x = 0;
  int y = 0;
  if (!mappedInput.wasScreenTapped(x, y) || !interactionsReady_) return;

  if (isFullscreen_) {
    isFullscreen_ = false;
    interactionsReady_ = false;
    requestUpdate();
    return;
  }

  fui::InputSnapshot input{};
  input.touchReleased = true;
  input.touchX = static_cast<int16_t>(x);
  input.touchY = static_cast<int16_t>(y);
  const fui::ActionEvent action = interactions_.route(input);

  switch (action.action) {
    case ActionTabShuffle:
      currentTab_ = wallpaper::Tab::Shuffle;
      notificationMsg_ = nullptr;
      requestUpdate();
      return;
    case ActionTabImages:
      currentTab_ = wallpaper::Tab::Images;
      notificationMsg_ = nullptr;
      wallpaper::scanBmpImages(images_);
      requestUpdate();
      return;
    case ActionTabQuotes:
      currentTab_ = wallpaper::Tab::Quotes;
      notificationMsg_ = nullptr;
      wallpaper::loadQuotesFromSd(quotes_);
      requestUpdate();
      return;
    case ActionTabSettings:
      currentTab_ = wallpaper::Tab::Settings;
      notificationMsg_ = nullptr;
      requestUpdate();
      return;
    case ActionPickRandom:
      pickRandom();
      requestUpdate();
      return;
    case ActionFullscreen:
      isFullscreen_ = true;
      requestUpdate();
      return;
    case ActionToggleAutoShuffle:
      toggleAutoShuffle();
      return;
    case ActionToggleInvertImages:
      toggleInvertImages();
      return;
    case ActionToggleInvertQuotes:
      toggleInvertQuotes();
      return;
    case ActionSetScheduleEverySleep:
      setSchedule(wallpaper::Schedule::EverySleep);
      return;
    case ActionSetScheduleEveryHour:
      setSchedule(wallpaper::Schedule::EveryHour);
      return;
    case ActionSetScheduleEveryDay:
      setSchedule(wallpaper::Schedule::EveryDay);
      return;
    case ActionSetPinnedWallpaper:
      setAsPinnedWallpaper();
      return;
    case ActionAddQuote:
      openKeyboardForNewQuote();
      return;
    case ActionCreateSample:
      createSampleWallpaper();
      return;
    case ActionPagePrev:
      if (currentTab_ == wallpaper::Tab::Quotes && quotesPage_ > 0) {
        --quotesPage_;
        requestUpdate();
      } else if (currentTab_ == wallpaper::Tab::Images && imagesPage_ > 0) {
        --imagesPage_;
        requestUpdate();
      }
      return;
    case ActionPageNext:
      if (currentTab_ == wallpaper::Tab::Quotes && (quotesPage_ + 1) * 3 < static_cast<int>(quotes_.size())) {
        ++quotesPage_;
        requestUpdate();
      } else if (currentTab_ == wallpaper::Tab::Images && (imagesPage_ + 1) * 3 < static_cast<int>(images_.size())) {
        ++imagesPage_;
        requestUpdate();
      }
      return;
    default:
      break;
  }

  if (action.action >= ActionItemView0 && action.action < ActionItemView0 + 10) {
    const int idx = action.action - ActionItemView0;
    const int actualIdx = ((currentTab_ == wallpaper::Tab::Quotes) ? quotesPage_ : imagesPage_) * 3 + idx;
    if (currentTab_ == wallpaper::Tab::Quotes && actualIdx < static_cast<int>(quotes_.size())) {
      selectedQuoteIdx_ = actualIdx;
      activeType_ = wallpaper::WallpaperType::Quote;
      isFullscreen_ = true;
      requestUpdate();
    } else if (currentTab_ == wallpaper::Tab::Images && actualIdx < static_cast<int>(images_.size())) {
      selectedImageIdx_ = actualIdx;
      activeType_ = wallpaper::WallpaperType::Image;
      isFullscreen_ = true;
      requestUpdate();
    }
    return;
  }

  if (action.action >= ActionItemSet0 && action.action < ActionItemSet0 + 10) {
    const int idx = action.action - ActionItemSet0;
    const int actualIdx = ((currentTab_ == wallpaper::Tab::Quotes) ? quotesPage_ : imagesPage_) * 3 + idx;
    if (currentTab_ == wallpaper::Tab::Quotes && actualIdx < static_cast<int>(quotes_.size())) {
      selectedQuoteIdx_ = actualIdx;
      activeType_ = wallpaper::WallpaperType::Quote;
      setAsPinnedWallpaper();
    } else if (currentTab_ == wallpaper::Tab::Images && actualIdx < static_cast<int>(images_.size())) {
      selectedImageIdx_ = actualIdx;
      activeType_ = wallpaper::WallpaperType::Image;
      setAsPinnedWallpaper();
    }
    return;
  }

  if (action.action >= ActionItemDel0 && action.action < ActionItemDel0 + 10) {
    const int idx = action.action - ActionItemDel0;
    if (currentTab_ == wallpaper::Tab::Quotes) {
      const int actualIdx = quotesPage_ * 3 + idx;
      if (actualIdx < static_cast<int>(quotes_.size()) && quotes_.size() > 1) {
        quotes_.erase(quotes_.begin() + actualIdx);
        wallpaper::saveQuotesToSd(quotes_);
        if (selectedQuoteIdx_ >= static_cast<int>(quotes_.size())) {
          selectedQuoteIdx_ = 0;
        }
        requestUpdate();
      }
    }
    return;
  }
}

__attribute__((noinline)) static void renderShuffleTab(
    toybox::Screen& screen, const fui::Paint& ink,
    int leftMargin, int contentWidth, int contentY,
    wallpaper::WallpaperType activeType,
    const std::vector<wallpaper::Quote>& quotes,
    int selectedQuoteIdx,
    const std::vector<wallpaper::BmpItem>& images,
    int selectedImageIdx,
    const wallpaper::WallpaperSettings& settings,
    const char* notificationMsg) {
  drawText(screen, fui::makeRect(leftMargin, contentY, contentWidth, 22),
           "ACTIVE WALLPAPER SELECTION:", toybox::kTileFont, fui::TextAlign::Left, fui::Color::DarkGray);
  contentY += 26;

  constexpr int cardH = 250;
  const fui::Rect previewBox = fui::makeRect(leftMargin, contentY, contentWidth, cardH);
  screen.target().stroke(previewBox, ink, 1, 8);

  if (activeType == wallpaper::WallpaperType::Quote && !quotes.empty()) {
    const int qIdx = std::clamp(selectedQuoteIdx, 0, static_cast<int>(quotes.size()) - 1);
    const auto& q = quotes[qIdx];

    const char* tagLabel = settings.invertQuotes ? "QUOTE [DARK MODE]" : "MOTIVATIONAL QUOTE";
    const fui::Rect tagBox = fui::makeRect(previewBox.x + 16, previewBox.y + 14, 160, 22);
    screen.target().stroke(tagBox, ink, 1, 4);
    drawText(screen, fui::makeRect(tagBox.x, tagBox.y + 3, tagBox.width, 16),
             tagLabel, toybox::kSmallFont, fui::TextAlign::Center);

    drawText(screen, fui::makeRect(previewBox.x + 20, previewBox.y + 42, previewBox.width - 40, 36),
             "\"", toybox::kUiFont, fui::TextAlign::Center);

    renderWrappedText(screen, q.text, 28, previewBox.x + 20, previewBox.y + 76, previewBox.width - 40, 28, 3, toybox::kTileFont);

    if (!q.author.empty()) {
      char aBuf[80];
      std::snprintf(aBuf, sizeof(aBuf), "- %s -", q.author.c_str());
      drawText(screen, fui::makeRect(previewBox.x + 20, previewBox.y + cardH - 34, previewBox.width - 40, 20),
               aBuf, toybox::kSmallFont, fui::TextAlign::Center, fui::Color::DarkGray);
    }
  } else if (activeType == wallpaper::WallpaperType::Image && !images.empty()) {
    const int iIdx = std::clamp(selectedImageIdx, 0, static_cast<int>(images.size()) - 1);
    const auto& img = images[iIdx];

    const char* tagLabel = settings.invertImages ? "BMP IMAGE [INVERTED]" : "BMP IMAGE";
    const fui::Rect tagBox = fui::makeRect(previewBox.x + 16, previewBox.y + 14, 160, 22);
    screen.target().stroke(tagBox, ink, 1, 4);
    drawText(screen, fui::makeRect(tagBox.x, tagBox.y + 3, tagBox.width, 16),
             tagLabel, toybox::kSmallFont, fui::TextAlign::Center);

    drawText(screen, fui::makeRect(previewBox.x + 20, previewBox.y + 74, previewBox.width - 40, 28),
             img.filename.c_str(), toybox::kUiFont, fui::TextAlign::Center);

    char infoBuf[64];
    std::snprintf(infoBuf, sizeof(infoBuf), "Size: %lu KB  ·  /XTData/Wallpaper",
                  static_cast<unsigned long>((img.sizeBytes + 1023) / 1024));
    drawText(screen, fui::makeRect(previewBox.x + 20, previewBox.y + 114, previewBox.width - 40, 20),
             infoBuf, toybox::kSmallFont, fui::TextAlign::Center, fui::Color::DarkGray);

    drawText(screen, fui::makeRect(previewBox.x + 20, previewBox.y + 164, previewBox.width - 40, 20),
             "Tap Fullscreen below to view image", toybox::kTileFont, fui::TextAlign::Center);
  } else {
    drawText(screen, fui::makeRect(previewBox.x + 20, previewBox.y + 100, previewBox.width - 40, 28),
             "Tap Pick Random to choose a wallpaper", toybox::kTileFont, fui::TextAlign::Center);
  }

  contentY += cardH + 14;

  // Status or notification bar
  const fui::Rect statusBox = fui::makeRect(leftMargin, contentY, contentWidth, 32);
  if (notificationMsg) {
    screen.target().fill(statusBox, ink, 4);
    drawText(screen, fui::makeRect(statusBox.x, statusBox.y + 6, statusBox.width, 20),
             notificationMsg, toybox::kTileFont, fui::TextAlign::Center, fui::Color::White);
  } else {
    screen.target().stroke(statusBox, ink, 1, 4);
    if (settings.autoShuffle) {
      const char* sStr = (settings.schedule == wallpaper::Schedule::EverySleep) ? "EVERY SLEEP" :
                         (settings.schedule == wallpaper::Schedule::EveryHour) ? "EVERY 1 HR" : "EVERYDAY";
      char statusBuf[64];
      std::snprintf(statusBuf, sizeof(statusBuf), "[●] AUTO-SHUFFLE: %s", sStr);
      drawText(screen, fui::makeRect(statusBox.x, statusBox.y + 6, statusBox.width, 20),
               statusBuf, toybox::kTileFont, fui::TextAlign::Center);
    } else {
      drawText(screen, fui::makeRect(statusBox.x, statusBox.y + 6, statusBox.width, 20),
               "[○] AUTO-SHUFFLE OFF (PINNED WALLPAPER)", toybox::kSmallFont, fui::TextAlign::Center, fui::Color::DarkGray);
    }
  }
  contentY += 42;

  constexpr int midBtnH = 44;
  constexpr int gap = 12;
  const int midBtnW = (contentWidth - gap) / 2;

  drawPillBtn(screen, ink, fui::makeRect(leftMargin, contentY, midBtnW, midBtnH),
              "RANDOM PICK", ActionPickRandom);
  drawPillBtn(screen, ink, fui::makeRect(leftMargin + midBtnW + gap, contentY, midBtnW, midBtnH),
              "FULLSCREEN", ActionFullscreen);

  contentY += midBtnH + 12;

  const char* autoLabel = settings.autoShuffle ? "DISABLE AUTO-SHUFFLE" : "AUTO-SHUFFLE ON SLEEP";
  drawPillBtn(screen, ink, fui::makeRect(leftMargin, contentY, contentWidth, 44),
              autoLabel, ActionToggleAutoShuffle, settings.autoShuffle, toybox::kTileFont);

  constexpr int bottomH = 46;
  const int bottomY = screen.device().height - bottomH - 16;
  drawPillBtn(screen, ink, fui::makeRect(leftMargin, bottomY, contentWidth, bottomH),
              "PIN CURRENT AS WALLPAPER", ActionSetPinnedWallpaper, !settings.autoShuffle, toybox::kTileFont);
}

__attribute__((noinline)) static void renderImagesTab(
    toybox::Screen& screen, const fui::Paint& ink,
    int leftMargin, int rightMargin, int contentWidth, int contentY,
    const std::vector<wallpaper::BmpItem>& images,
    int imagesPage) {
  char headerBuf[64];
  std::snprintf(headerBuf, sizeof(headerBuf), "FOUND %d BMP IMAGES IN /XTData/Wallpaper:",
                static_cast<int>(images.size()));
  drawText(screen, fui::makeRect(leftMargin, contentY, contentWidth, 22),
           headerBuf, toybox::kTileFont, fui::TextAlign::Left, fui::Color::DarkGray);
  contentY += 28;

  if (images.empty()) {
    const fui::Rect emptyBox = fui::makeRect(leftMargin, contentY, contentWidth, 180);
    screen.target().stroke(emptyBox, ink, 1, 6);

    drawText(screen, fui::makeRect(emptyBox.x + 20, emptyBox.y + 30, emptyBox.width - 40, 24),
             "NO BMP IMAGES FOUND", toybox::kUiFont, fui::TextAlign::Center);
    drawText(screen, fui::makeRect(emptyBox.x + 20, emptyBox.y + 64, emptyBox.width - 40, 20),
             "Place 480x800 .bmp files in /XTData/Wallpaper on SD.",
             toybox::kSmallFont, fui::TextAlign::Center, fui::Color::DarkGray);

    drawPillBtn(screen, ink, fui::makeRect(emptyBox.x + (emptyBox.width - 240) / 2, emptyBox.y + 106, 240, 42),
                "+ CREATE SAMPLE BMP", ActionCreateSample, true);
  } else {
    constexpr int rowH = 78;
    constexpr int rowGap = 10;
    const int startIdx = imagesPage * 3;
    const int endIdx = std::min(startIdx + 3, static_cast<int>(images.size()));

    for (int i = startIdx; i < endIdx; ++i) {
      const int slot = i - startIdx;
      const auto& img = images[i];
      const fui::Rect rBox = fui::makeRect(leftMargin, contentY, contentWidth, rowH);
      screen.target().stroke(rBox, ink, 1, 6);

      char numName[64];
      std::snprintf(numName, sizeof(numName), "%d. %s", i + 1, img.filename.c_str());
      drawText(screen, fui::makeRect(rBox.x + 14, rBox.y + 12, rBox.width - 170, 24),
               numName, toybox::kTileFont);

      char szBuf[32];
      std::snprintf(szBuf, sizeof(szBuf), "Size: %lu KB", static_cast<unsigned long>((img.sizeBytes + 1023) / 1024));
      drawText(screen, fui::makeRect(rBox.x + 14, rBox.y + 44, rBox.width - 170, 18),
               szBuf, toybox::kSmallFont, fui::TextAlign::Left, fui::Color::DarkGray);

      drawPillBtn(screen, ink, fui::makeRect(rBox.x + rBox.width - 150, rBox.y + 18, 68, 42),
                  "VIEW", static_cast<fui::ActionId>(ActionItemView0 + slot));
      drawPillBtn(screen, ink, fui::makeRect(rBox.x + rBox.width - 74, rBox.y + 18, 62, 42),
                  "SET", static_cast<fui::ActionId>(ActionItemSet0 + slot), true);

      contentY += rowH + rowGap;
    }

    renderPager(screen, ink, leftMargin, rightMargin, contentWidth, contentY + 10,
                imagesPage, (static_cast<int>(images.size()) + 2) / 3);
  }
}

__attribute__((noinline)) static void renderQuotesTab(
    toybox::Screen& screen, const fui::Paint& ink,
    int leftMargin, int rightMargin, int contentWidth, int contentY,
    const std::vector<wallpaper::Quote>& quotes,
    int quotesPage) {
  drawPillBtn(screen, ink, fui::makeRect(rightMargin - 120, contentY - 4, 120, 32),
              "+ ADD QUOTE", ActionAddQuote, true, toybox::kSmallFont);

  char countBuf[64];
  std::snprintf(countBuf, sizeof(countBuf), "MOTIVATIONAL QUOTES (%d):", static_cast<int>(quotes.size()));
  drawText(screen, fui::makeRect(leftMargin, contentY, contentWidth - 130, 22),
           countBuf, toybox::kTileFont, fui::TextAlign::Left, fui::Color::DarkGray);
  contentY += 34;

  constexpr int rowH = 104;
  constexpr int rowGap = 10;
  const int startIdx = quotesPage * 3;
  const int endIdx = std::min(startIdx + 3, static_cast<int>(quotes.size()));

  for (int i = startIdx; i < endIdx; ++i) {
    const int slot = i - startIdx;
    const auto& q = quotes[i];
    const fui::Rect rBox = fui::makeRect(leftMargin, contentY, contentWidth, rowH);
    screen.target().stroke(rBox, ink, 1, 6);

    renderWrappedText(screen, q.text, 26, rBox.x + 14, rBox.y + 12, rBox.width - 150, 22, 2, toybox::kTileFont, fui::TextAlign::Left);

    if (!q.author.empty()) {
      char aBuf[64];
      std::snprintf(aBuf, sizeof(aBuf), "- %s", q.author.c_str());
      drawText(screen, fui::makeRect(rBox.x + 14, rBox.y + 74, rBox.width - 150, 18),
               aBuf, toybox::kSmallFont, fui::TextAlign::Left, fui::Color::DarkGray);
    }

    drawPillBtn(screen, ink, fui::makeRect(rBox.x + rBox.width - 130, rBox.y + 14, 58, 36),
                "VIEW", static_cast<fui::ActionId>(ActionItemView0 + slot), false, toybox::kSmallFont);
    drawPillBtn(screen, ink, fui::makeRect(rBox.x + rBox.width - 66, rBox.y + 14, 54, 36),
                "SET", static_cast<fui::ActionId>(ActionItemSet0 + slot), true, toybox::kSmallFont);
    drawPillBtn(screen, ink, fui::makeRect(rBox.x + rBox.width - 66, rBox.y + 56, 54, 34),
                "DEL", static_cast<fui::ActionId>(ActionItemDel0 + slot), false, toybox::kSmallFont, fui::Color::DarkGray);

    contentY += rowH + rowGap;
  }

  renderPager(screen, ink, leftMargin, rightMargin, contentWidth, contentY + 6,
              quotesPage, (static_cast<int>(quotes.size()) + 2) / 3);
}

__attribute__((noinline)) static void renderSettingsTab(
    toybox::Screen& screen, const fui::Paint& ink,
    int leftMargin, int contentWidth, int contentY,
    const wallpaper::WallpaperSettings& settings,
    const char* notificationMsg) {
  if (notificationMsg) {
    const fui::Rect statusBox = fui::makeRect(leftMargin, contentY, contentWidth, 32);
    screen.target().fill(statusBox, ink, 4);
    drawText(screen, fui::makeRect(statusBox.x, statusBox.y + 6, statusBox.width, 20),
             notificationMsg, toybox::kTileFont, fui::TextAlign::Center, fui::Color::White);
    contentY += 38;
  }

  // 1. AUTO-SHUFFLE ON SLEEP
  drawText(screen, fui::makeRect(leftMargin, contentY, contentWidth, 20),
           "1. AUTO-SHUFFLE ON SLEEP", toybox::kTileFont, fui::TextAlign::Left, fui::Color::DarkGray);
  contentY += 22;

  const fui::Rect autoBox = fui::makeRect(leftMargin, contentY, contentWidth, 52);
  screen.target().stroke(autoBox, ink, 1, 6);
  drawText(screen, fui::makeRect(autoBox.x + 14, autoBox.y + 16, autoBox.width - 130, 22),
           "Shuffle wallpaper on sleep screen", toybox::kTileFont);
  drawPillBtn(screen, ink, fui::makeRect(autoBox.x + autoBox.width - 110, autoBox.y + 8, 96, 36),
              settings.autoShuffle ? "[●] ON" : "[○] OFF", ActionToggleAutoShuffle,
              settings.autoShuffle, toybox::kTileFont);
  contentY += 62;

  // 2. SHUFFLE SCHEDULE
  drawText(screen, fui::makeRect(leftMargin, contentY, contentWidth, 20),
           "2. SHUFFLE SCHEDULE", toybox::kTileFont, fui::TextAlign::Left, fui::Color::DarkGray);
  contentY += 22;

  const int schedGap = 8;
  const int schedBtnW = (contentWidth - schedGap * 2) / 3;
  drawPillBtn(screen, ink, fui::makeRect(leftMargin, contentY, schedBtnW, 40),
              "EVERY SLEEP", ActionSetScheduleEverySleep,
              settings.schedule == wallpaper::Schedule::EverySleep, toybox::kSmallFont);
  drawPillBtn(screen, ink, fui::makeRect(leftMargin + schedBtnW + schedGap, contentY, schedBtnW, 40),
              "EVERY 1 HR", ActionSetScheduleEveryHour,
              settings.schedule == wallpaper::Schedule::EveryHour, toybox::kSmallFont);
  drawPillBtn(screen, ink, fui::makeRect(leftMargin + (schedBtnW + schedGap) * 2, contentY, schedBtnW, 40),
              "EVERYDAY", ActionSetScheduleEveryDay,
              settings.schedule == wallpaper::Schedule::EveryDay, toybox::kSmallFont);
  contentY += 46;

  const char* schedDesc = ">> Wallpaper changes every time device enters sleep";
  if (settings.schedule == wallpaper::Schedule::EveryHour) {
    schedDesc = ">> Wallpaper changes on sleep if at least 1 hr has elapsed";
  } else if (settings.schedule == wallpaper::Schedule::EveryDay) {
    schedDesc = ">> Wallpaper changes once per day (daily fresh quote/image)";
  }
  drawText(screen, fui::makeRect(leftMargin + 4, contentY, contentWidth - 8, 18),
           schedDesc, toybox::kSmallFont, fui::TextAlign::Left, fui::Color::DarkGray);
  contentY += 26;

  // 3. INVERT COLORS (DARK MODE)
  drawText(screen, fui::makeRect(leftMargin, contentY, contentWidth, 20),
           "3. INVERT COLORS (DARK MODE)", toybox::kTileFont, fui::TextAlign::Left, fui::Color::DarkGray);
  contentY += 22;

  const fui::Rect invBox = fui::makeRect(leftMargin, contentY, contentWidth, 98);
  screen.target().stroke(invBox, ink, 1, 6);

  // Invert Images row
  drawText(screen, fui::makeRect(invBox.x + 14, invBox.y + 14, invBox.width - 130, 22),
           "Invert BMP Images (Negative)", toybox::kTileFont);
  drawPillBtn(screen, ink, fui::makeRect(invBox.x + invBox.width - 106, invBox.y + 8, 92, 34),
              settings.invertImages ? "[●] ON" : "[○] OFF", ActionToggleInvertImages,
              settings.invertImages, toybox::kTileFont);

  screen.target().fill(fui::makeRect(invBox.x + 10, invBox.y + 49, invBox.width - 20, 1), ink);

  // Invert Quotes row
  drawText(screen, fui::makeRect(invBox.x + 14, invBox.y + 62, invBox.width - 130, 22),
           "Invert Quotes (White text on black)", toybox::kTileFont);
  drawPillBtn(screen, ink, fui::makeRect(invBox.x + invBox.width - 106, invBox.y + 56, 92, 34),
              settings.invertQuotes ? "[●] ON" : "[○] OFF", ActionToggleInvertQuotes,
              settings.invertQuotes, toybox::kTileFont);
  contentY += 108;

  // 4. SUMMARY BOX
  drawText(screen, fui::makeRect(leftMargin, contentY, contentWidth, 20),
           "4. ACTIVE CONFIGURATION SUMMARY", toybox::kTileFont, fui::TextAlign::Left, fui::Color::DarkGray);
  contentY += 22;

  const fui::Rect sumBox = fui::makeRect(leftMargin, contentY, contentWidth, 68);
  screen.target().stroke(sumBox, ink, 1, 6);

  char line1[80];
  const char* schedStr = (settings.schedule == wallpaper::Schedule::EverySleep) ? "Every Sleep" :
                         (settings.schedule == wallpaper::Schedule::EveryHour) ? "Every 1 Hr" : "Everyday";
  std::snprintf(line1, sizeof(line1), "Auto-Shuffle: %s  |  Schedule: %s",
                settings.autoShuffle ? "ENABLED" : "OFF", schedStr);
  drawText(screen, fui::makeRect(sumBox.x + 14, sumBox.y + 12, sumBox.width - 28, 20),
           line1, toybox::kTileFont);

  char line2[80];
  std::snprintf(line2, sizeof(line2), "Images: %s  |  Quotes: %s",
                settings.invertImages ? "Inverted (Negative)" : "Normal",
                settings.invertQuotes ? "Dark Mode (White/Black)" : "Light (Black/White)");
  drawText(screen, fui::makeRect(sumBox.x + 14, sumBox.y + 38, sumBox.width - 28, 18),
           line2, toybox::kSmallFont, fui::TextAlign::Left, fui::Color::DarkGray);

  contentY += 80;

  // Bottom Preview Button
  drawPillBtn(screen, ink, fui::makeRect(leftMargin, contentY, contentWidth, 44),
              "PREVIEW CURRENT WALLPAPER FULLSCREEN", ActionFullscreen, true, toybox::kTileFont);
}

void WallpaperActivity::render(RenderLock&&) {
  if (isFullscreen_) {
    if (activeType_ == wallpaper::WallpaperType::Image && selectedImageIdx_ >= 0 &&
        selectedImageIdx_ < static_cast<int>(images_.size())) {
      wallpaper::renderBmpImage(renderer, "/XTData/Wallpaper/" + images_[selectedImageIdx_].filename, settings_.invertImages);
    } else if (!quotes_.empty()) {
      const int qIdx = std::clamp(selectedQuoteIdx_, 0, static_cast<int>(quotes_.size()) - 1);
      wallpaper::renderQuotePoster(renderer, quotes_[qIdx], settings_.invertQuotes);
    }
    interactionsReady_ = true;
    renderer.displayBuffer();
    return;
  }

  renderer.clearScreen();
  fui::GfxRendererTarget target = toybox::makeTarget(renderer);
  const fui::InputSnapshot noInput{};
  interactionsReady_ = false;
  toybox::Frame frame(target, target.deviceContext(), noInput, interactions_);
  toybox::Screen screen(frame);

  const auto ink = fui::Paint::solid(fui::Color::Black);
  const int leftMargin = toybox::kMargin;
  const int rightMargin = screen.device().width - toybox::kMargin;
  const int contentWidth = rightMargin - leftMargin;

  // Header Bar
  constexpr int topMargin = 14;
  constexpr int headerHeight = 34;
  drawText(screen, fui::makeRect(leftMargin, topMargin, contentWidth - 140, headerHeight), "WALLPAPERS", toybox::kUiFont);
  drawText(screen, fui::makeRect(rightMargin - 150, topMargin, 150, headerHeight), "MOTIVATION & BMP", toybox::kTileFont, fui::TextAlign::Right);

  const int ruleY = topMargin + headerHeight + 2;
  screen.target().fill(fui::makeRect(leftMargin, ruleY, contentWidth, 2), ink);

  // Tab Strip (4 tabs: SHUFFLE, IMAGES, QUOTES, SETTINGS)
  constexpr int tabH = 38;
  constexpr int tabGap = 8;
  const int tabW = (contentWidth - tabGap * 3) / 4;
  const int tabY = ruleY + 8;

  const char* const tabNames[4] = {"SHUFFLE", "IMAGES", "QUOTES", "SETTINGS"};
  for (int t = 0; t < 4; ++t) {
    const fui::Rect tBox = fui::makeRect(leftMargin + t * (tabW + tabGap), tabY, tabW, tabH);
    const bool isCur = (static_cast<int>(currentTab_) == t);
    drawPillBtn(screen, ink, tBox, tabNames[t], static_cast<fui::ActionId>(ActionTabShuffle + t), isCur, toybox::kSmallFont);
  }

  const int contentY = tabY + tabH + 12;

  if (currentTab_ == wallpaper::Tab::Shuffle) {
    renderShuffleTab(screen, ink, leftMargin, contentWidth, contentY,
                     activeType_, quotes_, selectedQuoteIdx_, images_, selectedImageIdx_, settings_, notificationMsg_);
  } else if (currentTab_ == wallpaper::Tab::Images) {
    renderImagesTab(screen, ink, leftMargin, rightMargin, contentWidth, contentY, images_, imagesPage_);
  } else if (currentTab_ == wallpaper::Tab::Quotes) {
    renderQuotesTab(screen, ink, leftMargin, rightMargin, contentWidth, contentY, quotes_, quotesPage_);
  } else if (currentTab_ == wallpaper::Tab::Settings) {
    renderSettingsTab(screen, ink, leftMargin, contentWidth, contentY, settings_, notificationMsg_);
  }

  interactionsReady_ = true;
  toybox::reportOverflow(interactions_, "Wallpaper");
  renderer.displayBuffer();
}
