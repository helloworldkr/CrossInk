#include "StarredBooksActivity.h"

#include <Arduino.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>

#include <algorithm>
#include <memory>

#include "BookActions.h"
#include "FileBrowserActionActivity.h"
#include "MappedInputManager.h"
#include "activities/reader/EpubReaderActivity.h"
#include "activities/util/ConfirmationActivity.h"
#include "activities/util/OptionSelectionActivity.h"
#include "components/CompactHeader.h"
#include "components/TouchHeaderBackButton.h"
#include "components/UITheme.h"
#include "components/UIThemeTokens.h"
#include "components/UiAppHelpers.h"
#include "fontIds.h"

namespace fui = freeink::ui;

namespace {
constexpr unsigned long LONG_PRESS_MS = 1000;
constexpr fui::ActionId ACTION_ROW = 1;
constexpr size_t MAX_LIST_STARRED_BOOKS = 50;
}  // namespace

StarredBooksActivity::StarredBooksActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("StarredBooks", renderer, mappedInput),
      uiTarget(makeUiTarget(renderer)),
      app(uiTarget, uiTarget.deviceContext()) {}

void StarredBooksActivity::loadStarredBooks() {
  starredBooks.clear();
  const auto& books = STARRED_BOOKS.getBooks();
  starredBooks.reserve(std::min(books.size(), MAX_LIST_STARRED_BOOKS));

  for (const auto& book : books) {
    if (starredBooks.size() >= MAX_LIST_STARRED_BOOKS) {
      break;
    }
    if (StarredBooksStore::isMissing(book)) {
      continue;
    }
    starredBooks.push_back(book);
  }
}

void StarredBooksActivity::onRowEvent(const fui::ActionEvent& event, void* user) {
  auto* self = static_cast<StarredBooksActivity*>(user);
  if (event.value < 0 || event.value >= static_cast<int16_t>(self->starredBooks.size())) return;
  self->selectorIndex = static_cast<size_t>(event.value);
  if (event.longPress) {
    self->app.clearTapFlash();
    self->showBookActionMenu(self->selectorIndex);
    return;
  }
  self->app.clearTapFlash();
  self->onSelectBook(self->starredBooks[self->selectorIndex].path);
}

void StarredBooksActivity::onEnter() {
  Activity::onEnter();

  if (STARRED_BOOKS.pruneMissing()) {
    STARRED_BOOKS.saveToFile();
  }

  loadStarredBooks();

  selectorIndex = 0;
  uiReady = false;
  visibleRows = 1;
  topIndex = 0;
  applySharedUiTheme(app, uiTarget);
  app.on(ACTION_ROW, &StarredBooksActivity::onRowEvent, this);
  app.setScreen(&StarredBooksActivity::listScreen, this);
  requestUpdate();
}

void StarredBooksActivity::onExit() {
  Activity::onExit();
  starredBooks.clear();
}

void StarredBooksActivity::loop() {
  if (TouchHeaderBackButton::wasTapped(mappedInput, renderer)) {
    onGoHome();
    return;
  }
  const int listSize = static_cast<int>(starredBooks.size());

  if (longPressFired) {
    if (!mappedInput.isPressed(MappedInputManager::Button::Confirm)) {
      longPressFired = false;
    }
    return;
  }

  if (!starredBooks.empty() && selectorIndex < starredBooks.size() &&
      mappedInput.isPressed(MappedInputManager::Button::Confirm) && mappedInput.getHeldTime() >= LONG_PRESS_MS) {
    longPressFired = true;
    showBookActionMenu(selectorIndex, true);
    return;
  }

  if (uiReady) {
    const fui::InputSnapshot snap = touchSnapshotFrom(mappedInput);
    if (snap.touchPressed || snap.touchReleased) {
      const auto event = app.route(snap);
      if (app.invalidated()) requestUpdate();
      if (event) return;
    }
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (!starredBooks.empty() && selectorIndex < starredBooks.size()) {
      onSelectBook(starredBooks[selectorIndex].path);
      return;
    }
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    onGoHome();
    return;
  }

  const auto swipe = mappedInput.wasSwipe();
  if (swipe == MappedInputManager::SwipeDir::Up || swipe == MappedInputManager::SwipeDir::Down) {
    const int delta = swipe == MappedInputManager::SwipeDir::Up ? visibleRows : -visibleRows;
    const int next = scrollListBy(topIndex, delta, visibleRows, listSize);
    if (next != topIndex) {
      topIndex = next;
      requestUpdate();
    }
    return;
  }

  const auto moveSelection = [this, listSize](const int index) {
    selectorIndex = static_cast<size_t>(index);
    topIndex = followListSelection(static_cast<int>(selectorIndex), topIndex, visibleRows, listSize);
    requestUpdate();
  };

  buttonNavigator.onNextRelease([this, listSize, &moveSelection] {
    moveSelection(ButtonNavigator::nextIndex(static_cast<int>(selectorIndex), listSize));
  });
  buttonNavigator.onPreviousRelease([this, listSize, &moveSelection] {
    moveSelection(ButtonNavigator::previousIndex(static_cast<int>(selectorIndex), listSize));
  });
  buttonNavigator.onNextContinuous([this, listSize, &moveSelection] {
    moveSelection(ButtonNavigator::nextPageIndex(static_cast<int>(selectorIndex), listSize, visibleRows));
  });
  buttonNavigator.onPreviousContinuous([this, listSize, &moveSelection] {
    moveSelection(ButtonNavigator::previousPageIndex(static_cast<int>(selectorIndex), listSize, visibleRows));
  });
}

