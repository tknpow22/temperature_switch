#include <avr/wdt.h>
#include <Wire.h>
#include <DS3232RTC.h>
//#include <VarSpeedServo.h>  // NOTE: 回転速度が早いと感じたら利用すること
#include "temperature_switch.h"
#include "MyServo.h"
#include "MyDisplay.h"
#include "Dusk2DawnWrap.h"
#include "VariablesStorage.h"
#include "TSTask.h"

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

// 設定の保存
VariablesStorage gStorage(&gTSB, &gTSV);

// モード毎の動作
TSTask gTSTask(&gTSB, &gTSV);

// 時刻設定モード移行カウンタ
int gSetTimeModeTransCount = 0;

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
  gStorage.load();

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
  int autoModeSwt = digitalRead(AUTO_MODE_PIN);
  int tempDownSwt = digitalRead(TEMP_DOWN_PIN);
  int tempUpSwt = digitalRead(TEMP_UP_PIN);
  int setModeSwt = digitalRead(SET_MODE_PIN);
  int finishSetModeSwt = digitalRead(FINISH_SET_MODE_PIN);

  //
  // タクトスイッチが押されている場合の処理
  //

  if (autoModeSwt == BUTTON_ON) {
    ++gSetTimeModeTransCount;
  } else if (autoModeSwt == BUTTON_OFF) {
    gSetTimeModeTransCount = 0;
  }

  if (10 < gSetTimeModeTransCount) {
    gTSV.mode = SET_TIME_MODE;
    gTSV.setTimeModeKind = MIN_SET_TIME_MODE_KIND;
    gSetTimeModeTransCount = 0;
    gTSV.setTm = gTSV.tm;
    gTSV.setTimeOk = false;
  } else if (autoModeSwt == BUTTON_ON) {
    if (gTSV.mode == MANUAL_MODE) {
      gTSV.mode = AUTO_MODE;
      gStorage.save();
    }  
  } else if (tempDownSwt == BUTTON_ON || tempUpSwt == BUTTON_ON) {
    if (gTSV.mode == AUTO_MODE) {
      gTSV.mode = MANUAL_MODE;
    }
  } else if (setModeSwt == BUTTON_ON) {
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
  } else if (finishSetModeSwt == BUTTON_ON) {
    if (gTSV.mode == SET_MODE || gTSV.mode == SET_TIME_MODE) {
      if (gTSV.mode == SET_MODE) {
        gTSV.setModeKind = SET_UNDEFINED;
        gStorage.save();
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
    gTSTask.autoMode();

  } else if (gTSV.mode == MANUAL_MODE) {
    // 手動
    gTSTask.manualMode(tempDownSwt, tempUpSwt);
    gStorage.save();

  } else if (gTSV.mode == SET_MODE) {
    // 設定
    gTSTask.setMode(tempDownSwt, tempUpSwt);

  } else if (gTSV.mode == SET_TIME_MODE) {
    // 時刻設定
    gTSTask.setTimeMode(tempDownSwt, tempUpSwt);
  }

  // ディスプレイに表示する
  gDisplay.print();

  // サーボへ温度を設定する
  gServo.setTemperature(gTSV.temperature, gTSB.angleCorrection);

  delay(200);

  wdt_reset();
}
