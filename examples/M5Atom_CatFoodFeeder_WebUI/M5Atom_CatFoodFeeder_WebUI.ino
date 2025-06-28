/********************
* Copywrite(c) 2022/10/16 @tomorrow56
* Pet Food Feeder IoT化プロジェクト
* No.1 基本動作検討版
* - NTPサーバからの時刻取得
* - フードトレイ動作
* - LINEへの通知
* - Web UI設定機能追加
* LED表示設定
*  0: WiFi接続済&モーター停止
*  1〜6: トレイ位置
*  7: リミットスイッチ検出
*  8: モーター動作中
*  9: WiFi未接続
* Modified 2025/6/21
* Line Notify->Line Messaging APIに変更
* Web UI機能追加
********************/
#include "M5Atom.h"
#include <WiFi.h>
#include "ESP32LineMessenger.h"
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "webui.h"

// Wi-Fi接続設定
const char* ssid       = "<YOUR SSID>";
const char* password   = "<YOUR PASSWORD>";

// NTP関連設定
struct tm timeInfo;
char s[20];//文字格納用

// LINEチャネルアクセストークン
const char* accessToken = "<YOUR TOKEN>";

// デバッグモード設定
#define debug true
// ライブラリインスタンス作成
ESP32LineMessenger line;
WebServer server(80);
Preferences preferences;

// Food Feederの制御用ポート
#define MOTOR_ON    22    // モーターを動かす
#define MOTOR_STOP  19    // モーターの両端を短絡
#define PR_IN       23    // Photo Reflector検出入力
#define SW_IN       33    // 位置検出スイッチ入力
#define PR_ON       25    // Photo Reflector用LEDのON(L:ON)
#define LED_ON      21    // メインボード上のLEDのON(L:ON)

// トレイを出す時間(24時間表示) - デフォルト値
int OPEN1 = 5;
int OPEN2 = 11;
int OPEN3 = 18;

boolean Motor_flg = false;  // モーター動作フラグ。動作時はtrueに設定
const int MOTOR_STOP_DELAY = 50; // モーターの電源を切ってからブレーキをかけるまでの遅延(ms) 
int TRAY_NUM; // 現在のトレイ番号 1〜6
boolean Error_flg = false;  // エラーフラグ

// 設定値を読み込む
void loadSettings() {
  preferences.begin("feeder", false); // Read-only
  OPEN1 = preferences.getInt("open1", 5);
  OPEN2 = preferences.getInt("open2", 11);
  OPEN3 = preferences.getInt("open3", 18);
  TRAY_NUM = preferences.getInt("trayNum", 0);
  preferences.end();
  
  bool needsCorrection = false;
  if (OPEN1 < 0 || OPEN1 > 23) {
    OPEN1 = 5;
    needsCorrection = true;
  }
  if (OPEN2 < 0 || OPEN2 > 23) {
    OPEN2 = 11;
    needsCorrection = true;
  }
  if (OPEN3 < 0 || OPEN3 > 23) {
    OPEN3 = 18;
    needsCorrection = true;
  }
  if (TRAY_NUM < 0 || TRAY_NUM > 6) {
    TRAY_NUM = 0;
    needsCorrection = true;
  }

  if (needsCorrection) {
    Serial.println("無効な設定値を検出し、デフォルト値に修正しました。");
    saveSettings();
  }

  Serial.println("設定値を読み込みました:");
  Serial.printf("OPEN1: %d, OPEN2: %d, OPEN3: %d\n", OPEN1, OPEN2, OPEN3);
}

// 設定値を保存する
void saveSettings() {
  preferences.begin("feeder", false);
  preferences.putInt("open1", OPEN1);
  preferences.putInt("open2", OPEN2);
  preferences.putInt("open3", OPEN3);
  preferences.putInt("trayNum", TRAY_NUM);
  preferences.end();
  
  Serial.println("設定値を保存しました:");
  Serial.printf("OPEN1: %d, OPEN2: %d, OPEN3: %d\n", OPEN1, OPEN2, OPEN3);
}

