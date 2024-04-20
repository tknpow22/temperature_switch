#include <avr/wdt.h>
#include <Wire.h>
#include <DS3232RTC.h>
#include <TimeLib.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
//#include <VarSpeedServo.h>  // NOTE: 回転速度が早いと感じたら利用すること
#include <Dusk2Dawn.h>
#include "temperature_switch.h"
#include "MyServo.h"

////////////////////////////////////////////////////////
// 定義
////////////////////////////////////////////////////////

//
// DS3231 リアルタイムクロック
//
// NOTE: DS3231 の I2C アドレスは DS3232RTC.h 内で定義されている。

//
// LCD
//
#define LCD_I2C_ADDRESS 0x27
#define LCD_COLS 20
#define LCD_ROWS 4
#define LCD_VIR_ROWS  5 // バッファ上の表示行

#define LCD_COLS_BUFFER_SIZE (LCD_COLS + 10)

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
// モード
//
#define AUTO_MODE 0   // 自動
#define MANUAL_MODE 1 // 手動
#define SET_MODE  2   // 設定
#define SET_TIME_MODE 3 // 時刻設定

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

// 稼働地点の緯度・経度
#define Longitude 133.8416  // 経度
#define Latitude  33.5087   // 緯度

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

//
// 設定変更
//

// 設定種別
#define SET_UNDEFINED (-1)  // 未定義
#define SET_AM_START_TEMPERATURE 0 // 午前の初期温度
#define SET_AM_END_TEMPERATURE 1 // 午前の最終温度
#define SET_PLUS_PM_TEMPERATURE_TIME  2 // 午後温度の開始時刻(分)
#define SET_PLUS_PM_TEMPERATURE  3 // 午後温度での追加温度
#define SET_PLUS_END_TEMPERATURE_SSBTIME2  4 // 午後温度2を開始する日の入り前の時間(分)
#define SET_PLUS_END_TEMPERATURE2  5 // 午後温度2での追加温度
#define SET_AM_START_SRATIME  6 // 日の出から処理開始までの時間(分)
#define SET_AM_END_TEMPERATURE_TIME  7 // 処理開始から午前の最終温度に達するまでの時間(分)
#define SET_ANGLE_CORRECTION  8 // 角度補正
#define MIN_SET_MODE_KIND SET_AM_START_TEMPERATURE  // 設定種別の最小値
#define MAX_SET_MODE_KIND SET_ANGLE_CORRECTION  // 設定種別の最大値

//
// 時刻設定
//

// 時刻設定種別
#define SET_TIME_UNDEFINED  (-1)  // 未定義
#define SET_TIME_YEAR 0 // 年
#define SET_TIME_MONTH  1 // 月
#define SET_TIME_DAY  2 // 日
#define SET_TIME_HOUR 3 // 時
#define SET_TIME_MINUTE 4 // 分
#define SET_TIME_SECOND  5 // 秒
#define SET_TIME_OK 6 // 確認

#define MIN_SET_TIME_MODE_KIND  SET_TIME_YEAR
#define MAX_SET_TIME_MODE_KIND  SET_TIME_OK

//
// 設定データ
//
#define TSB_TYPE_BEGIN  'T'
#define TSB_TYPE_END  'S'
#define TSB_TYPE_VERSION  3

struct TemperatureSwitchBag {
  char typeBegin;
  int typeVersion;
  int amStartSRATime; // 日の出から処理開始までの時間(分)  
  int amEndTemperatureTime;  // 処理開始から午前の最終温度に達するまでの時間(分)
  int amStartTemperature;  // 午前の初期温度
  int amEndTemperature;  // 午前の最終温度
  int angleCorrection; // 角度補正
  int pmPlusTempretureTime; // 午後温度の開始時刻(分)
  int pmPlusTempreture; // 午後温度での追加温度
  int pmPlusTempreture2SSBTime;  // 午後温度2を開始する日の入り前の時間(分)
  int pmPlusTempreture2; // 午後温度2での追加温度
  // 現在の設定を覚える
  bool isManualMode; // 手動時か否か
  int manualTemperature;  // 手動時の温度設定
  //
  char typeEnd;
};

