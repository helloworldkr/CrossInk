#pragma once

#include <cstdint>
#include <memory>

#include "../../activities/Activity.h"
#include "../../components/themes/BaseTheme.h"

class PomodoroActivity final : public Activity {
 public:
  enum class State : uint8_t {
    IDLE = 0,
    FOCUS = 1,
    SHORT_BREAK = 2,
    LONG_BREAK = 3,
    PAUSED = 4
  };

  enum class IdleField : uint8_t {
    FOCUS = 0,
    SHORT_BREAK = 1,
    LONG_BREAK = 2
  };

  explicit PomodoroActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  ~PomodoroActivity() override = default;

  static std::unique_ptr<Activity> create(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

  bool preventAutoSleep() override { return state_ != State::IDLE; }

 private:
  uint32_t getRemainingMs() const;
  void startTimer(State newState, uint32_t durationMs);
  void advanceState();
  const char* getStateLabel() const;

  void loadStats();
  void saveStats();

  State state_ = State::IDLE;
  State pausedFrom_ = State::FOCUS;

  uint32_t focusDurationMs_ = 25 * 60 * 1000;
  uint32_t shortBreakDurationMs_ = 5 * 60 * 1000;
  uint32_t longBreakDurationMs_ = 15 * 60 * 1000;
  uint32_t totalDurationMs_ = 25 * 60 * 1000;

  unsigned long timerStartMs_ = 0;
  unsigned long pausedElapsedMs_ = 0;
  unsigned long lastTickMs_ = 0;

  int completedSessions_ = 0;        // 0 to 4 in current cycle
  int totalCompletedToday_ = 0;      // Total completed pomodoros today
  IdleField selectedField_ = IdleField::FOCUS;

  static constexpr int kSessionsBeforeLongBreak = 4;

  // Touch button hit zones
  Rect exitBtnRect_{};
  Rect startBtnRect_{};
  Rect pauseResumeBtnRect_{};
  Rect skipBtnRect_{};
  Rect resetBtnRect_{};

  Rect focusMinusRect_{};
  Rect focusPlusRect_{};
  Rect shortMinusRect_{};
  Rect shortPlusRect_{};
  Rect longMinusRect_{};
  Rect longPlusRect_{};
};
