#pragma once
#include <FreeInkApp.h>
#include <FreeInkUIGfxRenderer.h>
#include <I18n.h>

#include <atomic>
#include <functional>
#include <string>
#include <vector>

#include "StarredBooksStore.h"
#include "activities/Activity.h"
#include "util/ButtonNavigator.h"

class StarredBooksActivity final : public Activity {
 private:
  using UiApp = freeink::ui::FreeInkApp<20, 4>;

  ButtonNavigator buttonNavigator;
  size_t selectorIndex = 0;
  bool longPressFired = false;

  std::vector<StarredBook> starredBooks;

  freeink::ui::GfxRendererTarget uiTarget;
  UiApp app;
  std::atomic<bool> uiReady{false};
  int visibleRows = 1;
  int topIndex = 0;

  static void listScreen(UiApp::ScreenType& screen, void* user);
  static void onRowEvent(const freeink::ui::ActionEvent& event, void* user);
  void buildListScreen(UiApp::ScreenType& screen);

  void loadStarredBooks();
  void reloadAfterBookAction();
  void promptRemoveStar(const std::string& path, const std::string& title);
  void promptDeleteBook(const StarredBook& book);
  void showBookActionMenu(size_t bookIndex, bool ignoreInitialConfirmRelease = false);

 public:
  explicit StarredBooksActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
};
