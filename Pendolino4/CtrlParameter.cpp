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
void CtrlParameter::read(float& ka, float& kb, float& kc, float& kd, float& theta0)
{
    // オフセットアドレス0にマジックワードが書かれていたら
    // 有効なデータがあると判断して読み出す
    if(EEPROM.read(m_baseAddress+0) == 0xA5)
    {
        ka     = eeprom_read_float(m_baseAddress+4);
        kb     = eeprom_read_float(m_baseAddress+8);
        kc     = eeprom_read_float(m_baseAddress+12);
        kd     = eeprom_read_float(m_baseAddress+16);
        theta0 = eeprom_read_float(m_baseAddress+20);
    }
    // 有効なデータが無いならゼロでリセットし、
    // EEPROMにも保存する
    else
    {
        ka     = 0;
        kb     = 0;
        kc     = 0;
        kd     = 0;
        theta0 = 0;
        this->write(ka, kb, kc, kd, theta0);
    }
}

// パラメータ書き込み
void CtrlParameter::write(float ka, float kb, float kc, float kd, float theta0)
{
    // オフセットアドレス0にマジックワードが書かれていなければ書く
    if(EEPROM.read(m_baseAddress+0) != 0xA5)
    {
        EEPROM.write(m_baseAddress+0, 0xA5);
    }
    // データ書き込み
    eeprom_write_float(m_baseAddress+4,  ka);
    eeprom_write_float(m_baseAddress+8,  kb);
    eeprom_write_float(m_baseAddress+12, kc);
    eeprom_write_float(m_baseAddress+16, kd);
    eeprom_write_float(m_baseAddress+20, theta0);
    
    EEPROM.commit(); // 忘れずに！
}