void StarredBooksActivity::reloadAfterBookAction() {
  loadStarredBooks();
  if (selectorIndex >= starredBooks.size()) {
    selectorIndex = starredBooks.empty() ? 0 : starredBooks.size() - 1;
  }
  const int listSize = static_cast<int>(starredBooks.size());
  topIndex = followListSelection(static_cast<int>(selectorIndex), topIndex, visibleRows, listSize);
  requestUpdate();
}

void StarredBooksActivity::promptRemoveStar(const std::string& path, const std::string& title) {
  auto handler = [this, path](const ActivityResult& res) {
    if (res.isCancelled) return;
    if (STARRED_BOOKS.removeStar(path)) {
      BookActions::drawToast(renderer, tr(STR_STAR_REMOVED));
    }
    reloadAfterBookAction();
  };

  startActivityForResult(
      std::make_unique<ConfirmationActivity>(renderer, mappedInput, tr(STR_REMOVE_STAR), title,
                                             /*ignoreInitialConfirmRelease=*/false),
      std::move(handler));
}

void StarredBooksActivity::promptDeleteBook(const StarredBook& book) {
  auto handler = [this, book](const ActivityResult& res) {
    if (res.isCancelled) return;
    BookActions::clearFileMetadata(book.path);
    if (!Storage.remove(book.path.c_str())) {
      LOG_ERR("StarredBooks", "Failed to delete file: %s", book.path.c_str());
      return;
    }
    STARRED_BOOKS.removeStar(book.path);
    reloadAfterBookAction();
  };

  const std::string heading = tr(STR_DELETE) + std::string("? ");
  startActivityForResult(
      std::make_unique<ConfirmationActivity>(renderer, mappedInput, heading, book.title,
                                             /*ignoreInitialConfirmRelease=*/false),
      std::move(handler));
}

void StarredBooksActivity::showBookActionMenu(const size_t bookIndex, const bool ignoreInitialConfirmRelease) {
  if (bookIndex >= starredBooks.size()) return;

  const StarredBook book = starredBooks[bookIndex];
  std::vector<FileBrowserActionActivity::MenuItem> items =
      BookActions::buildBookActionItems(book.path, /*includeRemoveFromRecents=*/false);
  if (BookActions::canSendNearby(book.path)) {
    items.push_back({FileBrowserAction::SendNearby, StrId::STR_SEND_NEARBY_BOOK});
  }

  startActivityForResult(
      std::make_unique<FileBrowserActionActivity>(renderer, mappedInput, book.title, std::move(items),
                                                   ignoreInitialConfirmRelease),
      [this, book](const ActivityResult& result) {
        longPressFired = false;
        if (result.isCancelled) return;

        const auto* actionResult = std::get_if<FileBrowserActionResult>(&result.data);
        if (!actionResult) return;

        switch (static_cast<FileBrowserAction>(actionResult->action)) {
          case FileBrowserAction::Delete:
            promptDeleteBook(book);
            return;
          case FileBrowserAction::ToggleStar:
            promptRemoveStar(book.path, book.title);
            return;
          case FileBrowserAction::DeleteCache:
            startActivityForResult(
                std::make_unique<ConfirmationActivity>(renderer, mappedInput,
                                                       BookActions::confirmationHeading(StrId::STR_DELETE_CACHE),
                                                       book.title),
                [this, book](const ActivityResult& confirmation) {
                  if (!confirmation.isCancelled) {
                    if (BookActions::clearBookCache(book.path)) {
                      BookActions::drawToast(renderer, tr(STR_BOOK_CACHE_DELETED));
                    }
                  }
                  reloadAfterBookAction();
                });
            return;
          case FileBrowserAction::DeleteStats:
            startActivityForResult(
                std::make_unique<ConfirmationActivity>(renderer, mappedInput,
                                                       BookActions::confirmationHeading(StrId::STR_DELETE_BOOK_STATS),
                                                       book.title),
                [this, book](const ActivityResult& confirmation) {
                  if (!confirmation.isCancelled) {
                    if (BookActions::deleteBookStats(book.path)) {
                      BookActions::drawToast(renderer, tr(STR_BOOK_STATS_DELETED));
                    }
                  }
                  reloadAfterBookAction();
                });
            return;
          case FileBrowserAction::ToggleCompleted: {
            bool completed = false;
            if (BookActions::toggleBookCompleted(book.path, book.title, completed)) {
              BookActions::drawToast(renderer, completed ? tr(STR_MARKED_FINISHED) : tr(STR_MARKED_UNFINISHED));
              delay(1000);
            }
            reloadAfterBookAction();
            return;
          }
          case FileBrowserAction::EpubRenderMode: {
            const uint8_t currentIndex =
                BookActions::epubRenderModeDisplayIndex(EpubReaderActivity::loadBookRenderMode(book.path));
            startActivityForResult(
                std::make_unique<OptionSelectionActivity>(renderer, mappedInput, "StarredEpubRenderModeSelect",
                                                          StrId::STR_EPUB_RENDER_MODE,
                                                          BookActions::epubRenderModeOptions(), currentIndex),
                [this, book](const ActivityResult& selectionResult) {
                  if (!selectionResult.isCancelled) {
                    const auto* selection = std::get_if<OptionSelectionResult>(&selectionResult.data);
                    if (selection != nullptr &&
                        !EpubReaderActivity::saveBookRenderMode(
                            book.path, BookActions::epubRenderModeForDisplayIndex(selection->index))) {
                      LOG_ERR("StarredBooks", "Failed to save render mode for: %s", book.path.c_str());
                    }
                  }
                  reloadAfterBookAction();
                });
            return;
          }
          case FileBrowserAction::SendNearby:
            activityManager.goToNearbyBookSend(book.path, false);
            return;
          case FileBrowserAction::RemoveFromRecents:
          case FileBrowserAction::PinFavorite:
          case FileBrowserAction::UnpinFavorite:
          case FileBrowserAction::PinBootFavorite:
          case FileBrowserAction::UnpinBootFavorite:
          case FileBrowserAction::SetSleepFolder:
          case FileBrowserAction::ClearSleepFolder:
          case FileBrowserAction::ViewBookmarks:
          case FileBrowserAction::ViewClippings:
          case FileBrowserAction::DeleteBookmarks:
          case FileBrowserAction::DeleteClippings:
          case FileBrowserAction::ResetReaderSettings:
          case FileBrowserAction::Rename:
            return;
        }
      });
}

