#pragma once

// DCモータの極性
#define MOTOR_NOR   1   // 通常
#define MOTOR_REV   -1  // 反転

// DCモータドライバ (DRV8835) クラス
class DcMotor_DRV8835
{
public:
    // コンストラクタ
    DcMotor_DRV8835(int pinIn1, int pinIn2,
                    int pwmIn1, int pwmIn2,
                    int polarity = MOTOR_NOR);
    // 初期化する
    void begin();
    // 駆動する
    void output(int value);
    
private:
    int m_pinIn1;   // IN1のピン番号
    int m_pinIn2;   // IN2のピン番号
    int m_pwmIn1;   // IN1のPWMチャンネル番号
    int m_pwmIn2;   // IN2のPWMチャンネル番号
    int m_polarity; // 極性(通常/反転)
};
