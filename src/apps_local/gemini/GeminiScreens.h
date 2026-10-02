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
};

struct WelcomeModel {
  bool wifiConnected = false;
  bool wifiConnecting = false;
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
  int linesPerPage = 25;
  std::string modelName = "gemini-2.5-flash";
  bool savedToNotes = false;
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
