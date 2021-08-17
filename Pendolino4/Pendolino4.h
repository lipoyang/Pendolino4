#include <stdio.h>
#include <stdint.h>
#include <EEPROM.h>
#include <Wire.h>
#include "PollingTimer.h"
#include "Imu_MPU6050.h"
#include "DcMotor_DRV8835.h"
#include "WebUI.h"
#include "CtrlParameter.h"
#include "BoardIO.h"
#include "Config.h"

// 数値のクリップ
#define CLIP(val, min, max)  ((val < min) ? min : ((val > max) ? max : val))
