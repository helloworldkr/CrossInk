#pragma once

#include <memory>

#include "../../activities/Activity.h"
#include "../ui/ToyboxScreen.h"

namespace breathe {

enum class Technique : uint8_t {
  Equal,     // 4s In, 4s Out (Sama Vritti)
  Relax478,  // 4s In, 7s Hold, 8s Out (Dr. Andrew Weil)
  Box        // 4s In, 4s Hold, 4s Out, 4s Hold (Navy SEALs)
};

enum class Phase : uint8_t {
  Inhale,
  HoldIn,
  Exhale,
  HoldOut
};

class SessionEngine {
 public:
  SessionEngine();

  void configure(Technique technique, int breathCount);
  void start();
  void pause();
  void resume();
  void stop();
  bool tick();

  Technique technique() const { return technique_; }
  Phase currentPhase() const { return currentPhase_; }
  int currentBreath() const { return currentBreath_; }
  int totalBreaths() const { return totalBreaths_; }
  int secondsRemainingInPhase() const { return secondsRemainingInPhase_; }
  int totalElapsedSeconds() const { return totalElapsedSeconds_; }
  int totalEstimatedSeconds() const { return cycleDuration(technique_) * totalBreaths_; }

  bool isPaused() const { return paused_; }
  bool isFinished() const { return finished_; }

  float phaseProgress() const;
  float sessionProgress() const;

  const char* phaseLabel() const;
  const char* mindfulCue() const;

  static const char* techniqueName(Technique t);
  static const char* techniqueSubtitle(Technique t);
  static const char* techniqueDescription(Technique t);
  static int cycleDuration(Technique t);
  static void formatTime(int totalSeconds, char* out, size_t outSize);

 private:
  void advanceToNextPhase();
  int phaseDuration() const;

  Technique technique_ = Technique::Equal;
  int totalBreaths_ = 8;
  int currentBreath_ = 1;
  Phase currentPhase_ = Phase::Inhale;
  int secondsRemainingInPhase_ = 4;
  int totalElapsedSeconds_ = 0;
  bool paused_ = false;
  bool finished_ = false;
};

}  // namespace breathe

class BreatheActivity final : public Activity {
 public:
  BreatheActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Breathe", renderer, mappedInput) {}
  ~BreatheActivity() override = default;

  static std::unique_ptr<Activity> create(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
  bool preventAutoSleep() override { return screenState_ == ScreenState::Active; }

 private:
  enum class ScreenState : uint8_t { Setup, Active, Complete };

  void openSetup();
  void startBreathing();
  void completeSession();

  breathe::SessionEngine engine_;
  ScreenState screenState_ = ScreenState::Setup;

  breathe::Technique selectedTech_ = breathe::Technique::Equal;
  int selectedBreaths_ = 8;

  uint32_t lastTickMs_ = 0;

  toybox::Interactions interactions_;
  bool interactionsReady_ = false;
};
