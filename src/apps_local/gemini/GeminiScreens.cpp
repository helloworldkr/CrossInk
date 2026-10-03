#include "GeminiScreens.h"

#include <cstdio>
#include <string>
#include <vector>

#include "../ui/ToyboxFonts.h"
#include "../ui/ToyboxText.h"
#include "../ui/ToyboxTokens.h"

namespace geminiui {
namespace {

constexpr int kBodyTop = toybox::kBodyTop;
constexpr int kFooterHeight = toybox::kPillHeight;

fui::TextStyle style(const fui::FontId font, const fui::TextAlign align = fui::TextAlign::Left,
                     const fui::Color color = fui::Color::Black, const uint8_t maxLines = 1) {
  fui::TextStyle s;
  s.font = font;
  s.align = align;
  s.color = color;
  s.maxLines = maxLines;
  return s;
}

fui::Rect footerBand(const fui::DeviceContext& device) {
  return fui::makeRect(toybox::kMargin, static_cast<int16_t>(device.height - toybox::kMargin - kFooterHeight),
                       static_cast<int16_t>(device.width - 2 * toybox::kMargin), kFooterHeight);
}

fui::Rect contentBand(const fui::DeviceContext& device) {
  const fui::Rect footer = footerBand(device);
  return fui::makeRect(toybox::kMargin, kBodyTop, footer.width,
                       static_cast<int16_t>(footer.y - toybox::kGutter - kBodyTop));
}

void chrome(toybox::Screen& screen, const char* title, const char* rightLabel = nullptr) {
  fui::TextStyle titleStyle = screen.theme().titleText;
  titleStyle.font = toybox::kDisplayFont;
  fui::HeaderProps header;
  header.title = title;
  header.titleText = titleStyle;
  header.rightLabel = rightLabel;
  header.borderEdges = fui::EdgesNone;
  if (rightLabel != nullptr) {
    header.subtitleText = screen.theme().smallText;
    header.subtitleText.font = toybox::kTileFont;
    header.subtitleText.color = fui::Color::White;
    header.subtitleText.align = fui::TextAlign::Right;
  }
  toybox::absoluteChrome(screen);
  toybox::headerBand(screen, header);
  screen.insetContent(fui::Insets{toybox::kBodyGutter, toybox::kMargin, toybox::kMargin, toybox::kMargin});
}

void cardBox(toybox::Screen& screen, const fui::Rect& box) {
  screen.target().stroke(box, fui::Paint::solid(fui::Color::Black), 2, 4);
}

}  // namespace

int responseTextHeight(const fui::DeviceContext& device) {
  const int footerY = device.height - toybox::kMargin - kFooterHeight;
  const int textY = kBodyTop + 36 + 10;
  const int textH = footerY - 10 - textY;
  return textH > 0 ? textH : 200;
}

int responseLinesPerPage(const fui::DrawTarget& target, const fui::DeviceContext& device) {
  const int textH = responseTextHeight(device);
  const int16_t lh = target.lineHeight(toybox::kBodyFont);
  if (lh <= 0) return 10;
  const int lines = textH / lh;
  return lines > 0 ? lines : 10;
}

int calculateTotalLines(const fui::DrawTarget& target, int16_t width, const std::string& text) {
  if (text.empty() || width <= 0) return 0;
  fui::TextStyle st = style(toybox::kBodyFont, fui::TextAlign::Left);
  fui::TextAreaMetrics m = fui::textAreaMeasure(target, width, text.c_str(), st, 0);
  return static_cast<int>(m.lineCount);
}

int calculateTotalPages(int totalLines, int linesPerPage) {
  if (linesPerPage <= 0) return 1;
  int pages = (totalLines + linesPerPage - 1) / linesPerPage;
  return pages > 0 ? pages : 1;
}

void drawWelcome(toybox::Screen& screen, const WelcomeModel& model) {
  chrome(screen, "GEMINI AI", model.modelName.c_str());

  const fui::DeviceContext& device = screen.device();
  const fui::Rect band = contentBand(device);
  const fui::Rect footer = footerBand(device);

  // Two-column split on landscape display:
  // Left: Assistant Status Card (compact, small headers and readable Wi-Fi)
  // Right: Editable Prompt Box with Send Button
  constexpr int leftW = 300;
  constexpr int gap = 14;
  const int rightW = band.width - leftW - gap;

  // --- Left Column: Status Card ---
  const fui::Rect leftCard = fui::makeRect(band.x, band.y, leftW, band.height);
  cardBox(screen, leftCard);

  const int innerX = leftCard.x + 14;
  const int innerW = leftCard.width - 28;
  int curY = leftCard.y + 14;

  // 1. Assistant status and Google Gemini (small)
  screen.target().text(fui::makeRect(innerX, curY, innerW, 16), "ASSISTANT STATUS",
                       style(toybox::kSmallFont, fui::TextAlign::Left));
  curY += 18;

  screen.target().text(fui::makeRect(innerX, curY, innerW, 16), "Google Gemini",
                       style(toybox::kSmallFont, fui::TextAlign::Left));
  curY += 22;

  // Divider
  screen.target().fill(fui::makeRect(innerX, curY, innerW, 1), fui::Paint::solid(fui::Color::Black));
  curY += 10;

  // 2. Wi-Fi (small yet readable)
  std::string wifiStr;
  if (model.wifiConnected) {
    wifiStr = "Wi-Fi: " + (model.wifiSsid.empty() ? std::string("Connected") : model.wifiSsid);
  } else if (model.wifiConnecting) {
    wifiStr = "Wi-Fi: Connecting...";
  } else {
    wifiStr = "Wi-Fi: Disconnected";
  }
  screen.target().text(fui::makeRect(innerX, curY, innerW, 18), wifiStr.c_str(),
                       style(toybox::kSmallFont, fui::TextAlign::Left));
  curY += 22;

  // Key line
  std::string keyStr = model.tokenFound ? ("Key: Ready [" + model.maskedToken + "]") : "Key: Missing in /XTData/llm_token";
  screen.target().text(fui::makeRect(innerX, curY, innerW, 18), keyStr.c_str(),
                       style(toybox::kSmallFont, fui::TextAlign::Left));
  curY += 22;

  // Engine line
  std::string engineStr = "Model: " + model.modelName;
  screen.target().text(fui::makeRect(innerX, curY, innerW, 18), engineStr.c_str(),
                       style(toybox::kSmallFont, fui::TextAlign::Left));
  curY += 24;

  // Divider
  screen.target().fill(fui::makeRect(innerX, curY, innerW, 1), fui::Paint::solid(fui::Color::Black));
  curY += 12;

  // Guidance tip
  const char* guide = "Tap the prompt box on the right to write or edit your question, then tap SEND.";
  screen.target().text(fui::makeRect(innerX, curY, innerW, 64), guide,
                       style(toybox::kSmallFont, fui::TextAlign::Left, fui::Color::Black, 3));

  // --- Right Column: Editable Prompt Box & Send Button ---
  const int rightX = band.x + leftW + gap;
  const fui::Rect rightCard = fui::makeRect(rightX, band.y, rightW, band.height);
  cardBox(screen, rightCard);

  const int rInnerX = rightCard.x + 14;
  const int rInnerW = rightCard.width - 28;
  int rCurY = rightCard.y + 14;

  screen.target().text(fui::makeRect(rInnerX, rCurY, rInnerW, 18), "PROMPT",
                       style(toybox::kSmallFont, fui::TextAlign::Left));
  rCurY += 22;

  const int sendBtnH = 46;
  const int sendBtnY = rightCard.y + rightCard.height - 14 - sendBtnH;
  const int promptBoxY = rCurY;
  const int promptBoxH = sendBtnY - 10 - promptBoxY;
  const fui::Rect promptBox = fui::makeRect(rInnerX, promptBoxY, rInnerW, promptBoxH);

  // 3 & 4. Editable prompt box (tapping opens keyboard; quick buttons removed)
  fui::ButtonProps promptBtn;
  promptBtn.label = "";
  promptBtn.action = ActionEditPrompt;
  promptBtn.styles = toybox::rowStyles();
  screen.button(promptBtn, promptBox);

  if (model.draftPrompt.empty()) {
    screen.target().text(
        fui::makeRect(promptBox.x + 14, promptBox.y + 14, promptBox.width - 28, 20),
        "Tap here to write your prompt...",
        style(toybox::kSmallFont, fui::TextAlign::Left));

    screen.target().text(
        fui::makeRect(promptBox.x + 14, promptBox.y + 40, promptBox.width - 28, promptBox.height - 76),
        "Ask a question, explain a book passage, summarize topics, or brainstorm creative ideas.",
        style(toybox::kSmallFont, fui::TextAlign::Left, fui::Color::Black, 5));

    screen.target().text(
        fui::makeRect(promptBox.x + 14, promptBox.y + promptBox.height - 26, promptBox.width - 28, 18),
        "[ Tap anywhere to open keyboard ]",
        style(toybox::kSmallFont, fui::TextAlign::Right));
  } else {
    screen.target().text(
        fui::makeRect(promptBox.x + 14, promptBox.y + 14, promptBox.width - 28, promptBox.height - 44),
        model.draftPrompt.c_str(),
        style(toybox::kSmallFont, fui::TextAlign::Left, fui::Color::Black, 9));

    screen.target().text(
        fui::makeRect(promptBox.x + 14, promptBox.y + promptBox.height - 24, promptBox.width - 28, 16),
        "[ Tap to edit prompt ]",
        style(toybox::kSmallFont, fui::TextAlign::Right));
  }

  // 3. Send button (and Clear button if text entered)
  if (model.draftPrompt.empty()) {
    fui::ButtonProps sendBtn;
    sendBtn.label = "WRITE & SEND PROMPT  \u2794";
    sendBtn.action = ActionSendPrompt;
    sendBtn.styles = toybox::invertedStyles();
    screen.button(sendBtn, fui::makeRect(rInnerX, sendBtnY, rInnerW, sendBtnH));
  } else {
    constexpr int clearBtnW = 90;
    constexpr int btnGap = 10;
    const int sendBtnW = rInnerW - clearBtnW - btnGap;

    fui::ButtonProps clearBtn;
    clearBtn.label = "CLEAR";
    clearBtn.action = ActionClearPrompt;
    clearBtn.styles = toybox::rowStyles();
    screen.button(clearBtn, fui::makeRect(rInnerX, sendBtnY, clearBtnW, sendBtnH));

    fui::ButtonProps sendBtn;
    sendBtn.label = "SEND TO GEMINI  \u2794";
    sendBtn.action = ActionSendPrompt;
    sendBtn.styles = toybox::invertedStyles();
    screen.button(sendBtn, fui::makeRect(rInnerX + clearBtnW + btnGap, sendBtnY, sendBtnW, sendBtnH));
  }

  // --- Bottom Footer Band ---
  constexpr int secW = 116;
  constexpr int gapW = 10;
  const int primW = footer.width - (secW * 3 + gapW * 3);

  if (!model.wifiConnected) {
    fui::ButtonProps wifiBtn;
    wifiBtn.label = model.wifiConnecting ? "CONNECTING..." : "CONNECT TO WI-FI";
    wifiBtn.action = ActionConnectWifi;
    wifiBtn.styles = toybox::invertedStyles();
    screen.button(wifiBtn, fui::makeRect(footer.x, footer.y, primW, footer.height));

    fui::ButtonProps modelBtn;
    modelBtn.label = "MODEL";
    modelBtn.action = ActionSelectModel;
    modelBtn.styles = toybox::rowStyles();
    screen.button(modelBtn, fui::makeRect(footer.x + primW + gapW, footer.y, secW, footer.height));

    fui::ButtonProps keyBtn;
    keyBtn.label = "SET KEY";
    keyBtn.action = ActionSetKey;
    keyBtn.styles = toybox::rowStyles();
    screen.button(keyBtn, fui::makeRect(footer.x + primW + gapW + secW + gapW, footer.y, secW, footer.height));

    fui::ButtonProps askBtn;
    askBtn.label = "SEND";
    askBtn.action = ActionSendPrompt;
    askBtn.styles = toybox::rowStyles();
    screen.button(askBtn, fui::makeRect(footer.x + primW + (gapW + secW) * 2 + gapW, footer.y, secW, footer.height));
  } else if (!model.tokenFound) {
    fui::ButtonProps keyBtn;
    keyBtn.label = "ENTER API KEY";
    keyBtn.action = ActionSetKey;
    keyBtn.styles = toybox::invertedStyles();
    screen.button(keyBtn, fui::makeRect(footer.x, footer.y, primW, footer.height));

    fui::ButtonProps modelBtn;
    modelBtn.label = "MODEL";
    modelBtn.action = ActionSelectModel;
    modelBtn.styles = toybox::rowStyles();
    screen.button(modelBtn, fui::makeRect(footer.x + primW + gapW, footer.y, secW, footer.height));

    fui::ButtonProps wifiBtn;
    wifiBtn.label = "WI-FI";
    wifiBtn.action = ActionConnectWifi;
    wifiBtn.styles = toybox::rowStyles();
    screen.button(wifiBtn, fui::makeRect(footer.x + primW + gapW + secW + gapW, footer.y, secW, footer.height));

    fui::ButtonProps askBtn;
    askBtn.label = "SEND";
    askBtn.action = ActionSendPrompt;
    askBtn.styles = toybox::rowStyles();
    screen.button(askBtn, fui::makeRect(footer.x + primW + (gapW + secW) * 2 + gapW, footer.y, secW, footer.height));
  } else {
    fui::ButtonProps askBtn;
    askBtn.label = model.draftPrompt.empty() ? "WRITE & SEND PROMPT (OK)" : "SEND TO GEMINI (OK)";
    askBtn.action = ActionSendPrompt;
    askBtn.styles = toybox::invertedStyles();
    screen.button(askBtn, fui::makeRect(footer.x, footer.y, primW, footer.height));

    fui::ButtonProps modelBtn;
    modelBtn.label = "MODEL";
    modelBtn.action = ActionSelectModel;
    modelBtn.styles = toybox::rowStyles();
    screen.button(modelBtn, fui::makeRect(footer.x + primW + gapW, footer.y, secW, footer.height));

    fui::ButtonProps keyBtn;
    keyBtn.label = "KEY";
    keyBtn.action = ActionSetKey;
    keyBtn.styles = toybox::rowStyles();
    screen.button(keyBtn, fui::makeRect(footer.x + primW + gapW + secW + gapW, footer.y, secW, footer.height));

    fui::ButtonProps wifiBtn;
    wifiBtn.label = "WI-FI";
    wifiBtn.action = ActionConnectWifi;
    wifiBtn.styles = toybox::rowStyles();
    screen.button(wifiBtn, fui::makeRect(footer.x + primW + (gapW + secW) * 2 + gapW, footer.y, secW, footer.height));
  }
}

void drawThinking(toybox::Screen& screen, const ThinkingModel& model) {
  chrome(screen, "GEMINI AI", "THINKING");

  const fui::DeviceContext& device = screen.device();
  const fui::Rect band = contentBand(device);
  const fui::Rect footer = footerBand(device);

  // Top Card: Prompt recap
  const fui::Rect promptBox = fui::makeRect(band.x, band.y, band.width, 96);
  cardBox(screen, promptBox);

  screen.target().text(fui::makeRect(promptBox.x + 16, promptBox.y + 12, promptBox.width - 32, 18), "QUESTION",
                       style(toybox::kSmallFont, fui::TextAlign::Left));

  std::string pFitted = toybox::fitLines(screen.target(), model.prompt.c_str(), promptBox.width - 32, 2,
                                         style(toybox::kBodyFont, fui::TextAlign::Left));
  screen.target().text(fui::makeRect(promptBox.x + 16, promptBox.y + 36, promptBox.width - 32, 50),
                       pFitted.c_str(), style(toybox::kBodyFont, fui::TextAlign::Left, fui::Color::Black, 2));

  // Center Card: Thinking Status
  const int thinkY = band.y + 110;
  const fui::Rect thinkBox = fui::makeRect(band.x, thinkY, band.width, band.height - 110);
  cardBox(screen, thinkBox);

  screen.target().text(fui::makeRect(thinkBox.x + 16, thinkBox.y + 36, thinkBox.width - 32, 30), "Thinking...",
                       style(toybox::kBodyFont, fui::TextAlign::Center));

  screen.target().text(fui::makeRect(thinkBox.x + 16, thinkBox.y + 76, thinkBox.width - 32, 22),
                       "Querying Google Gemini API over Wi-Fi...", style(toybox::kSmallFont, fui::TextAlign::Center));

  std::string modelStr = "Model: " + model.modelName;
  screen.target().text(fui::makeRect(thinkBox.x + 16, thinkBox.y + 104, thinkBox.width - 32, 20), modelStr.c_str(),
                       style(toybox::kSmallFont, fui::TextAlign::Center));

  // Footer: Cancel button
  fui::ButtonProps cancelBtn;
  cancelBtn.label = "CANCEL (OR PRESS BACK)";
  cancelBtn.action = ActionDismissNotice;
  cancelBtn.styles = toybox::rowStyles();
  screen.button(cancelBtn, footer);
}

void drawResponse(toybox::Screen& screen, const ResponseModel& model) {
  char pageBuf[32];
  if (model.totalPages > 1) {
    snprintf(pageBuf, sizeof(pageBuf), "Page %d of %d", model.currentPage + 1, model.totalPages);
  } else {
    snprintf(pageBuf, sizeof(pageBuf), "%s", model.modelName.c_str());
  }
  chrome(screen, "GEMINI", pageBuf);

  const fui::DeviceContext& device = screen.device();
  const fui::Rect band = contentBand(device);
  const fui::Rect footer = footerBand(device);

  // Top Question Banner (compact inverted black bar)
  constexpr int bannerH = 34;
  const fui::Rect qBanner = fui::makeRect(band.x, band.y, band.width, bannerH);
  screen.target().fill(qBanner, fui::Paint::solid(fui::Color::Black), 4);

  std::string qText = "Q: " + model.prompt;
  std::string qFitted = toybox::fitLines(screen.target(), qText.c_str(), qBanner.width - 24, 1,
                                         style(toybox::kSmallFont, fui::TextAlign::Left, fui::Color::White));
  screen.target().text(fui::makeRect(qBanner.x + 12, qBanner.y + 7, qBanner.width - 24, 20),
                       qFitted.c_str(), style(toybox::kSmallFont, fui::TextAlign::Left, fui::Color::White));

  // Response Text Area (strictly sized between banner and footer)
  const int textY = band.y + bannerH + 10;
  const int textH = footer.y - 10 - textY;
  const fui::Rect textRect = fui::makeRect(band.x, static_cast<int16_t>(textY), band.width, static_cast<int16_t>(textH));

  fui::TextAreaProps area;
  area.text = model.responseText.c_str();
  area.style = style(toybox::kBodyFont, fui::TextAlign::Left, fui::Color::Black, 0);
  area.topLine = static_cast<uint32_t>(model.currentPage * model.linesPerPage);
  area.showCaret = false;
  fui::textArea(screen.frame(), textRect, area);

  // Bottom Footer Navigation & Actions
  if (model.totalPages > 1) {
    // 5 buttons distributed side-by-side:
    // [ < PREV ] [ ASK NEXT ] [ SAVE NOTE ] [ NEW CHAT ] [ NEXT > ]
    constexpr int prevW = 116;
    constexpr int nextW = 116;
    constexpr int askW = 170;
    constexpr int saveW = 168;
    constexpr int newW = 144;
    constexpr int btnGap = 10;
    constexpr int totalW = prevW + askW + saveW + newW + nextW + 4 * btnGap; // 744
    const int startX = footer.x + (footer.width - totalW) / 2;

    int curX = startX;

    fui::ButtonProps prevBtn;
    prevBtn.label = "< PREV";
    prevBtn.action = ActionPrevPage;
    prevBtn.styles = (model.currentPage > 0) ? toybox::rowStyles() : toybox::disabledButtonStyles();
    screen.button(prevBtn, fui::makeRect(curX, footer.y, prevW, footer.height));
    curX += prevW + btnGap;

    fui::ButtonProps askBtn;
    askBtn.label = "ASK NEXT";
    askBtn.action = ActionAsk;
    askBtn.styles = toybox::invertedStyles();
    screen.button(askBtn, fui::makeRect(curX, footer.y, askW, footer.height));
    curX += askW + btnGap;

    fui::ButtonProps saveBtn;
    saveBtn.label = model.savedToNotes ? "SAVED ✓" : "SAVE NOTE";
    saveBtn.action = ActionSaveNote;
    saveBtn.styles = toybox::rowStyles();
    screen.button(saveBtn, fui::makeRect(curX, footer.y, saveW, footer.height));
    curX += saveW + btnGap;

    fui::ButtonProps newBtn;
    newBtn.label = "NEW CHAT";
    newBtn.action = ActionNewChat;
    newBtn.styles = toybox::rowStyles();
    screen.button(newBtn, fui::makeRect(curX, footer.y, newW, footer.height));
    curX += newW + btnGap;

    fui::ButtonProps nextBtn;
    nextBtn.label = "NEXT >";
    nextBtn.action = ActionNextPage;
    nextBtn.styles = (model.currentPage < model.totalPages - 1) ? toybox::rowStyles() : toybox::disabledButtonStyles();
    screen.button(nextBtn, fui::makeRect(curX, footer.y, nextW, footer.height));
  } else {
    // 3 buttons:
    // [ ASK AGAIN ] [ SAVE NOTE ] [ NEW CHAT ]
    const int btnW = (footer.width - 20) / 3;

    fui::ButtonProps askBtn;
    askBtn.label = "ASK AGAIN";
    askBtn.action = ActionAsk;
    askBtn.styles = toybox::invertedStyles();
    screen.button(askBtn, fui::makeRect(footer.x, footer.y, btnW, footer.height));

    fui::ButtonProps saveBtn;
    saveBtn.label = model.savedToNotes ? "SAVED TO NOTES ✓" : "SAVE NOTE";
    saveBtn.action = ActionSaveNote;
    saveBtn.styles = toybox::rowStyles();
    screen.button(saveBtn, fui::makeRect(footer.x + btnW + 10, footer.y, btnW, footer.height));

    fui::ButtonProps newBtn;
    newBtn.label = "NEW CHAT";
    newBtn.action = ActionNewChat;
    newBtn.styles = toybox::rowStyles();
    screen.button(newBtn, fui::makeRect(footer.x + (btnW + 10) * 2, footer.y, btnW, footer.height));
  }
}

void drawError(toybox::Screen& screen, const ErrorModel& model) {
  chrome(screen, "GEMINI", "ERROR");

  const fui::DeviceContext& device = screen.device();
  const fui::Rect band = contentBand(device);
  const fui::Rect footer = footerBand(device);

  // Error Card
  cardBox(screen, band);

  const int innerX = band.x + 20;
  const int innerW = band.width - 40;

  screen.target().text(fui::makeRect(innerX, band.y + 16, innerW, 18), "ERROR DETAILS",
                       style(toybox::kSmallFont, fui::TextAlign::Left));

  screen.target().text(fui::makeRect(innerX, band.y + 40, innerW, 26), model.title.c_str(),
                       style(toybox::kBodyFont, fui::TextAlign::Left));

  screen.target().fill(fui::makeRect(innerX, band.y + 72, innerW, 1), fui::Paint::solid(fui::Color::Black));

  screen.target().text(fui::makeRect(innerX, band.y + 84, innerW, band.height - 100), model.message.c_str(),
                       style(toybox::kBodyFont, fui::TextAlign::Left, fui::Color::Black, 6));

  // Contextual Footer Buttons
  if (model.showModelBtn) {
    constexpr int gap = 10;
    const int btnW = (footer.width - gap * 2) / 3;

    fui::ButtonProps modelBtn;
    modelBtn.label = "CHANGE MODEL";
    modelBtn.action = ActionSelectModel;
    modelBtn.styles = toybox::invertedStyles();
    screen.button(modelBtn, fui::makeRect(footer.x, footer.y, btnW, footer.height));

    fui::ButtonProps retryBtn;
    retryBtn.label = "RETRY";
    retryBtn.action = ActionRetry;
    retryBtn.styles = toybox::rowStyles();
    screen.button(retryBtn, fui::makeRect(footer.x + btnW + gap, footer.y, btnW, footer.height));

    fui::ButtonProps dismissBtn;
    dismissBtn.label = "DISMISS";
    dismissBtn.action = ActionDismissNotice;
    dismissBtn.styles = toybox::rowStyles();
    screen.button(dismissBtn, fui::makeRect(footer.x + (btnW + gap) * 2, footer.y, btnW, footer.height));
  } else if (model.showRetryBtn) {
    const int halfW = (footer.width - 12) / 2;

    fui::ButtonProps retryBtn;
    retryBtn.label = "RETRY";
    retryBtn.action = ActionRetry;
    retryBtn.styles = toybox::invertedStyles();
    screen.button(retryBtn, fui::makeRect(footer.x, footer.y, halfW, footer.height));

    fui::ButtonProps dismissBtn;
    dismissBtn.label = "DISMISS";
    dismissBtn.action = ActionDismissNotice;
    dismissBtn.styles = toybox::rowStyles();
    screen.button(dismissBtn, fui::makeRect(footer.x + halfW + 12, footer.y, halfW, footer.height));
  } else if (model.showWifiBtn) {
    const int halfW = (footer.width - 12) / 2;

    fui::ButtonProps wifiBtn;
    wifiBtn.label = "CONNECT TO WI-FI";
    wifiBtn.action = ActionConnectWifi;
    wifiBtn.styles = toybox::invertedStyles();
    screen.button(wifiBtn, fui::makeRect(footer.x, footer.y, halfW, footer.height));

    fui::ButtonProps dismissBtn;
    dismissBtn.label = "DISMISS";
    dismissBtn.action = ActionDismissNotice;
    dismissBtn.styles = toybox::rowStyles();
    screen.button(dismissBtn, fui::makeRect(footer.x + halfW + 12, footer.y, halfW, footer.height));
  } else if (model.showKeyBtn) {
    const int halfW = (footer.width - 12) / 2;

    fui::ButtonProps keyBtn;
    keyBtn.label = "ENTER API KEY";
    keyBtn.action = ActionSetKey;
    keyBtn.styles = toybox::invertedStyles();
    screen.button(keyBtn, fui::makeRect(footer.x, footer.y, halfW, footer.height));

    fui::ButtonProps dismissBtn;
    dismissBtn.label = "DISMISS";
    dismissBtn.action = ActionDismissNotice;
    dismissBtn.styles = toybox::rowStyles();
    screen.button(dismissBtn, fui::makeRect(footer.x + halfW + 12, footer.y, halfW, footer.height));
  } else {
    fui::ButtonProps dismissBtn;
    dismissBtn.label = "DISMISS";
    dismissBtn.action = ActionDismissNotice;
    dismissBtn.styles = toybox::invertedStyles();
    screen.button(dismissBtn, footer);
  }
}

void drawNotice(toybox::Screen& screen, const char* title, const char* message) {
  chrome(screen, "GEMINI", "NOTICE");

  const fui::DeviceContext& device = screen.device();
  const fui::Rect band = contentBand(device);
  const fui::Rect footer = footerBand(device);

  cardBox(screen, band);

  const int innerX = band.x + 20;
  const int innerW = band.width - 40;

  screen.target().text(fui::makeRect(innerX, band.y + 16, innerW, 18), "SYSTEM NOTICE",
                       style(toybox::kSmallFont, fui::TextAlign::Left));

  screen.target().text(fui::makeRect(innerX, band.y + 40, innerW, 26), title,
                       style(toybox::kBodyFont, fui::TextAlign::Left));

  screen.target().fill(fui::makeRect(innerX, band.y + 72, innerW, 1), fui::Paint::solid(fui::Color::Black));

  screen.target().text(fui::makeRect(innerX, band.y + 84, innerW, band.height - 100), message,
                       style(toybox::kBodyFont, fui::TextAlign::Left, fui::Color::Black, 6));

  fui::ButtonProps okBtn;
  okBtn.label = "OK";
  okBtn.action = ActionDismissNotice;
  okBtn.styles = toybox::invertedStyles();
  screen.button(okBtn, footer);
}

}  // namespace geminiui
