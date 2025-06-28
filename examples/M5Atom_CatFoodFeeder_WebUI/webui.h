#ifndef WEBUI_H
#define WEBUI_H

// Web UIのHTMLコンテンツ
const char* webui_html = R"rawliteral(
<!DOCTYPE html>
<html lang="ja">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>トレイ動作時刻設定</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }
        
        .container {
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 40px rgba(0,0,0,0.1);
            padding: 40px;
            max-width: 500px;
            width: 100%;
        }
        
        .header {
            text-align: center;
            margin-bottom: 30px;
        }
        
        .header h1 {
            color: #333;
            font-size: 28px;
            margin-bottom: 10px;
        }
        
        .status-info {
            background: #f8f9fa;
            border-radius: 10px;
            padding: 15px;
            margin-bottom: 30px;
        }
        
        .status-row {
            display: flex;
            justify-content: space-between;
            margin-bottom: 8px;
        }
        
        .status-row:last-child {
            margin-bottom: 0;
        }
        
        .status-label {
            font-weight: 600;
            color: #666;
        }
        
        .status-value {
            color: #333;
            font-weight: 500;
        }
        
        .form-group {
            margin-bottom: 25px;
        }
        
        .form-group label {
            display: block;
            margin-bottom: 8px;
            font-weight: 600;
            color: #333;
        }
        
        .form-group input {
            width: 100%;
            padding: 12px 16px;
            border: 2px solid #e1e5e9;
            border-radius: 10px;
            font-size: 16px;
            transition: border-color 0.3s ease;
        }
        
        .form-group input:focus {
            outline: none;
            border-color: #667eea;
        }
        
        .button-group {
            display: flex;
            gap: 15px;
            margin-top: 30px;
        }
        
        .btn {
            flex: 1;
            padding: 15px;
            border: none;
            border-radius: 10px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
        }
        
        .btn-primary {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
        }
        
        .btn-primary:hover {
            transform: translateY(-2px);
            box-shadow: 0 10px 20px rgba(102, 126, 234, 0.3);
        }
        
        .btn-secondary {
            background: #f8f9fa;
            color: #666;
            border: 2px solid #e1e5e9;
        }
        
        .btn-secondary:hover {
            background: #e9ecef;
        }
        
        .message {
            padding: 15px;
            border-radius: 10px;
            margin-bottom: 20px;
            font-weight: 500;
            display: none;
        }
        
        .message.success {
            background: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }
        
        .message.error {
            background: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
        }
        
        @media (max-width: 480px) {
            .container {
                padding: 20px;
            }
            
            .button-group {
                flex-direction: column;
            }
            
            .header h1 {
                font-size: 24px;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🐱 トレイ動作時刻設定</h1>
        </div>
        
        <div class="status-info">
            <div class="status-row">
                <span class="status-label">現在時刻:</span>
                <span class="status-value" id="currentTime">取得中...</span>
            </div>
            <div class="status-row">
                <span class="status-label">WiFi状態:</span>
                <span class="status-value">接続済み</span>
            </div>
            <div class="status-row">
                <span class="status-label">アクセスポイント:</span>
                <span class="status-value" id="wifiSSID">-</span>
            </div>
            <div class="status-row">
                <span class="status-label">IPアドレス:</span>
                <span class="status-value" id="ipAddress">-</span>
            </div>
            <div class="status-row">
                <span class="status-label">トレイ位置:</span>
                <span class="status-value" id="trayPosition">-</span>
            </div>
        </div>
        
        <div id="message" class="message"></div>
        
        <form id="settingsForm">
            <div class="form-group">
                <label for="open1">給餌時刻 1 (時)</label>
                <input type="number" id="open1" name="open1" min="0" max="23" value="5" required>
            </div>
            
            <div class="form-group">
                <label for="open2">給餌時刻 2 (時)</label>
                <input type="number" id="open2" name="open2" min="0" max="23" value="11" required>
            </div>
            
            <div class="form-group">
                <label for="open3">給餌時刻 3 (時)</label>
                <input type="number" id="open3" name="open3" min="0" max="23" value="18" required>
            </div>
            
            <div class="button-group">
                <button type="submit" class="btn btn-primary">設定を保存</button>
                <button type="button" class="btn btn-secondary" onclick="resetForm()">リセット</button>
            </div>
        </form>
    </div>
    
    <script>
        // 動的データを更新（フォームは除く）
        function updateDynamicData() {
            // 時刻を更新
            fetch('/api/time')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('currentTime').textContent = data.time;
                })
                .catch(error => console.error('時刻取得エラー:', error));

            // ステータス情報（トレイ位置など）を更新
            fetch('/api/settings')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('trayPosition').textContent = data.trayPosition;
                    document.getElementById('wifiSSID').textContent = data.wifiSSID || '-';
                    document.getElementById('ipAddress').textContent = data.ipAddress || '-';
                })
                .catch(error => console.error('設定ステータス読み込みエラー:', error));
        }

        // フォームに設定値を読み込む
        function loadFormSettings() {
            fetch('/api/settings')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('open1').value = data.open1;
                    document.getElementById('open2').value = data.open2;
                    document.getElementById('open3').value = data.open3;
                })
                .catch(error => console.error('設定フォーム読み込みエラー:', error));
        }
        
        // メッセージを表示
        function showMessage(text, type) {
            const messageDiv = document.getElementById('message');
            messageDiv.textContent = text;
            messageDiv.className = 'message ' + type;
            messageDiv.style.display = 'block';
            
            setTimeout(() => {
                messageDiv.style.display = 'none';
            }, 3000);
        }
        
        // フォーム送信処理
        document.getElementById('settingsForm').addEventListener('submit', function(e) {
            e.preventDefault();
            
            const formData = new FormData(this);
            const settings = {
                open1: parseInt(formData.get('open1')),
                open2: parseInt(formData.get('open2')),
                open3: parseInt(formData.get('open3'))
            };
            
            fetch('/api/settings', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify(settings)
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    showMessage('設定を保存しました', 'success');
                } else {
                    showMessage('設定の保存に失敗しました', 'error');
                }
            })
            .catch(error => {
                console.error('設定保存エラー:', error);
                showMessage('設定の保存に失敗しました', 'error');
            });
        });
        
        // リセット処理
        function resetForm() {
            // 保存されている最新の値をフォームに再読み込み
            loadFormSettings();
        }

        // 初期化
        document.addEventListener('DOMContentLoaded', function() {
            // ページ読み込み時に一度、すべてのデータを読み込む
            updateDynamicData();
            loadFormSettings();
            
            // 5秒ごとに動的データのみを更新
            setInterval(updateDynamicData, 5000);
        });
    </script>
</body>
</html>
)rawliteral";

#endif // WEBUI_H