////////////////////////////////////////////////////////
// 変数
////////////////////////////////////////////////////////

// DS3231 リアルタイムクロック
DS3232RTC gRtc;

// 1602 LCD
LiquidCrystal_I2C gLcd(LCD_I2C_ADDRESS/*I2Cスーレブアドレス*/, LCD_COLS/*COLS*/, LCD_ROWS/*ROWS*/);

// サーボモータ
MyServo gServo;

// 設定データ
TemperatureSwitchBag gTSB;

// モード
int gMode = AUTO_MODE;

// 日の出・日の入り時刻取得
Dusk2Dawn gDusk2Dawn(Latitude, Longitude, 9/*timezone*/);

// 現在の温度設定
int gTemperature = MAX_TEMPERATURE;

// Dusk2Dawn を何度も呼び出さないために、日付、日の出・日の入り時刻をキャッシュする
unsigned long gPrevSunriseYMD = 0;
int gSunriseTime = -1;
unsigned long gPrevSunsetYMD = 0;
int gSunsetTime = -1;

// 設定変更
int gSetModeKind = SET_UNDEFINED;

// 時刻設定種別
int gSetTimeModeKind = SET_TIME_UNDEFINED;

// 時刻設定モード移行カウンタ
int gSetTimeModeTransCount = 0;

// 時刻設定用変数
tmElements_t gSetTm;
bool gSetTimeOk = false;

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
// 日の出時刻(分)を得る
//------------------------------------------------------

int getSunriseTime(int year, int month, int day)
{
  unsigned long ymd = ((unsigned long)year * 10000) + ((unsigned long)month * 100) + (unsigned long)day;

  if (gPrevSunriseYMD == ymd && 0 <= gSunriseTime) {
    return gSunriseTime;
  }

  int sunriseTime = gDusk2Dawn.sunrise(year, month, day, false/*サマータイム*/);
  gSunriseTime = sunriseTime;
  gPrevSunriseYMD = ymd;

  return sunriseTime;
}

//------------------------------------------------------
// 日の入り時刻(分)を得る
//------------------------------------------------------

int getSunsetTime(int year, int month, int day)
{
  unsigned long ymd = ((unsigned long)year * 10000) + ((unsigned long)month * 100) + (unsigned long)day;

  if (gPrevSunsetYMD == ymd && 0 <= gSunsetTime) {
    return gSunsetTime;
  }

  int sunsetTime = gDusk2Dawn.sunset(year, month, day, false/*サマータイム*/);
  gSunsetTime = sunsetTime;
  gPrevSunsetYMD = ymd;

  return sunsetTime;
}

//------------------------------------------------------
// 自動モードの処理
//------------------------------------------------------

