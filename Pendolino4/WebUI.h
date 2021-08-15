#pragma once

#include <Arduino.h>
#include <WiFi.h>

// Web UIクラス
class WebUI
{
public:
    // コンストラクタ
    WebUI(int baseAddress);
    // APモードでWiFiを開始する
    void beginAP(char* ssid, char* password, char* hostName);
    // STAモードでWiFiを開始する
    void beginSTA(char* ssid, char* password, char* hostName);
    // ループ処理 (Arduinoタスク の loop()から呼ぶ)
    void loop();
    // データ送信
    void send(char* data);
    // WiFi接続されているか?
    bool isReady();
    // AP設定をEEPROMから読み出す(STAモードで使用)
    bool loadApSettings(char* ssid, char* password);
    // AP設定をEEPROMに書き込む(STAモードで使用)
    void saveApSettings(const char* ssid, const char* password);
    
    // 自分のSSIDとパスワード (APモードで使用)
    char mySSID[33];
    char myPassword[64];
    // APのSSIDとpassword (STAモードで使用)
    char hisSSID[33];
    char hisPassword[64];
    // 自分(Webサーバ)のIPアドレス
    IPAddress localIP;
    // 自分のホスト名
    char hostName[32];
    
private:
    // 周期処理 (APモード)
    void loopAP();
    // 周期処理 (STAモード)
    void loopSTA();
    // APモード(WIFI_AP)かSTAモード(WIFI_STA)か
    int mode;
    // APに接続されたか? (STAモードで使用)
    bool isConnected;
    // EEPROMのベースアドレス
    int m_baseAddress;
};

// 排他制御用
extern SemaphoreHandle_t mutex;
#define _LOCK()      xSemaphoreTake(mutex, portMAX_DELAY)
#define _UNLOCK()    xSemaphoreGive(mutex)
