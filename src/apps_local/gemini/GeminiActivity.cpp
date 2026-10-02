#include "GeminiActivity.h"

#include <HalStorage.h>
#include <Logging.h>
#include <Memory.h>

#include <algorithm>
#include <ctime>

#include "../../activities/ActivityManager.h"
#include "../../activities/network/WifiSelectionActivity.h"
#include "../../activities/util/KeyboardEntryActivity.h"
#include "../Shelf.h"
#include "../ui/ToyboxFonts.h"
#include "../ui/ToyboxTheme.h"

namespace {

namespace fui = freeink::ui;

std::string sanitizeFilename(const std::string& name) {
  std::string out;
  out.reserve(name.size());
  for (char c : name) {
    if (std::isalnum(static_cast<unsigned char>(c)) || c == ' ' || c == '-' || c == '_') {
      out.push_back(c);
    }
  }
  if (out.size() > 30) out.resize(30);
  while (!out.empty() && out.back() == ' ') out.pop_back();
  return out.empty() ? "Note" : out;
}

}  // namespace

GeminiActivity::GeminiActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("Gemini", renderer, mappedInput) {}

std::unique_ptr<Activity> GeminiActivity::create(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return makeUniqueNoThrow<GeminiActivity>(renderer, mappedInput);
}

void GeminiActivity::onEnter() {
  Activity::onEnter();
  toybox::ensureFonts(renderer);

  tokenInfo_ = gemini::loadToken();
  state_ = State::Welcome;
  renderedThinking_ = false;
  interactionsReady_ = false;

  requestUpdate();
}

void GeminiActivity::onExit() {
  Activity::onExit();
  renderedThinking_ = false;
}

void GeminiActivity::resetChat() {
  history_.clear();
  currentPrompt_.clear();
  fullResponseText_.clear();
  currentPage_ = 0;
  totalPages_ = 1;
  savedToNotes_ = false;
  state_ = State::Welcome;
  interactionsReady_ = false;
  requestUpdate();
}

void GeminiActivity::askPrompt(const std::string& prompt) {
  if (prompt.empty()) return;

  // Refresh token if needed
  if (!tokenInfo_.isFound) {
    tokenInfo_ = gemini::loadToken();
  }

  if (WiFi.status() != WL_CONNECTED) {
    errorTitle_ = "Wi-Fi Not Connected";
    errorMessage_ = "Google Gemini requires an active internet connection. Please connect to Wi-Fi.";
    errorShowWifi_ = true;
    errorShowKey_ = false;
    errorShowRetry_ = false;
    state_ = State::Error;
    interactionsReady_ = false;
    requestUpdate();
    return;
  }

  if (!tokenInfo_.isFound) {
    errorTitle_ = "API Key Missing";
    errorMessage_ = "Please put your Google Gemini API key in /XTData/llm_token on your SD card, or tap ENTER API KEY.";
    errorShowWifi_ = false;
    errorShowKey_ = true;
    errorShowRetry_ = false;
    state_ = State::Error;
    interactionsReady_ = false;
    requestUpdate();
    return;
  }

  currentPrompt_ = prompt;
  state_ = State::Thinking;
  renderedThinking_ = false;
  interactionsReady_ = false;
  requestUpdate();
}

void GeminiActivity::openKeyboardForPrompt(const std::string& prefill) {
  auto keyboard = makeUniqueNoThrow<KeyboardEntryActivity>(renderer, mappedInput, "ASK GEMINI", prefill, 160);
  if (!keyboard) return;

  startActivityForResult(std::move(keyboard), [this](const ActivityResult& result) {
    interactionsReady_ = false;
    if (result.isCancelled) {
      requestUpdate();
      return;
    }
    const auto& entered = std::get<KeyboardResult>(result.data);
    if (!entered.text.empty()) {
      askPrompt(entered.text);
    }
  });
}

void GeminiActivity::openKeyboardForToken() {
  std::string initial = tokenInfo_.isFound ? tokenInfo_.token : "";
  auto keyboard = makeUniqueNoThrow<KeyboardEntryActivity>(renderer, mappedInput, "GEMINI KEY", initial, 80);
  if (!keyboard) return;

  startActivityForResult(std::move(keyboard), [this](const ActivityResult& result) {
    interactionsReady_ = false;
    if (result.isCancelled) {
      requestUpdate();
      return;
    }
    const auto& entered = std::get<KeyboardResult>(result.data);
    if (!entered.text.empty()) {
      gemini::saveToken(entered.text);
      tokenInfo_ = gemini::loadToken();
      noticeTitle_ = "API Key Saved";
      noticeMessage_ = "Your Gemini API key has been saved to /XTData/llm_token.";
      noticeReturnState_ = State::Welcome;
      state_ = State::Notice;
      requestUpdate();
    }
  });
}