void processAutoMode(const tmElements_t& tm, int sunriseTime, int sunsetTime)
{
  int currentTime = tm.Hour * 60 + tm.Minute;

  // sunriseTime および sunsetTime には日の出時刻および日の入り時刻が、深夜 0 時からの時間(分)で入っている。
  // NOTE: 取得に失敗している場合は共にマイナスの値が設定される。
  if (0 <= sunriseTime) {

    int startTime = sunriseTime + gTSB.amStartSRATime;
    int endTime = startTime + gTSB.amEndTemperatureTime;

    if (currentTime < sunriseTime) {
      gTemperature = gTSB.amEndTemperature;
    } else if (sunriseTime <= currentTime && currentTime < startTime) {
      // 朝すぐの時間帯は温度があまり上がらず、かつ換気したいため、ある程度初期温度時間を長くとるために
      // 日の出から処理開始までの期間は初期温度を設定する
      gTemperature = gTSB.amStartTemperature;
    } else {
      int timeSpan = endTime - startTime; // NOTE: 現在の仕様では gTSB.amEndTemperatureTime に等しい
      int tempSpan = gTSB.amEndTemperature - gTSB.amStartTemperature;
      if (0 < tempSpan) {
        int timeRange = timeSpan / tempSpan;
        int timeStep = startTime;
        int setTemperature = gTSB.amStartTemperature;

        while (setTemperature <= gTSB.amEndTemperature) {
          if (timeStep <= currentTime && currentTime < timeStep + timeRange) {
            gTemperature = setTemperature;
            break;
          }
          timeStep += timeRange;
          ++setTemperature;
        }
        if (gTSB.amEndTemperature < setTemperature) {
          gTemperature = gTSB.amEndTemperature;
        }
      } else /* tempSpan == 0 */ {
        gTemperature = gTSB.amEndTemperature;
      }
    }
  }

  if (0 <= sunsetTime) {
    if ((currentTime < sunriseTime || gTSB.pmPlusTempretureTime <= currentTime) && 0 < gTSB.pmPlusTempreture) {
      // 日の出前または午後温度の開始時刻(分)を過ぎた場合
      int plusTempreture = gTemperature + gTSB.pmPlusTempreture;
      if (MAX_TEMPERATURE < plusTempreture) {
        plusTempreture = MAX_TEMPERATURE;
      }
      gTemperature = plusTempreture;
    }
  }

  if (0 <= sunsetTime) {
    int plusStartTime = sunsetTime - gTSB.pmPlusTempreture2SSBTime;
    if ((currentTime < sunriseTime || plusStartTime <= currentTime) && 0 < gTSB.pmPlusTempreture2) {
      // 日の出前または午後温度2を開始する日の入り前の時間(分)を過ぎた場合
      int plusTempreture = gTemperature + gTSB.pmPlusTempreture2;
      if (MAX_TEMPERATURE < plusTempreture) {
        plusTempreture = MAX_TEMPERATURE;
      }
      gTemperature = plusTempreture;
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
    gTemperature = decValue(gTemperature, MIN_TEMPERATURE);

  } else if (tempUp == BUTTON_ON) {
    // 温度設定を上げる
    gTemperature = incValue(gTemperature, MAX_TEMPERATURE);
  }
}

//------------------------------------------------------
// 設定モードの処理
//------------------------------------------------------

void processSetMode(int tempDown, int tempUp)
{
  if (tempDown == BUTTON_ON) {
    if (gSetModeKind == SET_AM_START_SRATIME) {
      gTSB.amStartSRATime = decValue(gTSB.amStartSRATime, MIN_AM_START_SRATIME, 10);
    } else if (gSetModeKind == SET_AM_END_TEMPERATURE_TIME) {
      gTSB.amEndTemperatureTime = decValue(gTSB.amEndTemperatureTime, MIN_AM_END_TEMPERATURE_TIME, 10);
    } else if (gSetModeKind == SET_AM_START_TEMPERATURE) {
      gTSB.amStartTemperature = decValue(gTSB.amStartTemperature, MIN_AM_START_TEMPERATURE);
    } else if (gSetModeKind == SET_AM_END_TEMPERATURE) {
      gTSB.amEndTemperature = decValue(gTSB.amEndTemperature, MIN_AM_END_TEMPERATURE);

      // 午前の最終温度設定が午前の初期温度設定より小さくなった場合、午前の初期温度設定を減らす
      if (gTSB.amEndTemperature < gTSB.amStartTemperature) {
        gTSB.amStartTemperature = decValue(gTSB.amStartTemperature, MIN_AM_START_TEMPERATURE);
      }

    } else if (gSetModeKind == SET_ANGLE_CORRECTION) {
      gTSB.angleCorrection = decValue(gTSB.angleCorrection, MIN_ANGLE_CORRECTION);
    } else if (gSetModeKind == SET_PLUS_PM_TEMPERATURE_TIME) {
      gTSB.pmPlusTempretureTime = decValue(gTSB.pmPlusTempretureTime, MIN_PM_PLUS_TEMPERATURE_TIME, 10);
    } else if (gSetModeKind == SET_PLUS_PM_TEMPERATURE) {
      gTSB.pmPlusTempreture = decValue(gTSB.pmPlusTempreture, MIN_PM_PLUS_TEMPERATURE);
    } else if (gSetModeKind == SET_PLUS_END_TEMPERATURE_SSBTIME2) {
      gTSB.pmPlusTempreture2SSBTime = decValue(gTSB.pmPlusTempreture2SSBTime, MIN_PM_PLUS_TEMPERATURE2_SSBTIME, 10);
    } else if (gSetModeKind == SET_PLUS_END_TEMPERATURE2) {
      gTSB.pmPlusTempreture2 = decValue(gTSB.pmPlusTempreture2, MIN_PM_PLUS_TEMPERATURE2);
    }
  } else if (tempUp == BUTTON_ON) {
    if (gSetModeKind == SET_AM_START_SRATIME) {
      gTSB.amStartSRATime = incValue(gTSB.amStartSRATime, MAX_AM_START_SRATIME, 10);
    } else if (gSetModeKind == SET_AM_END_TEMPERATURE_TIME) {
      gTSB.amEndTemperatureTime = incValue(gTSB.amEndTemperatureTime, MAX_AM_END_TEMPERATURE_TIME, 10);
    } else if (gSetModeKind == SET_AM_START_TEMPERATURE) {

      gTSB.amStartTemperature = incValue(gTSB.amStartTemperature, MAX_AM_START_TEMPERATURE);

      if (gTSB.amEndTemperature < gTSB.amStartTemperature) {
        // 午前の初期温度設定が午前の最終温度設定より大きくなった場合、午前の最終温度設定を増やす
        gTSB.amEndTemperature = incValue(gTSB.amEndTemperature, MAX_AM_END_TEMPERATURE);
      }

    } else if (gSetModeKind == SET_AM_END_TEMPERATURE) {
      gTSB.amEndTemperature = incValue(gTSB.amEndTemperature, MAX_AM_END_TEMPERATURE);
    } else if (gSetModeKind == SET_ANGLE_CORRECTION) {
      gTSB.angleCorrection = incValue(gTSB.angleCorrection, MAX_ANGLE_CORRECTION);
    } else if (gSetModeKind == SET_PLUS_PM_TEMPERATURE_TIME) {
      gTSB.pmPlusTempretureTime = incValue(gTSB.pmPlusTempretureTime, MAX_PM_PLUS_TEMPERATURE_TIME, 10);
    } else if (gSetModeKind == SET_PLUS_PM_TEMPERATURE) {
      gTSB.pmPlusTempreture = incValue(gTSB.pmPlusTempreture, MAX_PM_PLUS_TEMPERATURE);
    } else if (gSetModeKind == SET_PLUS_END_TEMPERATURE_SSBTIME2) {
      gTSB.pmPlusTempreture2SSBTime = incValue(gTSB.pmPlusTempreture2SSBTime, MAX_PM_PLUS_TEMPERATURE2_SSBTIME, 10);
    } else if (gSetModeKind == SET_PLUS_END_TEMPERATURE2) {
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
    if (gSetTimeModeKind == SET_TIME_YEAR) {
      gSetTm.Year = decValue(gSetTm.Year, 2023 - 1970);
    } else if (gSetTimeModeKind == SET_TIME_MONTH) {
      gSetTm.Month = decValue(gSetTm.Month, 1);
      int days = getMonthsDays(gSetTm.Year + 1970, gSetTm.Month);
      if (days < gSetTm.Day) {
        gSetTm.Day = days;
      }
    } else if (gSetTimeModeKind == SET_TIME_DAY) {
      gSetTm.Day = decValue(gSetTm.Day, 1);
    } else if (gSetTimeModeKind == SET_TIME_HOUR) {
      gSetTm.Hour = decValue(gSetTm.Hour, 0); // Hour 以降は unsigned なので注意
    } else if (gSetTimeModeKind == SET_TIME_MINUTE) {
      gSetTm.Minute = decValue(gSetTm.Minute, 0);
    } else if (gSetTimeModeKind == SET_TIME_SECOND) {
      gSetTm.Second = decValue(gSetTm.Second, 0);
    } else if (gSetTimeModeKind == SET_TIME_OK) {
      gSetTimeOk = !gSetTimeOk;
    }
  } else if (tempUp == BUTTON_ON) {
    if (gSetTimeModeKind == SET_TIME_YEAR) {
      gSetTm.Year = incValue(gSetTm.Year, 2050 - 1970);
    } else if (gSetTimeModeKind == SET_TIME_MONTH) {
      gSetTm.Month = incValue(gSetTm.Month, 12);
      int days = getMonthsDays(gSetTm.Year + 1970, gSetTm.Month);
      if (days < gSetTm.Day) {
        gSetTm.Day = days;
      }
    } else if (gSetTimeModeKind == SET_TIME_DAY) {
      int days = getMonthsDays(gSetTm.Year + 1970, gSetTm.Month);
      gSetTm.Day = incValue(gSetTm.Day, days);
    } else if (gSetTimeModeKind == SET_TIME_HOUR) {
      gSetTm.Hour = incValue(gSetTm.Hour, 23);
    } else if (gSetTimeModeKind == SET_TIME_MINUTE) {
      gSetTm.Minute = incValue(gSetTm.Minute, 59);
    } else if (gSetTimeModeKind == SET_TIME_SECOND) {
      gSetTm.Second = incValue(gSetTm.Second, 59);
    } else if (gSetTimeModeKind == SET_TIME_OK) {
      gSetTimeOk = !gSetTimeOk;
    }
  }
}

//------------------------------------------------------
// 表示のための文字列の処理
//------------------------------------------------------

void processDisplayStrings(char lcdLines[LCD_VIR_ROWS][LCD_COLS_BUFFER_SIZE], const tmElements_t& tm, int sunriseTime, int sunsetTime)
{
  // 例: "2023/11/13 15:15 A29"
  sprintf(lcdLines[0], "%04d/%02d/%02d %02d:%02d %c%02d",
      tm.Year + 1970,
      tm.Month,
      tm.Day,
      tm.Hour,
      tm.Minute,
      (gMode == AUTO_MODE) ? 'A' : (gMode == SET_MODE) ? 'X' : 'M',
      gTemperature
    );

  // 例: "R0603 S1719 S11 E32 "
  sprintf(lcdLines[1], "R%02d%02d S%02d%02d %c%02d %c%02d ",
      (0 <= sunriseTime) ? (sunriseTime / 60) : 99,
      (0 <= sunriseTime) ? (sunriseTime % 60) : 99,
      (0 <= sunsetTime) ? (sunsetTime / 60) : 99,
      (0 <= sunsetTime) ? (sunsetTime % 60) : 99,
      (gMode == SET_MODE && gSetModeKind == SET_AM_START_TEMPERATURE) ? 's' : 'S',
      gTSB.amStartTemperature,
      (gMode == SET_MODE && gSetModeKind == SET_AM_END_TEMPERATURE) ? 'e' : 'E',
      gTSB.amEndTemperature
    );

  // 例: "P1229A0 B14-1539A0  "
  {
    int plusStartTime2 = sunsetTime - gTSB.pmPlusTempreture2SSBTime;
    sprintf(lcdLines[2], "%c%02d%02d%c%01d %c%01d%01d-%02d%02d%c%01d  ",
        (gMode == SET_MODE && gSetModeKind == SET_PLUS_PM_TEMPERATURE_TIME) ? 'p' : 'P',
        gTSB.pmPlusTempretureTime / 60,
        gTSB.pmPlusTempretureTime % 60,
        (gMode == SET_MODE && gSetModeKind == SET_PLUS_PM_TEMPERATURE) ? 'a' : 'A',
        gTSB.pmPlusTempreture,
        (gMode == SET_MODE && gSetModeKind == SET_PLUS_END_TEMPERATURE_SSBTIME2) ? 'b' : 'B',
        gTSB.pmPlusTempreture2SSBTime / 60,
        (gTSB.pmPlusTempreture2SSBTime % 60) / 10,
        plusStartTime2 / 60,
        plusStartTime2 % 60,
        (gMode == SET_MODE && gSetModeKind == SET_PLUS_END_TEMPERATURE2) ? 'a' : 'A',
        gTSB.pmPlusTempreture2
      );
  }

  // 例: "S13-0753 E33-0933   "
  {
    int startTime = sunriseTime + gTSB.amStartSRATime;
    int endTime = startTime + gTSB.amEndTemperatureTime;
    sprintf(lcdLines[3], "%c%01d%01d-%02d%02d %c%01d%01d-%02d%02d   ",
        (gMode == SET_MODE && gSetModeKind == SET_AM_START_SRATIME) ? 's' : 'S',
        gTSB.amStartSRATime / 60,
        (gTSB.amStartSRATime % 60) / 10,
        startTime / 60,
        startTime % 60,
        (gMode == SET_MODE && gSetModeKind == SET_AM_END_TEMPERATURE_TIME) ? 'e' : 'E',
        gTSB.amEndTemperatureTime / 60,
        (gTSB.amEndTemperatureTime % 60) / 10,
        endTime / 60,
        endTime % 60
      );
  }

  // 例: "C00                 "
  {
    sprintf(lcdLines[4], "%c%02d                 ",
        (gMode == SET_MODE && gSetModeKind == SET_ANGLE_CORRECTION) ? 'c' : 'C',
        gTSB.angleCorrection
      );
  }

}

//------------------------------------------------------
// 時刻設定モードの表示のための文字列の処理
//------------------------------------------------------

void processSetTimeModeDisplayStrings(char lcdLines[LCD_VIR_ROWS][LCD_COLS_BUFFER_SIZE])
{
  // 例: "">2023>11>13>18>18>18"
  sprintf(lcdLines[0], "%c%04d%c%02d%c%02d%c%02d%c%02d%c%02d",
      (gSetTimeModeKind == SET_TIME_YEAR) ? '>' : ' ',
      gSetTm.Year + 1970,
      (gSetTimeModeKind == SET_TIME_MONTH) ? '>' : ' ',
      gSetTm.Month,
      (gSetTimeModeKind == SET_TIME_DAY) ? '>' : ' ',
      gSetTm.Day,
      (gSetTimeModeKind == SET_TIME_HOUR) ? '>' : ' ',
      gSetTm.Hour,
      (gSetTimeModeKind == SET_TIME_MINUTE) ? '>' : ' ',
      gSetTm.Minute,
      (gSetTimeModeKind == SET_TIME_SECOND) ? '>' : ' ',
      gSetTm.Second
    );

  // 例: ">N                  "
  sprintf(lcdLines[1], "%c%c                  ",
      (gSetTimeModeKind == SET_TIME_OK) ? '>' : ' ',
      gSetTimeOk ? 'Y' : 'N'
    );

  // 例: "                    "
  sprintf(lcdLines[2], "                    ");

  // 例: "                    "
  sprintf(lcdLines[3], "                    ");
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
    gTSB.isManualMode = (gMode == MANUAL_MODE);
    gTSB.manualTemperature = gTemperature;
    //
    gTSB.typeEnd = TSB_TYPE_END;
  } else {
    if (gTSB.isManualMode) {
      gMode = MANUAL_MODE;
      gTemperature = gTSB.manualTemperature;
    }
  }
}

//------------------------------------------------------
// 設定書き込み
//------------------------------------------------------

void saveTemperatureSwitchBag()
{
  gTSB.isManualMode = (gMode == MANUAL_MODE);
  if (gTSB.isManualMode) {
    gTSB.manualTemperature = gTemperature;
  }

  byte* pTsb = (byte*) &gTSB;
  for (int i = 0; i < sizeof(gTSB); ++i) {
    i2cEepromWriteByte(EEPROM_I2C_ADDRESS, i, pTsb[i]);
    delay(5);
  }
}

//------------------------------------------------------
// LCD への表示
//------------------------------------------------------

void displayLCD(char lcdLines[LCD_VIR_ROWS][LCD_COLS_BUFFER_SIZE])
{
  int row = 0;
  if (gMode == SET_MODE && SET_ANGLE_CORRECTION <= gSetModeKind) {
    row = 1;
  }

  for (int i = 0; i < LCD_ROWS; ++i) {
    gLcd.setCursor(0, i);
    gLcd.print(lcdLines[row]);
    ++row;
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
  // LCD
  //
  gLcd.init(); 
  gLcd.backlight();

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
  tmElements_t tm;

  char lcdLines[LCD_VIR_ROWS][LCD_COLS_BUFFER_SIZE];

  gRtc.read(tm);

  int autoMode = digitalRead(AUTO_MODE_PIN);
  int tempDown = digitalRead(TEMP_DOWN_PIN);
  int tempUp = digitalRead(TEMP_UP_PIN);
  int setMode = digitalRead(SET_MODE_PIN);
  int finishSetMode = digitalRead(FINISH_SET_MODE_PIN);

  int sunriseTime = getSunriseTime(tm.Year + 1970, tm.Month, tm.Day);
  int sunsetTime = getSunsetTime(tm.Year + 1970, tm.Month, tm.Day);

  //
  // タクトスイッチが押されている場合の処理
  //

  if (autoMode == BUTTON_ON) {
    ++gSetTimeModeTransCount;
  } else if (autoMode == BUTTON_OFF) {
    gSetTimeModeTransCount = 0;
  }

  if (10 < gSetTimeModeTransCount) {
    gMode = SET_TIME_MODE;
    gSetTimeModeKind = MIN_SET_TIME_MODE_KIND;
    gSetTimeModeTransCount = 0;
    gSetTm = tm;
    gSetTimeOk = false;
  } else if (autoMode == BUTTON_ON) {
    if (gMode == MANUAL_MODE) {
      gMode = AUTO_MODE;
      saveTemperatureSwitchBag();
    }  
  } else if (tempDown == BUTTON_ON || tempUp == BUTTON_ON) {
    if (gMode == AUTO_MODE) {
      gMode = MANUAL_MODE;
    }
  } else if (setMode == BUTTON_ON) {
    if (gMode != SET_TIME_MODE) {
      gMode = SET_MODE;

      ++gSetModeKind;
      if (gSetModeKind < MIN_SET_MODE_KIND || MAX_SET_MODE_KIND < gSetModeKind) {
        gSetModeKind = MIN_SET_MODE_KIND;
      }

    } else {

      ++gSetTimeModeKind;
      if (gSetTimeModeKind < MIN_SET_TIME_MODE_KIND || MAX_SET_TIME_MODE_KIND < gSetTimeModeKind) {
        gSetTimeModeKind = MIN_SET_TIME_MODE_KIND;
      }
    }
  } else if (finishSetMode == BUTTON_ON) {
    if (gMode == SET_MODE || gMode == SET_TIME_MODE) {
      if (gMode == SET_MODE) {
        gSetModeKind = SET_UNDEFINED;
        saveTemperatureSwitchBag();
      } else if (gMode == SET_TIME_MODE) {
        gSetTimeModeKind = SET_TIME_UNDEFINED;
        if (gSetTimeOk) {
          gRtc.write(gSetTm);
          gRtc.read(tm);
        }
      }

      gMode = AUTO_MODE;
      gServo.reset(); // 内部変数をリセットしてサーボモーターを動かす
    }
  }

  //
  // 現在のモードにあわせて処理を行う
  //

  if (gMode == AUTO_MODE) {
    // 自動
    processAutoMode(tm, sunriseTime, sunsetTime);

  } else if (gMode == MANUAL_MODE) {
    // 手動
    processManualMode(tempDown, tempUp);
    saveTemperatureSwitchBag();

  } else if (gMode == SET_MODE) {
    // 設定
    processSetMode(tempDown, tempUp);

  } else if (gMode == SET_TIME_MODE) {
    // 時刻設定
    processSetTimeMode(tempDown, tempUp);
  }

  // LCD へ表示する文字列を作成する
  if (gMode != SET_TIME_MODE) {
    processDisplayStrings(lcdLines, tm, sunriseTime, sunsetTime);
  } else {
    processSetTimeModeDisplayStrings(lcdLines);
  }

  // LCD への表示
  displayLCD(lcdLines);

  // サーボへ温度を設定する
  gServo.setTemperature(gTemperature, gTSB.angleCorrection);

  delay(200);

  wdt_reset();
}
