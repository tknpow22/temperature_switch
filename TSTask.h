#ifndef __TS_TASK_H__
#define __TS_TASK_H__

#include <Arduino.h>
#include <DS3232RTC.h>
#include "temperature_switch.h"
#include "MyServo.h"
#include "VariablesStorage.h"

////////////////////////////////////////////////////////
// モード毎の動作など
////////////////////////////////////////////////////////
// NOTE: メイン側の処理と区別するためにクラス化したが、処理方法はグローバル変数を使っていた時と変わりない。

class TSTask {
public:
  // コンストラクタ
  TSTask(TemperatureSwitchBag* plTSB, TSVariables* plTSV, DS3232RTC* plRtc, MyServo* plServo, VariablesStorage* plStorage)
  {
    this->pTSB = plTSB;
    this->pTSV = plTSV;
    this->pRtc = plRtc;
    this->pServo = plServo;
    this->pStorage = plStorage;
  }

public:
  // タクトスイッチの処理
  void processSwt();

public:
  // 自動モードの処理
  void autoMode();
  // 手動モードの処理
  void manualMode();
  // 設定モードの処理
  void setMode();
  // 時刻設定モードの処理
  void setTimeMode();

private:
  // 指定月の日数を返す
  int getMonthsDays(int year, int month);
  // 値を増やして返す
  int incValue(int orgVal, int maxValue, int addVal = 1);
  // 値を減らして返す
  int decValue(int orgVal, int minValue, int subVal = 1);

public:
  // タクトスイッチの状態
  int autoModeSwt = BUTTON_OFF;
  int tempDownSwt = BUTTON_OFF;
  int tempUpSwt = BUTTON_OFF;
  int setModeSwt = BUTTON_OFF;
  int finishSetModeSwt = BUTTON_OFF;

private:
  // 設定データ
  TemperatureSwitchBag* pTSB = NULL;
  // 変数
  TSVariables* pTSV = NULL;
  // DS3231 リアルタイムクロック
  DS3232RTC* pRtc = NULL;
  // サーボモータ
  MyServo* pServo;
  // 設定の保存
  VariablesStorage* pStorage;
  // 時刻設定モード移行カウンタ
  int setTimeModeTransCount = 0;
};


#endif