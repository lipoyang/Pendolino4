#pragma once

// ALGYAN 6th IoT基板のピン番号
#define PIN_LED1        12
#define PIN_LED2        15
#define PIN_LED3        23
#define PIN_SW2         0
#define PIN_SW3         35
#define PIN_MOTOR_AIN1  4
#define PIN_MOTOR_AIN2  26
#define PIN_MOTOR_BIN1  27
#define PIN_MOTOR_BIN2  25
#define PIN_TEST        13

// PWMのチャンネル番号
#define PWM_MOTOR_AIN1  0
#define PWM_MOTOR_AIN2  1
#define PWM_MOTOR_BIN1  2
#define PWM_MOTOR_BIN2  3

// EEPROMのメモリマップ
#define EEP_IMU_CALIB   0x0000  // IMUセンサのキャリブレーションデータ
#define EEP_CTRL_PARAM  0x0020  // 倒立振子の制御パラメータ
#define EEP_AP_SETTINGS 0x0040  // アクセスポイントの設定 (STAモード用)

// WiFi接続のパスワード (APモードの場合)
// (SSIDは pendolino-xxxxxx となる。xxxxxxの部分はボード固有)
#define MY_PASS     "12345678"

#if 0
// WiFi接続のSSIDとパスワード (STAモードの場合)
#define AP_SSID     "SSID"
#define AP_PASS     "PASSWORD"
#endif

// ALGYAN 6th IoT基板のホスト名
#define MY_HOST_NAME "pendolino4"

// WiFiがAPモードのときCaptieve Portalを有効にする
// (Webアクセスを全て横取りして自分のページを表示させる)
#define USE_CAPTIVE_PORTAL

