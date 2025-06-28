/***********
 * ESP32 LINE Messaging API Library
 * Based on SPRESENSE_ESP8266_LINE_Messaging_API by @tomorrow56
 * ESP32 native WiFi implementation
 ***********/

#include "ESP32LineMessenger.h"

// コンストラクタ
ESP32LineMessenger::ESP32LineMessenger() {
  accessToken = nullptr;  // 初期値
  debugMode = false;      // デバッグモード初期値
}

// アクセストークン付きコンストラクタ
ESP32LineMessenger::ESP32LineMessenger(const char* token) {
  accessToken = token;
  debugMode = false;      // デバッグモード初期値
}

// アクセストークン設定
void ESP32LineMessenger::setAccessToken(const char* token) {
  accessToken = token;
}

// WiFi接続
bool ESP32LineMessenger::connectWiFi(const char* ssid, const char* password, bool showConnect) {
  if (debugMode || showConnect) {
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
  }
  
  WiFi.begin(ssid, password);
  
  // WiFi接続待機
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    if (debugMode || showConnect) {
      Serial.print(".");
    }
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    if (debugMode || showConnect) {
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    }
    return true;
  } else {
    if (debugMode || showConnect) {
      Serial.println("");
      Serial.println("WiFi connection failed");
    }
    return false;
  }
}

// メッセージ送信
bool ESP32LineMessenger::sendMessage(const char* message, bool showSend) {
  if (accessToken == nullptr) {
    if (debugMode || showSend) {
      Serial.println("Error: Access token not set");
    }
    return false;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    if (debugMode || showSend) {
      Serial.println("Error: WiFi not connected");
    }
    return false;
  }
  
  // JSONペイロード構築
  String payload = "{\"messages\":[{\"type\":\"text\",\"text\":\"";
  payload += message;
  payload += "\"}]}";
  
  if (debugMode || showSend) {
    Serial.println("Sending message to LINE...");
    if (debugMode) {
      Serial.print("Payload: ");
      Serial.println(payload);
    }
  }
  
  // POSTリクエスト送信
  bool success = postRequest("/v2/bot/message/broadcast", payload.c_str());
  
  if (success) {
    if (debugMode || showSend) {
      Serial.println("Message sent successfully");
    }
  } else {
    if (debugMode || showSend) {
      Serial.println("Failed to send message");
    }
  }
  
  return success;
}

// WiFi接続状態確認
bool ESP32LineMessenger::isWiFiConnected() {
  return (WiFi.status() == WL_CONNECTED);
}

// デバッグモード設定
void ESP32LineMessenger::setDebug(bool debug) {
  debugMode = debug;
}

// 内部処理用：POSTリクエスト送信
bool ESP32LineMessenger::postRequest(const char* endpoint, const char* payload) {
  WiFiClientSecure client;
  HTTPClient https;
  
  // SSL証明書チェックを無効化（本番環境では適切な証明書を使用すべき）
  client.setInsecure();
  
  // HTTPSクライアント初期化
  String url = "https://";
  url += host;
  url += endpoint;
  
  if (debugMode) {
    Serial.print("Connecting to: ");
    Serial.println(url);
  }
  
  https.begin(client, url);
  
  // ヘッダー設定
  https.addHeader("Content-Type", "application/json");
  String auth = "Bearer ";
  auth += accessToken;
  https.addHeader("Authorization", auth);
  
  // POSTリクエスト送信
  int httpCode = https.POST(payload);
  
  if (debugMode) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpCode);
  }
  
  // レスポンス取得
  if (httpCode > 0) {
    String response = https.getString();
    if (debugMode) {
      Serial.println("Response:");
      Serial.println(response);
    }
  }
  
  // 接続終了
  https.end();
  
  return (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED);
}
