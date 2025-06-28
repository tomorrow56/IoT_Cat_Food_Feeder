/***********
 * ESP32 LINE Messaging API Library
 * Based on SPRESENSE_ESP8266_LINE_Messaging_API by @tomorrow56
 * ESP32 native WiFi implementation
 ***********/

#ifndef ESP32LineMessenger_h
#define ESP32LineMessenger_h

#include "Arduino.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

class ESP32LineMessenger {
public:
  // コンストラクタ
  ESP32LineMessenger();
  ESP32LineMessenger(const char* token);  // アクセストークン付きコンストラクタ
  
  // 互換性を持たせる関数
  void setAccessToken(const char* token);  // アクセストークン設定
  bool connectWiFi(const char* ssid, const char* password, bool showConnect = true);  // WiFi接続
  bool sendMessage(const char* message, bool showSend = true);  // メッセージ送信
  
  // ESP32向け拡張機能
  bool isWiFiConnected();  // WiFi接続状態確認
  void setDebug(bool debug);  // デバッグモード設定
  
private:
  const char* host = "api.line.me";  // LINE APIホスト
  const int httpsPort = 443;  // HTTPSポート
  const char* accessToken;  // LINEアクセストークン
  bool debugMode;  // デバッグモード
  
  // 内部処理用関数
  bool postRequest(const char* endpoint, const char* payload);
};

#endif
