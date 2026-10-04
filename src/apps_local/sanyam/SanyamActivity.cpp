#include "SanyamActivity.h"

#include <Memory.h>

#include <cstdio>
#include <cstring>

#include "../Shelf.h"
#include "../ui/ToyboxFonts.h"
#include "../ui/ToyboxTheme.h"

std::unique_ptr<Activity> SanyamActivity::create(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return makeUniqueNoThrow<SanyamActivity>(renderer, mappedInput);
}

void SanyamActivity::onEnter() {
  Activity::onEnter();
  toybox::ensureFonts(renderer);

  sanyam::Store::getTodayDate(todayDate_, sizeof(todayDate_), nullptr, 0);
  dayOffset_ = 0;
  currentItemIndex_ = 0;
  activeTab_ = 0;
  listPage_ = 0;
  sutraIndex_ = 0;

  updateDisplayDate();
  openCard(0);
}

void SanyamActivity::onExit() {
  store_.saveDay();
  Activity::onExit();
}

void SanyamActivity::updateDisplayDate() {
  sanyam::Store::formatDisplayDate(todayDate_, dayOffset_, displayDate_, sizeof(displayDate_), displayHeader_,
                                   sizeof(displayHeader_));
  store_.loadDay(displayDate_);
}

void SanyamActivity::openCard(int itemIndex) {
  if (itemIndex < 0) itemIndex = 0;
  if (itemIndex >= sanyam::kTotalItems) itemIndex = sanyam::kTotalItems - 1;
  currentItemIndex_ = itemIndex;
  view_ = View::Card;
  interactionsReady_ = false;
  requestUpdate();
}

void SanyamActivity::openList() {
  view_ = View::List;
  interactionsReady_ = false;
  requestUpdate();
}

void SanyamActivity::openSutra(int sutraIndex) {
  if (sutraIndex < 0) sutraIndex = 0;
  if (sutraIndex >= sanyam::kTotalQuotes) sutraIndex = 0;
  sutraIndex_ = sutraIndex;
  view_ = View::Sutra;
  interactionsReady_ = false;
  requestUpdate();
}

