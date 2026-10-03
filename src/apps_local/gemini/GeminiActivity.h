#pragma once

#include <WiFi.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../../activities/Activity.h"
#include "../ui/ToyboxScreen.h"
#include "GeminiClient.h"
#include "GeminiScreens.h"
#include "GeminiToken.h"

class GeminiActivity final : public Activity {
 public:
  GeminiActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  ~GeminiActivity() override = default;

  static std::unique_ptr<Activity> create(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

  bool preventAutoSleep() override { return autoConnectingWifi_ || state_ == State::Thinking || state_ == State::Querying; }
  bool skipLoopDelay() override { return autoConnectingWifi_ || state_ == State::Thinking || state_ == State::Querying; }

 private:
  void tryAutoConnectWifi();
  void checkWifiAutoConnect();
  enum class State {
    Welcome,
    Thinking,
    Querying,
    Response,
    Error,
    Notice,
  };

  void askPrompt(const std::string& prompt);
  void openKeyboardForPrompt(const std::string& prefill = "", bool autoSend = false);
  void openKeyboardForToken();
  void openModelSelection();
  void openWifiSelection();
  void openSettingsMenu();
  void openQuickPromptsSelection();
  void openSaveMenu();
  void openFolderSelectionForSave();
  void openCreateFolderForSave();
  void saveConversationToFolder(const std::string& folder);
  void openSavedChatsSelection();
  bool loadConversationFromFile(const std::string& path);
  void openReplyPrompt();
  void saveResponseToNotes();
  void resetChat();

  State state_ = State::Welcome;
  gemini::TokenInfo tokenInfo_;
  gemini::Client client_;
  std::string modelName_ = "gemini-2.5-flash";

  std::vector<gemini::Message> history_;
  std::string currentPrompt_;
  std::string draftPrompt_;
  std::string lastSavedFilePath_;
  bool shifted_ = false;
  bool symbols_ = false;
  std::string fullResponseText_;
  std::string conversationTranscript_;
  int currentPage_ = 0;
  int totalPages_ = 1;
  int linesPerPage_ = 25;
  bool savedToNotes_ = false;

  bool renderedThinking_ = false;

  std::string errorTitle_;
  std::string errorMessage_;
  bool errorShowWifi_ = false;
  bool errorShowKey_ = false;
  bool errorShowRetry_ = false;
  bool errorShowModel_ = false;

  std::string noticeTitle_;
  std::string noticeMessage_;
  State noticeReturnState_ = State::Welcome;

  toybox::Interactions interactions_;
  bool interactionsReady_ = false;

  bool autoConnectingWifi_ = false;
  unsigned long wifiConnectStartTime_ = 0;
  std::string autoConnectSsid_;
};
