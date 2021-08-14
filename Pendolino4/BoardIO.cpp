// ボードI/O (スイッチ・LED) クラス
#include <Arduino.h>
#include "BoardIO.h"
#include "Config.h"

// 外部変数
#include "WebUI.h"
extern WebUI webUI;         // Web UI
extern bool resetTheta;     // 角度推定のリセットフラグ
extern bool resetCtrl;      // 制御則のリセットフラグ
extern bool resetPos;       // 位置推定のリセットフラグ
extern bool isCtrlOn;       // 制御ON/OFFフラグ
extern bool requestCalib;   // キャリブレーション要求フラグ
extern float theta;         // 相補フィルタによる推定姿勢角 θ [deg]
extern float v;             // 推定速度 v (出力電圧に比例すると仮定)
extern float x;             // 推定位置 x [cm]

// SWピン
static const int PIN_SW[SW_MAX]={
    PIN_SW2,
    PIN_SW3
};

// LEDピン
static const int PIN_LED[LED_MAX]={
    PIN_LED1,
    PIN_LED2,
    PIN_LED3
};

// コンストラクタ
BoardIO::BoardIO()
{
    for(int sw=0; sw<SW_MAX; sw++){
        m_edge[sw].setMode(RISING_EDGE);
    }
    for(int led=0; led<LED_MAX; led++){
        m_level[led] = LOW;
        m_blink[led] = false;
    }
}

// 初期化する
void BoardIO::begin()
{
    for(int sw=0; sw<SW_MAX; sw++){
        pinMode(PIN_SW[sw], INPUT);
    }
    for(int led=0; led<LED_MAX; led++){
        pinMode(PIN_LED[led], OUTPUT);
        digitalWrite(PIN_LED[led], LOW);
    }
    pinMode(PIN_TEST, OUTPUT);
    digitalWrite(PIN_TEST, LOW);
}

// メインループで呼ぶ
void BoardIO::loop()
{
#if 1
    // テストピン
    static int toggle = 0;
    toggle = 1 - toggle;
    digitalWrite(PIN_TEST, toggle);
#endif
    
    // SW2押下 → 制御ON/OFF
    if(detectRisingEdge(SW2)){
        if(isCtrlOn){
            isCtrlOn = false;
        }else{
            theta = 0;
            v = 0;
            x = 0;
            resetTheta = true;
            resetCtrl  = true;
            resetPos   = true;
            isCtrlOn   = true;
        }
    }
    // SW3押下 → IMUセンサのキャリブレーション
    if(detectRisingEdge(SW3)){
        requestCalib = true;
    }
    
    // WiFi接続ずみか？
    if(webUI.isReady()){
        // 接続ずみなら緑LEDの点滅を停止して消灯
        if(m_blink[LED_GREEN]){
            m_blink[LED_GREEN] = false;
            turnOff(LED_GREEN);
        }
        // WiFi受信時の緑LEDの1回点滅の消灯
        if(m_timer2[LED_GREEN].elapsed()) turnOff(LED_GREEN);
    }else{
        // 未接続なら緑LED点滅
        if(!m_blink[LED_GREEN]){
            m_blink[LED_GREEN] = true;
            blink(LED_GREEN);
        }
    }
    // 制御ONか？
    if(isCtrlOn){
        // 制御リセット中か？
        if(resetCtrl){
            // 制御リセット中なら黄色LED点滅
            if(!m_blink[LED_YELLOW]){
                m_blink[LED_YELLOW] = true;
                blink(LED_YELLOW);
            }
        }else{
            // 制御動作中なら黄色LED点灯
            m_blink[LED_YELLOW] = false;
            turnOn(LED_YELLOW);
        }
    }else{
        // 制御OFF中なら黄色LEDを消灯
        m_blink[LED_YELLOW] = false;
        turnOff(LED_YELLOW);
    }
    // LEDの点滅処理
    for(int led=0; led<LED_MAX; led++){
        if(m_blink[led]){
            if(m_timer1[led].elapsed()){
                m_level[led] = 1 - m_level[led];
                digitalWrite(PIN_LED[led], m_level[led] );
            }
        }
    }
}

// スイッチが押されているか？
bool BoardIO::pushed(int sw)
{
    return (digitalRead( PIN_SW[sw] ) == HIGH);
}

// スイッチが押されたか？(立ち上がり検出)
bool BoardIO::detectRisingEdge(int sw)
{
    return m_edge[sw].detect(digitalRead( PIN_SW[sw] ));
}

// LEDを点灯する
void BoardIO::turnOn(int led)
{
    digitalWrite(PIN_LED[led], HIGH);
}

// LEDを消灯する
void BoardIO::turnOff(int led)
{
    digitalWrite(PIN_LED[led], LOW);
}

// LEDを点滅する
void BoardIO::blink(int led)
{
    m_level[led] = HIGH;
    digitalWrite(PIN_LED[led], m_level[led]);
    m_blink[led] = true;
    m_timer1[led].set(500);
}

// LEDを1回点滅する
void BoardIO::blinkOnce(int led)
{
    digitalWrite(PIN_LED[led], HIGH);
    m_timer2[led].set(500);
}
