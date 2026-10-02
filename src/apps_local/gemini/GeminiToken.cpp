#include "GeminiToken.h"

#include <ArduinoJson.h>
#include <HalStorage.h>
#include <Logging.h>

#include <algorithm>
#include <cctype>

namespace gemini {
namespace {

constexpr const char* kCandidatePaths[] = {
    "/XTData/llm_token",
    "/XTData/llm_token.txt",
    "/xtdata/llm_token",
    "/xtdata/llm_token.txt",
    "/.crosspoint/gemini_token.txt",
};

std::string trim(const std::string& str) {
  size_t start = 0;
  while (start < str.size() && (std::isspace(static_cast<unsigned char>(str[start])) || str[start] == '"' || str[start] == '\'')) {
    start++;
  }
  size_t end = str.size();
  while (end > start && (std::isspace(static_cast<unsigned char>(str[end - 1])) || str[end - 1] == '"' || str[end - 1] == '\'')) {
    end--;
  }
  return str.substr(start, end - start);
}

std::string extractToken(const std::string& raw) {
  std::string cleaned = trim(raw);
  if (cleaned.empty()) return "";

  // Check if it's formatted as JSON: {"token": "...", "key": "..."}
  if (cleaned.front() == '{' && cleaned.back() == '}') {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, cleaned);
    if (!error) {
      if (doc["token"].is<const char*>()) return trim(doc["token"].as<std::string>());
      if (doc["key"].is<const char*>()) return trim(doc["key"].as<std::string>());
      if (doc["api_key"].is<const char*>()) return trim(doc["api_key"].as<std::string>());
      if (doc["llm_token"].is<const char*>()) return trim(doc["llm_token"].as<std::string>());
    }
  }

  // Check if it has key=... or token=... prefix
  auto extractAfterPrefix = [&](const char* prefix) -> std::string {
    const size_t len = strlen(prefix);
    if (cleaned.compare(0, len, prefix) == 0) {
      return trim(cleaned.substr(len));
    }
    return "";
  };

  std::string val;
  val = extractAfterPrefix("key=");
  if (!val.empty()) return val;
  val = extractAfterPrefix("token=");
  if (!val.empty()) return val;
  val = extractAfterPrefix("api_key=");
  if (!val.empty()) return val;
  val = extractAfterPrefix("GEMINI_API_KEY=");
  if (!val.empty()) return val;

  return cleaned;
}

}  // namespace

TokenInfo loadToken() {
  TokenInfo info;

  for (const char* path : kCandidatePaths) {
    if (!Storage.exists(path)) continue;

    auto file = Storage.open(path);
    if (!file) continue;

    std::string content;
    content.reserve(256);
    char buf[128];
    while (file.available() > 0 && content.size() < 1024) {
      const int bytesRead = file.read(buf, sizeof(buf));
      if (bytesRead <= 0) break;
      content.append(buf, bytesRead);
    }
    file.close();

    std::string token = extractToken(content);
    if (!token.empty()) {
      info.token = token;
      info.sourcePath = path;
      info.isFound = true;
      LOG_INF("GEMINI", "Loaded token from %s (length %u)", path, (unsigned)token.length());
      return info;
    }
  }

  LOG_INF("GEMINI", "No Gemini token found in candidate paths");
  return info;
}

bool saveToken(const std::string& token) {
  std::string cleaned = trim(token);
  if (cleaned.empty()) return false;

  // Ensure directories exist
  Storage.ensureDirectoryExists("/.crosspoint");
  Storage.ensureDirectoryExists("/XTData");

  // Save to /XTData/llm_token
  bool saved = Storage.writeFile("/XTData/llm_token", cleaned.c_str());
  if (saved) {
    LOG_INF("GEMINI", "Saved token to /XTData/llm_token");
  }

  // Also save to /.crosspoint/gemini_token.txt as fallback
  Storage.writeFile("/.crosspoint/gemini_token.txt", cleaned.c_str());

  return saved;
}

std::string maskToken(const std::string& token) {
  if (token.empty()) return "None";
  if (token.length() <= 8) return "********";
  return token.substr(0, 6) + "..." + token.substr(token.length() - 4);
}

}  // namespace gemini
