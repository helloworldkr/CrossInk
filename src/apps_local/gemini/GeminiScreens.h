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
};

struct WelcomeModel {
  bool wifiConnected = false;
  std::string wifiSsid;
  bool tokenFound = false;
  std::string tokenSource;
  std::string maskedToken;
  std::string modelName = "gemini-2.5-flash";
};

struct ThinkingModel {
  std::string prompt;
  std::string modelName = "gemini-2.5-flash";
};

struct ResponseModel {
  std::string prompt;
  std::string responseText;
  int currentPage = 0;
  int totalPages = 1;
  std::string modelName = "gemini-2.5-flash";
  bool savedToNotes = false;
};

struct ErrorModel {
  std::string title;
  std::string message;
  bool showWifiBtn = false;
  bool showKeyBtn = false;
  bool showRetryBtn = false;
};

// Screen renderers
void drawWelcome(toybox::Screen& screen, const WelcomeModel& model);
void drawThinking(toybox::Screen& screen, const ThinkingModel& model);
void drawResponse(toybox::Screen& screen, const ResponseModel& model);
void drawError(toybox::Screen& screen, const ErrorModel& model);
void drawNotice(toybox::Screen& screen, const char* title, const char* message);

// Calculate page splits for long response text
std::vector<std::string> paginateResponse(const fui::DeviceContext& device, const std::string& text,
                                          int availableHeight);

}  // namespace geminiui
