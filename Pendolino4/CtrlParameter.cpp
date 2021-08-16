// 制御パラメータのストレージ クラス

#include <stdint.h>
#include <EEPROM.h>
#include "CtrlParameter.h"

// 他のArduino系ボードとの互換性を重視し、
// ESP32の EEPROM.readFloat() や EEPROM.writeFloat() は使わず
static float eeprom_read_float(int address)
{
    const int size = sizeof(float);
    uint8_t buff[size];
    for(int i=0; i<size; i++){
        buff[i] = EEPROM.read(address + i);
    }
    float value;
    memcpy(&value, buff, size);
    
    return value;
}
static void eeprom_write_float(int address, float value)
{
    const int size = sizeof(float);
    uint8_t buff[size];
    memcpy(buff, &value, size);
    for(int i=0; i<size; i++){
        EEPROM.write(address + i, buff[i]);
    }
}

// コンストラクタ
CtrlParameter::CtrlParameter(int baseAddress)
{
    m_baseAddress = baseAddress;
}

// パラメータ読み出し
void CtrlParameter::read(float& K1, float& K2, float& K3, float& K4, float& theta0)
{
    // オフセットアドレス0にマジックワードが書かれていたら
    // 有効なデータがあると判断して読み出す
    if(EEPROM.read(m_baseAddress+0) == 0xA5)
    {
        K1     = eeprom_read_float(m_baseAddress+4);
        K2     = eeprom_read_float(m_baseAddress+8);
        K3     = eeprom_read_float(m_baseAddress+12);
        K4     = eeprom_read_float(m_baseAddress+16);
        theta0 = eeprom_read_float(m_baseAddress+20);
    }
    // 有効なデータが無いならゼロでリセットし、
    // EEPROMにも保存する
    else
    {
        K1     = 0;
        K2     = 0;
        K3     = 0;
        K4     = 0;
        theta0 = 0;
        this->write(K1, K2, K3, K4, theta0);
    }
}

// パラメータ書き込み
void CtrlParameter::write(float K1, float K2, float K3, float K4, float theta0)
{
    // オフセットアドレス0にマジックワードが書かれていなければ書く
    if(EEPROM.read(m_baseAddress+0) != 0xA5)
    {
        EEPROM.write(m_baseAddress+0, 0xA5);
    }
    // データ書き込み
    eeprom_write_float(m_baseAddress+4,  K1);
    eeprom_write_float(m_baseAddress+8,  K2);
    eeprom_write_float(m_baseAddress+12, K3);
    eeprom_write_float(m_baseAddress+16, K4);
    eeprom_write_float(m_baseAddress+20, theta0);
    
    EEPROM.commit(); // 忘れずに！
}
