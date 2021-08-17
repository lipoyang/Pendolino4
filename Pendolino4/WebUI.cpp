// Web UIクラス
#include <string.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <EEPROM.h>

#include "WebUI.h"

// 送信データサイズ
#define TX_BUFF_SIZE    90

// 外部変数
#include "Config.h"
#include "CtrlParameter.h"
#include "BoardIO.h"
extern CtrlParameter parameter; // 制御パラメータのストレージ
extern BoardIO boardIO;         // スイッチ・LED
extern bool resetTheta;     // 角度推定のリセットフラグ
extern bool resetCtrl;      // 制御則のリセットフラグ
extern bool resetPos;       // 位置推定のリセットフラグ
extern bool isCtrlOn;       // 制御ON/OFFフラグ
extern bool requestCalib;   // キャリブレーション要求フラグ
extern float theta;         // 相補フィルタによる推定姿勢角 θ [deg]
extern float v;             // 推定速度 v (出力電圧に比例すると仮定)
extern float x;             // 推定位置 x [cm]
extern float K1, K2, K3, K4;   // 制御則の係数 K1, K2, K3, K4
extern float theta0;           // 姿勢角のオフセット(平衡点の角度)

// 排他制御用
SemaphoreHandle_t mutex;
// 送信データのキュー
static QueueHandle_t txQueue;

// Webサーバ
static WebServer webServer(80);
// WebSocketサーバ
static WebSocketsServer webSocket(81);
// DNSサーバ
static DNSServer dnsServer;
// WebUI (このクラス)
static WebUI *webUI;

// リクエストパラメータの取得
// name: パラメータ名
// val: 値を返す
// return: 成否
static bool getParameter(const String& name, float& val)
{
    if (!webServer.hasArg(name)) return false;
    val = webServer.arg(name).toFloat();
    return true;
}

// "/" へのアクセスの処理
static void handleRoot()
{
    boardIO.blinkOnce(LED_GREEN);
    //Serial.println("handleRoot");
    
    // リクエストパラメータがある場合
    if(webServer.args() > 0)
    {
        float val;
        
        // kaの設定
        if(getParameter("K1", val)){
            _LOCK();
            K1 = val;
            _UNLOCK();
        }
        // kbの設定
        if(getParameter("K2", val)){
            _LOCK();
            K2 = val;
            _UNLOCK();
        }
        // kcの設定
        if(getParameter("K3", val)){
            _LOCK();
            K3 = val;
            _UNLOCK();
        }
        // kdの設定
        if(getParameter("K4", val)){
            _LOCK();
            K4 = val;
            _UNLOCK();
        }
        // θ0の設定
        if(getParameter("theta0", val)){
            _LOCK();
            theta0 = val;
            _UNLOCK();
        }
        // パラメータのリロード
        if(getParameter("load", val)){
            if(val == 1){
                // リロード
                float _K1, _K2, _K3, _K4, _theta0;
                parameter.read(_K1, _K2, _K3, _K4, _theta0);
                _LOCK();
                K1 = _K1;
                K2 = _K2;
                K3 = _K3;
                K4 = _K4;
                theta0 = _theta0;
                _UNLOCK();
                Serial.println("Parameters loaded!");
                // 値をブラウザに送る
                String message  = "<?xml version = \"1.0\" ?>";
                       message += "<values>";
                       message += "<K1>" + String(K1) + "</K1>";
                       message += "<K2>" + String(K2) + "</K2>";
                       message += "<K3>" + String(K3) + "</K3>";
                       message += "<K4>" + String(K4) + "</K4>";
                       message += "<theta0>" + String(theta0) + "</theta0>";
                       message += "</values>";
                webServer.sendHeader("Cache-Control", "no-cache");
                webServer.send(200, "text/xml", message);
            }
        }
        // パラメータのセーブ
        if(getParameter("save", val)){
            if(val == 1){
                parameter.write(K1, K2, K3, K4, theta0);
                Serial.println("Parameters saved!");
            }
        }
        // 制御モード切替
        if(getParameter("ctrl_on", val)){
            // 制御OFF
            if(val == 0){
                _LOCK();
                isCtrlOn = false;
                _UNLOCK();
            }
            // 制御ON
            if(val == 1){
                _LOCK();
                theta = 0;
                v = 0;
                x = 0;
                resetTheta = true;
                resetCtrl  = true;
                resetPos   = true;
                isCtrlOn   = true;
                _UNLOCK();
            }
        }
        // IMUセンサのキャリブ
        if(getParameter("calib", val)){
            if(val == 1){
                _LOCK();
                requestCalib = true;
                _UNLOCK();
            }
        }
    }
    // リクエストパラメータが無い場合
    else
    {
        // index.htmlをブラウザに送る
        File file = SPIFFS.open("/index.html", FILE_READ);
        size_t sent = webServer.streamFile(file, "text/html");
        file.close();
    }
}

