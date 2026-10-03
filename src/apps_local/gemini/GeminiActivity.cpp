#include "GeminiActivity.h"

#include <HalClock.h>
#include <HalStorage.h>
#include <Logging.h>
#include <Memory.h>

#include <algorithm>
#include <ctime>

#include "../../CrossPointSettings.h"
#include "../../WifiCredentialStore.h"
#include "../../activities/ActivityManager.h"
#include "../../activities/network/WifiSelectionActivity.h"
#include "../../activities/util/KeyboardEntryActivity.h"
#include "../../activities/util/OptionSelectionActivity.h"
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

void logGemini(const std::string& msg) {
  LOG_INF("GEMINI", "%s", msg.c_str());
  Storage.ensureDirectoryExists("/XTData");
  auto file = Storage.open("/XTData/gemini.log", O_WRITE | O_CREAT | O_APPEND);
  if (file) {
    std::string line = msg + "\n";
    file.write(line.c_str(), line.length());
    file.close();
  }
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
  modelName_ = gemini::loadModel();
  if (!tokenInfo_.model.empty()) {
    modelName_ = tokenInfo_.model;
  }
  state_ = State::Welcome;
  renderedThinking_ = false;
  interactionsReady_ = false;

  std::string info = "App entered. Model: " + modelName_ + " Token: " + (tokenInfo_.isFound ? ("Found in " + tokenInfo_.sourcePath) : "Not found in /XTData/llm_token");
  logGemini(info);

  tryAutoConnectWifi();

  requestUpdate();
}

void GeminiActivity::onExit() {
  Activity::onExit();
  if (autoConnectingWifi_ && WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect();
  }
  autoConnectingWifi_ = false;
  renderedThinking_ = false;
}

void GeminiActivity::tryAutoConnectWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    autoConnectingWifi_ = false;
    return;
  }

  const size_t count = WIFI_STORE.getCredentialCount();
  if (count == 0) {
    LOG_DBG("GEMINI", "No saved Wi-Fi networks in store for auto-connect");
    return;
  }

  std::string targetSsid = WIFI_STORE.getLastConnectedSsid();
  std::optional<WifiCredential> targetCred;
  if (!targetSsid.empty()) {
    targetCred = WIFI_STORE.findCredential(targetSsid);
  }
  if (!targetCred.has_value()) {
    targetCred = WIFI_STORE.getCredentialAt(0);
    if (targetCred.has_value()) {
      targetSsid = targetCred->ssid;
    }
  }

  if (!targetCred.has_value() || targetSsid.empty()) {
    return;
  }

  LOG_INF("GEMINI", "Auto-connecting to saved Wi-Fi: %s", targetSsid.c_str());
  logGemini("Auto-connecting to saved Wi-Fi: " + targetSsid);
  autoConnectingWifi_ = true;
  wifiConnectStartTime_ = millis();
  autoConnectSsid_ = targetSsid;

  WiFi.persistent(false);
  if (!WiFi.mode(WIFI_STA)) {
    LOG_ERR("GEMINI", "Failed to set WIFI_STA mode for auto-connect");
    autoConnectingWifi_ = false;
    return;
  }

  WiFi.disconnect(false, false, 500);

  WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
  WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);

  String mac = WiFi.macAddress();
  mac.replace(":", "");
  String hostname = "CrossPoint-Reader-" + mac;
  WiFi.setHostname(hostname.c_str());

  if (!targetCred->password.empty()) {
    WiFi.begin(targetCred->ssid.c_str(), targetCred->password.c_str());
  } else {
    WiFi.begin(targetCred->ssid.c_str());
  }
}

void GeminiActivity::checkWifiAutoConnect() {
  if (!autoConnectingWifi_) return;

  const wl_status_t status = WiFi.status();
  const unsigned long now = millis();

  if (status == WL_CONNECTED) {
    IPAddress ip = WiFi.localIP();
    char ipStr[32];
    snprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    LOG_INF("GEMINI", "Auto-connected to Wi-Fi: %s (IP: %s, RSSI: %d)", autoConnectSsid_.c_str(), ipStr, WiFi.RSSI());
    logGemini("Auto-connected to Wi-Fi: " + autoConnectSsid_ + " (IP: " + ipStr + ")");
    autoConnectingWifi_ = false;
    WIFI_STORE.setLastConnectedSsid(autoConnectSsid_);

    if (halClock.isAvailable() && (!SETTINGS.clockHasBeenSynced || !SETTINGS.clockDateHasBeenSynced)) {
      if (halClock.syncFromNTP()) {
        SETTINGS.clockHasBeenSynced = 1;
        SETTINGS.clockDateHasBeenSynced = 1;
        SETTINGS.saveToFile();
      }
    }

    if (state_ == State::Notice && noticeTitle_ == "Connecting to Wi-Fi") {
      state_ = State::Welcome;
    } else if (state_ == State::Error && errorShowWifi_) {
      state_ = State::Welcome;
    }

    interactionsReady_ = false;
    requestUpdate();
    return;
  }

  // Timeout or failure check (15 seconds)
  if (status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL || (now - wifiConnectStartTime_ > 15000)) {
    LOG_INF("GEMINI", "Auto-connect to Wi-Fi (%s) failed or timed out (status=%d)", autoConnectSsid_.c_str(),
            static_cast<int>(status));
    logGemini("Auto-connect to Wi-Fi failed or timed out for: " + autoConnectSsid_);
    autoConnectingWifi_ = false;
    interactionsReady_ = false;
    requestUpdate();
    return;
  }
}

