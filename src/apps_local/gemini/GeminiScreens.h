#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../ui/ToyboxScreen.h"

namespace geminiui {

namespace fui = freeink::ui;

enum : fui::ActionId {
  ActionAsk = 410,
  ActionNewChat = 411,
  ActionPrevPage = 412,
  ActionNextPage = 413,
  ActionSaveNote = 414,
  ActionSetKey = 415,
  ActionConnectWifi = 416,
  ActionRetry = 417,
  ActionQuickPrompt = 418,
  ActionDismissNotice = 419,
  ActionSelectModel = 420,
  ActionEditPrompt = 421,
  ActionSendPrompt = 422,
  ActionClearPrompt = 423,
  ActionOpenSettings = 424,
  ActionBackToPrompt = 425,
  ActionResumeChat = 426,
  ActionKeyChar = 450,
  ActionKeyShift = 451,
  ActionKeyDelete = 452,
  ActionKeyMode = 453,
  ActionKeySpace = 454,
  ActionQuickPrompts = 455,
  ActionSavedChats = 456,
};

struct WelcomeModel {
  bool wifiConnected = false;
  bool wifiConnecting = false;
  std::string wifiSsid;
  bool tokenFound = false;
  std::string tokenSource;
  std::string maskedToken;
  std::string modelName = "gemini-2.5-flash";
  std::string draftPrompt;
  bool shifted = false;
  bool symbols = false;
  bool hasActiveChat = false;
  int activeChatTurns = 0;
};

struct ThinkingModel {
  std::string prompt;
  std::string modelName = "gemini-2.5-flash";
};

struct ResponseModel {
  std::string prompt;
  std::string responseText;
  std::string conversationText;
  int currentPage = 0;
  int totalPages = 1;
  int linesPerPage = 25;
  std::string modelName = "gemini-2.5-flash";
  bool savedToNotes = false;
  int turnNumber = 1;
  int totalTurns = 1;
};

struct ErrorModel {
  std::string title;
  std::string message;
  bool showWifiBtn = false;
  bool showKeyBtn = false;
  bool showRetryBtn = false;
  bool showModelBtn = false;
};

// Screen renderers
void drawWelcome(toybox::Screen& screen, const WelcomeModel& model);
void drawThinking(toybox::Screen& screen, const ThinkingModel& model);
void drawResponse(toybox::Screen& screen, const ResponseModel& model);
void drawError(toybox::Screen& screen, const ErrorModel& model);
void drawNotice(toybox::Screen& screen, const char* title, const char* message);

// Text layout & pagination metrics
int responseTextHeight(const fui::DeviceContext& device);
int responseLinesPerPage(const fui::DrawTarget& target, const fui::DeviceContext& device);
int calculateTotalLines(const fui::DrawTarget& target, int16_t width, const std::string& text);
int calculateTotalPages(int totalLines, int linesPerPage);

}  // namespace geminiui
