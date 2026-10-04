#pragma once

#include <GfxRenderer.h>
#include <cstdint>
#include <string>
#include <vector>

namespace wallpaper {

enum class Schedule : uint8_t {
  EverySleep = 0,
  EveryHour = 1,
  EveryDay = 2,
};

struct WallpaperSettings {
  bool autoShuffle = false;
  bool invertImages = false;
  bool invertQuotes = false;
  Schedule schedule = Schedule::EverySleep;

  uint32_t lastShuffleEpoch = 0;
  uint16_t lastShuffleYear = 0;
  uint8_t lastShuffleMonth = 0;
  uint8_t lastShuffleDay = 0;
  uint8_t lastShuffleHour = 0;

  uint8_t activeType = 0;  // 0 = Quote, 1 = Image
  int activeQuoteIdx = 0;
  std::string activeImageFilename;
};

struct Quote {
  std::string text;
  std::string author;
};

struct BmpItem {
  std::string filename;
  uint32_t sizeBytes = 0;
};

Quote parseQuote(const std::string& line);
void ensureStorageDirs();
void loadQuotesFromSd(std::vector<Quote>& out);
void saveQuotesToSd(const std::vector<Quote>& quotes);
void scanBmpImages(std::vector<BmpItem>& out);
void renderQuotePoster(GfxRenderer& renderer, const Quote& q, bool inverted = false);
bool renderBmpImage(GfxRenderer& renderer, const std::string& path, bool inverted = false);

WallpaperSettings loadSettings();
void saveSettings(const WallpaperSettings& settings);

bool isAutoShuffleEnabled();
void setAutoShuffleEnabled(bool enable);
bool drawAsleep(GfxRenderer& renderer);

}  // namespace wallpaper
