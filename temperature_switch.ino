#include <Wire.h>
#include <DS3232RTC.h>
#include <TimeLib.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
//#include <VarSpeedServo.h>  // NOTE: 回転速度が早いと感じたら利用すること
#include <Dusk2Dawn.h>

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

#define LCD_BUFFER_SIZE (LCD_COLS + 10)

//
// サーボモーター
//

// 制御ピン
#define SERVO_PIN  10

// サーボモーターのパルス範囲
// NOTE: FS5103B のデータシートだと 600 - 2400 なのだが、少し行き過ぎるため上を少なめにした
#define MIN_SERVO_PULSE 600
#define MAX_SERVO_PULSE 2275

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

// 現在の温度設定
#define MIN_TEMPERATURE 11
#define MAX_TEMPERATURE 34

// 温度設定に対応する角度
//#define MIN_TEMPERATURE_ANGLE 0
#define MAX_TEMPERATURE_ANGLE 180

// 角度補正
#define MIN_ANGLE_CORRECTION  -9
#define MAX_ANGLE_CORRECTION  9

//
// 日の出・日の入り時刻取得
//

// 日の出から処理開始までの時間(分)
#define MIN_START_SRATIME  0
#define MAX_START_SRATIME  (2 * 60)

// 稼働地点の緯度・経度
#define Longitude 133.8416  // 経度
#define Latitude  33.5087   // 緯度

//
// 温度指定
//

// 初期温度(朝の温度)
#define MIN_START_TEMPERATURE 18
#define MAX_START_TEMPERATURE 26

// 最終温度(昼前の温度)
#define MIN_END_TEMPERATURE 22
#define MAX_END_TEMPERATURE MAX_TEMPERATURE

// 処理開始から最終温度に達するまでの時間(分)
#define MIN_END_TEMPERATURE_TIME  (1 * 60)
#define MAX_END_TEMPERATURE_TIME  (4 * 60)

// 午後の時間帯の温度をあげたい場合に最終温度にさらに追加する設定を行う
#define MIN_PLUS_END_TEMPERATURE  0
#define MAX_PLUS_END_TEMPERATURE  3

// 午後の時間帯の温度をあげたい場合に最終温度にさらに追加する設定を行う2
#define MIN_PLUS_END_TEMPERATURE2  0
#define MAX_PLUS_END_TEMPERATURE2  3

// 午後の時間帯の温度をあげる日の入り前の時間(分)
#define MIN_PLUS_END_TEMPERATURE_SSBTIME  (1 * 60)
#define MAX_PLUS_END_TEMPERATURE_SSBTIME  (4 * 60)

// 午後の時間帯の温度をあげる日の入り前の時間2(分)
#define MIN_PLUS_END_TEMPERATURE_SSBTIME2  (1 * 60)
#define MAX_PLUS_END_TEMPERATURE_SSBTIME2  (2 * 60)

//
// 設定変更
//

// 設定種別
#define SET_UNDEFINED (-1)  // 未定義
#define SET_START_TEMPERATURE 0 // 初期温度(朝の温度)
#define SET_END_TEMPERATURE 1 // 最終温度(昼前の温度)
#define SET_START_SRATIME  2 // 日の出から処理開始までの時間(分)
#define SET_END_TEMPERATURE_TIME  3 // 処理開始から最終温度に達するまでの時間(分)
#define SET_ANGLE_CORRECTION  4 // 角度補正
#define SET_PLUS_END_TEMPERATURE_SSBTIME  5 // 追加温度を行う日の入り前の時間(分)
#define SET_PLUS_END_TEMPERATURE  6 // 午後の追加温度
#define SET_PLUS_END_TEMPERATURE_SSBTIME2  7 // 追加温度を行う日の入り前の時間2(分)
#define SET_PLUS_END_TEMPERATURE2  8 // 午後の追加温度2

#define MIN_SET_MODE_KIND SET_START_TEMPERATURE  // 設定種別の最小値
#define MAX_SET_MODE_KIND SET_PLUS_END_TEMPERATURE2  // 設定種別の最大値

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
#define TSB_TYPE_VERSION  1

