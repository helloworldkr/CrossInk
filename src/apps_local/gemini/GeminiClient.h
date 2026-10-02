#pragma once

#include <string>
#include <vector>

namespace gemini {

struct Message {
  std::string role;  // "user" or "model"
  std::string text;
};

struct Response {
  bool success = false;
  std::string text;
  std::string error;
  std::string usedModel;
  int httpCode = 0;
};

class Client {
 public:
  Client() = default;
  ~Client() = default;

  // Sends prompt to Gemini API, including prior conversation history turns
  Response query(const std::string& prompt, const std::vector<Message>& history, const std::string& token,
                 const std::string& model = "gemini-2.5-flash", int depth = 0);

  // Tests connection with an empty ping/prompt
  bool testConnection(const std::string& token, std::string& outError);

 private:
  std::string buildPayload(const std::string& prompt, const std::vector<Message>& history) const;
  Response parseResponse(int httpCode, const std::string& responseBody) const;
};

}  // namespace gemini
