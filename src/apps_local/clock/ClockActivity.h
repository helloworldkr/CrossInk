#pragma once

#include <cstdint>
#include <memory>

#include "../../activities/Activity.h"
#include "../../components/themes/BaseTheme.h"

class ClockActivity final : public Activity {
 public:
  enum class DisplayMode : uint8_t {
    Analog = 0,
    Digital = 1
  };

  explicit ClockActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  ~ClockActivity() override = default;

  static std::unique_ptr<Activity> create(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

  bool preventAutoSleep() override { return keepAwake_; }

 private:
  struct WallTime {
    int year = 2026;
    int month = 1;     // 1-12
    int day = 1;       // 1-31
    int weekday = 0;   // 0 = Sunday, 6 = Saturday
    int hour = 0;      // 0-23
    int minute = 0;    // 0-59
    int second = 0;    // 0-59
  };

  WallTime getCurrentWallTime() const;
  void loadConfig();
  void saveConfig();

  void renderAnalog(const WallTime& t);
  void renderDigital(const WallTime& t);

  DisplayMode mode_ = DisplayMode::Analog;
  bool use24Hour_ = false;
  bool darkMode_ = false;
  bool keepAwake_ = true;

  uint8_t lastMinute_ = 255;
  unsigned long lastPollMs_ = 0;
  uint32_t fullRefreshCycle_ = 0;

  // Touch button bounds
  Rect exitBtnRect_{};
  Rect modeBtnRect_{};
  Rect formatBtnRect_{};
  Rect themeBtnRect_{};
  Rect clockFaceRect_{};
};