// Webサーバーのルートハンドラ
void handleRoot() {
  server.send(200, "text/html", webui_html);
}

// 現在時刻API
void handleApiTime() {
  getLocalTime(&timeInfo);
  sprintf(s, "%04d/%02d/%02d %02d:%02d:%02d",
    timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
    timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
  
  DynamicJsonDocument doc(1024);
  doc["time"] = String(s);
  
  String response;
  serializeJson(doc, response);
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", response);
}

// 設定取得API
void handleApiGetSettings() {
  DynamicJsonDocument doc(1024);
  doc["open1"] = OPEN1;
  doc["open2"] = OPEN2;
  doc["open3"] = OPEN3;
  doc["trayPosition"] = TRAY_NUM;
  doc["wifiSSID"] = WiFi.SSID();
  doc["ipAddress"] = WiFi.localIP().toString();
  
  String response;
  serializeJson(doc, response);
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", response);
}

// 設定保存API
void handleApiPostSettings() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, server.arg("plain"));
    
    if (doc.containsKey("open1") && doc.containsKey("open2") && doc.containsKey("open3")) {
      int newOpen1 = doc["open1"];
      int newOpen2 = doc["open2"];
      int newOpen3 = doc["open3"];
      
      // 値の妥当性チェック
      if (newOpen1 >= 0 && newOpen1 <= 23 && 
          newOpen2 >= 0 && newOpen2 <= 23 && 
          newOpen3 >= 0 && newOpen3 <= 23) {
        
        OPEN1 = newOpen1;
        OPEN2 = newOpen2;
        OPEN3 = newOpen3;
        
        saveSettings();
        
        DynamicJsonDocument responseDoc(1024);
        responseDoc["success"] = true;
        responseDoc["message"] = "設定を保存しました";
        
        String response;
        serializeJson(responseDoc, response);
        
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(200, "application/json", response);
        
        // LINEに通知
        String message = "[Food]給餌時刻を " + String(OPEN1) + "時, " + String(OPEN2) + "時, " + String(OPEN3) + "時 に変更しました";
        line.sendMessage(message.c_str());
        
        return;
      }
    }
  }
  
  DynamicJsonDocument responseDoc(1024);
  responseDoc["success"] = false;
  responseDoc["message"] = "無効な設定値です";
  
  String response;
  serializeJson(responseDoc, response);
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(400, "application/json", response);
}

// CORS対応
void handleApiOptions() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(200);
}

