#include <avr/wdt.h>
#include <Wire.h>
#include <DS3232RTC.h>
//#include <VarSpeedServo.h>  // NOTE: 回転速度が早いと感じたら利用すること
#include "temperature_switch.h"
#include "MyServo.h"
#include "MyDisplay.h"
#include "Dusk2DawnWrap.h"

////////////////////////////////////////////////////////
// 定義
////////////////////////////////////////////////////////

//
// DS3231 リアルタイムクロック
//
// NOTE: DS3231 の I2C アドレスは DS3232RTC.h 内で定義されている。

//
// サーボモーター
//

// 制御ピン
#define SERVO_PIN  10

//
// EEPROM - 設定保存用
//
#define EEPROM_I2C_ADDRESS  0x50

//
// タクトスイッチ
//

// 使用ピン 2,4,7,8,12

// 自動モードピン - 長押しで年月日時刻設定へ遷移する
#define AUTO_MODE_PIN 2

// 温度設定ダウンピン
#define TEMP_DOWN_PIN 4

// 温度設定アップピン
#define TEMP_UP_PIN 7

// 設定モードピン
#define SET_MODE_PIN  8

// 設定モード終了ピン
#define FINISH_SET_MODE_PIN 12

// ボタンの状態
// プルアップ接続なので、押されると 0、離すと 1
#define BUTTON_ON LOW
#define BUTTON_OFF  HIGH

//
// 温度設定
//

// 角度補正の最小・最大
#define MIN_ANGLE_CORRECTION  -9
#define MAX_ANGLE_CORRECTION  9

//
// 日の出・日の入り時刻取得
//

// 日の出から処理開始までの時間(分)の最小・最大
#define MIN_AM_START_SRATIME  0
#define MAX_AM_START_SRATIME  (3 * 60)

//
// 温度指定
//

// 午前の初期温度の最小・最大
#define MIN_AM_START_TEMPERATURE 14
#define MAX_AM_START_TEMPERATURE 28

// 午前の最終温度の最小・最大
#define MIN_AM_END_TEMPERATURE 16
#define MAX_AM_END_TEMPERATURE MAX_TEMPERATURE

// 処理開始から午前の最終温度に達するまでの時間(分)の最小・最大
#define MIN_AM_END_TEMPERATURE_TIME  (0)
#define MAX_AM_END_TEMPERATURE_TIME  (6 * 60)

// 午後温度で追加する温度の最小・最大
#define MIN_PM_PLUS_TEMPERATURE  0
#define MAX_PM_PLUS_TEMPERATURE  5

// 午後温度2で追加する温度の最小・最大
#define MIN_PM_PLUS_TEMPERATURE2  0
#define MAX_PM_PLUS_TEMPERATURE2  5

// 午後温度の開始時刻(分)の最小・最大
#define MIN_PM_PLUS_TEMPERATURE_TIME  (10 * 60)
#define MAX_PM_PLUS_TEMPERATURE_TIME  (19 * 60)

// 午後温度2を開始する日の入り前の時間(分)の最小・最大
#define MIN_PM_PLUS_TEMPERATURE2_SSBTIME  (0)
#define MAX_PM_PLUS_TEMPERATURE2_SSBTIME  (5 * 60)

////////////////////////////////////////////////////////
// 変数
////////////////////////////////////////////////////////

// 設定データ
TemperatureSwitchBag gTSB;

// 変数
TSVariables gTSV;

// DS3231 リアルタイムクロック
DS3232RTC gRtc;

// ディスプレイ
MyDisplay gDisplay(&gTSB, &gTSV);

// サーボモータ
MyServo gServo;

// 日の出・日の入り時刻取得
Dusk2DawnWrap gDusk2DawnWrap;

// 時刻設定モード移行カウンタ
int gSetTimeModeTransCount = 0;

//------------------------------------------------------
// EEPROM への書き込み - from https://playground.arduino.cc/Code/I2CEEPROM/
//------------------------------------------------------

void i2cEepromWriteByte(int deviceaddress, unsigned int eeaddress, byte data)
{
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8)); // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.write(data);
  Wire.endTransmission();
}

//------------------------------------------------------
// EEPROM からの読み込み - from https://playground.arduino.cc/Code/I2CEEPROM/
//------------------------------------------------------