void GeminiActivity::openWifiSelection() {
  startActivityForResult(makeUniqueNoThrow<WifiSelectionActivity>(renderer, mappedInput),
                         [this](const ActivityResult& result) {
                           interactionsReady_ = false;
                           if (!result.isCancelled && WiFi.status() == WL_CONNECTED) {
                             noticeTitle_ = "Connected";
                             noticeMessage_ = std::string("Connected to Wi-Fi: ") + WiFi.SSID().c_str();
                             noticeReturnState_ = State::Welcome;
                             state_ = State::Notice;
                           }
                           requestUpdate();
                         });
}

void GeminiActivity::saveResponseToNotes() {
  if (fullResponseText_.empty()) return;

  Storage.ensureDirectoryExists("/notes");

  std::string sanitized = sanitizeFilename(currentPrompt_);
  std::string path = "/notes/Gemini - " + sanitized + ".md";
  std::string content = "# " + currentPrompt_ + "\n\n" + fullResponseText_ + "\n";

  if (!Storage.writeFile(path.c_str(), content.c_str())) {
    noticeTitle_ = "Save Failed";
    noticeMessage_ = "Could not write note file to SD card.";
    noticeReturnState_ = State::Response;
    state_ = State::Notice;
    interactionsReady_ = false;
    requestUpdate();
    return;
  }

  savedToNotes_ = true;
  noticeTitle_ = "Saved to Notes";
  noticeMessage_ = "Response saved to:\n" + path + "\nYou can view it in the Notes app.";
  noticeReturnState_ = State::Response;
  state_ = State::Notice;
  interactionsReady_ = false;
  requestUpdate();
}

void GeminiActivity::loop() {
  // Back button / gesture
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    switch (state_) {
      case State::Welcome:
        shelf::leave(renderer, mappedInput);
        return;
      case State::Response:
      case State::Error:
        state_ = State::Welcome;
        interactionsReady_ = false;
        requestUpdate();
        return;
      case State::Notice:
        state_ = noticeReturnState_;
        interactionsReady_ = false;
        requestUpdate();
        return;
      case State::Thinking:
      case State::Querying:
        state_ = State::Welcome;
        renderedThinking_ = false;
        interactionsReady_ = false;
        requestUpdate();
        return;
    }
  }

  // Physical page keys
  const bool down = mappedInput.wasReleased(MappedInputManager::Button::Down);
  const bool up = mappedInput.wasReleased(MappedInputManager::Button::Up);
  if (down || up) {
    if (state_ == State::Response) {
      if (down && currentPage_ + 1 < totalPages_) {
        currentPage_++;
        interactionsReady_ = false;
        requestUpdate();
        return;
      }
      if (up && currentPage_ > 0) {
        currentPage_--;
        interactionsReady_ = false;
        requestUpdate();
        return;
      }
    }
  }

  // Confirm key
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (state_ == State::Welcome || state_ == State::Response) {
      openKeyboardForPrompt();
      return;
    }
  }

  // Once the Thinking screen has visibly rendered, perform the API query
  if (state_ == State::Thinking && renderedThinking_) {
    state_ = State::Querying;
    LOG_INF("GEMINI", "Executing query for prompt: %s", currentPrompt_.c_str());

    gemini::Response res = client_.query(currentPrompt_, history_, tokenInfo_.token);

    if (res.success) {
      fullResponseText_ = res.text;
      history_.push_back({"user", currentPrompt_});
      history_.push_back({"model", res.text});

      fui::GfxRendererTarget target = toybox::makeTarget(renderer, toybox::readingChromeFaces());
      int contentW = target.deviceContext().width - 2 * toybox::kMargin;
      int totalLines = geminiui::calculateTotalLines(target, static_cast<int16_t>(contentW), res.text);
      int16_t lh = target.lineHeight(toybox::kBodyFont);
      int bodyH = target.deviceContext().height - 210 - 48;
      linesPerPage_ = (lh > 0) ? (bodyH / lh) : 25;
      totalPages_ = (totalLines + linesPerPage_ - 1) / linesPerPage_;
      if (totalPages_ < 1) totalPages_ = 1;
      currentPage_ = 0;
      savedToNotes_ = false;
      state_ = State::Response;
    } else {
      errorTitle_ = "Gemini API Error";
      errorMessage_ = res.error;
      errorShowRetry_ = true;
      errorShowWifi_ = (WiFi.status() != WL_CONNECTED);
      errorShowKey_ = !tokenInfo_.isFound;
      state_ = State::Error;
    }
    interactionsReady_ = false;
    requestUpdate();
    return;
  }

  // Touch routing
  int x = 0;
  int y = 0;
  if (!mappedInput.wasScreenTapped(x, y) || !interactionsReady_) return;

  fui::InputSnapshot input{};
  input.touchReleased = true;
  input.touchX = static_cast<int16_t>(x);
  input.touchY = static_cast<int16_t>(y);
  const fui::ActionEvent action = interactions_.route(input);

  switch (action.action) {
    case geminiui::ActionAsk:
      openKeyboardForPrompt();
      return;
    case geminiui::ActionQuickPrompt: {
      const char* chips[] = {
          "Explain this in simple terms: ",
          "Summarize the main ideas of: ",
          "Key vocabulary and definitions for: ",
          "Brainstorm creative story ideas about: ",
      };
      int idx = action.value;
      if (idx >= 0 && idx < 4) {
        openKeyboardForPrompt(chips[idx]);
      } else {
        openKeyboardForPrompt();
      }
      return;
    }
    case geminiui::ActionNewChat:
      resetChat();
      return;
    case geminiui::ActionPrevPage:
      if (currentPage_ > 0) {
        currentPage_--;
        interactionsReady_ = false;
        requestUpdate();
      }
      return;
    case geminiui::ActionNextPage:
      if (currentPage_ + 1 < totalPages_) {
        currentPage_++;
        interactionsReady_ = false;
        requestUpdate();
      }
      return;
    case geminiui::ActionSaveNote:
      saveResponseToNotes();
      return;
    case geminiui::ActionSetKey:
      openKeyboardForToken();
      return;
    case geminiui::ActionConnectWifi:
      openWifiSelection();
      return;
    case geminiui::ActionRetry:
      if (!currentPrompt_.empty()) {
        askPrompt(currentPrompt_);
      }
      return;
    case geminiui::ActionDismissNotice:
      state_ = noticeReturnState_;
      interactionsReady_ = false;
      requestUpdate();
      return;
  }
}