void setup(){
  // M5.begin(SerialEnable, I2CEnable, DisplayEnable);
  M5.begin(true, false, true);
  LEDnum(9);  //LEDに引数の数字を表示
  delay(100);

  // 設定値を読み込み
  loadSettings();

  // アクセストークン設定
  line.setAccessToken(accessToken);
  // デバッグモード設定（オプション）
  line.setDebug(debug);
  //無線LANに接続
  if (line.connectWiFi(ssid, password, debug)) {
    Serial.println("WiFi connected");
  }

  if(WiFi.status() == WL_CONNECTED){
    LEDnum(0);  //LEDに引数の数字を表示
    delay(100);
    Serial.println("connected to router(^^)");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());  //IP address assigned to ATOM
    Serial.println("");

    // Webサーバーの設定
    server.on("/", handleRoot);
    server.on("/api/time", HTTP_GET, handleApiTime);
    server.on("/api/settings", HTTP_GET, handleApiGetSettings);
    server.on("/api/settings", HTTP_POST, handleApiPostSettings);
    server.on("/api/settings", HTTP_OPTIONS, handleApiOptions);
    server.on("/api/time", HTTP_OPTIONS, handleApiOptions);
    
    server.begin();
    Serial.println("Webサーバーを開始しました");
    Serial.printf("Web UI: http://%s/\n", WiFi.localIP().toString().c_str());

    // NTPサーバーから時刻を取得する
    configTime(9 * 3600L, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp");//NTPの設定
   
    // Get local time
    //static const char *wd[7] = {"Sun","Mon","Tue","Wed","Thr","Fri","Sat"};
    getLocalTime(&timeInfo);  //tmオブジェクトのtimeInfoに現在時刻を入れ込む
    sprintf(s, " %04d/%02d/%02d %02d:%02d:%02d",
    timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
    timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
    Serial.println(s);
    
    // LINEにメッセージを送信
    String message = "[Food]WiFiに接続されました。トレイが開く時刻は http://" + WiFi.localIP().toString() + " で変更できます";
    line.sendMessage(message.c_str());
  }else{
    LEDnum(9);  //LEDに引数の数字を表示
    delay(100);
    Serial.println(" Failed to connect to WiFi");
  }

  //各I/Oの初期設定
  pinMode(MOTOR_ON, OUTPUT);
  pinMode(MOTOR_STOP, OUTPUT);
  pinMode(PR_ON, OUTPUT);
  pinMode(LED_ON, OUTPUT);
  pinMode(PR_IN, INPUT_PULLDOWN);
  pinMode(SW_IN, INPUT_PULLDOWN);

  digitalWrite(MOTOR_ON, LOW);
  digitalWrite(MOTOR_STOP, LOW);
  digitalWrite(PR_ON, HIGH);
  digitalWrite(LED_ON, HIGH);

  // 最初のトレイ位置へ移動させる
  LEDnum(8);
  moveMotor();
  if(detSwitch()){
    if(detTray()){
      stopMotor();
      line.sendMessage("[Food]トレイの初期設定ができました");
    }else{
      Error_flg = true;
      line.sendMessage("[Food]トレイの初期設定に失敗しました");
    }
    stopMotor();
    TRAY_NUM = 0;
    saveSettings(); // トレイ位置を保存
    LEDnum(TRAY_NUM);
    delay(3000);
    M5.dis.clear(); // LEDを消す
  }else{
    Error_flg = true;
  }
}

unsigned long lastCheckTime = 0;

void loop(){
  // Webサーバーのリクエストを処理
  server.handleClient();

  // 30秒ごとにチェック
  if (millis() - lastCheckTime >= 30000) {
    lastCheckTime = millis();
    
    getLocalTime(&timeInfo);  //tmオブジェクトのtimeInfoに現在時刻を入れ込む

    if(timeInfo.tm_min == 0){   // 毎時0分にチェック
      if(timeInfo.tm_hour == OPEN1 || timeInfo.tm_hour == OPEN2 || timeInfo.tm_hour == OPEN3){  // 設定時間になったらトレイを動かす
        if(TRAY_NUM < 6 && Error_flg == false){   // トレイ位置が6より小さい場合はトレイを動かす
          LEDnum(8);
          moveMotor();
          if(detTray()){  // トレイの移動が正常に検出されたら位置を更新してLINEへ通知する
            stopMotor();
            TRAY_NUM++;
            saveSettings(); // トレイ位置を保存
            LEDnum(TRAY_NUM); // LEDにトレイ番号を表示
            String message = "[Food]トレイ" + (String)TRAY_NUM + "のゴハンを出しました";
            line.sendMessage(message.c_str());
            delay(3000);
            M5.dis.clear(); // LEDを消す
          }else{  // トレイの移動の検出がエラーになったらモーターを停止する
            stopMotor();
            Error_flg = true;
            line.sendMessage("[Food]トレイの移動時にエラーが発生しました。動作を停止します");
          }
        }else{  // トレイ位置が6以上の場合は全てのゴハンを出し終わったので終了メッセージを送信
          Error_flg = true;
          line.sendMessage("[Food]全トレイのゴハンは出し終わっています！");
        }
        delay(1000 * 60); // 連続動作を避けるために1分待つ
      }
    }
  }
}

/********************
* モーターを動かす
********************/
void moveMotor(){
  if(Motor_flg == false){
    digitalWrite(MOTOR_STOP, LOW);
    delay(200);
    digitalWrite(MOTOR_ON, HIGH);
    Motor_flg = true;
  }
}

/********************
* モーターを止める
********************/
void stopMotor(){
  if(Motor_flg == true){
    digitalWrite(MOTOR_ON, LOW);
    delay(MOTOR_STOP_DELAY);
    digitalWrite(MOTOR_STOP, HIGH);
    delay(200);
    digitalWrite(MOTOR_STOP, LOW);
    Motor_flg = false;
  }
}

/********************
* リミットスイッチがONしたことを検出
********************/
boolean detSwitch(){
  boolean sw_on = false;
  boolean timeOutFlag = false;
  int i = 0;
  int j = 0;
  int timeOut_i = 600;
  int timeOut_j = 50;
 
  while(sw_on == false){
    if(digitalRead(SW_IN)){
      delay(100); // チャタリング除去
      if(digitalRead(SW_IN)){
        while(digitalRead(SW_IN)){
          delay(100);
          j++;
          if(j > timeOut_j){
            return false;            
          }
        }
        sw_on = true;
        LEDnum(7);
      }
    }
    delay(100);
    i++;
    if(i > timeOut_i){
      return false;            
    }
  }
  return true;
}

/********************
* トレイがセットされたことを検出
********************/
boolean detTray(){
  boolean sw_on = false;  
  boolean timeOutFlag = false;
  int i = 0;
  int j = 0;
  int timeOut_i = 1000;
  int timeOut_j = 500;

  digitalWrite(PR_ON, LOW); // フォトインタラプタのLEDをONにする

  while(sw_on == false){
    if(digitalRead(PR_IN)){
      delay(10); // チャタリング除去
      if(digitalRead(PR_IN)){
        while(digitalRead(PR_IN)){
          delay(10);
          j++;
          if(j > timeOut_j){
            digitalWrite(PR_ON, HIGH); // フォトインタラプタのLEDをOFFにする
            return false;            
          }
        }
        sw_on = true;
        digitalWrite(PR_ON, HIGH); // フォトインタラプタのLEDをOFFにする
      }
    }
    delay(10);
    i++;
    if(i > timeOut_i){
      digitalWrite(PR_ON, HIGH); // フォトインタラプタのLEDをOFFにする
      return false;            
    }
  }
  return true;
}

/********************
* ATOM MatrixのLEDで数字を表示
********************/
void LEDnum(int led_num) {
  M5.dis.clear();
  switch(led_num){
    case 1:
      M5.dis.drawpix(2, 0x707070);
      M5.dis.drawpix(6, 0x707070);
      M5.dis.drawpix(7, 0x707070);
      M5.dis.drawpix(12, 0x707070);
      M5.dis.drawpix(17, 0x707070);
      M5.dis.drawpix(21, 0x707070);
      M5.dis.drawpix(22, 0x707070);
      M5.dis.drawpix(23, 0x707070);
      break;
    case 2:
      M5.dis.drawpix(1, 0x707070);
      M5.dis.drawpix(2, 0x707070);
      M5.dis.drawpix(5, 0x707070);
      M5.dis.drawpix(8, 0x707070);
      M5.dis.drawpix(12, 0x707070);
      M5.dis.drawpix(16, 0x707070);
      M5.dis.drawpix(20, 0x707070);
      M5.dis.drawpix(21, 0x707070);
      M5.dis.drawpix(22, 0x707070);
      M5.dis.drawpix(23, 0x707070);
      break;
    case 3:
      M5.dis.drawpix(0, 0x707070);
      M5.dis.drawpix(1, 0x707070);
      M5.dis.drawpix(2, 0x707070);
      M5.dis.drawpix(8, 0x707070);
      M5.dis.drawpix(11, 0x707070);
      M5.dis.drawpix(12, 0x707070);
      M5.dis.drawpix(18, 0x707070);
      M5.dis.drawpix(20, 0x707070);
      M5.dis.drawpix(21, 0x707070);
      M5.dis.drawpix(22, 0x707070);
      break;
    case 4:
      M5.dis.drawpix(2, 0x707070);
      M5.dis.drawpix(6, 0x707070);
      M5.dis.drawpix(7, 0x707070);
      M5.dis.drawpix(10, 0x707070);
      M5.dis.drawpix(12, 0x707070);
      M5.dis.drawpix(15, 0x707070);
      M5.dis.drawpix(16, 0x707070);
      M5.dis.drawpix(17, 0x707070);
      M5.dis.drawpix(18, 0x707070);
      M5.dis.drawpix(22, 0x707070);
      break;
    case 5:
      M5.dis.drawpix(0, 0x707070);
      M5.dis.drawpix(1, 0x707070);
      M5.dis.drawpix(2, 0x707070);
      M5.dis.drawpix(5, 0x707070);
      M5.dis.drawpix(10, 0x707070);
      M5.dis.drawpix(11, 0x707070);
      M5.dis.drawpix(12, 0x707070);
      M5.dis.drawpix(18, 0x707070);
      M5.dis.drawpix(20, 0x707070);
      M5.dis.drawpix(21, 0x707070);
      M5.dis.drawpix(22, 0x707070);
      break;
    case 6:
      M5.dis.drawpix(1, 0x707070);
      M5.dis.drawpix(2, 0x707070);
      M5.dis.drawpix(5, 0x707070);
      M5.dis.drawpix(10, 0x707070);
      M5.dis.drawpix(11, 0x707070);
      M5.dis.drawpix(12, 0x707070);
      M5.dis.drawpix(15, 0x707070);
      M5.dis.drawpix(18, 0x707070);
      M5.dis.drawpix(21, 0x707070);
      M5.dis.drawpix(22, 0x707070);
      break;
    case 7:
      M5.dis.drawpix(0, 0x707070);
      M5.dis.drawpix(1, 0x707070);
      M5.dis.drawpix(2, 0x707070);
      M5.dis.drawpix(3, 0x707070);
      M5.dis.drawpix(8, 0x707070);
      M5.dis.drawpix(12, 0x707070);
      M5.dis.drawpix(16, 0x707070);
      M5.dis.drawpix(21, 0x707070);
      break;
    case 8:
      M5.dis.drawpix(1, 0x707070);
      M5.dis.drawpix(2, 0x707070);
      M5.dis.drawpix(5, 0x707070);
      M5.dis.drawpix(8, 0x707070);
      M5.dis.drawpix(11, 0x707070);
      M5.dis.drawpix(12, 0x707070);
      M5.dis.drawpix(15, 0x707070);
      M5.dis.drawpix(18, 0x707070);
      M5.dis.drawpix(21, 0x707070);
      M5.dis.drawpix(22, 0x707070);
      break;
    case 9:
      M5.dis.drawpix(1, 0x707070);
      M5.dis.drawpix(2, 0x707070);
      M5.dis.drawpix(5, 0x707070);
      M5.dis.drawpix(8, 0x707070);
      M5.dis.drawpix(11, 0x707070);
      M5.dis.drawpix(12, 0x707070);
      M5.dis.drawpix(13, 0x707070);
      M5.dis.drawpix(18, 0x707070);
      M5.dis.drawpix(21, 0x707070);
      M5.dis.drawpix(22, 0x707070);
      break;    
    case 0:
      M5.dis.drawpix(1, 0x707070);
      M5.dis.drawpix(2, 0x707070);
      M5.dis.drawpix(5, 0x707070);
      M5.dis.drawpix(8, 0x707070);
      M5.dis.drawpix(10, 0x707070);
      M5.dis.drawpix(13, 0x707070);
      M5.dis.drawpix(15, 0x707070);
      M5.dis.drawpix(18, 0x707070);
      M5.dis.drawpix(21, 0x707070);
      M5.dis.drawpix(22, 0x707070);
      break;    
  }
  delay(100);
}

