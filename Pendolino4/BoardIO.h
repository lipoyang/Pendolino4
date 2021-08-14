#pragma once

#include "PollingTimer.h"

// スイッチ名
enum{
    SW2 = 0,
    SW3,
    SW_MAX // スイッチの数(リセットスイッチ以外)
};

// LED名
enum{
    LED_RED = 0,
    LED_YELLOW,
    LED_GREEN,
    LED_MAX     // LEDの数
};

// ボードI/O (スイッチ・LED) クラス
class BoardIO
{
public:
    // コンストラクタ
    BoardIO();
    // 初期化する
    void begin();
    // メインループで呼ぶ
    void loop();
    
    // スイッチが押されているか？
    bool pushed(int sw);
    // スイッチが押されたか？(立ち上がり検出)
    bool detectRisingEdge(int sw);
    
    // LEDを点灯する
    void turnOn(int led);
    // LEDを消灯する
    void turnOff(int led);
    // LEDを点滅する
    void blink(int led);
    // LEDを1回点滅する
    void blinkOnce(int led);
    
private:
    // スイッチ用
    EdgeDetector m_edge[SW_MAX];     // 立ち上がりエッジ検出
    
    // LED用
    IntervalTimer m_timer1[LED_MAX]; // 点滅用周期タイマ
    OneShotTimer  m_timer2[LED_MAX]; // 1回点滅用タイマ
    int  m_level[LED_MAX];           // 出力レベル HIGH(1) or LOW(0)
    bool m_blink[LED_MAX];           // 点滅フラグ
};