byte i2cEepromReadByte(int deviceaddress, unsigned int eeaddress)
{
  byte rdata = 0xFF;
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8)); // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
  Wire.requestFrom(deviceaddress, 1);
  if (Wire.available()) {
    rdata = Wire.read();
  }
  return rdata;
}

//------------------------------------------------------
// 指定月の日数を返す
//------------------------------------------------------

int getMonthsDays(int year, int month) {
  int days = 0;

  switch (month) {
    case 1: case 3: case 5: case 7: case 8: case 10: case 12:
      days =  31;
      break;
    case 2:
      {
        bool isLeapYear = false;
        if ((year % 4) == 0) {
          isLeapYear = true;
        }
        if ((year % 100) == 0) {
          isLeapYear = false;
        }
        if ((year % 400) == 0) {
          isLeapYear = true;
        }

        days = (isLeapYear) ? 29 : 28;
      }
      break;
    default:  // case 4: case 6: case 9: case 11:
      days = 30;
      break;
  }
  return days;
}

//------------------------------------------------------
// 値を増やして返す
//------------------------------------------------------

int incValue(int orgVal, int maxValue, int addVal = 1)
{
  int result = orgVal + addVal;
  if (maxValue < result) {
    result = maxValue;
  }
  return result;
}

//------------------------------------------------------
// 値を減らして返す
//------------------------------------------------------

int decValue(int orgVal, int minValue, int subVal = 1)
{
  int result = orgVal - subVal;
  if (result < minValue) {
    result = minValue;
  }
  return result;
}

//------------------------------------------------------
// 自動モードの処理
//------------------------------------------------------

void processAutoMode()
{
  int currentTime = gTSV.tm.Hour * 60 + gTSV.tm.Minute;

  // sunriseTime および sunsetTime には日の出時刻および日の入り時刻が、深夜 0 時からの時間(分)で入っている。
  // NOTE: 取得に失敗している場合は共にマイナスの値が設定される。
  if (0 <= gTSV.sunriseTime) {

    int startTime = gTSV.sunriseTime + gTSB.amStartSRATime;
    int endTime = startTime + gTSB.amEndTemperatureTime;

    if (currentTime < gTSV.sunriseTime) {
      gTSV.temperature = gTSB.amEndTemperature;
    } else if (gTSV.sunriseTime <= currentTime && currentTime < startTime) {
      // 朝すぐの時間帯は温度があまり上がらず、かつ換気したいため、ある程度初期温度時間を長くとるために
      // 日の出から処理開始までの期間は初期温度を設定する
      gTSV.temperature = gTSB.amStartTemperature;
    } else {
      int timeSpan = endTime - startTime; // NOTE: 現在の仕様では gTSB.amEndTemperatureTime に等しい
      int tempSpan = gTSB.amEndTemperature - gTSB.amStartTemperature;
      if (0 < tempSpan) {
        int timeRange = timeSpan / tempSpan;
        int timeStep = startTime;
        int setTemperature = gTSB.amStartTemperature;

        while (setTemperature <= gTSB.amEndTemperature) {
          if (timeStep <= currentTime && currentTime < timeStep + timeRange) {
            gTSV.temperature = setTemperature;
            break;
          }
          timeStep += timeRange;
          ++setTemperature;
        }
        if (gTSB.amEndTemperature < setTemperature) {
          gTSV.temperature = gTSB.amEndTemperature;
        }
      } else /* tempSpan == 0 */ {
        gTSV.temperature = gTSB.amEndTemperature;
      }
    }
  }

  if (0 <= gTSV.sunsetTime) {
    if ((currentTime < gTSV.sunriseTime || gTSB.pmPlusTempretureTime <= currentTime) && 0 < gTSB.pmPlusTempreture) {
      // 日の出前または午後温度の開始時刻(分)を過ぎた場合
      int plusTempreture = gTSV.temperature + gTSB.pmPlusTempreture;
      if (MAX_TEMPERATURE < plusTempreture) {
        plusTempreture = MAX_TEMPERATURE;
      }
      gTSV.temperature = plusTempreture;
    }
  }

  if (0 <= gTSV.sunsetTime) {
    int plusStartTime = gTSV.sunsetTime - gTSB.pmPlusTempreture2SSBTime;
    if ((currentTime < gTSV.sunriseTime || plusStartTime <= currentTime) && 0 < gTSB.pmPlusTempreture2) {
      // 日の出前または午後温度2を開始する日の入り前の時間(分)を過ぎた場合
      int plusTempreture = gTSV.temperature + gTSB.pmPlusTempreture2;
      if (MAX_TEMPERATURE < plusTempreture) {
        plusTempreture = MAX_TEMPERATURE;
      }
      gTSV.temperature = plusTempreture;
    }
  }
}

