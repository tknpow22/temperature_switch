#include <avr/wdt.h>
#include <Wire.h>
#include <DS3232RTC.h>
#include "TemperatureSwitch.h"
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

// モード毎の動作など
TSTask gTSTask(&gTSB, &gTSV, &gRtc, &gServo, &gStorage, &gDusk2DawnWrap);

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
  // 日の出・日の入り時刻取得
  //
  gDusk2DawnWrap.init(&gTSB.latlngBag);

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
  pinMode(SETTING_MODE_PIN, INPUT);
  pinMode(FINISH_SETTING_MODE_PIN, INPUT);
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

  // 準備処理
  gTSTask.prepare();

  // タクトスイッチの処理
  gTSTask.processSwt();

  // モード毎の処理
  gTSTask.processMode();

  // ディスプレイに表示する
  gDisplay.print();

  // 温度を設定する
  gTSTask.setTemperature();


  delay(125);

  wdt_reset();
}