// "/ap_settings.html" へのアクセスの処理
static void handleApSettings()
{
    boardIO.blinkOnce(LED_GREEN);
    Serial.println("handleApSettings");
    
    if (webServer.method() == HTTP_POST)
    {
        String ap_ssid     = webServer.arg("ap_ssid");
        String ap_password = webServer.arg("ap_password");
        webUI->saveApSettings(ap_ssid.c_str(), ap_password.c_str());
        
        Serial.println("AP Settings");
        Serial.println(ap_ssid);
        Serial.println(ap_password);
    }
    // ap_settings.htmlをブラウザに送る
    File file = SPIFFS.open("/ap_settings.html", FILE_READ);
    size_t sent = webServer.streamFile(file, "text/html");
    file.close();
}

// 401 Not Found の処理
static void handleNotFound()
{
    boardIO.blinkOnce(LED_GREEN);
    
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += webServer.uri();
    message += "\nMethod: ";
    message += (webServer.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += webServer.args();
    message += "\n";
    for (uint8_t i = 0; i < webServer.args(); i++) {
        message += " " + webServer.argName(i) + ": " + webServer.arg(i) + "\n";
    }
    webServer.send(404, "text/plain", message);
}

// コンストラクタ
WebUI::WebUI(int baseAddress)
{
    webUI = this;
    
    // EEPROMのベースアドレス
    m_baseAddress = baseAddress;
    
    // 排他制御の初期化
    mutex = xSemaphoreCreateMutex();
    // 送信キューの初期化
    txQueue = xQueueCreate(4, TX_BUFF_SIZE);
}

// APモードでWiFiを開始する
void WebUI::beginAP(char* ssid, char* password, char* hostName)
{
    mode = WIFI_AP;
    
    // SSID文字列
    if(ssid == NULL){
        sprintf(mySSID, "esp32-%06x", ESP.getEfuseMac());
    }else{
        strncpy(mySSID, ssid, sizeof(mySSID)-1);
        mySSID[sizeof(mySSID)-1] = '\0';
    }
    Serial.println("AP MODE");
    Serial.print("AP SSID: "); Serial.println(mySSID);
    
    // パスワード文字列
    strncpy(myPassword, password, sizeof(myPassword)-1);
    myPassword[sizeof(myPassword)-1] = '\0';
    
    // ホスト名文字列
    if(hostName == NULL){
        sprintf(this->hostName, "esp32-%06x", ESP.getEfuseMac());
    }else{
        strncpy(this->hostName, hostName, sizeof(this->hostName)-1);
        this->hostName[sizeof(this->hostName)-1] = '\0';
    }
    Serial.print("HostName: ");  Serial.println(hostName);
    
    // APの設定
    WiFi.mode(WIFI_AP);
    WiFi.softAP(mySSID, myPassword);
    IPAddress localIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");Serial.println(localIP);
    
#ifdef USE_CAPTIVE_PORTAL
    // DNSサーバ開始
    // Captive Portal (すべてのアクセスを横取りして自分に仕向ける)
    dnsServer.start(53, "*", localIP);
#else
    // mDNSを開始
    if ( MDNS.begin ( hostName ) ) {
        Serial.println ( "MDNS responder started" );
        Serial.print("HostName: ");  Serial.println(hostName);
    }else{
        Serial.println("Error setting up MDNS responder!");
    }
#endif
    
    // SPIFFSの開始 (内蔵Flashファイルシステム)
    SPIFFS.begin();
    
    // Webサーバの設定
    webServer.on("/", handleRoot);
    webServer.on("/index.html", handleRoot);
    webServer.on("/ap_settings.html", handleApSettings);
    webServer.onNotFound(handleRoot); // その他全て "/" にリダイレクト
    webServer.begin();
    // WebSocketサーバの開始
    webSocket.begin();
}

// STAモードでWiFiを開始する
void WebUI::beginSTA(char* ssid, char* password, char* hostName)
{
    mode = WIFI_STA;
    isConnected = false;
    
    // SSID文字列
    strncpy(hisSSID, ssid, sizeof(hisSSID)-1);
    hisSSID[sizeof(hisSSID)-1] = '\0';
    Serial.println("STA MODE");
    Serial.print("AP SSID: ");Serial.println(hisSSID);
    
    // パスワード文字列
    strncpy(hisPassword, password, sizeof(hisPassword)-1);
    hisPassword[sizeof(hisPassword)-1] = '\0';
    
    // ホスト名文字列
    if(hostName == NULL){
        sprintf(this->hostName, "esp32-%06x", ESP.getEfuseMac());
    }else{
        strncpy(this->hostName, hostName, sizeof(this->hostName)-1);
        this->hostName[sizeof(this->hostName)-1] = '\0';
    }
    Serial.print("HostName: ");  Serial.println(hostName);
    
    // STAの設定
    WiFi.mode(WIFI_STA);
    WiFi.begin(hisSSID, hisPassword);
    
    // SPIFFSの開始 (内蔵Flashファイルシステム)
    SPIFFS.begin();
    
    // Webサーバの設定
    webServer.on("/", handleRoot);
    webServer.on("/index.html", handleRoot);
    webServer.on("/ap_settings.html", handleApSettings);
    webServer.onNotFound(handleNotFound);
    webServer.begin();
    // WebSocketサーバの開始
    webSocket.begin();
}

// ループ処理 (Arduinoタスク の loop()から呼ぶ)
void WebUI::loop()
{
    if(mode == WIFI_AP)
    {
        loopAP();
    }else{
        loopSTA();
    }
}

// 周期処理 (APモード)
void WebUI::loopAP()
{
#ifdef USE_CAPTIVE_PORTAL
    // DNSサーバの処理
    dnsServer.processNextRequest();
#endif
    // Webサーバの処理
    webServer.handleClient();
    // WebSocketサーバの処理
    webSocket.loop();
    static char txbuff[TX_BUFF_SIZE];
    BaseType_t result = xQueueReceive(txQueue, txbuff, 0);
    if ( result == pdTRUE)
    {
        webSocket.broadcastTXT(txbuff, strlen(txbuff));
    }
}

// 周期処理 (STAモード)
void WebUI::loopSTA()
{
    // APに接続されていないとき
    if (WiFi.status() != WL_CONNECTED) {
        if(isConnected){
            isConnected = false;
        }
        delay(500);
        Serial.print(".");
    }
    // APに接続されているとき
    else
    {
        // 接続時の処理
        if(!isConnected){
            isConnected = true;
            
            // 自分のIPアドレスを取得
            IPAddress localIP = WiFi.localIP();
            Serial.print("Connected to "); Serial.println(hisSSID);
            Serial.print("STA IP address: "); Serial.println(localIP);
            
            // mDNSを開始
            if ( MDNS.begin ( hostName ) ) {
                Serial.println ( "mDNS responder started" );
                Serial.print("HostName: ");  Serial.println(hostName);
            }else{
                Serial.println("Error: setting up MDNS responder!");
            }
        }
        // Webサーバの処理
        webServer.handleClient();
        // WebSocketサーバの処理
        webSocket.loop();
        static char txbuff[TX_BUFF_SIZE];
        BaseType_t result = xQueueReceive(txQueue, txbuff, 0);
        if ( result == pdTRUE)
        {
            webSocket.broadcastTXT(txbuff, strlen(txbuff));
        }
    }
}

// データを送信する
void WebUI::send(char* data)
{
    // STAモードでAPに未接続なら送信しない
    if((mode == WIFI_STA) && !isConnected) return;
    
    BaseType_t result = xQueueSendToBack(txQueue, data, 0);
    
    if(result != pdTRUE){
        Serial.println("Error: TX Queue is Busy!");
    }
}

// APに接続されたか? (STAモードで使用)
bool WebUI::isReady()
{
    if(mode == WIFI_AP)
    {
        return true;
    }else{
        return isConnected;
    }
}

// AP設定をEEPROMから読み出す(STAモードで使用)
// return: 有効な設定を読み出せたか？
bool WebUI::loadApSettings(char* ssid, char* password)
{
    // オフセットアドレス0にマジックワードが書かれていたら
    // 有効なデータがあると判断して読み出す
    if(EEPROM.read(m_baseAddress+0) == 0xA5)
    {
        for(int i=0; i<32; i++){
            ssid[i] = EEPROM.read(m_baseAddress + 1 + i);
            if(ssid[i] == 0) break;
        }
        ssid[32] = 0;
        
        for(int i=0; i<63; i++){
            password[i] = EEPROM.read(m_baseAddress + 1 + 33 + i);
            if(password[i] == 0) break;
        }
        password[63] = 0;
        
        return true;
    }
    else
    {
        return false;
    }
}

// AP設定をEEPROMに書き込む(STAモードで使用)
void WebUI::saveApSettings(const char* ssid, const char* password)
{
    // オフセットアドレス0にマジックワードが書かれていなければ書く
    if(EEPROM.read(m_baseAddress+0) != 0xA5)
    {
        EEPROM.write(m_baseAddress+0, 0xA5);
    }
    // データ書き込み
    for(int i=0; i<33; i++){
        EEPROM.write(m_baseAddress + 1 + i, ssid[i]);
    }
    for(int i=0; i<64; i++){
        EEPROM.write(m_baseAddress + 1 + 33 + i, password[i]);
    }
    
    EEPROM.commit(); // 忘れずに！
}