//------------------------------------------------------
// 手動モードの処理
//------------------------------------------------------

void processManualMode(int tempDown, int tempUp)
{
  if (tempDown == BUTTON_ON) {
    // 温度設定を下げる
    gTSV.temperature = decValue(gTSV.temperature, MIN_TEMPERATURE);

  } else if (tempUp == BUTTON_ON) {
    // 温度設定を上げる
    gTSV.temperature = incValue(gTSV.temperature, MAX_TEMPERATURE);
  }
}

//------------------------------------------------------
// 設定モードの処理
//------------------------------------------------------

void processSetMode(int tempDown, int tempUp)
{
  if (tempDown == BUTTON_ON) {
    if (gTSV.setModeKind == SET_AM_START_SRATIME) {
      gTSB.amStartSRATime = decValue(gTSB.amStartSRATime, MIN_AM_START_SRATIME, 10);
    } else if (gTSV.setModeKind == SET_AM_END_TEMPERATURE_TIME) {
      gTSB.amEndTemperatureTime = decValue(gTSB.amEndTemperatureTime, MIN_AM_END_TEMPERATURE_TIME, 10);
    } else if (gTSV.setModeKind == SET_AM_START_TEMPERATURE) {
      gTSB.amStartTemperature = decValue(gTSB.amStartTemperature, MIN_AM_START_TEMPERATURE);
    } else if (gTSV.setModeKind == SET_AM_END_TEMPERATURE) {
      gTSB.amEndTemperature = decValue(gTSB.amEndTemperature, MIN_AM_END_TEMPERATURE);

      // 午前の最終温度設定が午前の初期温度設定より小さくなった場合、午前の初期温度設定を減らす
      if (gTSB.amEndTemperature < gTSB.amStartTemperature) {
        gTSB.amStartTemperature = decValue(gTSB.amStartTemperature, MIN_AM_START_TEMPERATURE);
      }

    } else if (gTSV.setModeKind == SET_ANGLE_CORRECTION) {
      gTSB.angleCorrection = decValue(gTSB.angleCorrection, MIN_ANGLE_CORRECTION);
    } else if (gTSV.setModeKind == SET_PLUS_PM_TEMPERATURE_TIME) {
      gTSB.pmPlusTempretureTime = decValue(gTSB.pmPlusTempretureTime, MIN_PM_PLUS_TEMPERATURE_TIME, 10);
    } else if (gTSV.setModeKind == SET_PLUS_PM_TEMPERATURE) {
      gTSB.pmPlusTempreture = decValue(gTSB.pmPlusTempreture, MIN_PM_PLUS_TEMPERATURE);
    } else if (gTSV.setModeKind == SET_PLUS_END_TEMPERATURE_SSBTIME2) {
      gTSB.pmPlusTempreture2SSBTime = decValue(gTSB.pmPlusTempreture2SSBTime, MIN_PM_PLUS_TEMPERATURE2_SSBTIME, 10);
    } else if (gTSV.setModeKind == SET_PLUS_END_TEMPERATURE2) {
      gTSB.pmPlusTempreture2 = decValue(gTSB.pmPlusTempreture2, MIN_PM_PLUS_TEMPERATURE2);
    }
  } else if (tempUp == BUTTON_ON) {
    if (gTSV.setModeKind == SET_AM_START_SRATIME) {
      gTSB.amStartSRATime = incValue(gTSB.amStartSRATime, MAX_AM_START_SRATIME, 10);
    } else if (gTSV.setModeKind == SET_AM_END_TEMPERATURE_TIME) {
      gTSB.amEndTemperatureTime = incValue(gTSB.amEndTemperatureTime, MAX_AM_END_TEMPERATURE_TIME, 10);
    } else if (gTSV.setModeKind == SET_AM_START_TEMPERATURE) {

      gTSB.amStartTemperature = incValue(gTSB.amStartTemperature, MAX_AM_START_TEMPERATURE);

      if (gTSB.amEndTemperature < gTSB.amStartTemperature) {
        // 午前の初期温度設定が午前の最終温度設定より大きくなった場合、午前の最終温度設定を増やす
        gTSB.amEndTemperature = incValue(gTSB.amEndTemperature, MAX_AM_END_TEMPERATURE);
      }

    } else if (gTSV.setModeKind == SET_AM_END_TEMPERATURE) {
      gTSB.amEndTemperature = incValue(gTSB.amEndTemperature, MAX_AM_END_TEMPERATURE);
    } else if (gTSV.setModeKind == SET_ANGLE_CORRECTION) {
      gTSB.angleCorrection = incValue(gTSB.angleCorrection, MAX_ANGLE_CORRECTION);
    } else if (gTSV.setModeKind == SET_PLUS_PM_TEMPERATURE_TIME) {
      gTSB.pmPlusTempretureTime = incValue(gTSB.pmPlusTempretureTime, MAX_PM_PLUS_TEMPERATURE_TIME, 10);
    } else if (gTSV.setModeKind == SET_PLUS_PM_TEMPERATURE) {
      gTSB.pmPlusTempreture = incValue(gTSB.pmPlusTempreture, MAX_PM_PLUS_TEMPERATURE);
    } else if (gTSV.setModeKind == SET_PLUS_END_TEMPERATURE_SSBTIME2) {
      gTSB.pmPlusTempreture2SSBTime = incValue(gTSB.pmPlusTempreture2SSBTime, MAX_PM_PLUS_TEMPERATURE2_SSBTIME, 10);
    } else if (gTSV.setModeKind == SET_PLUS_END_TEMPERATURE2) {
      gTSB.pmPlusTempreture2 = incValue(gTSB.pmPlusTempreture2, MAX_PM_PLUS_TEMPERATURE2);
    }
  }
}