void GeminiActivity::resetChat() {
  history_.clear();
  currentPrompt_.clear();
  draftPrompt_.clear();
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

  errorShowWifi_ = false;
  errorShowKey_ = false;
  errorShowRetry_ = false;
  errorShowModel_ = false;

  if (WiFi.status() != WL_CONNECTED) {
    if (autoConnectingWifi_) {
      noticeTitle_ = "Connecting to Wi-Fi";
      noticeMessage_ = "Connecting to " + autoConnectSsid_ + "...\nPlease wait a few seconds for Wi-Fi to establish.";
      noticeReturnState_ = State::Welcome;
      state_ = State::Notice;
      interactionsReady_ = false;
      requestUpdate();
      return;
    }
    errorTitle_ = "Wi-Fi Not Connected";
    errorMessage_ = "Google Gemini requires an active internet connection. Please connect to Wi-Fi.";
    errorShowWifi_ = true;
    errorShowKey_ = false;
    errorShowRetry_ = false;
    state_ = State::Error;
    interactionsReady_ = false;
    logGemini("Error: Wi-Fi not connected when asking: " + prompt);
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
    logGemini("Error: API Key missing when asking: " + prompt);
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
      draftPrompt_ = entered.text;
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

void GeminiActivity::openModelSelection() {
  std::vector<std::string> options = {
      "gemini-2.5-flash (Default)",
      "gemini-2.5-flash-lite",
      "gemini-2.5-pro",
      "gemini-2.0-flash",
      "Custom Model...",
  };
  uint8_t selected = 0;
  if (modelName_ == "gemini-2.5-flash-lite") selected = 1;
  else if (modelName_ == "gemini-2.5-pro") selected = 2;
  else if (modelName_ == "gemini-2.0-flash") selected = 3;

  auto activity = makeUniqueNoThrow<OptionSelectionActivity>(
      renderer, mappedInput, "GeminiModelSelect", StrId::STR_SELECT, options, selected, false, true);
  if (!activity) return;

  startActivityForResult(std::move(activity), [this](const ActivityResult& result) {
    interactionsReady_ = false;
    if (result.isCancelled) {
      requestUpdate();
      return;
    }
    const auto& sel = std::get<OptionSelectionResult>(result.data);
    if (sel.index == 0) {
      modelName_ = "gemini-2.5-flash";
      gemini::saveModel(modelName_);
      noticeTitle_ = "Model Selected";
      noticeMessage_ = "Active model updated to:\ngemini-2.5-flash";
      noticeReturnState_ = State::Welcome;
      state_ = State::Notice;
      requestUpdate();
    } else if (sel.index == 1) {
      modelName_ = "gemini-2.5-flash-lite";
      gemini::saveModel(modelName_);
      noticeTitle_ = "Model Selected";
      noticeMessage_ = "Active model updated to:\ngemini-2.5-flash-lite";
      noticeReturnState_ = State::Welcome;
      state_ = State::Notice;
      requestUpdate();
    } else if (sel.index == 2) {
      modelName_ = "gemini-2.5-pro";
      gemini::saveModel(modelName_);
      noticeTitle_ = "Model Selected";
      noticeMessage_ = "Active model updated to:\ngemini-2.5-pro";
      noticeReturnState_ = State::Welcome;
      state_ = State::Notice;
      requestUpdate();
    } else if (sel.index == 3) {
      modelName_ = "gemini-2.0-flash";
      gemini::saveModel(modelName_);
      noticeTitle_ = "Model Selected";
      noticeMessage_ = "Active model updated to:\ngemini-2.0-flash";
      noticeReturnState_ = State::Welcome;
      state_ = State::Notice;
      requestUpdate();
    } else if (sel.index == 4) {
      auto keyboard = makeUniqueNoThrow<KeyboardEntryActivity>(renderer, mappedInput, "GEMINI MODEL", modelName_, 64);
      if (!keyboard) return;
      startActivityForResult(std::move(keyboard), [this](const ActivityResult& kResult) {
        interactionsReady_ = false;
        if (kResult.isCancelled) {
          requestUpdate();
          return;
        }
        const auto& entered = std::get<KeyboardResult>(kResult.data);
        if (!entered.text.empty()) {
          modelName_ = entered.text;
          gemini::saveModel(modelName_);
          noticeTitle_ = "Model Saved";
          noticeMessage_ = "Active model set to:\n" + modelName_;
          noticeReturnState_ = State::Welcome;
          state_ = State::Notice;
          requestUpdate();
        }
      });
    }
  });
}

void GeminiActivity::openWifiSelection() {
  autoConnectingWifi_ = false;
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
  checkWifiAutoConnect();

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
    if (state_ == State::Welcome) {
      if (!draftPrompt_.empty()) {
        askPrompt(draftPrompt_);
      } else {
        openKeyboardForPrompt("");
      }
      return;
    } else if (state_ == State::Response) {
      openKeyboardForPrompt("");
      return;
    }
  }

  // Once the Thinking screen has visibly rendered, perform the API query
  if (state_ == State::Thinking && renderedThinking_) {
    state_ = State::Querying;
    LOG_INF("GEMINI", "Executing query for prompt: %s (model: %s)", currentPrompt_.c_str(), modelName_.c_str());

    gemini::Response res = client_.query(currentPrompt_, history_, tokenInfo_.token, modelName_);

    if (res.success) {
      if (!res.usedModel.empty() && res.usedModel != modelName_) {
        LOG_INF("GEMINI", "Auto-switched active model to: %s", res.usedModel.c_str());
        modelName_ = res.usedModel;
        gemini::saveModel(modelName_);
      }
      logGemini("Query succeeded for: " + currentPrompt_ + " (" + std::to_string(res.text.size()) + " chars) via " + modelName_);
      fullResponseText_ = res.text;
      history_.push_back({"user", currentPrompt_});
      history_.push_back({"model", res.text});

      fui::GfxRendererTarget target = toybox::makeTarget(renderer, toybox::readingChromeFaces());
      int contentW = target.deviceContext().width - 2 * toybox::kMargin;
      int totalLines = geminiui::calculateTotalLines(target, static_cast<int16_t>(contentW), res.text);
      linesPerPage_ = geminiui::responseLinesPerPage(target, target.deviceContext());
      totalPages_ = geminiui::calculateTotalPages(totalLines, linesPerPage_);
      currentPage_ = 0;
      savedToNotes_ = false;
      state_ = State::Response;
    } else {
      if (res.httpCode == 404 || res.error.find("not found") != std::string::npos || res.error.find("models/") != std::string::npos) {
        errorTitle_ = "Model Not Found (404)";
        errorMessage_ = "Model '" + modelName_ + "' is not supported or not found. Tap CHANGE MODEL to pick an active model (e.g. gemini-2.5-flash or gemini-2.5-flash-lite).";
        errorShowModel_ = true;
      } else {
        errorTitle_ = "Gemini API Error";
        errorMessage_ = res.error;
        errorShowModel_ = false;
      }
      errorShowRetry_ = true;
      errorShowWifi_ = (WiFi.status() != WL_CONNECTED);
      errorShowKey_ = !tokenInfo_.isFound;
      state_ = State::Error;
      logGemini("Query error for '" + currentPrompt_ + "': " + res.error);
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
    case geminiui::ActionSendPrompt:
      if (!draftPrompt_.empty()) {
        askPrompt(draftPrompt_);
      } else {
        openKeyboardForPrompt("");
      }
      return;
    case geminiui::ActionEditPrompt:
      openKeyboardForPrompt(draftPrompt_);
      return;
    case geminiui::ActionClearPrompt:
      draftPrompt_.clear();
      interactionsReady_ = false;
      requestUpdate();
      return;
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
    case geminiui::ActionSelectModel:
      openModelSelection();
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
      model.wifiConnecting = autoConnectingWifi_;
      model.wifiSsid = model.wifiConnected ? WiFi.SSID().c_str() : (autoConnectingWifi_ ? autoConnectSsid_ : "");
      model.tokenFound = tokenInfo_.isFound;
      model.tokenSource = tokenInfo_.sourcePath;
      model.maskedToken = gemini::maskToken(tokenInfo_.token);
      model.modelName = modelName_;
      model.draftPrompt = draftPrompt_;
      geminiui::drawWelcome(screen, model);
      break;
    }
    case State::Thinking:
    case State::Querying: {
      geminiui::ThinkingModel model;
      model.prompt = currentPrompt_;
      model.modelName = modelName_;
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
      model.modelName = modelName_;
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
      model.showModelBtn = errorShowModel_;
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
  renderer.displayBuffer();
}
