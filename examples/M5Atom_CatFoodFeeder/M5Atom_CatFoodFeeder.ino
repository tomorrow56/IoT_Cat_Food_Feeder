/********************
* Copywrite(c) 2022/10/16 @tomorrow56
* Pet Food Feeder IoT化プロジェクト
* No.1 基本動作検討版
* - NTPサーバからの時刻取得
* - フードトレイ動作
* - LINEへの通知
* LED表示設定
*  0: WiFi接続済&モーター停止
*  1〜6: トレイ位置
*  7: リミットスイッチ検出
*  8: モーター動作中
*  9: WiFi未接続
********************/
#include "M5Atom.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>

// Wi-Fi接続設定
char* ssid       = "xxxxxxxxx";
char* password   = "xxxxxxxxxxxxxxxx";
WiFiClientSecure client;

// NTP関連設定
struct tm timeInfo;
char s[20];//文字格納用

// LINE Notify設定
char* line_host = "notify-api.line.me"; // LINE Notifyサーバのアドレス
char* line_token = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"; // LINEで通知するグループのトークン

// Food Feederの制御用ポート
#define MOTOR_ON    22    // モーターを動かす
#define MOTOR_STOP  19    // モーターの両端を短絡
#define PR_IN       23    // Photo Reflector検出入力
#define SW_IN       33    // 位置検出スイッチ入力
#define PR_ON       25    // Photo Reflector用LEDのON(L:ON)
#define LED_ON      21    // メインボード上のLEDのON(L:ON)

boolean Motor_flg = false;  // モーター動作フラグ。動作時はtrueに設定
const int MOTOR_STOP_DELAY = 50; // モーターの電源を切ってからブレーキをかけるまでの遅延(ms) 
int TRAY_NUM; // 現在のトレイ番号 1〜6
boolean Error_flg = false;  // エラーフラグ

void setup(){
  // M5.begin(SerialEnable, I2CEnable, DisplayEnable);
  M5.begin(true, false, true);
  LEDnum(9);  //LEDに引数の数字を表示
  delay(100);

  //無線LANに接続
  WiFi.begin(ssid, password);
  int i = 0;
  int timeOut = 60;
  while (WiFi.status() != WL_CONNECTED && i < timeOut){
      delay(500);
      Serial.print(".");
      i++;
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

    // NTPサーバーから時刻を取得する
    configTime(9 * 3600L, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp");//NTPの設定
   
    // Get local time
    //static const char *wd[7] = {"Sun","Mon","Tue","Wed","Thr","Fri","Sat"};
    getLocalTime(&timeInfo);  //tmオブジェクトのtimeInfoに現在時刻を入れ込む
    sprintf(s, " %04d/%02d/%02d %02d:%02d:%02d",
    timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
    timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
    Serial.println(s);
    LINE_Notify(line_host, line_token, "時刻を " + (String)s + " に設定しました");
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
      LINE_Notify(line_host, line_token, "トレイの初期設定ができました");
    }else{
      Error_flg = true;
      LINE_Notify(line_host, line_token, "トレイの初期設定に失敗しました");
    }
    stopMotor();
    TRAY_NUM = 0;
    LEDnum(TRAY_NUM);
    delay(3000);
    M5.dis.clear(); // LEDを消す
  }else{
    Error_flg = true;
  }
}

void loop(){
  getLocalTime(&timeInfo);  //tmオブジェクトのtimeInfoに現在時刻を入れ込む

  if(timeInfo.tm_min == 0){   // 毎時0分にチェック
    if(timeInfo.tm_hour == 5 || timeInfo.tm_hour == 11 || timeInfo.tm_hour == 18){  // 設定時間だったらトレイを動かす
      if(TRAY_NUM < 6 && Error_flg == false){   // トレイ位置が6より小さい場合はトレイを動かす
        LEDnum(8);
        moveMotor();
        if(detTray()){  // トレイの移動が正常に検出されたら位置を更新してLINEへ通知する
          stopMotor();
          TRAY_NUM++;
          LEDnum(TRAY_NUM); // LEDにトレイ番号を表示
          LINE_Notify(line_host, line_token, "トレイ" + (String)TRAY_NUM + "のゴハンを出しました");
          delay(3000);
          M5.dis.clear(); // LEDを消す
        }else{  // トレイの移動の検出がエラーになったらモーターを停止する
          stopMotor();
          Error_flg = true;
          LINE_Notify(line_host, line_token, "トレイの移動時にエラーが発生しましたので動作を停止します");
        }
      }else{  // トレイ位置が6以上の場合は全てのゴハンを出し終わったので終了メッセージを送信
        Error_flg = true;
        LINE_Notify(line_host, line_token, "全トレイのゴハンは出し終わっています！");
      }
      delay(1000 * 60); // 連続動作を避けるために1分待つ
    }
  }
  delay(1000 * 30); // 30秒周期でループを回す
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
* Line Notifyへ通知
********************/
void LINE_Notify(char* host, char* token, String message){
  client.setInsecure();  // Line Notifyのアクセスエラー対応
  if(!client.connect(host, 443)){
    Serial.println("Connection failed!");
    return;
  }else{
    Serial.println("Connected to " + String(host));
    String query = String("message=") + message;
    String request = String("") +
                 "POST /api/notify HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Authorization: Bearer " + token + "\r\n" +
                 "Content-Length: " + String(query.length()) +  "\r\n" + 
                 "Content-Type: application/x-www-form-urlencoded\r\n\r\n" +
                  query + "\r\n";
    client.print(request);
    client.println("Connection: close");
    client.println();

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        Serial.println("headers received");
        break;
      }
    }
    // if there are incoming bytes available
    // from the server, read them and print them:
    while (client.available()) {
      char c = client.read();
      Serial.write(c);
    }

    client.stop();
    Serial.println("closing connection");
    Serial.println("");
  }
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