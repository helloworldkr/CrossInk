#include "GeminiScreens.h"

#include <cstdio>
#include <string>
#include <vector>

#include "../ui/ToyboxFonts.h"
#include "../ui/ToyboxText.h"
#include "../ui/ToyboxTokens.h"

namespace geminiui {
namespace {

fui::TextStyle style(const fui::FontId font, const fui::TextAlign align = fui::TextAlign::Left,
                     const fui::Color color = fui::Color::Black, const uint8_t maxLines = 1) {
  fui::TextStyle s;
  s.font = font;
  s.align = align;
  s.color = color;
  s.maxLines = maxLines;
  return s;
}

void chrome(toybox::Screen& screen, const char* title, const char* rightLabel = nullptr) {
  fui::TextStyle titleStyle = screen.theme().titleText;
  titleStyle.font = toybox::kDisplayFontId;
  fui::HeaderProps header;
  header.title = title;
  header.titleText = titleStyle;
  header.rightLabel = rightLabel;
  header.borderEdges = fui::EdgesNone;
  if (rightLabel != nullptr) {
    header.subtitleText = screen.theme().smallText;
    header.subtitleText.font = toybox::kTileFontId;
    header.subtitleText.color = fui::Color::White;
    header.subtitleText.align = fui::TextAlign::Right;
  }
  toybox::absoluteChrome(screen);
  toybox::headerBand(screen, header);
  screen.insetContent(fui::Insets{toybox::kBodyGutter, toybox::kMargin, toybox::kMargin, toybox::kMargin});
}

void cardBox(toybox::Screen& screen, const fui::Rect& box) {
  const fui::Paint ink = fui::Paint::solid(fui::Color::Black);
  const int w = toybox::kFrame;
  screen.target().fill(fui::makeRect(box.x, box.y, box.width, w), ink);
  screen.target().fill(fui::makeRect(box.x, box.bottom() - w, box.width, w), ink);
  screen.target().fill(fui::makeRect(box.x, box.y, w, box.height), ink);
  screen.target().fill(fui::makeRect(box.right() - w, box.y, w, box.height), ink);
}

}  // namespace

void drawWelcome(toybox::Screen& screen, const WelcomeModel& model) {
  chrome(screen, "GEMINI AI", model.modelName.c_str());

  const fui::DeviceContext& device = screen.device();
  const int contentW = device.width - 2 * toybox::kMargin;

  // Status card
  const fui::Rect statusBox = screen.takeTop(150, 16);
  cardBox(screen, statusBox);

  fui::TextStyle headerStyle = style(toybox::kSerifTitleFontId, fui::TextAlign::Left);
  screen.target().text(fui::makeRect(statusBox.x + 16, statusBox.y + 14, statusBox.width - 32, 24),
                       "Google Gemini Assistant", headerStyle);

  // Wi-Fi line
  fui::TextStyle bodyStyle = style(toybox::kReadingFontId, fui::TextAlign::Left);
  std::string wifiStr = model.wifiConnected ? ("Wi-Fi: Connected (" + model.wifiSsid + ")") : "Wi-Fi: Disconnected";
  screen.target().text(fui::makeRect(statusBox.x + 16, statusBox.y + 48, statusBox.width - 32, 22), wifiStr.c_str(),
                       bodyStyle);

  // Token line
  std::string tokenStr;
  if (model.tokenFound) {
    tokenStr = "Key: " + model.tokenSource + " [" + model.maskedToken + "]";
  } else {
    tokenStr = "Key: Not found in /XTData/llm_token";
  }
  screen.target().text(fui::makeRect(statusBox.x + 16, statusBox.y + 74, statusBox.width - 32, 22), tokenStr.c_str(),
                       bodyStyle);

  // Hint line
  fui::TextStyle hintStyle = style(toybox::kTileFontId, fui::TextAlign::Left);
  std::string hintStr = model.tokenFound ? "Ready to answer questions, explain concepts & summarize."
                                         : "Place API key in /XTData/llm_token or tap Enter Key.";
  screen.target().text(fui::makeRect(statusBox.x + 16, statusBox.y + 104, statusBox.width - 32, 20), hintStr.c_str(),
                       hintStyle);

  // Quick prompt suggestions
  screen.takeTop(8);
  fui::TextStyle sectionTitle = style(toybox::kReadingBoldFontId, fui::TextAlign::Left);
  const fui::Rect secBox = screen.takeTop(26, 8);
  screen.target().text(secBox, "QUICK PROMPTS", sectionTitle);

  const char* chips[] = {
      "Explain this in simple terms",
      "Summarize the main ideas",
      "Give key vocabulary & definitions",
      "Brainstorm creative story ideas",
  };

  for (int i = 0; i < 4; ++i) {
    const fui::Rect chipBox = screen.takeTop(44, 8);
    fui::ButtonProps btn;
    btn.label = chips[i];
    btn.action = ActionQuickPrompt;
    btn.value = static_cast<int16_t>(i);
    btn.styles = toybox::rowStyles();
    screen.button(btn, chipBox);
  }

  // Bottom action buttons
  screen.takeTop(16);
  if (!model.wifiConnected) {
    const fui::Rect wifiBtnBox = screen.takeTop(52, 10);
    fui::ButtonProps btn;
    btn.label = "CONNECT TO WI-FI";
    btn.action = ActionConnectWifi;
    btn.styles = toybox::invertedStyles();
    screen.button(btn, wifiBtnBox);
  } else if (!model.tokenFound) {
    const fui::Rect keyBtnBox = screen.takeTop(52, 10);
    fui::ButtonProps btn;
    btn.label = "ENTER API KEY";
    btn.action = ActionSetKey;
    btn.styles = toybox::invertedStyles();
    screen.button(btn, keyBtnBox);
  } else {
    const fui::Rect askBtnBox = screen.takeTop(52, 10);
    fui::ButtonProps btn;
    btn.label = "ASK GEMINI...";
    btn.action = ActionAsk;
    btn.styles = toybox::invertedStyles();
    screen.button(btn, askBtnBox);
  }

  // Footer options
  const fui::Rect footerBox = screen.takeTop(44, 0);
  const int halfW = (contentW - 12) / 2;

  fui::ButtonProps keyBtn;
  keyBtn.label = model.tokenFound ? "CHANGE KEY" : "SET KEY";
  keyBtn.action = ActionSetKey;
  keyBtn.styles = toybox::rowStyles();
  screen.button(keyBtn, fui::makeRect(footerBox.x, footerBox.y, halfW, footerBox.height));

  fui::ButtonProps netBtn;
  netBtn.label = "WI-FI SETTINGS";
  netBtn.action = ActionConnectWifi;
  netBtn.styles = toybox::rowStyles();
  screen.button(netBtn, fui::makeRect(footerBox.x + halfW + 12, footerBox.y, halfW, footerBox.height));
}

void drawThinking(toybox::Screen& screen, const ThinkingModel& model) {
  chrome(screen, "GEMINI AI", "THINKING");

  // Prompt display box
  const fui::Rect promptBox = screen.takeTop(100, 24);
  cardBox(screen, promptBox);

  fui::TextStyle qTag = style(toybox::kReadingBoldFontId, fui::TextAlign::Left);
  screen.target().text(fui::makeRect(promptBox.x + 14, promptBox.y + 10, promptBox.width - 28, 22), "PROMPT:", qTag);

  fui::TextStyle promptStyle = style(toybox::kReadingFontId, fui::TextAlign::Left, fui::Color::Black, 3);
  screen.target().text(fui::makeRect(promptBox.x + 14, promptBox.y + 34, promptBox.width - 28, 56),
                       model.prompt.c_str(), promptStyle);

  // Thinking card
  screen.takeTop(20);
  const fui::Rect thinkBox = screen.takeTop(160, 20);
  cardBox(screen, thinkBox);

  fui::TextStyle thinkTitle = style(toybox::kSerifTitleFontId, fui::TextAlign::Center);
  screen.target().text(fui::makeRect(thinkBox.x + 16, thinkBox.y + 36, thinkBox.width - 32, 28),
                       "Thinking...", thinkTitle);

  fui::TextStyle thinkSub = style(toybox::kReadingFontId, fui::TextAlign::Center);
  screen.target().text(fui::makeRect(thinkBox.x + 16, thinkBox.y + 74, thinkBox.width - 32, 24),
                       "Querying Google Gemini API over Wi-Fi...", thinkSub);

  fui::TextStyle modelSub = style(toybox::kTileFontId, fui::TextAlign::Center);
  std::string modelStr = "Model: " + model.modelName;
  screen.target().text(fui::makeRect(thinkBox.x + 16, thinkBox.y + 106, thinkBox.width - 32, 20), modelStr.c_str(),
                       modelSub);
}

void drawResponse(toybox::Screen& screen, const ResponseModel& model) {
  char pageBuf[32];
  snprintf(pageBuf, sizeof(pageBuf), "Page %d/%d", model.currentPage + 1, model.totalPages);
  chrome(screen, "GEMINI", pageBuf);

  const fui::DeviceContext& device = screen.device();
  const int contentW = device.width - 2 * toybox::kMargin;

  // Prompt banner at top (compact)
  const fui::Rect promptBanner = screen.takeTop(50, 10);
  screen.target().fill(promptBanner, fui::Paint::solid(fui::Color::Black));
  fui::TextStyle pStyle = style(toybox::kReadingBoldFontId, fui::TextAlign::Left, fui::Color::White);
  std::string qText = "Q: " + model.prompt;
  screen.target().text(fui::makeRect(promptBanner.x + 10, promptBanner.y + 14, promptBanner.width - 20, 24),
                       qText.c_str(), pStyle);

  // Response text area
  const int bodyH = device.height - 210;
  fui::TextAreaProps area;
  area.text = model.responseText.c_str();
  area.style = style(toybox::kReadingFontId, fui::TextAlign::Left, fui::Color::Black, 35);
  screen.textArea(area, static_cast<int16_t>(bodyH));

  // Footer button rows
  const fui::Rect footerBox = screen.takeTop(48, 0);

  if (model.totalPages > 1) {
    const int btnW = (contentW - 16) / 3;

    fui::ButtonProps prevBtn;
    prevBtn.label = "< PREV";
    prevBtn.action = ActionPrevPage;
    prevBtn.styles = (model.currentPage > 0) ? toybox::rowStyles() : toybox::disabledButtonStyles();
    screen.button(prevBtn, fui::makeRect(footerBox.x, footerBox.y, btnW, footerBox.height));

    fui::ButtonProps askBtn;
    askBtn.label = "ASK NEXT";
    askBtn.action = ActionAsk;
    askBtn.styles = toybox::invertedStyles();
    screen.button(askBtn, fui::makeRect(footerBox.x + btnW + 8, footerBox.y, btnW, footerBox.height));

    fui::ButtonProps nextBtn;
    nextBtn.label = "NEXT >";
    nextBtn.action = ActionNextPage;
    nextBtn.styles = (model.currentPage < model.totalPages - 1) ? toybox::rowStyles() : toybox::disabledButtonStyles();
    screen.button(nextBtn, fui::makeRect(footerBox.x + (btnW + 8) * 2, footerBox.y, btnW, footerBox.height));
  } else {
    const int btnW = (contentW - 16) / 3;

    fui::ButtonProps askBtn;
    askBtn.label = "ASK AGAIN";
    askBtn.action = ActionAsk;
    askBtn.styles = toybox::invertedStyles();
    screen.button(askBtn, fui::makeRect(footerBox.x, footerBox.y, btnW, footerBox.height));

    fui::ButtonProps saveBtn;
    saveBtn.label = model.savedToNotes ? "SAVED ✓" : "SAVE NOTE";
    saveBtn.action = ActionSaveNote;
    saveBtn.styles = toybox::rowStyles();
    screen.button(saveBtn, fui::makeRect(footerBox.x + btnW + 8, footerBox.y, btnW, footerBox.height));

    fui::ButtonProps newBtn;
    newBtn.label = "NEW CHAT";
    newBtn.action = ActionNewChat;
    newBtn.styles = toybox::rowStyles();
    screen.button(newBtn, fui::makeRect(footerBox.x + (btnW + 8) * 2, footerBox.y, btnW, footerBox.height));
  }
}

void drawError(toybox::Screen& screen, const ErrorModel& model) {
  chrome(screen, "GEMINI", "ERROR");

  const fui::Rect errBox = screen.takeTop(180, 20);
  cardBox(screen, errBox);

  fui::TextStyle titleStyle = style(toybox::kSerifTitleFontId, fui::TextAlign::Left);
  screen.target().text(fui::makeRect(errBox.x + 16, errBox.y + 16, errBox.width - 32, 26), model.title.c_str(),
                       titleStyle);

  fui::TextStyle msgStyle = style(toybox::kReadingFontId, fui::TextAlign::Left, fui::Color::Black, 5);
  screen.target().text(fui::makeRect(errBox.x + 16, errBox.y + 50, errBox.width - 32, 110), model.message.c_str(),
                       msgStyle);

  screen.takeTop(20);

  if (model.showRetryBtn) {
    const fui::Rect retryBox = screen.takeTop(48, 10);
    fui::ButtonProps btn;
    btn.label = "RETRY";
    btn.action = ActionRetry;
    btn.styles = toybox::invertedStyles();
    screen.button(btn, retryBox);
  }

  if (model.showWifiBtn) {
    const fui::Rect wifiBox = screen.takeTop(48, 10);
    fui::ButtonProps btn;
    btn.label = "CONNECT TO WI-FI";
    btn.action = ActionConnectWifi;
    btn.styles = toybox::invertedStyles();
    screen.button(btn, wifiBox);
  }

  if (model.showKeyBtn) {
    const fui::Rect keyBox = screen.takeTop(48, 10);
    fui::ButtonProps btn;
    btn.label = "ENTER API KEY";
    btn.action = ActionSetKey;
    btn.styles = toybox::invertedStyles();
    screen.button(btn, keyBox);
  }

  const fui::Rect backBox = screen.takeTop(48, 0);
  fui::ButtonProps backBtn;
  backBtn.label = "DISMISS";
  backBtn.action = ActionDismissNotice;
  backBtn.styles = toybox::rowStyles();
  screen.button(backBtn, backBox);
}

void drawNotice(toybox::Screen& screen, const char* title, const char* message) {
  chrome(screen, "GEMINI", "NOTICE");

  const fui::Rect box = screen.takeTop(160, 20);
  cardBox(screen, box);

  fui::TextStyle titleStyle = style(toybox::kSerifTitleFontId, fui::TextAlign::Left);
  screen.target().text(fui::makeRect(box.x + 16, box.y + 16, box.width - 32, 26), title, titleStyle);

  fui::TextStyle msgStyle = style(toybox::kReadingFontId, fui::TextAlign::Left, fui::Color::Black, 4);
  screen.target().text(fui::makeRect(box.x + 16, box.y + 50, box.width - 32, 90), message, msgStyle);

  screen.takeTop(20);
  const fui::Rect okBox = screen.takeTop(48, 0);
  fui::ButtonProps okBtn;
  okBtn.label = "OK";
  okBtn.action = ActionDismissNotice;
  okBtn.styles = toybox::invertedStyles();
  screen.button(okBtn, okBox);
}

std::vector<std::string> paginateResponse(const fui::DeviceContext& /*device*/, const std::string& text,
                                          int /*availableHeight*/) {
  std::vector<std::string> pages;
  if (text.empty()) {
    pages.push_back("");
    return pages;
  }

  // Roughly ~1000 characters per page for comfortable reading with 14px font
  constexpr size_t kPageChars = 1000;

  size_t cursor = 0;
  while (cursor < text.size()) {
    if (text.size() - cursor <= kPageChars) {
      pages.push_back(text.substr(cursor));
      break;
    }

    // Try to find a paragraph break (\n\n) near kPageChars
    size_t splitPoint = cursor + kPageChars;
    size_t searchStart = (splitPoint > 250) ? (splitPoint - 250) : cursor;
    size_t searchEnd = std::min(text.size(), splitPoint + 150);

    size_t para = text.rfind("\n\n", searchEnd);
    if (para != std::string::npos && para >= searchStart) {
      splitPoint = para + 2;
    } else {
      // Look for a sentence boundary (. )
      size_t sentence = text.rfind(". ", searchEnd);
      if (sentence != std::string::npos && sentence >= searchStart) {
        splitPoint = sentence + 2;
      } else {
        // Look for a newline or space
        size_t space = text.rfind(' ', searchEnd);
        if (space != std::string::npos && space >= searchStart) {
          splitPoint = space + 1;
        }
      }
    }

    if (splitPoint <= cursor) splitPoint = cursor + kPageChars;
    pages.push_back(text.substr(cursor, splitPoint - cursor));
    cursor = splitPoint;
  }

  return pages;
}

}  // namespace geminiui
