#include "GeminiClient.h"

#include <ArduinoJson.h>
#include <Logging.h>

#ifdef SIMULATOR
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#else
#include <SecureHttpClient.h>
#endif

namespace gemini {
namespace {

constexpr const char* kSystemInstruction =
    "You are Gemini, an intelligent reading and research assistant running on an e-ink e-reader (800x480). "
    "Provide clear, direct, and well-structured answers using clean markdown formatting. "
    "Avoid unnecessary fluff, conversational filler, or ASCII tables that don't fit narrow displays. "
    "Be concise unless the user explicitly asks for detailed explanations.";

}  // namespace

std::string Client::buildPayload(const std::string& prompt, const std::vector<Message>& history) const {
  JsonDocument doc;

  // System instruction
  JsonObject sys = doc["systemInstruction"].to<JsonObject>();
  JsonArray sysParts = sys["parts"].to<JsonArray>();
  JsonObject sysText = sysParts.add<JsonObject>();
  sysText["text"] = kSystemInstruction;

  // Contents (history + current prompt)
  JsonArray contents = doc["contents"].to<JsonArray>();

  // Add up to last 4 turns of history to preserve context while controlling token size
  size_t startIdx = 0;
  if (history.size() > 4) {
    startIdx = history.size() - 4;
  }
  for (size_t i = startIdx; i < history.size(); ++i) {
    JsonObject msgObj = contents.add<JsonObject>();
    msgObj["role"] = (history[i].role == "user") ? "user" : "model";
    JsonArray parts = msgObj["parts"].to<JsonArray>();
    JsonObject partObj = parts.add<JsonObject>();
    partObj["text"] = history[i].text;
  }

  // Current prompt
  JsonObject curObj = contents.add<JsonObject>();
  curObj["role"] = "user";
  JsonArray curParts = curObj["parts"].to<JsonArray>();
  JsonObject curPart = curParts.add<JsonObject>();
  curPart["text"] = prompt;

  // Generation config
  JsonObject cfg = doc["generationConfig"].to<JsonObject>();
  cfg["temperature"] = 0.7;
  cfg["maxOutputTokens"] = 2048;

  std::string output;
  serializeJson(doc, output);
  return output;
}

Response Client::parseResponse(int httpCode, const std::string& responseBody) const {
  Response res;
  res.httpCode = httpCode;

  if (httpCode < 0) {
    res.success = false;
    res.error = "Network connection failed or timed out.";
    return res;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, responseBody);
  if (err) {
    res.success = false;
    char buf[128];
    snprintf(buf, sizeof(buf), "HTTP %d: Failed to parse JSON response (%s)", httpCode, err.c_str());
    res.error = buf;
    LOG_ERR("GEMINI", "JSON parse error: %s (HTTP %d, body length %u)", err.c_str(), httpCode,
            (unsigned)responseBody.length());
    return res;
  }

  if (httpCode >= 200 && httpCode < 300) {
    // Check candidates
    if (doc["candidates"].is<JsonArray>() && doc["candidates"].size() > 0) {
      JsonObject candidate = doc["candidates"][0];
      if (candidate["content"]["parts"].is<JsonArray>()) {
        JsonArray parts = candidate["content"]["parts"];
        std::string fullText;
        for (JsonObject part : parts) {
          if (part["text"].is<const char*>()) {
            fullText += part["text"].as<std::string>();
          }
        }
        if (!fullText.empty()) {
          res.success = true;
          res.text = fullText;
          return res;
        }
      }
    }
    res.success = false;
    res.error = "Gemini returned empty content.";
    return res;
  }

  // Error block
  if (doc["error"].is<JsonObject>()) {
    std::string msg = doc["error"]["message"].is<const char*>() ? doc["error"]["message"].as<std::string>() : "";
    std::string status = doc["error"]["status"].is<const char*>() ? doc["error"]["status"].as<std::string>() : "";
    res.success = false;
    char buf[256];
    snprintf(buf, sizeof(buf), "Error (%d): %s", httpCode, msg.empty() ? status.c_str() : msg.c_str());
    res.error = buf;
    return res;
  }

  char buf[64];
  snprintf(buf, sizeof(buf), "HTTP error code: %d", httpCode);
  res.error = buf;
  return res;
}

Response Client::query(const std::string& prompt, const std::vector<Message>& history, const std::string& token,
                       const std::string& model) {
  Response res;
  if (token.empty()) {
    res.success = false;
    res.error = "Missing Gemini API token. Check /XTData/llm_token.";
    return res;
  }
  if (prompt.empty()) {
    res.success = false;
    res.error = "Prompt cannot be empty.";
    return res;
  }

  std::string activeModel = model.empty() ? "gemini-2.5-flash" : model;
  std::string url = "https://generativelanguage.googleapis.com/v1beta/models/" + activeModel +
                    ":generateContent?key=" + token;
  std::string payload = buildPayload(prompt, history);

  LOG_INF("GEMINI", "Sending request to %s (payload %u bytes)", activeModel.c_str(), (unsigned)payload.length());

  int httpCode = -1;
  std::string responseBody;

#ifdef SIMULATOR
  HTTPClient http;
  std::unique_ptr<WiFiClientSecure> secureClient = std::make_unique<WiFiClientSecure>();
  secureClient->setInsecure();
  http.setTimeout(30000);
  http.begin(*secureClient, url.c_str());
  http.addHeader("Content-Type", "application/json");
  http.addHeader("User-Agent", "CrossInk-X4Pro/1.6");
  httpCode = http.POST(payload.c_str());
  if (httpCode > 0) {
    responseBody = http.getString().c_str();
  }
  http.end();
#else
  freeink::SecureHttpClient http;
  http.setInsecure();
  http.setTimeout(30000);
  if (!http.begin(url)) {
    res.success = false;
    res.error = "Failed to parse API URL";
    return res;
  }
  http.addHeader("Content-Type", "application/json");
  http.addHeader("User-Agent", "CrossInk-X4Pro/1.6");
  httpCode = http.POST(payload);
  if (httpCode > 0) {
    responseBody = http.getString();
  }
  http.end();
#endif

  LOG_INF("GEMINI", "Response received: HTTP %d (%u bytes)", httpCode, (unsigned)responseBody.length());

  // If gemini-2.5-flash returned 404 (e.g. model not available in API version), try gemini-2.0-flash fallback
  if (httpCode == 404 && activeModel != "gemini-2.0-flash") {
    LOG_INF("GEMINI", "Model %s returned 404, falling back to gemini-2.0-flash", activeModel.c_str());
    return query(prompt, history, token, "gemini-2.0-flash");
  }

  return parseResponse(httpCode, responseBody);
}

bool Client::testConnection(const std::string& token, std::string& outError) {
  std::vector<Message> emptyHistory;
  Response res = query("Hi", emptyHistory, token);
  if (!res.success) {
    outError = res.error;
    return false;
  }
  return true;
}

}  // namespace gemini
