#include "Pendolino4.h"

// 制御周期Δt
const int        DELTA_T_MSEC = 10; //[msec]
const float      DELTA_T      = DELTA_T_MSEC / 1000.0f; //[sec]
const TickType_t DELTA_T_TICK = DELTA_T_MSEC / portTICK_PERIOD_MS; //[tick]
TickType_t       last_time; // 前回の制御周期の時刻[tick]

// 倒立振子制御の変数
float theta_a;          // 加速度による推定姿勢角 θa [deg]
float theta;            // 相補フィルタによる推定姿勢角 θ [deg]
float v;                // 推定速度 v (出力電圧に比例すると仮定)
float x;                // 推定位置 x [cm]
float K1, K2, K3, K4;   // 制御則の係数 K1, K2, K3, K4
float theta0;           // 姿勢角のオフセット(平衡点の角度)
int   t = 0;            // 経過時刻(ログ用) [msec]

// 6軸IMUセンサ
Imu_MPU6050 imu;

// DCモータドライバ
DcMotor_DRV8835 motor_L(PIN_MOTOR_AIN1, PIN_MOTOR_AIN2,
                        PWM_MOTOR_AIN1, PWM_MOTOR_AIN2,
                        MOTOR_NOR);
DcMotor_DRV8835 motor_R(PIN_MOTOR_BIN1, PIN_MOTOR_BIN2,
                        PWM_MOTOR_BIN1, PWM_MOTOR_BIN2,
                        MOTOR_REV);

// 制御パラメータのストレージ
CtrlParameter parameter(EEP_CTRL_PARAM);
// Web UI
WebUI webUI(EEP_AP_SETTINGS);
// スイッチ・LED
BoardIO boardIO;

// フラグ類
bool resetTheta = true;         // 角度推定のリセットフラグ
bool resetCtrl  = true;         // 制御則のリセットフラグ
bool resetPos   = true;         // 位置推定のリセットフラグ
bool isCtrlOn   = true;         // 制御ON/OFFフラグ
bool requestCalib = false;      // キャリブレーション要求フラグ

// カウンタ類
ModuloCounter modulo(10);       // 10回に1回の処理用カウンタ
ModuloCounter runawayCnt(200);  // 暴走カウンタ

// 制御タスク
void ctrl_task();

// 初期化
void setup()
{
    // シリアルポートの初期化
    Serial.begin(115200);
    
    // スイッチとLEDの初期化
    boardIO.begin();
    
    // EEPROMの初期化 (256バイト確保)
    EEPROM.begin(256);
    
    // IMUセンサの初期化
    Wire.begin();
    imu.begin();
    imu.readCalibData(EEP_IMU_CALIB);
    
    // DCモータドライバの初期化
    motor_L.begin();
    motor_R.begin();
    
    // 制御パラメータ読み出し
    parameter.read(K1, K2, K3, K4, theta0);
    
    // WiFiの設定 (起動時にSW3が押されていればAPモード)
    if(boardIO.pushed(SW3)){
        boardIO.turnOn(LED_GREEN);
        
        // APモード
        webUI.beginAP(NULL, MY_PASS, MY_HOST_NAME);
        
        while(boardIO.pushed(SW3)){;}
        boardIO.turnOff(LED_GREEN);
    }else{
        char ssid[33], password[64];
        if(webUI.loadApSettings(ssid, password)){
            // STAモード
            webUI.beginSTA(ssid, password, MY_HOST_NAME);
        }else{
            // 接続するAPが未設定ならAPモード
            webUI.beginAP(NULL, MY_PASS, MY_HOST_NAME);
        }
    }
    
    // 制御周期の初期化
    last_time = xTaskGetTickCount();
   
    // 制御タスクの生成
    xTaskCreatePinnedToCore(
        ctrl_task,
        "ctrl_task",
        4096,   // スタックサイズ
        NULL,   // 引数なし
        12,     // 優先度 0-25 (大きい方が優先度が高い) ※要検討
        NULL,   // ハンドルせず
        1       // コア番号 ※要検討
    );
}

// メインループ
void loop()
{
    // Web UIの処理
    webUI.loop();
}

