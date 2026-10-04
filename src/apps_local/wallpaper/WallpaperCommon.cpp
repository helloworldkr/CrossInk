#include "WallpaperCommon.h"

#include <Bitmap.h>
#include <FsHelpers.h>
#include <GfxRenderer.h>
#include <HalClock.h>
#include <HalDisplay.h>
#include <HalStorage.h>
#include <Logging.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "../../CrossPointSettings.h"
#include "../../CrossPointState.h"
#include "../../components/themes/BaseTheme.h"
#include "../ui/ToyboxFonts.h"
#include "../ui/ToyboxScreen.h"
#include "../ui/ToyboxTheme.h"

namespace fui = freeink::ui;

namespace wallpaper {

namespace {

static const char* const kDefaultQuotes[] = {
    // Habits, Systems & Identity (James Clear)
    "Every action you take is a vote for the type of person you wish to become. No single instance will transform your beliefs, but as the votes build up, so does the evidence of your new identity. - James Clear",
    "You do not rise to the level of your goals. You fall to the level of your systems. - James Clear",
    "You should be far more concerned with your current trajectory than with your current results. - James Clear",
    "When you fall in love with the process rather than the product, you don't have to wait to give yourself permission to be happy. You can be satisfied anytime your system is running. - James Clear",
    "Professionals stick to the schedule; amateurs let life get in the way. - James Clear",

    // Patience & The Stonecutter's Credo (Jacob Riis)
    "When nothing seems to help, I go and look at a stonecutter hammering away at his rock, perhaps a hundred times without as much as a crack showing in it. Yet at the hundred and first blow it will split in two, and I know it was not that last blow that did it—but all that had gone before. - Jacob Riis",

    // Deep Work & Digital Minimalism (Cal Newport)
    "If you want to love what you do, abandon the passion mindset ('what can the world offer me?') and instead adopt the craftsman mindset ('what can I offer the world?'). - Cal Newport",
    "Human beings, it seems, are at their best when immersed deeply in something challenging. - Cal Newport",
    "Digital Minimalism: A philosophy of technology use in which you focus your online time on a small number of carefully selected activities that support things you value, and happily miss out on everything else. - Cal Newport",
    "Simply put, humans are not wired to be constantly wired. - Cal Newport",

    // Focus & The ONE Thing (Gary Keller)
    "Multitasking is a lie. - Gary Keller",
    "Your next step is simple. You are the first domino. - Gary Keller",
    "You need to be doing fewer things for more effect instead of doing more things with side effects. - Gary Keller",

    // Dopamine Nation & Overcoming Pain (Dr. Anna Lembke)
    "The relentless pursuit of pleasure and avoidance of pain leads to pain. - Dr. Anna Lembke",
    "Recovery begins with abstinence. Abstinence resets the brain's reward pathway and with it our capacity to take joy in simpler pleasures. - Dr. Anna Lembke",
    "The reason we're all so miserable may be because we're working so hard to avoid being miserable. - Dr. Anna Lembke",

    // Discipline & Tapas
    "Tapas. Uncomfortable actions. Pain is the way. - Ancient Wisdom",

    // Classic Stoic & Mindset Wisdom
    "The soul becomes dyed with the color of its thoughts. - Marcus Aurelius",
    "A journey of a thousand miles begins with a single step. - Lao Tzu",
    "Simplicity is the ultimate sophistication. - Leonardo da Vinci",
    "Stay hungry, stay foolish. - Steve Jobs",
};

static constexpr const char* kAutoShuffleMarker = "/XTData/Wallpaper/.auto_shuffle";
static constexpr const char* kSettingsFile = "/XTData/Wallpaper/settings.ini";
static constexpr const char* kSettingsTemp = "/XTData/Wallpaper/settings.ini.part";

static inline void drawText(toybox::Screen& screen, const fui::Rect& r, const char* str, fui::FontId font,
                            fui::TextAlign align = fui::TextAlign::Left, fui::Color color = fui::Color::Black) {
  fui::TextStyle style;
  style.font = font;
  style.align = align;
  style.color = color;
  style.maxLines = 1;
  screen.target().text(r, str, style);
}

static int renderWrappedText(toybox::Screen& screen, const std::string& text, size_t maxCharsPerLine,
                             int startX, int startY, int width, int lineH, int maxLines,
                             fui::FontId font, fui::TextAlign align = fui::TextAlign::Center,
                             fui::Color color = fui::Color::Black) {
  size_t start = 0;
  int lineCount = 0;
  char lineBuf[128];
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
      ++lineCount;
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
  return lineCount;
}

}  // namespace

Quote parseQuote(const std::string& line) {
  Quote q;
  size_t dash = line.rfind(" — ");
  if (dash == std::string::npos) dash = line.rfind(" - ");
  if (dash == std::string::npos) dash = line.rfind(" ~ ");
  if (dash != std::string::npos) {
    q.text = line.substr(0, dash);
    q.author = line.substr(dash + 3);
  } else {
    q.text = line;
    q.author = "";
  }
  return q;
}

void ensureStorageDirs() {
  if (!Storage.exists("/XTData")) {
    Storage.mkdir("/XTData");
  }
  if (!Storage.exists("/XTData/Wallpaper")) {
    Storage.mkdir("/XTData/Wallpaper");
  }
}

void loadQuotesFromSd(std::vector<Quote>& out) {
  out.clear();
  HalFile file;
  if (Storage.openFileForRead("WALLPAPER", "/XTData/Wallpaper/quotes.txt", file)) {
    char* buf = new (std::nothrow) char[8192];
    if (buf) {
      const int bytesRead = file.read(buf, 8191);
      file.close();
      if (bytesRead > 0) {
        buf[bytesRead] = '\0';
        char* line = std::strtok(buf, "\r\n");
        while (line != nullptr) {
          if (line[0] != '\0') {
            out.push_back(parseQuote(line));
          }
          line = std::strtok(nullptr, "\r\n");
        }
      }
      delete[] buf;
    } else {
      file.close();
    }
  }

  // Ensure all default quotes are present (merges newly added wisdom seamlessly)
  bool anyAdded = false;
  for (const char* dq : kDefaultQuotes) {
    Quote parsed = parseQuote(dq);
    bool exists = false;
    for (const auto& existing : out) {
      if (existing.text == parsed.text) {
        exists = true;
        break;
      }
    }
    if (!exists) {
      out.push_back(parsed);
      anyAdded = true;
    }
  }

  if (anyAdded) {
    saveQuotesToSd(out);
  }
}

void saveQuotesToSd(const std::vector<Quote>& quotes) {
  ensureStorageDirs();
  HalFile file;
  if (Storage.openFileForWrite("WALLPAPER", "/XTData/Wallpaper/quotes.txt", file)) {
    for (const auto& q : quotes) {
      std::string line = q.text;
      if (!q.author.empty()) {
        line += " - ";
        line += q.author;
      }
      line += "\n";
      file.write(reinterpret_cast<const uint8_t*>(line.data()), line.size());
    }
    file.close();
  }
}

void scanBmpImages(std::vector<BmpItem>& out) {
  out.clear();
  auto dir = Storage.open("/XTData/Wallpaper");
  if (!dir || !dir.isDirectory()) return;

  char name[128];
  for (auto entry = dir.openNextFile(); entry; entry = dir.openNextFile()) {
    if (entry.isDirectory()) {
      entry.close();
      continue;
    }
    entry.getName(name, sizeof(name));
    const std::string fn(name);
    if (!fn.empty() && fn[0] != '.' && fn != "_active_wallpaper.bmp") {
      if (FsHelpers::hasBmpExtension(fn)) {
        BmpItem item;
        item.filename = fn;
        item.sizeBytes = entry.fileSize();
        out.push_back(item);
      }
    }
    entry.close();
  }
  dir.close();
}

void renderQuotePoster(GfxRenderer& renderer, const Quote& q, bool inverted) {
  toybox::ensureFonts(renderer);
  renderer.clearScreen();
  fui::GfxRendererTarget target = toybox::makeTarget(renderer);
  const fui::InputSnapshot noInput{};
  toybox::Interactions ignored;
  toybox::Frame frame(target, target.deviceContext(), noInput, ignored);
  toybox::Screen screen(frame);

  const auto ink = fui::Paint::solid(fui::Color::Black);
  screen.target().stroke(fui::makeRect(20, 20, 440, 760), ink, 2, 12);
  screen.target().stroke(fui::makeRect(28, 28, 424, 744), ink, 1, 8);

  const fui::Rect badge = fui::makeRect(175, 48, 130, 26);
  screen.target().stroke(badge, ink, 1, 4);
  drawText(screen, fui::makeRect(badge.x, badge.y + 5, badge.width, 16), "DAILY WISDOM",
           toybox::kSmallFont, fui::TextAlign::Center);

  drawText(screen, fui::makeRect(40, 88, 400, 44), "\"", toybox::kUiFont, fui::TextAlign::Center);

  const size_t qLen = q.text.length();
  int startY = 230;
  int lineH = 34;
  size_t maxChars = 26;
  int maxLines = 7;
  fui::FontId font = toybox::kUiFont;

  if (qLen <= 70) {
    startY = 270;
    lineH = 36;
    maxChars = 24;
    maxLines = 5;
    font = toybox::kUiFont;
  } else if (qLen > 140) {
    startY = 160;
    lineH = 28;
    maxChars = 34;
    maxLines = 13;
    font = toybox::kTileFont;
  }

  const int linesDrawn = renderWrappedText(screen, q.text, maxChars, 48, startY, 384, lineH, maxLines, font);

  const int divY = std::min(680, std::max(490, startY + linesDrawn * lineH + 18));
  screen.target().fill(fui::makeRect(210, divY, 60, 2), ink);

  if (!q.author.empty()) {
    char authorBuf[80];
    std::snprintf(authorBuf, sizeof(authorBuf), "- %s -", q.author.c_str());
    drawText(screen, fui::makeRect(48, divY + 14, 384, 24), authorBuf,
             toybox::kTileFont, fui::TextAlign::Center);
  }

  drawText(screen, fui::makeRect(48, 735, 384, 20), "CROSSINK WALLPAPER",
           toybox::kSmallFont, fui::TextAlign::Center, fui::Color::DarkGray);

  if (inverted) {
    renderer.invertScreen();
  }
}

bool renderBmpImage(GfxRenderer& renderer, const std::string& path, bool inverted) {
  renderer.clearScreen();
  HalFile file;
  if (!Storage.openFileForRead("WALLPAPER", path, file)) {
    return false;
  }
  Bitmap bitmap(file, true, renderer.supportsAbsoluteGrayscale());
  if (bitmap.parseHeaders() != BmpReaderError::Ok) {
    file.close();
    return false;
  }
  const int pw = renderer.getScreenWidth();
  const int ph = renderer.getScreenHeight();
  const int bw = bitmap.getWidth();
  const int bh = bitmap.getHeight();
  const int x = (pw - bw) / 2;
  const int y = (ph - bh) / 2;
  renderer.drawBitmap(bitmap, x, y, pw, ph);
  file.close();

  if (inverted) {
    renderer.invertScreen();
  }
  return true;
}

WallpaperSettings loadSettings() {
  WallpaperSettings s;
  HalFile file;
  if (Storage.openFileForRead("WALLPAPER", kSettingsFile, file)) {
    char buf[1024];
    const int readBytes = file.read(buf, sizeof(buf) - 1);
    file.close();
    if (readBytes > 0) {
      buf[readBytes] = '\0';
      char* line = std::strtok(buf, "\r\n");
      while (line != nullptr) {
        char* eq = std::strchr(line, '=');
        if (eq) {
          *eq = '\0';
          const char* key = line;
          const char* val = eq + 1;
          if (std::strcmp(key, "auto_shuffle") == 0) {
            s.autoShuffle = (std::atoi(val) != 0);
          } else if (std::strcmp(key, "invert_images") == 0) {
            s.invertImages = (std::atoi(val) != 0);
          } else if (std::strcmp(key, "invert_quotes") == 0) {
            s.invertQuotes = (std::atoi(val) != 0);
          } else if (std::strcmp(key, "schedule") == 0) {
            s.schedule = static_cast<Schedule>(std::atoi(val));
          } else if (std::strcmp(key, "last_epoch") == 0) {
            s.lastShuffleEpoch = static_cast<uint32_t>(std::strtoul(val, nullptr, 10));
          } else if (std::strcmp(key, "last_year") == 0) {
            s.lastShuffleYear = static_cast<uint16_t>(std::atoi(val));
          } else if (std::strcmp(key, "last_month") == 0) {
            s.lastShuffleMonth = static_cast<uint8_t>(std::atoi(val));
          } else if (std::strcmp(key, "last_day") == 0) {
            s.lastShuffleDay = static_cast<uint8_t>(std::atoi(val));
          } else if (std::strcmp(key, "last_hour") == 0) {
            s.lastShuffleHour = static_cast<uint8_t>(std::atoi(val));
          } else if (std::strcmp(key, "active_type") == 0) {
            s.activeType = static_cast<uint8_t>(std::atoi(val));
          } else if (std::strcmp(key, "active_quote_idx") == 0) {
            s.activeQuoteIdx = std::atoi(val);
          } else if (std::strcmp(key, "active_image") == 0) {
            s.activeImageFilename = val;
          }
        }
        line = std::strtok(nullptr, "\r\n");
      }
      return s;
    }
  }

  // Fallback / initial setup: check legacy marker
  if (Storage.exists(kAutoShuffleMarker)) {
    s.autoShuffle = true;
  }
  return s;
}

void saveSettings(const WallpaperSettings& settings) {
  ensureStorageDirs();
  HalFile file;
  if (Storage.openFileForWrite("WALLPAPER", kSettingsTemp, file)) {
    char line[128];
    std::snprintf(line, sizeof(line), "auto_shuffle=%d\n", settings.autoShuffle ? 1 : 0);
    file.write(reinterpret_cast<const uint8_t*>(line), std::strlen(line));

    std::snprintf(line, sizeof(line), "invert_images=%d\n", settings.invertImages ? 1 : 0);
    file.write(reinterpret_cast<const uint8_t*>(line), std::strlen(line));

    std::snprintf(line, sizeof(line), "invert_quotes=%d\n", settings.invertQuotes ? 1 : 0);
    file.write(reinterpret_cast<const uint8_t*>(line), std::strlen(line));

    std::snprintf(line, sizeof(line), "schedule=%d\n", static_cast<int>(settings.schedule));
    file.write(reinterpret_cast<const uint8_t*>(line), std::strlen(line));

    std::snprintf(line, sizeof(line), "last_epoch=%lu\n", static_cast<unsigned long>(settings.lastShuffleEpoch));
    file.write(reinterpret_cast<const uint8_t*>(line), std::strlen(line));

    std::snprintf(line, sizeof(line), "last_year=%u\n", settings.lastShuffleYear);
    file.write(reinterpret_cast<const uint8_t*>(line), std::strlen(line));

    std::snprintf(line, sizeof(line), "last_month=%u\n", settings.lastShuffleMonth);
    file.write(reinterpret_cast<const uint8_t*>(line), std::strlen(line));

    std::snprintf(line, sizeof(line), "last_day=%u\n", settings.lastShuffleDay);
    file.write(reinterpret_cast<const uint8_t*>(line), std::strlen(line));

    std::snprintf(line, sizeof(line), "last_hour=%u\n", settings.lastShuffleHour);
    file.write(reinterpret_cast<const uint8_t*>(line), std::strlen(line));

    std::snprintf(line, sizeof(line), "active_type=%d\n", settings.activeType);
    file.write(reinterpret_cast<const uint8_t*>(line), std::strlen(line));

    std::snprintf(line, sizeof(line), "active_quote_idx=%d\n", settings.activeQuoteIdx);
    file.write(reinterpret_cast<const uint8_t*>(line), std::strlen(line));

    std::snprintf(line, sizeof(line), "active_image=%s\n", settings.activeImageFilename.c_str());
    file.write(reinterpret_cast<const uint8_t*>(line), std::strlen(line));

    file.close();

    if (Storage.exists(kSettingsFile)) {
      Storage.remove(kSettingsFile);
    }
    Storage.rename(kSettingsTemp, kSettingsFile);
  }

  // Maintain legacy marker file for compatibility with any outside checks
  if (settings.autoShuffle) {
    if (!Storage.exists(kAutoShuffleMarker)) {
      HalFile marker;
      if (Storage.openFileForWrite("WALLPAPER", kAutoShuffleMarker, marker)) {
        marker.write(reinterpret_cast<const uint8_t*>("1"), 1);
        marker.close();
      }
    }
  } else {
    if (Storage.exists(kAutoShuffleMarker)) {
      Storage.remove(kAutoShuffleMarker);
    }
  }
}

bool isAutoShuffleEnabled() {
  return loadSettings().autoShuffle;
}

void setAutoShuffleEnabled(bool enable) {
  WallpaperSettings s = loadSettings();
  s.autoShuffle = enable;
  saveSettings(s);
}

bool drawAsleep(GfxRenderer& renderer) {
  WallpaperSettings settings = loadSettings();
  if (!settings.autoShuffle) return false;

  std::vector<Quote> quotes;
  loadQuotesFromSd(quotes);
  std::vector<BmpItem> images;
  scanBmpImages(images);

  const bool hasQuotes = !quotes.empty();
  const bool hasImages = !images.empty();
  if (!hasQuotes && !hasImages) return false;

  uint16_t curYear = 0;
  uint8_t curMonth = 0, curDay = 0, curHour = 0, curMinute = 0;
  const bool hasRtc = halClock.isAvailable() && halClock.getDateTime(curYear, curMonth, curDay, curHour, curMinute);

  bool shouldShuffle = true;
  if (settings.schedule == Schedule::EveryHour) {
    if (hasRtc) {
      struct tm t = {};
      t.tm_year = curYear - 1900;
      t.tm_mon = curMonth - 1;
      t.tm_mday = curDay;
      t.tm_hour = curHour;
      t.tm_min = curMinute;
      time_t nowSec = mktime(&t);
      if (settings.lastShuffleEpoch != 0 && nowSec >= settings.lastShuffleEpoch &&
          (nowSec - settings.lastShuffleEpoch) < 3600) {
        shouldShuffle = false;
      }
    } else {
      if (settings.lastShuffleHour == curHour && settings.lastShuffleDay == curDay && settings.lastShuffleEpoch != 0) {
        shouldShuffle = false;
      }
    }
  } else if (settings.schedule == Schedule::EveryDay) {
    if (hasRtc) {
      if (settings.lastShuffleYear == curYear && settings.lastShuffleMonth == curMonth &&
          settings.lastShuffleDay == curDay && settings.lastShuffleEpoch != 0) {
        shouldShuffle = false;
      }
    } else {
      if (settings.lastShuffleDay == curDay && settings.lastShuffleEpoch != 0) {
        shouldShuffle = false;
      }
    }
  }

  bool renderedActive = false;
  if (!shouldShuffle) {
    if (settings.activeType == 1 && !settings.activeImageFilename.empty()) {
      const std::string path = "/XTData/Wallpaper/" + settings.activeImageFilename;
      if (renderBmpImage(renderer, path, settings.invertImages)) {
        renderedActive = true;
      }
    } else if (hasQuotes) {
      const int qIdx = std::clamp(settings.activeQuoteIdx, 0, static_cast<int>(quotes.size()) - 1);
      renderQuotePoster(renderer, quotes[qIdx], settings.invertQuotes);
      renderedActive = true;
    }
  }

  if (!renderedActive) {
    bool chooseImage = false;
    if (hasQuotes && hasImages) {
      chooseImage = (random(2) == 1);
    } else {
      chooseImage = hasImages;
    }

    if (chooseImage) {
      const int idx = static_cast<int>(random(static_cast<long>(images.size())));
      const std::string path = "/XTData/Wallpaper/" + images[idx].filename;
      if (renderBmpImage(renderer, path, settings.invertImages)) {
        settings.activeType = 1;
        settings.activeImageFilename = images[idx].filename;
      } else if (hasQuotes) {
        const int qIdx = static_cast<int>(random(static_cast<long>(quotes.size())));
        renderQuotePoster(renderer, quotes[qIdx], settings.invertQuotes);
        settings.activeType = 0;
        settings.activeQuoteIdx = qIdx;
      } else {
        return false;
      }
    } else {
      const int idx = static_cast<int>(random(static_cast<long>(quotes.size())));
      renderQuotePoster(renderer, quotes[idx], settings.invertQuotes);
      settings.activeType = 0;
      settings.activeQuoteIdx = idx;
    }

    if (hasRtc) {
      struct tm t = {};
      t.tm_year = curYear - 1900;
      t.tm_mon = curMonth - 1;
      t.tm_mday = curDay;
      t.tm_hour = curHour;
      t.tm_min = curMinute;
      settings.lastShuffleEpoch = static_cast<uint32_t>(mktime(&t));
    } else {
      settings.lastShuffleEpoch = 1;
    }
    settings.lastShuffleYear = curYear;
    settings.lastShuffleMonth = curMonth;
    settings.lastShuffleDay = curDay;
    settings.lastShuffleHour = curHour;

    saveSettings(settings);
  }

  renderer.displayBuffer(HalDisplay::HALF_REFRESH, true);
  return true;
}

}  // namespace wallpaper