struct TemperatureSwitchBag {
  char typeBegin;
  int typeVersion;
  int startSRATime; // 日の出から処理開始までの時間(分)  
  int endTemperatureTime;  // 処理開始から最終温度に達するまでの時間(分)
  int startTemperature;  // 初期温度(朝の温度)
  int endTemperature;  // 最終温度(昼前の温度)
  int angleCorrection; // 角度補正
  int plusEndTempretureSSBTime;  // 午後の時間帯の温度をあげる日の入り前の時刻(分)
  int plusEndTempreture; // 午後の時間帯の追加温度
  int plusEndTempretureSSBTime2;  // 午後の時間帯の温度をあげる日の入り前の時刻2(分)
  int plusEndTempreture2; // 午後の時間帯の追加温度2
  char typeEnd;
};

////////////////////////////////////////////////////////
// 変数
////////////////////////////////////////////////////////

// DS3231 リアルタイムクロック
DS3232RTC gRtc;

// 1602 LCD
LiquidCrystal_I2C gLcd(LCD_I2C_ADDRESS/*I2Cスーレブアドレス*/, LCD_COLS/*COLS*/, LCD_ROWS/*ROWS*/);

// FS5103B サーボモータ
//VarSpeedServo gServo;   // NOTE: 回転速度が早いと感じたら利用すること
Servo gServo;

// パルス値の設定用定数
const double PULSE_PER_TEMP = (double)(MAX_SERVO_PULSE - MIN_SERVO_PULSE) / (double)(MAX_TEMPERATURE - MIN_TEMPERATURE);
const double PULSE_PER_DEGREE = (double)(MAX_SERVO_PULSE - MIN_SERVO_PULSE) / (double)MAX_TEMPERATURE_ANGLE;

// 設定データ
TemperatureSwitchBag gTSB;

// LCD 表示情報
char gLcdLines[LCD_ROWS][LCD_BUFFER_SIZE];

// モード
int gMode = AUTO_MODE;

// 日の出・日の入り時刻取得
Dusk2Dawn gDusk2Dawn(Latitude, Longitude, 9/*timezone*/);

// 現在の温度設定
int gTemperature = MAX_TEMPERATURE;
int gPrevTemperature = 0; // 直前の設定

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
// サーボ設定
//------------------------------------------------------