void StarredBooksActivity::listScreen(UiApp::ScreenType& screen, void* user) {
  static_cast<StarredBooksActivity*>(user)->buildListScreen(screen);
}

void StarredBooksActivity::buildListScreen(UiApp::ScreenType& screen) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  screen.setContentMargin(
      fui::Insets{static_cast<int16_t>(metrics.topPadding + TouchHeaderBackButton::height(metrics, mappedInput)), 0,
                  static_cast<int16_t>(metrics.buttonHintsHeight), 0});
  screen.spacer(static_cast<int16_t>(metrics.verticalSpacing));

  if (starredBooks.empty()) {
    screen.centeredText(tr(STR_NO_STARRED_BOOKS), screen.theme().bodyText);
    return;
  }

  std::vector<fui::ListItem> items;
  items.reserve(starredBooks.size());
  for (const auto& book : starredBooks) {
    fui::ListItem item;
    item.label = book.title.c_str();
    if (!book.author.empty()) item.subtitle = book.author.c_str();
    item.icon = listIconFor(UIIcon::Star, 32);
    item.actionValue = static_cast<int16_t>(items.size());
    items.push_back(item);
  }

  fui::ListProps props;
  props.items = items.data();
  props.count = static_cast<uint16_t>(items.size());
  props.selectedIndex = static_cast<int16_t>(selectorIndex);
  props.action = ACTION_ROW;
  props.inputMask = static_cast<uint16_t>(fui::InputTouch | fui::InputLongPress);
  props.iconSize = 28;
  props.labelText = screen.theme().bodyText;
  props.labelText.bold = true;
  const fui::Rect listBounds = screen.body();
  const auto rows = configureUiList(props, screen.theme(), listBounds, UiListRowType::WithSubtitle);
  visibleRows = rows > 0 ? rows : 1;
  topIndex = scrollListBy(topIndex, 0, visibleRows, static_cast<int>(starredBooks.size()));
  props.topIndex = static_cast<uint16_t>(topIndex);
  screen.list(props);
}

void StarredBooksActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const Rect header = TouchHeaderBackButton::headerRect(renderer, mappedInput);
  if (mappedInput.hasTouchHardware()) {
    TouchHeaderBackButton::draw(renderer, uiTarget, header, tr(STR_STARRED_BOOKS), false);
  } else {
    GUI.drawHeader(renderer, header, tr(STR_STARRED_BOOKS));
  }

  uiReady = false;
  app.render();
  uiReady = true;

  const auto labels =
      mappedInput.mapLabels(mappedInput.withBackArrow(tr(STR_HOME)), tr(STR_OPEN), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}