void SanyamActivity::loop() {
  // 1. Hardware Buttons
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    if (view_ == View::Sutra) {
      openCard(currentItemIndex_);
      return;
    }
    if (view_ == View::Card) {
      openList();
      return;
    }
    if (dayOffset_ != 0) {
      dayOffset_ = 0;
      updateDisplayDate();
      requestUpdate();
      return;
    }
    shelf::leave(renderer, mappedInput);
    return;
  }

  // Prev / Left
  if (mappedInput.wasReleased(MappedInputManager::Button::PageBack) ||
      mappedInput.wasReleased(MappedInputManager::Button::Left) ||
      mappedInput.wasReleased(MappedInputManager::Button::Up)) {
    if (view_ == View::Card) {
      if (currentItemIndex_ > 0) {
        currentItemIndex_--;
        requestUpdate();
        return;
      }
    } else if (view_ == View::List) {
      if (listPage_ > 0) {
        listPage_--;
        requestUpdate();
        return;
      }
    } else if (view_ == View::Sutra) {
      sutraIndex_ = (sutraIndex_ - 1 + sanyam::kTotalQuotes) % sanyam::kTotalQuotes;
      requestUpdate();
      return;
    }
  }

  // Next / Right
  if (mappedInput.wasReleased(MappedInputManager::Button::PageForward) ||
      mappedInput.wasReleased(MappedInputManager::Button::Right) ||
      mappedInput.wasReleased(MappedInputManager::Button::Down)) {
    if (view_ == View::Card) {
      if (currentItemIndex_ < sanyam::kTotalItems - 1) {
        currentItemIndex_++;
        requestUpdate();
        return;
      }
    } else if (view_ == View::List) {
      listPage_++;
      requestUpdate();
      return;
    } else if (view_ == View::Sutra) {
      sutraIndex_ = (sutraIndex_ + 1) % sanyam::kTotalQuotes;
      requestUpdate();
      return;
    }
  }

  // Confirm / Select Button
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (view_ == View::Card) {
      store_.cycleRating(currentItemIndex_);
      requestUpdate();
      return;
    } else if (view_ == View::Sutra) {
      sutraIndex_ = (sutraIndex_ + 1) % sanyam::kTotalQuotes;
      requestUpdate();
      return;
    }
  }

  // 2. Touch Screen Interactions
  int x = 0;
  int y = 0;
  if (!mappedInput.wasScreenTapped(x, y) || !interactionsReady_) return;

  fui::InputSnapshot input{};
  input.touchReleased = true;
  input.touchX = static_cast<int16_t>(x);
  input.touchY = static_cast<int16_t>(y);
  const fui::ActionEvent action = interactions_.route(input);

  // Category Tabs (in List view)
  if (action.action >= sanyamui::ActionTabAll && action.action <= sanyamui::ActionTabSymptom) {
    activeTab_ = action.action - sanyamui::ActionTabAll;
    listPage_ = 0;
    requestUpdate();
    return;
  }

  // Date Navigation
  if (action.action == sanyamui::ActionDayPrev) {
    --dayOffset_;
    updateDisplayDate();
    requestUpdate();
    return;
  }
  if (action.action == sanyamui::ActionDayNext) {
    if (dayOffset_ < 0) {
      ++dayOffset_;
      updateDisplayDate();
      requestUpdate();
    }
    return;
  }
  if (action.action == sanyamui::ActionGoToday) {
    dayOffset_ = 0;
    updateDisplayDate();
    requestUpdate();
    return;
  }

  // View switches
  if (action.action == sanyamui::ActionSwitchToList) {
    if (view_ == View::Card) {
      openList();
    } else {
      shelf::leave(renderer, mappedInput);
    }
    return;
  }
  if (action.action == sanyamui::ActionOpenSutra) {
    openSutra(sutraIndex_);
    return;
  }
  if (action.action == sanyamui::ActionCloseSutra) {
    openCard(currentItemIndex_);
    return;
  }
  if (action.action == sanyamui::ActionNextSutra) {
    sutraIndex_ = (sutraIndex_ + 1) % sanyam::kTotalQuotes;
    requestUpdate();
    return;
  }
  if (action.action == sanyamui::ActionPrevSutra) {
    sutraIndex_ = (sutraIndex_ - 1 + sanyam::kTotalQuotes) % sanyam::kTotalQuotes;
    requestUpdate();
    return;
  }

  // Item Navigation in Card View
  if (action.action == sanyamui::ActionPrevItem) {
    if (currentItemIndex_ > 0) {
      --currentItemIndex_;
      requestUpdate();
    }
    return;
  }
  if (action.action == sanyamui::ActionNextItem) {
    if (currentItemIndex_ < sanyam::kTotalItems - 1) {
      ++currentItemIndex_;
      requestUpdate();
    }
    return;
  }

  // Rating Choices in Card View
  if (action.action == sanyamui::ActionRate1) {
    store_.setRating(currentItemIndex_, 1);
    requestUpdate();
    return;
  }
  if (action.action == sanyamui::ActionRate2) {
    store_.setRating(currentItemIndex_, 2);
    requestUpdate();
    return;
  }
  if (action.action == sanyamui::ActionRate3) {
    store_.setRating(currentItemIndex_, 3);
    requestUpdate();
    return;
  }

  // Pagination in List View
  if (action.action == sanyamui::ActionPagePrev) {
    if (listPage_ > 0) {
      --listPage_;
      requestUpdate();
    }
    return;
  }
  if (action.action == sanyamui::ActionPageNext) {
    ++listPage_;
    requestUpdate();
    return;
  }

  // Item Tap in List View
  if (action.action >= sanyamui::ActionItemCardBase &&
      action.action < sanyamui::ActionItemCardBase + sanyam::kTotalItems) {
    const int tappedIdx = action.action - sanyamui::ActionItemCardBase;
    openCard(tappedIdx);
    return;
  }
}

void SanyamActivity::render(RenderLock&&) {
  renderer.clearScreen();
  fui::GfxRendererTarget target = toybox::makeTarget(renderer);
  const fui::InputSnapshot noInput{};
  interactionsReady_ = false;
  toybox::Frame frame(target, target.deviceContext(), noInput, interactions_);
  toybox::Screen screen(frame);

  switch (view_) {
    case View::Card:
      sanyamui::buildCardView(screen, store_, currentItemIndex_, displayDate_, displayHeader_, dayOffset_);
      break;

    case View::List:
      sanyamui::buildListView(screen, store_, activeTab_, listPage_, displayDate_, displayHeader_, dayOffset_);
      break;

    case View::Sutra:
      sanyamui::buildSutraView(screen, sutraIndex_);
      break;
  }

  interactionsReady_ = true;
  toybox::reportOverflow(interactions_, "Sanyam");
  renderer.displayBuffer();
}