void servoWrite(int temperature) {
  if (gPrevTemperature != temperature) {
    double pulse = MIN_SERVO_PULSE + (PULSE_PER_TEMP * (temperature - MIN_TEMPERATURE));

    // 補正を行う
    pulse += PULSE_PER_DEGREE * gTSB.angleCorrection;

    int nPulse = round(pulse);

    nPulse = min(nPulse, MAX_SERVO_PULSE);
    nPulse = max(nPulse, MIN_SERVO_PULSE);

    //Serial.println(nPulse);
    gServo.writeMicroseconds(nPulse);
    delay(10);

    gPrevTemperature = temperature;
  }
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

  // 深夜 0 時から日の出時刻までの時間が分で戻る
  if (0 <= sunriseTime) {

    // 処理開始時刻(分)
    int startTime = sunriseTime + gTSB.startSRATime;
    // 最終温度に達する時刻(分)
    int endTime = startTime + gTSB.endTemperatureTime;

    if (currentTime < sunriseTime) {
      gTemperature = gTSB.endTemperature;
    } else if (sunriseTime <= currentTime && currentTime < startTime) {
      // 朝すぐの時間帯は温度があまり上がらず、かつ換気したいため、ある程度初期温度時間を長くとるために
      // 日の出から処理開始までの期間は初期温度を設定する
      gTemperature = gTSB.startTemperature;
    } else {
      int timeSpan = endTime - startTime; // NOTE: 現在の仕様では gTSB.endTemperatureTime に等しい
      int tempSpan = gTSB.endTemperature - gTSB.startTemperature;
      if (0 < tempSpan) {
        int timeRange = timeSpan / tempSpan;

        int timeStep = startTime;
        int setTemperature = gTSB.startTemperature;

        while (setTemperature <= gTSB.endTemperature) {
          if (timeStep <= currentTime && currentTime < timeStep + timeRange) {
            gTemperature = setTemperature;
            break;
          }
          timeStep += timeRange;
          ++setTemperature;
        }
        if (gTSB.endTemperature < setTemperature) {
          gTemperature = gTSB.endTemperature;
        }
      }
    }
  }

  // 午後の時間帯の温度をあげたい場合
  if (0 <= sunsetTime) {
    int plusStartTime = sunsetTime - gTSB.plusEndTempretureSSBTime;
    if ((currentTime < sunriseTime || plusStartTime <= currentTime) && 0 < gTSB.plusEndTempreture) {
      // 日の出前または午後の時間帯の温度をあげる日の入り前の時刻(分)を過ぎた
      int plusTempreture = gTemperature + gTSB.plusEndTempreture;
      if (MAX_TEMPERATURE < plusTempreture) {
        plusTempreture = MAX_TEMPERATURE;
      }
      gTemperature = plusTempreture;
    }
  }

  // 午後の時間帯の温度をあげたい場合2
  if (0 <= sunsetTime) {
    int plusStartTime = sunsetTime - gTSB.plusEndTempretureSSBTime2;
    if ((currentTime < sunriseTime || plusStartTime <= currentTime) && 0 < gTSB.plusEndTempreture2) {
      // 日の出前または午後の時間帯の温度をあげる日の入り前の時刻2(分)を過ぎた
      int plusTempreture = gTemperature + gTSB.plusEndTempreture2;
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
    if (gSetModeKind == SET_START_SRATIME) {
      gTSB.startSRATime = decValue(gTSB.startSRATime, MIN_START_SRATIME, 10);
    } else if (gSetModeKind == SET_END_TEMPERATURE_TIME) {
      gTSB.endTemperatureTime = decValue(gTSB.endTemperatureTime, MIN_END_TEMPERATURE_TIME, 10);
    } else if (gSetModeKind == SET_START_TEMPERATURE) {
      gTSB.startTemperature = decValue(gTSB.startTemperature, MIN_START_TEMPERATURE);
    } else if (gSetModeKind == SET_END_TEMPERATURE) {
      gTSB.endTemperature = decValue(gTSB.endTemperature, MIN_END_TEMPERATURE);
      if (gTSB.endTemperature < gTSB.startTemperature + 1) {
        gTSB.endTemperature = gTSB.startTemperature + 1;
      }
    } else if (gSetModeKind == SET_ANGLE_CORRECTION) {
      gTSB.angleCorrection = decValue(gTSB.angleCorrection, MIN_ANGLE_CORRECTION);
    } else if (gSetModeKind == SET_PLUS_END_TEMPERATURE_SSBTIME) {
      gTSB.plusEndTempretureSSBTime = decValue(gTSB.plusEndTempretureSSBTime, MIN_PLUS_END_TEMPERATURE_SSBTIME, 10);
    } else if (gSetModeKind == SET_PLUS_END_TEMPERATURE) {
      gTSB.plusEndTempreture = decValue(gTSB.plusEndTempreture, MIN_PLUS_END_TEMPERATURE);
    } else if (gSetModeKind == SET_PLUS_END_TEMPERATURE_SSBTIME2) {
      gTSB.plusEndTempretureSSBTime2 = decValue(gTSB.plusEndTempretureSSBTime2, MIN_PLUS_END_TEMPERATURE_SSBTIME2, 10);
    } else if (gSetModeKind == SET_PLUS_END_TEMPERATURE2) {
      gTSB.plusEndTempreture2 = decValue(gTSB.plusEndTempreture2, MIN_PLUS_END_TEMPERATURE2);
    }
  } else if (tempUp == BUTTON_ON) {
    if (gSetModeKind == SET_START_SRATIME) {
      gTSB.startSRATime = incValue(gTSB.startSRATime, MAX_START_SRATIME, 10);
    } else if (gSetModeKind == SET_END_TEMPERATURE_TIME) {
      gTSB.endTemperatureTime = incValue(gTSB.endTemperatureTime, MAX_END_TEMPERATURE_TIME, 10);
    } else if (gSetModeKind == SET_START_TEMPERATURE) {
      gTSB.startTemperature = incValue(gTSB.startTemperature, MAX_START_TEMPERATURE);
      if (gTSB.endTemperature - 1 < gTSB.startTemperature) {
        gTSB.startTemperature = gTSB.endTemperature - 1;
      }
    } else if (gSetModeKind == SET_END_TEMPERATURE) {
      gTSB.endTemperature = incValue(gTSB.endTemperature, MAX_END_TEMPERATURE);
    } else if (gSetModeKind == SET_ANGLE_CORRECTION) {
      gTSB.angleCorrection = incValue(gTSB.angleCorrection, MAX_ANGLE_CORRECTION);
    } else if (gSetModeKind == SET_PLUS_END_TEMPERATURE_SSBTIME) {
      gTSB.plusEndTempretureSSBTime = incValue(gTSB.plusEndTempretureSSBTime, MAX_PLUS_END_TEMPERATURE_SSBTIME, 10);
    } else if (gSetModeKind == SET_PLUS_END_TEMPERATURE) {
      gTSB.plusEndTempreture = incValue(gTSB.plusEndTempreture, MAX_PLUS_END_TEMPERATURE);
    } else if (gSetModeKind == SET_PLUS_END_TEMPERATURE_SSBTIME2) {
      gTSB.plusEndTempretureSSBTime2 = incValue(gTSB.plusEndTempretureSSBTime2, MAX_PLUS_END_TEMPERATURE_SSBTIME2, 10);
    } else if (gSetModeKind == SET_PLUS_END_TEMPERATURE2) {
      gTSB.plusEndTempreture2 = incValue(gTSB.plusEndTempreture2, MAX_PLUS_END_TEMPERATURE2);
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

void processDisplayStrings(char lcdLines[LCD_ROWS][LCD_BUFFER_SIZE], const tmElements_t& tm, int sunriseTime, int sunsetTime)
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
      (gMode == SET_MODE && gSetModeKind == SET_START_TEMPERATURE) ? 's' : 'S',
      gTSB.startTemperature,
      (gMode == SET_MODE && gSetModeKind == SET_END_TEMPERATURE) ? 'e' : 'E',
      gTSB.endTemperature
    );

  // 例: "S13-0753E33-0933 C00"
  {
    int startTime = sunriseTime + gTSB.startSRATime;
    int endTime = startTime + gTSB.endTemperatureTime;
    sprintf(lcdLines[2], "%c%01d%01d-%02d%02d%c%01d%01d-%02d%02d %c%02d",
        (gMode == SET_MODE && gSetModeKind == SET_START_SRATIME) ? 's' : 'S',
        gTSB.startSRATime / 60,
        (gTSB.startSRATime % 60) / 10,
        startTime / 60,
        startTime % 60,
        (gMode == SET_MODE && gSetModeKind == SET_END_TEMPERATURE_TIME) ? 'e' : 'E',
        gTSB.endTemperatureTime / 60,
        (gTSB.endTemperatureTime % 60) / 10,
        endTime / 60,
        endTime % 60,
        (gMode == SET_MODE && gSetModeKind == SET_ANGLE_CORRECTION) ? 'c' : 'C',
        gTSB.angleCorrection
      );
  }
  // 例: "B14-1539A0B14-1539A0"
  {
    int plusStartTime = sunsetTime - gTSB.plusEndTempretureSSBTime;
    int plusStartTime2 = sunsetTime - gTSB.plusEndTempretureSSBTime2;
    sprintf(lcdLines[3], "%c%01d%01d-%02d%02d%c%01d%c%01d%01d-%02d%02d%c%01d",
        (gMode == SET_MODE && gSetModeKind == SET_PLUS_END_TEMPERATURE_SSBTIME) ? 'b' : 'B',
        gTSB.plusEndTempretureSSBTime / 60,
        (gTSB.plusEndTempretureSSBTime % 60) / 10,
        plusStartTime / 60,
        plusStartTime % 60,
        (gMode == SET_MODE && gSetModeKind == SET_PLUS_END_TEMPERATURE) ? 'a' : 'A',
        gTSB.plusEndTempreture,
        (gMode == SET_MODE && gSetModeKind == SET_PLUS_END_TEMPERATURE_SSBTIME2) ? 'b' : 'B',
        gTSB.plusEndTempretureSSBTime2 / 60,
        (gTSB.plusEndTempretureSSBTime2 % 60) / 10,
        plusStartTime2 / 60,
        plusStartTime2 % 60,
        (gMode == SET_MODE && gSetModeKind == SET_PLUS_END_TEMPERATURE2) ? 'a' : 'A',
        gTSB.plusEndTempreture2
      );
  }
}

//------------------------------------------------------
// 時刻設定モードの表示のための文字列の処理
//------------------------------------------------------

void processSetTimeModeDisplayStrings(char lcdLines[LCD_ROWS][LCD_BUFFER_SIZE])
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
    gTSB.startSRATime = (2 * 60); // 日の出から処理開始までの時間(分)  
    gTSB.endTemperatureTime = (2 * 60);  // 処理開始から最終温度に達するまでの時間(分)
    gTSB.startTemperature = 20;  // 初期温度(朝の温度)
    gTSB.endTemperature = 28;  // 最終温度(昼前の温度)
    gTSB.angleCorrection = 0; // 角度補正
    gTSB.plusEndTempretureSSBTime = (3 * 60);  // 午後の時間帯の温度をあげる日の入り前の時刻(分)
    gTSB.plusEndTempreture = MIN_PLUS_END_TEMPERATURE; // 午後の時間帯の追加温度
    gTSB.plusEndTempretureSSBTime2 = (1 * 60);  // 午後の時間帯の温度をあげる日の入り前の時刻2(分)
    gTSB.plusEndTempreture2 = MIN_PLUS_END_TEMPERATURE2; // 午後の時間帯の追加温度2
    //
    gTSB.typeEnd = TSB_TYPE_END;
  }
}

