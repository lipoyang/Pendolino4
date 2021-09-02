#include <Arduino.h>
#include "DcMotor_DRV8835.h"

#define PWM_FREQUENCY   250     // PWM周波数[Hz]
#define PWM_RESOLUTION  10      // PWM解像度(ビット幅)
#define PWM_MAX         1023    // PWM最大値

// コンストラクタ
// pinIn1: IN1のピン番号
// pinIn2: IN2のピン番号
// pwmIn1: IN1のPWMチャンネル番号
// pwmIn2: IN2のPWMチャンネル番号
// polarity: 極性 (MOTOR_NOR:通常, MOTOR_REV:反転) 省略可
DcMotor_DRV8835::DcMotor_DRV8835(int pinIn1, int pinIn2, 
                                 int pwmIn1, int pwmIn2,
                                 int polarity)
{
    m_pinIn1 = pinIn1;
    m_pinIn2 = pinIn2;
    m_pwmIn1 = pwmIn1;
    m_pwmIn2 = pwmIn2;
    m_polarity = polarity;
}

// 初期化する
void DcMotor_DRV8835::begin()
{
    ledcAttachPin(m_pinIn1, m_pwmIn1);
    ledcAttachPin(m_pinIn2, m_pwmIn2);
    ledcSetup(m_pwmIn1, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcSetup(m_pwmIn2, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcWrite(m_pwmIn1, 0);
    ledcWrite(m_pwmIn2, 0);
}

// 駆動する
// value: 出力値 (-1023 to +1023)
void DcMotor_DRV8835::output(int value)
{
    value = value * m_polarity;
    
    int pwm = abs(value);
    if(pwm<0) pwm = 0;
    if(pwm>PWM_MAX) pwm = PWM_MAX;
    
    if(value > 0){
        ledcWrite(m_pwmIn1, pwm);
        ledcWrite(m_pwmIn2, 1);
    }else if(value < 0){
        ledcWrite(m_pwmIn1, 1);
        ledcWrite(m_pwmIn2, pwm);
    }else{
        ledcWrite(m_pwmIn1, 1);
        ledcWrite(m_pwmIn2, 1);
    }
}