//------------------------------------------------------
// 時刻設定モードの処理
//------------------------------------------------------

void processSetTimeMode(int tempDown, int tempUp)
{
  if (tempDown == BUTTON_ON) {
    if (gTSV.setTimeModeKind == SET_TIME_YEAR) {
      gTSV.setTm.Year = decValue(gTSV.setTm.Year, 2023 - 1970);
    } else if (gTSV.setTimeModeKind == SET_TIME_MONTH) {
      gTSV.setTm.Month = decValue(gTSV.setTm.Month, 1);
      int days = getMonthsDays(gTSV.setTm.Year + 1970, gTSV.setTm.Month);
      if (days < gTSV.setTm.Day) {
        gTSV.setTm.Day = days;
      }
    } else if (gTSV.setTimeModeKind == SET_TIME_DAY) {
      gTSV.setTm.Day = decValue(gTSV.setTm.Day, 1);
    } else if (gTSV.setTimeModeKind == SET_TIME_HOUR) {
      gTSV.setTm.Hour = decValue(gTSV.setTm.Hour, 0); // Hour 以降は unsigned なので注意
    } else if (gTSV.setTimeModeKind == SET_TIME_MINUTE) {
      gTSV.setTm.Minute = decValue(gTSV.setTm.Minute, 0);
    } else if (gTSV.setTimeModeKind == SET_TIME_SECOND) {
      gTSV.setTm.Second = decValue(gTSV.setTm.Second, 0);
    } else if (gTSV.setTimeModeKind == SET_TIME_OK) {
      gTSV.setTimeOk = !gTSV.setTimeOk;
    }
  } else if (tempUp == BUTTON_ON) {
    if (gTSV.setTimeModeKind == SET_TIME_YEAR) {
      gTSV.setTm.Year = incValue(gTSV.setTm.Year, 2050 - 1970);
    } else if (gTSV.setTimeModeKind == SET_TIME_MONTH) {
      gTSV.setTm.Month = incValue(gTSV.setTm.Month, 12);
      int days = getMonthsDays(gTSV.setTm.Year + 1970, gTSV.setTm.Month);
      if (days < gTSV.setTm.Day) {
        gTSV.setTm.Day = days;
      }
    } else if (gTSV.setTimeModeKind == SET_TIME_DAY) {
      int days = getMonthsDays(gTSV.setTm.Year + 1970, gTSV.setTm.Month);
      gTSV.setTm.Day = incValue(gTSV.setTm.Day, days);
    } else if (gTSV.setTimeModeKind == SET_TIME_HOUR) {
      gTSV.setTm.Hour = incValue(gTSV.setTm.Hour, 23);
    } else if (gTSV.setTimeModeKind == SET_TIME_MINUTE) {
      gTSV.setTm.Minute = incValue(gTSV.setTm.Minute, 59);
    } else if (gTSV.setTimeModeKind == SET_TIME_SECOND) {
      gTSV.setTm.Second = incValue(gTSV.setTm.Second, 59);
    } else if (gTSV.setTimeModeKind == SET_TIME_OK) {
      gTSV.setTimeOk = !gTSV.setTimeOk;
    }
  }
}

