// Web UIクラス
#include <string.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>

#include "WebUI.h"

// 外部変数
#include "CtrlParameter.h"
extern CtrlParameter parameter; // 制御パラメータのストレージ
extern bool resetTheta;     // 角度推定のリセットフラグ
extern bool resetCtrl;      // 制御則のリセットフラグ
extern bool resetPos;       // 位置推定のリセットフラグ
extern bool isCtrlOn;       // 制御ON/OFFフラグ
extern bool requestCalib;   // キャリブレーション要求フラグ
extern float theta;         // 相補フィルタによる推定姿勢角 θ [deg]
extern float v;             // 推定速度 v (出力電圧に比例すると仮定)
extern float x;             // 推定位置 x [cm]
extern float ka, kb, kc, kd;   // 制御則の係数 ka, kb, kc, kd
extern float theta0;           // 姿勢角のオフセット(平衡点の角度)

// 排他制御用
SemaphoreHandle_t mutex;

// Webサーバ
static WebServer webServer(80);
// DNSサーバ
static DNSServer dnsServer;
// WebUI (このクラス)
static WebUI *webUI;

// デフォルトポート番号
//#define DEFAULT_LOCAL_PORT  80

// 無効なIPアドレス
static const IPAddress NULL_ADDR(0,0,0,0);

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
    Serial.println("handleRoot");
    
    // リクエストパラメータがある場合
    if(webServer.args() > 0)
    {
        float val;
        
        // kaの設定
        if(getParameter("K1", val)){
            _LOCK();
            ka = val;
            _UNLOCK();
        }
        // kbの設定
        if(getParameter("K2", val)){
            _LOCK();
            kb = val;
            _UNLOCK();
        }
        // kcの設定
        if(getParameter("K3", val)){
            _LOCK();
            kc = val;
            _UNLOCK();
        }
        // kdの設定
        if(getParameter("K4", val)){
            _LOCK();
            kd = val;
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
                float _k1, _k2, _k3, _k4, _theta0;
                parameter.read(_k1, _k2, _k3, _k4, _theta0);
                _LOCK();
                ka = _k1;
                kb = _k2;
                kc = _k3;
                kd = _k4;
                theta0 = _theta0;
               _UNLOCK();
                Serial.println("Parameters loaded!");
                // 値をブラウザに送る
                String message  = "<?xml version = \"1.0\" ?>";
                       message += "<values>";
                       message += "<K1>" + String(ka) + "</K1>";
                       message += "<K2>" + String(kb) + "</K2>";
                       message += "<K3>" + String(kc) + "</K3>";
                       message += "<K4>" + String(kd) + "</K4>";
                       message += "<theta0>" + String(theta0) + "</theta0>";
                       message += "</values>";
                webServer.sendHeader("Cache-Control", "no-cache");
                webServer.send(200, "text/xml", message);
            }
        }
        // パラメータのセーブ
        if(getParameter("save", val)){
            if(val == 1){
                parameter.write(ka, kb, kc, kd, theta0);
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

// 401 Not Found の処理
static void handleNotFound()
{
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
WebUI::WebUI()
{
    // 排他制御の初期化
    mutex = xSemaphoreCreateMutex();

//    this->localPort = DEFAULT_LOCAL_PORT;
    
//    onRequest = NULL;
//    remoteIP = NULL_ADDR;
}

// APモードでWiFiを開始する
void WebUI::beginAP(char* ssid, char* password, char* hostName)
{
    mode = WIFI_AP;
    
    // SSID文字列
    if(ssid == NULL){
        sprintf(this->mySSID, "pendolino-%06x", ESP.getEfuseMac());
    }else{
        strncpy(mySSID, ssid, sizeof(mySSID)-1);
        mySSID[sizeof(mySSID)-1] = '\0';
    }
    // パスワード文字列
    strncpy(myPassword, password, sizeof(myPassword)-1);
    myPassword[sizeof(myPassword)-1] = '\0';
    
    // APの設定
    WiFi.mode(WIFI_AP);
    WiFi.softAP(mySSID, myPassword);
    localIP = WiFi.softAPIP();
    Serial.println();
    Serial.print("AP SSID: ");Serial.println(mySSID);
    Serial.print("AP IP address: ");Serial.println(localIP);
//    remoteIP = NULL_ADDR;
    
    // DNSサーバ開始
    // Captive Portal (すべてのアクセスを横取りして自分に仕向ける)
    dnsServer.start(53, "*", localIP);
    
    // SPIFFSの開始 (内蔵Flashファイルシステム)
    SPIFFS.begin();
    
    // Webサーバの設定
    webUI = this;
    webServer.on("/", handleRoot);
    webServer.onNotFound(handleRoot); // 全て "/" にリダイレクト
    webServer.begin();
}

// STAモードでWiFiを開始する
void WebUI::beginSTA(char* ssid, char* password, char* hostName)
{
    mode = WIFI_STA;
    isConnected = false;
    
    // SSID文字列
    strncpy(hisSSID, ssid, sizeof(hisSSID)-1);
    hisSSID[sizeof(hisSSID)-1] = '\0';
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
    webUI = this;
    webServer.on("/", handleRoot);
    webServer.onNotFound(handleNotFound);
    webServer.begin();
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
    // DNSサーバの処理
    dnsServer.processNextRequest();
    // Webサーバの処理
    webServer.handleClient();
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
            localIP = WiFi.localIP();
            Serial.print("Connected to "); Serial.println(hisSSID);
            Serial.print("STA IP address: "); Serial.println(localIP);
//            remoteIP = NULL_ADDR;
            
            // mDNSを開始
            if ( MDNS.begin ( hostName ) ) {
                Serial.println ( "MDNS responder started" );
                Serial.print("HostName: ");  Serial.println(hostName);
            }else{
                Serial.println("Error setting up MDNS responder!");
            }
        }
        // Webサーバの処理
        webServer.handleClient();
    }
}

#if 0
// send data
void WebUI::send(char* data)
{
    if(remoteIP != NULL_ADDR){
        udp.beginPacket(remoteIP, remotePort);
        udp.write((uint8_t*)data, strlen(data));
        udp.endPacket();
        //Serial.println(data);
    }
}
#endif

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
