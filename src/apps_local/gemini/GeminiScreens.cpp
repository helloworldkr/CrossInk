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
  const int textY = kBodyTop + 38 + 12;
  const int textH = footerY - 12 - textY;
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

// --- iPhone iOS-style Key Tables ---
// Row 1: 10 keys
static const char* kRow1Low[] = {"q", "w", "e", "r", "t", "y", "u", "i", "o", "p"};
static const char* kRow1Up[]  = {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"};
static const char* kRow1Sym[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};

// Row 2: 9 keys (0.5-key indent on both sides: 23px)
static const char* kRow2Low[] = {"a", "s", "d", "f", "g", "h", "j", "k", "l"};
static const char* kRow2Up[]  = {"A", "S", "D", "F", "G", "H", "J", "K", "L"};
static const char* kRow2Sym[] = {"-", "/", ":", ";", "(", ")", "$", "&", "@"};

// Row 3: 7 keys
static const char* kRow3Low[] = {"z", "x", "c", "v", "b", "n", "m"};
static const char* kRow3Up[]  = {"Z", "X", "C", "V", "B", "N", "M"};
static const char* kRow3Sym[] = {"?", "!", "'", "\"", ",", ";", "%"};

void drawIphoneKey(toybox::Screen& screen, const fui::Rect& rect, const char* label, fui::ActionId action, int16_t value, bool inverted = false) {
  fui::ButtonProps bp;
  bp.label = label;
  bp.action = action;
  bp.value = value;
  bp.styles = inverted ? toybox::invertedStyles() : toybox::rowStyles();
  bp.text.font = (strlen(label) == 1) ? toybox::kBodyFont : toybox::kSmallFont;
  screen.button(bp, rect);
}

void drawWelcome(toybox::Screen& screen, const WelcomeModel& model) {
  chrome(screen, "GEMINI AI", model.modelName.c_str());

  const fui::DeviceContext& device = screen.device();
  const fui::Rect band = contentBand(device);

  // --- 1. Top Action Bar: PROMPTS | CHATS | SETTINGS ---
  constexpr int barH = 38;
  constexpr int gap = 8;
  const int btnW = (band.width - gap * 2) / 3;

  int curX = band.x;
  fui::ButtonProps promptsBtn;
  promptsBtn.label = "PROMPTS";
  promptsBtn.action = ActionQuickPrompts;
  promptsBtn.styles = toybox::rowStyles();
  promptsBtn.text.font = toybox::kSmallFont;
  screen.button(promptsBtn, fui::makeRect(curX, band.y, btnW, barH));
  curX += btnW + gap;

  fui::ButtonProps chatsBtn;
  chatsBtn.label = "CHATS";
  chatsBtn.action = ActionSavedChats;
  chatsBtn.styles = toybox::rowStyles();
  chatsBtn.text.font = toybox::kSmallFont;
  screen.button(chatsBtn, fui::makeRect(curX, band.y, btnW, barH));
  curX += btnW + gap;

  fui::ButtonProps settingsBtn;
  settingsBtn.label = "SETTINGS";
  settingsBtn.action = ActionOpenSettings;
  settingsBtn.styles = toybox::rowStyles();
  settingsBtn.text.font = toybox::kSmallFont;
  screen.button(settingsBtn, fui::makeRect(curX, band.y, band.width - (curX - band.x), barH));

  // --- 2. Status Line (Separated columns, no overlap) ---
  const int statusY = band.y + barH + 6;
  std::string wifiStr = model.wifiConnected ? ("Wi-Fi: " + (model.wifiSsid.empty() ? "Connected" : model.wifiSsid))
                                            : (model.wifiConnecting ? "Wi-Fi: Connecting..." : "Wi-Fi: Offline");
  std::string keyStr = model.tokenFound ? "Key: Ready" : "Key: Missing";

  const int wifiW = 270;
  const int keyW = band.width - wifiW;
  screen.target().text(fui::makeRect(band.x, statusY, wifiW, 16), wifiStr.c_str(),
                       style(toybox::kSmallFont, fui::TextAlign::Left));
  screen.target().text(fui::makeRect(band.x + wifiW, statusY, keyW, 16), keyStr.c_str(),
                       style(toybox::kSmallFont, fui::TextAlign::Right));

  // --- 3. Keyboard Geometry (Docked at bottom of 800px Portrait screen) ---
  constexpr int kRowH = 56;
  constexpr int kRowGap = 6;
  constexpr int kKeyGap = 4;
  constexpr int kStartX = 7;
  constexpr int kKeyW = 43;

  const int row4Y = device.height - 18 - kRowH;      // 800 - 18 - 56 = 726
  const int row3Y = row4Y - kRowGap - kRowH;          // 726 - 6 - 56 = 664
  const int row2Y = row3Y - kRowGap - kRowH;          // 664 - 6 - 56 = 602
  const int row1Y = row2Y - kRowGap - kRowH;          // 602 - 6 - 56 = 540

  // --- 4. Action Strip above Keyboard: CLEAR | SEND > ---
  const int stripH = 34;
  const int stripY = row1Y - 8 - stripH;              // 540 - 8 - 34 = 498
  const int clearW = 90;
  const int sendW = band.width - clearW - 8;

  fui::ButtonProps clearBtn;
  clearBtn.label = "CLEAR";
  clearBtn.action = ActionClearPrompt;
  clearBtn.styles = toybox::rowStyles();
  clearBtn.text.font = toybox::kSmallFont;
  screen.button(clearBtn, fui::makeRect(band.x, stripY, clearW, stripH));

  fui::ButtonProps sendBtn;
  sendBtn.label = "SEND >";
  sendBtn.action = ActionSendPrompt;
  sendBtn.styles = toybox::invertedStyles();
  sendBtn.text.font = toybox::kSmallFont;
  screen.button(sendBtn, fui::makeRect(band.x + clearW + 8, stripY, sendW, stripH));

  // --- 5. Interactive Prompt Card (Between Status Line and Action Strip) ---
  const int boxY = statusY + 20;
  const int boxH = stripY - 8 - boxY;
  const fui::Rect promptBox = fui::makeRect(band.x, boxY, band.width, boxH);

  // Button interaction over prompt box so tapping it opens full-screen typing
  fui::ButtonProps promptCardBtn;
  promptCardBtn.label = "";
  promptCardBtn.action = ActionEditPrompt;
  promptCardBtn.styles = toybox::rowStyles();
  screen.button(promptCardBtn, promptBox);

  if (model.draftPrompt.empty()) {
    screen.target().text(
        fui::makeRect(promptBox.x + 14, promptBox.y + 14, promptBox.width - 28, 22),
        "Tap here to open full keyboard...",
        style(toybox::kBodyFont, fui::TextAlign::Left));

    screen.target().text(
        fui::makeRect(promptBox.x + 14, promptBox.y + 44, promptBox.width - 28, 80),
        "Ask questions, explain concepts, or brainstorm.\nOr use the on-screen keyboard below.\nTap [ PROMPTS ] for ready-made templates.",
        style(toybox::kSmallFont, fui::TextAlign::Left, fui::Color::Black, 4));

    screen.target().text(
        fui::makeRect(promptBox.x + 14, promptBox.y + promptBox.height - 24, promptBox.width - 28, 18),
        "[ Tap card to type | 0/200 ]",
        style(toybox::kSmallFont, fui::TextAlign::Right));
  } else {
    std::string promptWithCursor = model.draftPrompt + "|";
    screen.target().text(
        fui::makeRect(promptBox.x + 14, promptBox.y + 12, promptBox.width - 28, promptBox.height - 36),
        promptWithCursor.c_str(),
        style(toybox::kBodyFont, fui::TextAlign::Left, fui::Color::Black, 11));

    std::string countStr = std::to_string(model.draftPrompt.size()) + "/200  (Tap to edit)";
    screen.target().text(
        fui::makeRect(promptBox.x + 14, promptBox.y + promptBox.height - 22, promptBox.width - 28, 16),
        countStr.c_str(),
        style(toybox::kSmallFont, fui::TextAlign::Right));
  }

  // --- 6. Render the 4 Rows of iPhone Keyboard with 100% clean ASCII ---

  // Row 1 (10 keys)
  const char** r1 = model.symbols ? kRow1Sym : (model.shifted ? kRow1Up : kRow1Low);
  for (int i = 0; i < 10; ++i) {
    fui::Rect kRect = fui::makeRect(kStartX + i * (kKeyW + kKeyGap), row1Y, kKeyW, kRowH);
    drawIphoneKey(screen, kRect, r1[i], ActionKeyChar, r1[i][0]);
  }

  // Row 2 (9 keys with authentic 0.5-key indent on both sides: 23px)
  const int row2StartX = kStartX + 23;
  const char** r2 = model.symbols ? kRow2Sym : (model.shifted ? kRow2Up : kRow2Low);
  for (int i = 0; i < 9; ++i) {
    fui::Rect kRect = fui::makeRect(row2StartX + i * (kKeyW + kKeyGap), row2Y, kKeyW, kRowH);
    drawIphoneKey(screen, kRect, r2[i], ActionKeyChar, r2[i][0]);
  }

  // Row 3 (Shift 66px + 7 keys + Del 67px)
  fui::Rect shiftRect = fui::makeRect(kStartX, row3Y, 66, kRowH);
  const char* shiftLabel = model.symbols ? "#+=" : (model.shifted ? "CAPS" : "SHIFT");
  drawIphoneKey(screen, shiftRect, shiftLabel, ActionKeyShift, 0, model.shifted);

  const int row3KeysX = kStartX + 66 + kKeyGap;
  const char** r3 = model.symbols ? kRow3Sym : (model.shifted ? kRow3Up : kRow3Low);
  for (int i = 0; i < 7; ++i) {
    fui::Rect kRect = fui::makeRect(row3KeysX + i * (kKeyW + kKeyGap), row3Y, kKeyW, kRowH);
    drawIphoneKey(screen, kRect, r3[i], ActionKeyChar, r3[i][0]);
  }

  const int delX = row3KeysX + 7 * (kKeyW + kKeyGap);
  fui::Rect delRect = fui::makeRect(delX, row3Y, 67, kRowH);
  drawIphoneKey(screen, delRect, "DEL", ActionKeyDelete, 0);

  // Row 4 (Mode 66px + Space 260px + Dot 43px + Send 85px)
  fui::Rect modeRect = fui::makeRect(kStartX, row4Y, 66, kRowH);
  drawIphoneKey(screen, modeRect, model.symbols ? "ABC" : "123", ActionKeyMode, 0);

  const int spaceX = kStartX + 66 + kKeyGap;
  fui::Rect spaceRect = fui::makeRect(spaceX, row4Y, 260, kRowH);
  drawIphoneKey(screen, spaceRect, "space", ActionKeySpace, ' ');

  const int dotX = spaceX + 260 + kKeyGap;
  fui::Rect dotRect = fui::makeRect(dotX, row4Y, kKeyW, kRowH);
  drawIphoneKey(screen, dotRect, ".", ActionKeyChar, '.');

  const int sendKx = dotX + kKeyW + kKeyGap;
  fui::Rect sendKRect = fui::makeRect(sendKx, row4Y, 85, kRowH);
  drawIphoneKey(screen, sendKRect, "SEND", ActionSendPrompt, 0, true);
}

void drawThinking(toybox::Screen& screen, const ThinkingModel& model) {
  chrome(screen, "GEMINI AI", "THINKING");

  const fui::DeviceContext& device = screen.device();
  const fui::Rect band = contentBand(device);
  const fui::Rect footer = footerBand(device);

  // Top Card: Prompt recap (130px)
  const fui::Rect promptBox = fui::makeRect(band.x, band.y, band.width, 130);
  cardBox(screen, promptBox);

  screen.target().text(fui::makeRect(promptBox.x + 16, promptBox.y + 12, promptBox.width - 32, 18), "QUESTION",
                       style(toybox::kSmallFont, fui::TextAlign::Left));

  std::string pFitted = toybox::fitLines(screen.target(), model.prompt.c_str(), promptBox.width - 32, 3,
                                         style(toybox::kBodyFont, fui::TextAlign::Left));
  screen.target().text(fui::makeRect(promptBox.x + 16, promptBox.y + 36, promptBox.width - 32, 80),
                       pFitted.c_str(), style(toybox::kBodyFont, fui::TextAlign::Left, fui::Color::Black, 3));

  // Center Card: Thinking Status
  const int thinkY = band.y + 144;
  const int thinkH = band.height - 144;
  const fui::Rect thinkBox = fui::makeRect(band.x, thinkY, band.width, thinkH);
  cardBox(screen, thinkBox);

  screen.target().text(fui::makeRect(thinkBox.x + 16, thinkBox.y + 60, thinkBox.width - 32, 32), "Thinking...",
                       style(toybox::kDisplayFont, fui::TextAlign::Center));

  screen.target().text(fui::makeRect(thinkBox.x + 16, thinkBox.y + 110, thinkBox.width - 32, 22),
                       "Querying Google Gemini API over Wi-Fi...", style(toybox::kSmallFont, fui::TextAlign::Center));

  std::string modelStr = "Model: " + model.modelName;
  screen.target().text(fui::makeRect(thinkBox.x + 16, thinkBox.y + 140, thinkBox.width - 32, 20), modelStr.c_str(),
                       style(toybox::kSmallFont, fui::TextAlign::Center));

  // Footer: Cancel button
  fui::ButtonProps cancelBtn;
  cancelBtn.label = "CANCEL";
  cancelBtn.action = ActionDismissNotice;
  cancelBtn.styles = toybox::rowStyles();
  screen.button(cancelBtn, footer);
}

void drawResponse(toybox::Screen& screen, const ResponseModel& model) {
  char pageBuf[48];
  if (model.turnNumber > 1) {
    if (model.totalPages > 1) {
      snprintf(pageBuf, sizeof(pageBuf), "Turn %d | %d/%d", model.turnNumber, model.currentPage + 1, model.totalPages);
    } else {
      snprintf(pageBuf, sizeof(pageBuf), "Turn %d | %s", model.turnNumber, model.modelName.c_str());
    }
  } else {
    if (model.totalPages > 1) {
      snprintf(pageBuf, sizeof(pageBuf), "Page %d of %d", model.currentPage + 1, model.totalPages);
    } else {
      snprintf(pageBuf, sizeof(pageBuf), "%s", model.modelName.c_str());
    }
  }
  chrome(screen, "GEMINI", pageBuf);

  const fui::DeviceContext& device = screen.device();
  const fui::Rect band = contentBand(device);
  const fui::Rect footer = footerBand(device);

  // Top Question Banner (compact inverted black bar)
  constexpr int bannerH = 38;
  const fui::Rect qBanner = fui::makeRect(band.x, band.y, band.width, bannerH);
  screen.target().fill(qBanner, fui::Paint::solid(fui::Color::Black), 4);

  std::string qText = "Q: " + model.prompt;
  std::string qFitted = toybox::fitLines(screen.target(), qText.c_str(), qBanner.width - 24, 1,
                                         style(toybox::kSmallFont, fui::TextAlign::Left, fui::Color::White));
  screen.target().text(fui::makeRect(qBanner.x + 12, qBanner.y + 9, qBanner.width - 24, 20),
                       qFitted.c_str(), style(toybox::kSmallFont, fui::TextAlign::Left, fui::Color::White));

  // Response Text Area (generous vertical room in portrait)
  const int textY = band.y + bannerH + 12;
  const int textH = footer.y - 12 - textY;
  const fui::Rect textRect = fui::makeRect(band.x, static_cast<int16_t>(textY), band.width, static_cast<int16_t>(textH));

  fui::TextAreaProps area;
  area.text = model.responseText.c_str();
  area.style = style(toybox::kBodyFont, fui::TextAlign::Left, fui::Color::Black, 0);
  area.topLine = static_cast<uint32_t>(model.currentPage * model.linesPerPage);
  area.showCaret = false;
  fui::textArea(screen.frame(), textRect, area);

  // Bottom Footer Navigation & Actions for Portrait (448px width)
  if (model.totalPages > 1) {
    // 6 buttons: [ < ] [ REPLY ] [ SAVE ] [ BACK ] [ NEW ] [ > ]
    constexpr int arrowW = 42;
    constexpr int btnGap = 4;
    constexpr int newW = 62;
    constexpr int backW = 74;
    constexpr int replyW = 108;
    const int saveW = footer.width - (arrowW * 2 + newW + backW + replyW + btnGap * 5);

    int curX = footer.x;

    fui::ButtonProps prevBtn;
    prevBtn.label = "<";
    prevBtn.action = ActionPrevPage;
    prevBtn.styles = (model.currentPage > 0) ? toybox::rowStyles() : toybox::disabledButtonStyles();
    screen.button(prevBtn, fui::makeRect(curX, footer.y, arrowW, footer.height));
    curX += arrowW + btnGap;

    fui::ButtonProps replyBtn;
    replyBtn.label = "REPLY";
    replyBtn.action = ActionAsk;
    replyBtn.styles = toybox::invertedStyles();
    screen.button(replyBtn, fui::makeRect(curX, footer.y, replyW, footer.height));
    curX += replyW + btnGap;

    fui::ButtonProps saveBtn;
    saveBtn.label = model.savedToNotes ? "SAVED" : "SAVE";
    saveBtn.action = ActionSaveNote;
    saveBtn.styles = toybox::rowStyles();
    screen.button(saveBtn, fui::makeRect(curX, footer.y, saveW, footer.height));
    curX += saveW + btnGap;

    fui::ButtonProps backBtn;
    backBtn.label = "BACK";
    backBtn.action = ActionBackToPrompt;
    backBtn.styles = toybox::rowStyles();
    screen.button(backBtn, fui::makeRect(curX, footer.y, backW, footer.height));
    curX += backW + btnGap;

    fui::ButtonProps newBtn;
    newBtn.label = "NEW";
    newBtn.action = ActionNewChat;
    newBtn.styles = toybox::rowStyles();
    screen.button(newBtn, fui::makeRect(curX, footer.y, newW, footer.height));
    curX += newW + btnGap;

    fui::ButtonProps nextBtn;
    nextBtn.label = ">";
    nextBtn.action = ActionNextPage;
    nextBtn.styles = (model.currentPage < model.totalPages - 1) ? toybox::rowStyles() : toybox::disabledButtonStyles();
    screen.button(nextBtn, fui::makeRect(curX, footer.y, arrowW, footer.height));
  } else {
    // 4 buttons across portrait width: [ REPLY ] [ SAVE ] [ BACK ] [ NEW CHAT ]
    constexpr int gap = 8;
    const int btnW = (footer.width - gap * 3) / 4;

    fui::ButtonProps replyBtn;
    replyBtn.label = "REPLY";
    replyBtn.action = ActionAsk;
    replyBtn.styles = toybox::invertedStyles();
    screen.button(replyBtn, fui::makeRect(footer.x, footer.y, btnW, footer.height));

    fui::ButtonProps saveBtn;
    saveBtn.label = model.savedToNotes ? "SAVED" : "SAVE";
    saveBtn.action = ActionSaveNote;
    saveBtn.styles = toybox::rowStyles();
    screen.button(saveBtn, fui::makeRect(footer.x + (btnW + gap), footer.y, btnW, footer.height));

    fui::ButtonProps backBtn;
    backBtn.label = "BACK";
    backBtn.action = ActionBackToPrompt;
    backBtn.styles = toybox::rowStyles();
    screen.button(backBtn, fui::makeRect(footer.x + (btnW + gap) * 2, footer.y, btnW, footer.height));

    fui::ButtonProps newBtn;
    newBtn.label = "NEW CHAT";
    newBtn.action = ActionNewChat;
    newBtn.styles = toybox::rowStyles();
    screen.button(newBtn, fui::makeRect(footer.x + (btnW + gap) * 3, footer.y, footer.width - (btnW + gap) * 3, footer.height));
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
