# IoTキャットフードフィーダー Web UI版

## 概要
M5AtomベースのIoTキャットフードフィーダーにWeb UI機能を追加したバージョンです。
従来の固定時刻設定から、Webブラウザから動的に給餌時刻を変更できるようになりました。

## 新機能
- **Web UI設定画面**: ブラウザから給餌時刻を設定可能
- **リアルタイム設定変更**: 設定変更が即座に反映
- **設定値の永続化**: 電源OFF後も設定値を保持
- **レスポンシブデザイン**: スマートフォンからも操作可能
- **状態表示**: 現在時刻、トレイ位置、WiFi状況を表示

## ファイル構成
- `M5Atom_CatFoodFeeder_LineMessagingAPI_WebUI.ino` - メインプログラム
- `webui.h` - Web UIのHTML/CSS/JavaScript（ヘッダファイル）

## 必要なライブラリ
以下のライブラリをArduino IDEでインストールしてください：

### 標準ライブラリ（ESP32コア付属）
- `WiFi.h`
- `WebServer.h`
- `Preferences.h`

### 追加ライブラリ
- `M5Atom` - M5Stack社製
- `ESP32LineMessenger` - LINE Messaging API用
- `ArduinoJson` - JSON処理用

## セットアップ手順

### 1. ライブラリのインストール
Arduino IDEのライブラリマネージャーから以下をインストール：
- M5Atom
- ArduinoJson

ESP32LineMessengerは同梱しています。

### 2. 設定の変更
`M5Atom_CatFoodFeeder_LineMessagingAPI_WebUI.ino`の以下の部分を編集：

```cpp
// Wi-Fi接続設定
const char* ssid       = "<YOUR SSID>";        // WiFiのSSIDに変更
const char* password   = "<YOUR PASSWORD>";    // WiFiのパスワードに変更

// LINEチャネルアクセストークン
const char* accessToken = "<YOUR TOKEN>";      // LINE Messaging APIのトークンに変更
```

### 3. アップロード
M5Atomにプログラムをアップロードします。

## 使用方法

### 1. 初期起動
- M5Atomの電源を入れると、WiFiに接続します
- シリアルモニターでIPアドレスを確認できます
- LINEにもIPアドレスが通知されます

### 2. Web UIへのアクセス
- ブラウザで `http://[M5AtomのIPアドレス]/` にアクセス
- 例: `http://192.168.1.100/`

### 3. 設定変更
- Web UIで3つの給餌時刻を設定（0-23時）
- 「設定を保存」ボタンで変更を保存
- 設定変更はLINEにも通知されます

## API仕様

### GET /api/time
現在時刻を取得
```json
{
  "time": "2025/06/21 15:30:45"
}
```

### GET /api/settings
現在の設定値を取得
```json
{
  "open1": 5,
  "open2": 11,
  "open3": 18,
  "trayPosition": 2
}
```

### POST /api/settings
設定値を更新
```json
{
  "open1": 6,
  "open2": 12,
  "open3": 19
}
```

## 変更点（元のコードからの差分）

### 追加された機能
1. **Webサーバー機能**
   - ESP32のWebServerライブラリを使用
   - ルートハンドラとAPIエンドポイントを追加

2. **設定値の永続化**
   - Preferencesライブラリで設定をフラッシュメモリに保存
   - 電源OFF後も設定値を保持

3. **動的時刻設定**
   - 固定の#defineから変数に変更
   - Web UIから動的に変更可能

4. **Web UI**
   - モダンなレスポンシブデザイン
   - リアルタイム状態表示
   - 直感的な操作インターフェース

### 元のコードとの互換性
- 基本的な動作ロジックは変更なし
- LINE通知機能は維持
- ハードウェア制御部分は変更なし

## トラブルシューティング

### WiFi接続できない場合
- SSID/パスワードが正しいか確認
- 2.4GHz帯のWiFiを使用しているか確認

### Web UIにアクセスできない場合
- M5AtomとPCが同じネットワークにあるか確認
- IPアドレスが正しいか確認
- ファイアウォールの設定を確認

### 設定が保存されない場合
- シリアルモニターでエラーメッセージを確認
- フラッシュメモリの容量を確認

## ライセンス
元のコードと同様のライセンスに従います。

