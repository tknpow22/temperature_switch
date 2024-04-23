#ifndef __TS_TASK_H__
#define __TS_TASK_H__

#include <Arduino.h>
#include "temperature_switch.h"

////////////////////////////////////////////////////////
// モード毎の動作
////////////////////////////////////////////////////////
// NOTE: メイン側の処理と区別するためにクラス化したが、処理方法はグローバル変数を使っていた時と変わりない。

class TSTask {
public:
  // コンストラクタ
  TSTask(TemperatureSwitchBag* plTSB, TSVariables* plTSV)
  {
    this->pTSB = plTSB;
    this->pTSV = plTSV;
  }

public:
  // 自動モードの処理
  void autoMode();
  // 手動モードの処理
  void manualMode(int tempDownSwt, int tempUpSwt);
  // 設定モードの処理
  void setMode(int tempDownSwt, int tempUpSwt);
  // 時刻設定モードの処理
  void setTimeMode(int tempDownSwt, int tempUpSwt);

private:
  // 指定月の日数を返す
  int getMonthsDays(int year, int month);
  // 値を増やして返す
  int incValue(int orgVal, int maxValue, int addVal = 1);
  // 値を減らして返す
  int decValue(int orgVal, int minValue, int subVal = 1);

private:
  // 設定データ
  TemperatureSwitchBag* pTSB = NULL;
  // 変数
  TSVariables* pTSV = NULL;
};


#endif