//------------------------------------------------------
// 設定読込
//------------------------------------------------------

void loadTemperatureSwitchBag()
{
  byte* pTsb = (byte*) &gTSB;
  for (int i = 0; i < sizeof(gTSB); ++i) {
    pTsb[i] = i2cEepromReadByte(EEPROM_I2C_ADDRESS, i);
  }

  if (gTSB.typeBegin != TSB_TYPE_BEGIN || gTSB.typeEnd != TSB_TYPE_END || gTSB.typeVersion != TSB_TYPE_VERSION) {
    gTSB.typeBegin = TSB_TYPE_BEGIN;
    gTSB.typeVersion = TSB_TYPE_VERSION;
    //
    gTSB.amStartSRATime = (2 * 60);
    gTSB.amEndTemperatureTime = (2 * 60);
    gTSB.amStartTemperature = 20;
    gTSB.amEndTemperature = 28;
    gTSB.angleCorrection = 0;
    gTSB.pmPlusTempretureTime = (12 * 60);
    gTSB.pmPlusTempreture = MIN_PM_PLUS_TEMPERATURE;
    gTSB.pmPlusTempreture2SSBTime = (1 * 60);
    gTSB.pmPlusTempreture2 = MIN_PM_PLUS_TEMPERATURE2;
    //
    gTSB.isManualMode = (gTSV.mode == MANUAL_MODE);
    gTSB.manualTemperature = gTSV.temperature;
    //
    gTSB.typeEnd = TSB_TYPE_END;
  } else {
    if (gTSB.isManualMode) {
      gTSV.mode = MANUAL_MODE;
      gTSV.temperature = gTSB.manualTemperature;
    }
  }
}

//------------------------------------------------------
// 設定書き込み
//------------------------------------------------------

void saveTemperatureSwitchBag()
{
  gTSB.isManualMode = (gTSV.mode == MANUAL_MODE);
  if (gTSB.isManualMode) {
    gTSB.manualTemperature = gTSV.temperature;
  }

  byte* pTsb = (byte*) &gTSB;
  for (int i = 0; i < sizeof(gTSB); ++i) {
    i2cEepromWriteByte(EEPROM_I2C_ADDRESS, i, pTsb[i]);
    delay(5);
  }
}

//------------------------------------------------------
// 初期化
//------------------------------------------------------

void setup()
{
  wdt_enable(WDTO_8S);  // フリーズ対応

  Serial.begin(115200);

  Wire.begin();

  //
  // 設定読込
  //
  loadTemperatureSwitchBag();

  //
  // リアルタイムクロック
  //
  gRtc.begin();

  //
  // ディスプレイ
  //
  gDisplay.init();

  //
  // サーボモータ
  //
  gServo.attach(SERVO_PIN);

  //
  // タクトスイッチ
  //
  pinMode(AUTO_MODE_PIN, INPUT);
  pinMode(TEMP_DOWN_PIN, INPUT);
  pinMode(TEMP_UP_PIN, INPUT);
  pinMode(SET_MODE_PIN, INPUT);
  pinMode(FINISH_SET_MODE_PIN, INPUT);
}

//------------------------------------------------------
// 処理
//------------------------------------------------------

