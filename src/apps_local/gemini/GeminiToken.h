#pragma once

#include <string>

namespace gemini {

struct TokenInfo {
  std::string token;
  std::string sourcePath;
  bool isFound = false;
};

// Loads the Gemini API token from /XTData/llm_token or fallback paths.
TokenInfo loadToken();

// Saves token to local storage and /XTData if available.
bool saveToken(const std::string& token);

// Returns a masked representation of the token (e.g. "AIzaSy...4aX9")
std::string maskToken(const std::string& token);

}  // namespace gemini
