#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <Arduino.h>
#include <MPU6050.h> // ArduinoのMPU-6050ライブラリに依存

// IMUセンサ (MPU6050/MPU9250/MPU6886) クラス
class Imu_MPU6050
{
public:
    // コンストラクタ
    Imu_MPU6050();
    // 初期化する
    void begin();
    // センサから6軸のデータを取得する
    void update();
    // キャリブレーションを開始する
    void beginCalib(int count);
    // キャリブレーションを実行する
    bool calibrate();
    // EEPROMからキャリブレーションデータを読み出す
    void readCalibData(int baseAddress);
    // EEPROMにキャリブレーションデータを書き込む
    void writeCalibData(int baseAddress);
    
public:
    // 6軸のキャリブレーションデータ(16bit整数)
    int16_t ax0;
    int16_t ay0;
    int16_t az0;
    int16_t gx0;
    int16_t gy0;
    int16_t gz0;

    // 6軸の生データ(16bit整数)
    int16_t ax_raw;
    int16_t ay_raw;
    int16_t az_raw;
    int16_t gx_raw;
    int16_t gy_raw;
    int16_t gz_raw;

    // 6軸の校正ずみデータ(16bit整数)
    int16_t ax_cal;
    int16_t ay_cal;
    int16_t az_cal;
    int16_t gx_cal;
    int16_t gy_cal;
    int16_t gz_cal;

    // 6軸の単位換算データ(加速度[G] / 角速度[rad/sec])
    float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;

private:
    // ArduinoのMPU-6050ライブラリのオブジェクト
    MPU6050 mpu6050;
    
    // キャリブレーション用
    int calibMax;
    int calibCnt;
    int32_t ax_sum;
    int32_t ay_sum;
    int32_t az_sum;
    int32_t gx_sum;
    int32_t gy_sum;
    int32_t gz_sum;
};