void GeminiActivity::render(RenderLock&&) {
  renderer.clearScreen();
  fui::GfxRendererTarget target = toybox::makeTarget(renderer, toybox::readingChromeFaces());
  const fui::InputSnapshot noInput{};
  interactionsReady_ = false;
  toybox::Frame frame(target, target.deviceContext(), noInput, interactions_);
  toybox::Screen screen(frame);

  switch (state_) {
    case State::Welcome: {
      geminiui::WelcomeModel model;
      model.wifiConnected = (WiFi.status() == WL_CONNECTED);
      model.wifiSsid = model.wifiConnected ? WiFi.SSID().c_str() : "";
      model.tokenFound = tokenInfo_.isFound;
      model.tokenSource = tokenInfo_.sourcePath;
      model.maskedToken = gemini::maskToken(tokenInfo_.token);
      model.modelName = "gemini-2.0-flash";
      geminiui::drawWelcome(screen, model);
      break;
    }
    case State::Thinking:
    case State::Querying: {
      geminiui::ThinkingModel model;
      model.prompt = currentPrompt_;
      model.modelName = "gemini-2.0-flash";
      geminiui::drawThinking(screen, model);
      renderedThinking_ = true;
      break;
    }
    case State::Response: {
      geminiui::ResponseModel model;
      model.prompt = currentPrompt_;
      model.responseText = fullResponseText_;
      model.currentPage = currentPage_;
      model.totalPages = totalPages_;
      model.linesPerPage = linesPerPage_;
      model.modelName = "gemini-2.0-flash";
      model.savedToNotes = savedToNotes_;
      geminiui::drawResponse(screen, model);
      break;
    }
    case State::Error: {
      geminiui::ErrorModel model;
      model.title = errorTitle_;
      model.message = errorMessage_;
      model.showWifiBtn = errorShowWifi_;
      model.showKeyBtn = errorShowKey_;
      model.showRetryBtn = errorShowRetry_;
      geminiui::drawError(screen, model);
      break;
    }
    case State::Notice: {
      geminiui::drawNotice(screen, noticeTitle_.c_str(), noticeMessage_.c_str());
      break;
    }
  }

  interactionsReady_ = true;
  toybox::reportOverflow(interactions_, "Gemini");
}
