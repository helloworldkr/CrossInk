#pragma once

#include <memory>

#include "../../activities/Activity.h"
#include "../ui/ToyboxScreen.h"
#include "SanyamCore.h"
#include "SanyamScreens.h"

class SanyamActivity final : public Activity {
 public:
  SanyamActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Sanyam", renderer, mappedInput) {}
  ~SanyamActivity() override = default;

  static std::unique_ptr<Activity> create(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  enum class View : uint8_t { Card, List, Sutra };

  void openCard(int itemIndex);
  void openList();
  void openSutra(int sutraIndex);

  void updateDisplayDate();

  sanyam::Store store_;
  View view_ = View::Card;
  int currentItemIndex_ = 0;
  int activeTab_ = 0;
  int listPage_ = 0;
  int sutraIndex_ = 0;
  int dayOffset_ = 0;

  char todayDate_[sanyam::kDateLen] = {};
  char displayDate_[sanyam::kDateLen] = {};
  char displayHeader_[32] = {};

  toybox::Interactions interactions_;
  bool interactionsReady_ = false;
};
