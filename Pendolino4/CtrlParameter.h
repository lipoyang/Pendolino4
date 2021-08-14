#pragma once

// 制御パラメータのストレージ クラス
class CtrlParameter
{
public:
    // コンストラクタ
    CtrlParameter(int baseAddress);
    // パラメータ読み出し
    void read(float& ka, float& kb, float& kc, float& kd, float& theta0);
    // パラメータ書き込み
    void write(float ka, float kb, float kc, float kd, float theta0);

private:
    // EEPROMのベースアドレス
    int m_baseAddress;
};

