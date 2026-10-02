#pragma once

#include <string>

namespace gemini {

struct TokenInfo {
  std::string token;
  std::string sourcePath;
  std::string model;
  bool isFound = false;
};

// Loads the configured model name from /XTData/llm_model, defaulting to "gemini-2.5-flash".
std::string loadModel();

// Saves the active model name to /XTData/llm_model.
bool saveModel(const std::string& modelName);

// Loads the Gemini API token from /XTData/llm_token or fallback paths.
TokenInfo loadToken();

// Saves token to local storage and /XTData if available.
bool saveToken(const std::string& token);

// Returns a masked representation of the token (e.g. "AIzaSy...4aX9")
std::string maskToken(const std::string& token);

}  // namespace gemini
