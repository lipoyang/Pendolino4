#pragma once

// 制御パラメータのストレージ クラス
class CtrlParameter
{
public:
    // コンストラクタ
    CtrlParameter(int baseAddress);
    // パラメータ読み出し
    void read(float& K1, float& K2, float& K3, float& K4, float& theta0);
    // パラメータ書き込み
    void write(float K1, float K2, float K3, float K4, float theta0);

private:
    // EEPROMのベースアドレス
    int m_baseAddress;
};

