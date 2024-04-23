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
// 変数
////////////////////////////////////////////////////////

// 設定データ
TemperatureSwitchBag gTSB;

// 変数
TSVariables gTSV;

// DS3231 リアルタイムクロック
// NOTE: DS3231 の I2C アドレスは DS3232RTC.h 内で定義されている。
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
TSTask gTSTask(&gTSB, &gTSV, &gRtc, &gServo, &gStorage);

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
  gTSTask.autoModeSwt = digitalRead(AUTO_MODE_PIN);
  gTSTask.tempDownSwt = digitalRead(TEMP_DOWN_PIN);
  gTSTask.tempUpSwt = digitalRead(TEMP_UP_PIN);
  gTSTask.setModeSwt = digitalRead(SET_MODE_PIN);
  gTSTask.finishSetModeSwt = digitalRead(FINISH_SET_MODE_PIN);

  //
  // タクトスイッチの処理
  //
  gTSTask.processSwt();

  //
  // 現在のモードにあわせて処理を行う
  //

  if (gTSV.mode == AUTO_MODE) {
    // 自動
    gTSTask.autoMode();

  } else if (gTSV.mode == MANUAL_MODE) {
    // 手動
    gTSTask.manualMode();

  } else if (gTSV.mode == SET_MODE) {
    // 設定
    gTSTask.setMode();

  } else if (gTSV.mode == SET_TIME_MODE) {
    // 時刻設定
    gTSTask.setTimeMode();
  }

  // ディスプレイに表示する
  gDisplay.print();

  // サーボへ温度を設定する
  gServo.setTemperature(gTSV.temperature, gTSB.angleCorrection);

  delay(200);

  wdt_reset();
}
