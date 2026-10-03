#pragma once

#include <I18n.h>

#include <array>
#include <cstddef>
#include <string>
#include <vector>

#include "activities/Activity.h"
#include "components/UiAppHost.h"
#include "util/ButtonNavigator.h"

class XtcReaderMenuActivity final : public Activity {
 public:
  enum class MenuAction {
    SELECT_CHAPTER,
    START_FROM_BEGINNING,
    STAR_TOGGLE,
    READING_STATS,
    GO_TO_BOOK_FOLDER,
    TOGGLE_COMPLETED,
    DELETE_STATS,
    DELETE_CACHE,
    SEND_NEARBY_BOOK,
    DISABLE_TOUCHSCREEN,
  };

  XtcReaderMenuActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string title, bool hasChapters,
                        bool isBookCompleted, bool isBookStarred = false);

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
  bool isReaderActivity() const override { return true; }
  bool allowPowerAsConfirmInReaderMode() const override { return true; }
  bool allowGlobalHomeGesture() const override { return false; }

 private:
  using UiHost = UiAppHost<12, 2>;
  using UiApp = UiHost::App;
  static constexpr size_t kMaxMenuItems = 10;

  struct MenuItem {
    MenuAction action;
    StrId labelId;
  };

  static std::vector<MenuItem> buildMenuItems(bool hasChapters, bool isBookCompleted, bool hasTouch,
                                              bool isBookStarred);
  void finishCancelled();

  ButtonNavigator buttonNavigator;
  std::string title;
  std::vector<MenuItem> items;
  std::array<freeink::ui::ListItem, kMaxMenuItems> listItems{};
  int selectedIndex = 0;
  UiHost ui;
  int visibleRows = 1;
  int topIndex = 0;
  int listHeaderHeight = 0;

  static void listScreen(UiApp::ScreenType& screen, void* user);
  static void onRowEvent(const freeink::ui::ActionEvent& event, void* user);
  void buildListScreen(UiApp::ScreenType& screen);
  void refreshListItems();
  void selectCurrent();
};
