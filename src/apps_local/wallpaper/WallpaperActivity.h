#pragma once

#include <GfxRenderer.h>
#include <MappedInputManager.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../../activities/Activity.h"
#include "../ui/ToyboxScreen.h"
#include "WallpaperCommon.h"

namespace wallpaper {

enum class Tab : uint8_t {
  Shuffle = 0,
  Images,
  Quotes,
  Settings,
};

enum class WallpaperType : uint8_t {
  Quote = 0,
  Image,
};

}  // namespace wallpaper

class WallpaperActivity : public Activity {
 public:
  static std::unique_ptr<Activity> create(GfxRenderer& renderer, MappedInputManager& mappedInput);

  WallpaperActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Wallpaper", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&& lock) override;

 private:
  void pickRandom();
  void setAsPinnedWallpaper();
  void toggleAutoShuffle();
  void toggleInvertImages();
  void toggleInvertQuotes();
  void setSchedule(wallpaper::Schedule sched);
  void createSampleWallpaper();
  void openKeyboardForNewQuote();

  wallpaper::Tab currentTab_ = wallpaper::Tab::Shuffle;
  bool isFullscreen_ = false;

  wallpaper::WallpaperType activeType_ = wallpaper::WallpaperType::Quote;
  int selectedQuoteIdx_ = 0;
  int selectedImageIdx_ = 0;
  const char* notificationMsg_ = nullptr;

  int quotesPage_ = 0;
  int imagesPage_ = 0;

  wallpaper::WallpaperSettings settings_;
  std::vector<wallpaper::Quote> quotes_;
  std::vector<wallpaper::BmpItem> images_;

  toybox::Interactions interactions_;
  bool interactionsReady_ = false;
};