void loop()
{
  // 現在時刻を取得する
  gRtc.read(gTSV.tm);

  // 日の出・日の入り時刻を取得する
  gTSV.sunriseTime = gDusk2DawnWrap.getSunriseTime(gTSV.tm.Year + 1970, gTSV.tm.Month, gTSV.tm.Day);
  gTSV.sunsetTime = gDusk2DawnWrap.getSunsetTime(gTSV.tm.Year + 1970, gTSV.tm.Month, gTSV.tm.Day);

  // タクトスイッチの状態を得る
  int autoMode = digitalRead(AUTO_MODE_PIN);
  int tempDown = digitalRead(TEMP_DOWN_PIN);
  int tempUp = digitalRead(TEMP_UP_PIN);
  int setMode = digitalRead(SET_MODE_PIN);
  int finishSetMode = digitalRead(FINISH_SET_MODE_PIN);

  //
  // タクトスイッチが押されている場合の処理
  //

  if (autoMode == BUTTON_ON) {
    ++gSetTimeModeTransCount;
  } else if (autoMode == BUTTON_OFF) {
    gSetTimeModeTransCount = 0;
  }

  if (10 < gSetTimeModeTransCount) {
    gTSV.mode = SET_TIME_MODE;
    gTSV.setTimeModeKind = MIN_SET_TIME_MODE_KIND;
    gSetTimeModeTransCount = 0;
    gTSV.setTm = gTSV.tm;
    gTSV.setTimeOk = false;
  } else if (autoMode == BUTTON_ON) {
    if (gTSV.mode == MANUAL_MODE) {
      gTSV.mode = AUTO_MODE;
      saveTemperatureSwitchBag();
    }  
  } else if (tempDown == BUTTON_ON || tempUp == BUTTON_ON) {
    if (gTSV.mode == AUTO_MODE) {
      gTSV.mode = MANUAL_MODE;
    }
  } else if (setMode == BUTTON_ON) {
    if (gTSV.mode != SET_TIME_MODE) {
      gTSV.mode = SET_MODE;

      ++gTSV.setModeKind;
      if (gTSV.setModeKind < MIN_SET_MODE_KIND || MAX_SET_MODE_KIND < gTSV.setModeKind) {
        gTSV.setModeKind = MIN_SET_MODE_KIND;
      }

    } else {

      ++gTSV.setTimeModeKind;
      if (gTSV.setTimeModeKind < MIN_SET_TIME_MODE_KIND || MAX_SET_TIME_MODE_KIND < gTSV.setTimeModeKind) {
        gTSV.setTimeModeKind = MIN_SET_TIME_MODE_KIND;
      }
    }
  } else if (finishSetMode == BUTTON_ON) {
    if (gTSV.mode == SET_MODE || gTSV.mode == SET_TIME_MODE) {
      if (gTSV.mode == SET_MODE) {
        gTSV.setModeKind = SET_UNDEFINED;
        saveTemperatureSwitchBag();
      } else if (gTSV.mode == SET_TIME_MODE) {
        gTSV.setTimeModeKind = SET_TIME_UNDEFINED;
        if (gTSV.setTimeOk) {
          gRtc.write(gTSV.setTm);
          gRtc.read(gTSV.tm);
        }
      }

      gTSV.mode = AUTO_MODE;
      gServo.reset(); // 内部変数をリセットしてサーボモーターを動かす
    }
  }

  //
  // 現在のモードにあわせて処理を行う
  //

  if (gTSV.mode == AUTO_MODE) {
    // 自動
    processAutoMode();

  } else if (gTSV.mode == MANUAL_MODE) {
    // 手動
    processManualMode(tempDown, tempUp);
    saveTemperatureSwitchBag();

  } else if (gTSV.mode == SET_MODE) {
    // 設定
    processSetMode(tempDown, tempUp);

  } else if (gTSV.mode == SET_TIME_MODE) {
    // 時刻設定
    processSetTimeMode(tempDown, tempUp);
  }

  // ディスプレイに表示する
  gDisplay.print();

  // サーボへ温度を設定する
  gServo.setTemperature(gTSV.temperature, gTSB.angleCorrection);

  delay(200);

  wdt_reset();
}