//------------------------------------------------------
// 設定書き込み
//------------------------------------------------------

void saveTemperatureSwitchBag()
{
  byte* pTsb = (byte*) &gTSB;
  for (int i = 0; i < sizeof(gTSB); ++i) {
    i2cEepromWriteByte(EEPROM_I2C_ADDRESS, i, pTsb[i]);
    delay(5);
  }
}

//------------------------------------------------------
// LCD への表示
//------------------------------------------------------

void displayLCD(char lcdLines[LCD_ROWS][LCD_BUFFER_SIZE])
{
  for (int i = 0; i < LCD_ROWS; ++i) {
    if (strcmp(gLcdLines[i], lcdLines[i]) != 0) {
      gLcd.setCursor(0, i);
      gLcd.print(lcdLines[i]);
      strcpy(gLcdLines[i], lcdLines[i]);
    }
  }
}

//------------------------------------------------------
// 初期化
//------------------------------------------------------

void setup()
{
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

  for (int i = 0; i < LCD_ROWS; ++i) {
    memset(gLcdLines[i], ' ', LCD_BUFFER_SIZE);
    gLcdLines[i][LCD_BUFFER_SIZE - 1] = '\0';
  }

  //
  // サーボモータ
  //
  gServo.attach(SERVO_PIN, MIN_SERVO_PULSE, MAX_SERVO_PULSE);

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

  char lcdLines[LCD_ROWS][LCD_BUFFER_SIZE];

  gRtc.read(tm);

  int autoMode = digitalRead(AUTO_MODE_PIN);
  int tempDown = digitalRead(TEMP_DOWN_PIN);
  int tempUp = digitalRead(TEMP_UP_PIN);
  int setMode = digitalRead(SET_MODE_PIN);
  int finishSetMode = digitalRead(FINISH_SET_MODE_PIN);

  int sunriseTime = getSunriseTime(tm.Year + 1970, tm.Month, tm.Day);
  int sunsetTime = getSunsetTime(tm.Year + 1970, tm.Month, tm.Day);

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
      gPrevTemperature = 0;  // リセットしてサーボモーターを動かす
    }
  }

  if (gMode == AUTO_MODE) {
    // 自動
    processAutoMode(tm, sunriseTime, sunsetTime);

  } else if (gMode == MANUAL_MODE) {
    // 手動
    processManualMode(tempDown, tempUp);

  } else if (gMode == SET_MODE) {
    // 設定
    processSetMode(tempDown, tempUp);

  } else if (gMode == SET_TIME_MODE) {
    // 時刻設定
    processSetTimeMode(tempDown, tempUp);
  }

  if (gMode != SET_TIME_MODE) {
    processDisplayStrings(lcdLines, tm, sunriseTime, sunsetTime);
  } else {
    processSetTimeModeDisplayStrings(lcdLines);
  }

  // LCD への表示
  displayLCD(lcdLines);

  // サーボへ設定
  servoWrite(gTemperature);

  delay(200);
}