// 制御タスク
void ctrl_task(void *param)
{
    while(1) // 制御周期Δtでループする
    {
        _LOCK();
        //digitalWrite(PIN_TEST, HIGH);
        
        // スイッチとLEDの処理
        boardIO.loop();
        
        // IMUセンサのキャリブレーション
        if(requestCalib)
        {
            requestCalib = false;
            boardIO.turnOn(LED_RED);
            
            // 100msecごとに50回測定してキャリブレーションデータを保存
            imu.beginCalib(50);
            while(imu.calibrate() == false){
                delay(100);
            }
            imu.writeCalibData(EEP_IMU_CALIB);
            
            boardIO.turnOff(LED_RED);
            
            // 周期タイマをリセット(メインループを止めたので)
            last_time = xTaskGetTickCount();
        }
        //----------------------------------------------- 制御のキモ ここから
        // IMUセンサ更新
        imu.update();
        
        // 加速度による姿勢角推定
        // θa = atan( az / ax )
        float theta_a = atan2(imu.az, imu.ax) * 180 / M_PI;
        
        // 角速度(ジャイロ) ω
        float omega = imu.gy;
        
        // 簡易相補フィルタによる姿勢角推定
        // θ = k * (θ + ωy * Δt) + (1-k) * θa
        if(resetTheta){
            resetTheta = false;
            theta = theta_a; // 初期値 θ = θa
        }else{
            const float k = 0.996; // 相補フィルタ係数
            theta = k*(theta + omega * DELTA_T) + (1 - k)*(theta_a);
        }
        
        // 制御則
        // u = K1*(θ-θ0) + K2*ω + K3*x + K4*v
        float a,b,d,c;
        if(resetCtrl && (abs(theta - theta0) > 1)){
            a = b = c= d = 0;
        }else{
            resetCtrl = false;
            a = K1 * (theta - theta0);
            b = K2 * omega;
            c = K3 * (x / 100);
            d = K4 * (v / 100);
        }
        int u = (int)(a+b+c+d);
        
        // 速度と位置の推定
        v = u;          // 速度は電圧に比例すると仮定
        x += v / 500;   // 実測でおよそcm単位
        if(resetPos && (abs(theta - theta0) <= 1)){
            resetPos = false;
            x = 0;
        }
        
        // モータ出力
        if(isCtrlOn){
            motor_L.output(u * 4); // 従来8bit→10bitに変更
            motor_R.output(u * 4);
        }else{
            motor_L.output(0);
            motor_R.output(0);
        }
        //----------------------------------------------- 制御のキモ ここまで
        
        // 暴走対策
        if( (abs(u) >= 128) && runawayCnt.count() ){
            isCtrlOn = false;
        }
        
        // 10回に1回ログ出力
        if(modulo.count()){
            static char txbuff[256];
            //sprintf(txbuff, "%5.2f %5.2f %5.2f  %7.2f %7.2f %7.2f / %7.2f %7.2f", 
            //    imu.ax, imu.ay, imu.az, imu.gx ,imu.gy , imu.gz, theta_a, theta);
            //Serial.println(txbuff);
            
            if(isCtrlOn){
                sprintf(txbuff, "#d%04X%04X%04X%04X%04X%04X$", 
                    t&0xFFFF,
                    (signed short)((theta - theta0)*100)&0xFFFF,
                    (signed short)(omega*100)&0xFFFF,
                    (signed short)(x*100)&0xFFFF,
                    (signed short)(v*100)&0xFFFF,
                    u&0xFFFF);
            }else{
                sprintf(txbuff, "#d%04X%04X%04X%04X%04X%04X$", 
                    t&0xFFFF,
                    (signed short)(theta*100)&0xFFFF,
                    0,0,0,0);
            }
            //webUI.send(txbuff);
        }
        
        // 時刻経過
        t += DELTA_T_MSEC;
        
        //digitalWrite(PIN_TEST, LOW);
        _UNLOCK();

        // 次の制御周期Δtまで休止
        vTaskDelayUntil( &last_time, DELTA_T_TICK );
    }
}
