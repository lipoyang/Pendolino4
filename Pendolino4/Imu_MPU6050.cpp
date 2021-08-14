// IMUセンサ (MPU6050/MPU9250/MPU6886) クラス

#include <stdio.h>
#include <Arduino.h>
#include <MPU6050.h>
#include <EEPROM.h>
#include "Imu_MPU6050.h"

// コンストラクタ
Imu_MPU6050::Imu_MPU6050()
{
    // 変数の初期化のみ
    ax0 = 0;
    ay0 = 0;
    az0 = 0;
    gx0 = 0;
    gy0 = 0;
    gz0 = 0;
}

// 初期化する
// ※あらかじめWire.begin() しておくこと
void Imu_MPU6050::begin()
{
    // MPU-6050の初期化
    // 加速度±2G, 角速度±250deg/secをフルスケールとする
    mpu6050.initialize();
}

// センサから6軸のデータを取得する
void Imu_MPU6050::update()
{
    // センサから生データを取得
    mpu6050.getMotion6(
        &ax_raw, &ay_raw, &az_raw,
        &gx_raw, &gy_raw, &gz_raw
    );
    
    // 校正
    ax_cal = ax_raw - ax0;
    ay_cal = ay_raw - ay0;
    az_cal = az_raw - az0;
    gx_cal = gx_raw - gx0;
    gy_cal = gy_raw - gy0;
    gz_cal = gz_raw - gz0;
    
    // 換算 (加速度[G] / 角速度[rad/sec])
    ax = (float)ax_cal * 2.0f / 32768.0f;
    ay = (float)ay_cal * 2.0f / 32768.0f;
    az = (float)az_cal * 2.0f / 32768.0f;
    gx = (float)gx_cal * 250.0f / 32768.0f;
    gy = (float)gy_cal * 250.0f / 32768.0f;
    gz = (float)gz_cal * 250.0f / 32768.0f;
}

// キャリブレーションを開始する
// count: 測定回数
void Imu_MPU6050::beginCalib(int count)
{
    calibMax = count;
    calibCnt = 0;
    ax_sum = 0;
    ay_sum = 0;
    az_sum = 0;
    gx_sum = 0;
    gy_sum = 0;
    gz_sum = 0;
}

// キャリブレーションを実行する
// beginCalib()で設定した回数だけ実行する
// return : 完了したか？
bool Imu_MPU6050::calibrate()
{
    // センサから生データを取得
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
    mpu6050.getMotion6(
        &ax_raw, &ay_raw, &az_raw,
        &gx_raw, &gy_raw, &gz_raw
    );
    
    // 重力加速度をキャンセル
    // ※ XYZのいずれかの軸が鉛直方向に置かれているものとする
    const int16_t ONE_G  = 16384;     // 1G
    const int16_t HALF_G = 16384 / 2; // 0.5G
    if(ax_raw >  HALF_G) ax_raw -= ONE_G;
    if(ax_raw < -HALF_G) ax_raw += ONE_G;
    if(ay_raw >  HALF_G) ay_raw -= ONE_G;
    if(ay_raw < -HALF_G) ay_raw += ONE_G;
    if(az_raw >  HALF_G) az_raw -= ONE_G;
    if(az_raw < -HALF_G) az_raw += ONE_G;
    
    // 生データを累積
    ax_sum += ax_raw;
    ay_sum += ay_raw;
    az_sum += az_raw;
    gx_sum += gx_raw;
    gy_sum += gy_raw;
    gz_sum += gz_raw;
    
    // 設定した測定回数に達したら平均値を計算し、
    // キャリブレーションデータとする
    calibCnt++;
    if(calibCnt == calibMax){
        ax0 = (int16_t)(ax_sum / calibMax);
        ay0 = (int16_t)(ay_sum / calibMax);
        az0 = (int16_t)(az_sum / calibMax);
        gx0 = (int16_t)(gx_sum / calibMax);
        gy0 = (int16_t)(gy_sum / calibMax);
        gz0 = (int16_t)(gz_sum / calibMax);
        return true; // 完了
    }else{
        return false; // 未完了
    }
}

// 他のArduino系ボードとの互換性を重視し、
// ESP32の EEPROM.readShort() や EEPROM.writeShort() は使わず
static int16_t eeprom_read_int16(int addr)
{
    uint16_t val;
    val  = (uint16_t)EEPROM.read(addr) << 8;
    val |= (uint16_t)EEPROM.read(addr+1);
    return (int16_t)val;
}
static void eeprom_write_int16(int addr, int16_t val)
{
    uint8_t hb = (uint8_t)(val >> 8);
    uint8_t lb = (uint8_t)(val & 0xFF);
    EEPROM.write(addr,     hb);
    EEPROM.write(addr + 1, lb);
}

// EEPROMからキャリブレーションデータを読み出す
// ※あらかじめEEPROM.begin()しておくこと
// baseAddress: EEPROMのベースアドレス
void Imu_MPU6050::readCalibData(int baseAddress)
{
    // オフセットアドレス0にマジックワードが書かれていたら
    // 有効なデータがあると判断して読み出す
    if(EEPROM.read(baseAddress+0) == 0xA5)
    {
        ax0 = eeprom_read_int16(baseAddress+2);
        ay0 = eeprom_read_int16(baseAddress+4);
        az0 = eeprom_read_int16(baseAddress+6);
        gx0 = eeprom_read_int16(baseAddress+8);
        gy0 = eeprom_read_int16(baseAddress+10);
        gz0 = eeprom_read_int16(baseAddress+12);
    }
    // 有効なデータが無いならゼロでリセットし、
    // EEPROMにも保存する
    else
    {
        ax0 = ay0 = az0 = 0;
        gx0 = gy0 = gz0 = 0;
        
        writeCalibData(baseAddress);
    }
}

// EEPROMにキャリブレーションデータを書き込む
// ※あらかじめEEPROM.begin()しておくこと
// baseAddress: EEPROMのベースアドレス
void Imu_MPU6050::writeCalibData(int baseAddress)
{
    // オフセットアドレス0にマジックワードが書かれていなければ書く
    if(EEPROM.read(baseAddress+0) != 0xA5)
    {
        EEPROM.write(baseAddress+0, 0xA5);
    }
    // データ書き込み
    eeprom_write_int16(baseAddress+2,  ax0);
    eeprom_write_int16(baseAddress+4,  ay0);
    eeprom_write_int16(baseAddress+6,  az0);
    eeprom_write_int16(baseAddress+8,  gx0);
    eeprom_write_int16(baseAddress+10, gy0);
    eeprom_write_int16(baseAddress+12, gz0);
    
    EEPROM.commit(); // 忘れずに！
}
