#ifndef __TS_TASK_H__
#define __TS_TASK_H__

#include <Arduino.h>
#include <DS3232RTC.h>
#include "TemperatureSwitch.h"
#include "MyServo.h"
#include "Dusk2DawnWrap.h"
#include "VariablesStorage.h"

////////////////////////////////////////////////////////
// モード毎の動作など
////////////////////////////////////////////////////////
// NOTE: メイン側の処理と区別するためにクラス化したが、処理方法はグローバル変数を使っていた時と変わりない。

class TSTask {
public:
  // コンストラクタ
  TSTask(TemperatureSwitchBag* plTSB, TSVariables* plTSV, DS3232RTC* plRtc, MyServo* plServo, VariablesStorage* plStorage, Dusk2DawnWrap* plDusk2DawnWrap)
  {
    this->pTSB = plTSB;
    this->pTSV = plTSV;
    this->pRtc = plRtc;
    this->pServo = plServo;
    this->pStorage = plStorage;
    this->pDusk2DawnWrap = plDusk2DawnWrap;
  }

public:
  // 準備処理(一連の処理の前に呼び出すこと)
  void prepare();
  // タクトスイッチの処理
  void processSwt();
  // モード毎の処理
  void processMode();

private:
  // 自動モードの処理
  void processAutoMode();

private:
  // 午前の温度処理
  void processAmTask();
  // 午後の温度処理
  void processPmTask();

private:
  // 温度処理
  int processTemperature(int startTime, int startTemperature, int endTime, int endTemperature);

private:
  // リセットの処理
  void processResetTask();
  // リセットのチェック処理
  void processResetCheck();
  // リセットの設定処理
  void setReset();
  // リセットのキャンセル処理
  void cancelReset();

private:
  // 手動モードの処理
  void processManualMode();
  // 設定モードの処理
  void processSettingMode();
  // 時刻設定モードの処理
  void processTimeSettingMode();

private:
  // モードの設定
  void setMode(int mode);

private:
  // 指定月の日数を返す
  int getMonthsDays(int year, int month);
  // 値を増やして返す
  int incValue(int orgVal, int maxValue, int addVal = 1);
  // 値を減らして返す
  int decValue(int orgVal, int minValue, int subVal = 1);

private:
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
  MyServo* pServo = NULL;
  // 設定の保存
  VariablesStorage* pStorage = NULL;
  // 日の出・日の入り時刻取得
  Dusk2DawnWrap* pDusk2DawnWrap = NULL;
  // 現在時刻(分)
  int currentTime = 0;
  // 現在時刻(秒)
  long currentSecTime = 0;
  // 時刻設定モード移行カウンタ
  int setTimeModeTransCount = 0;
  // 即時リセット以降カウンタ
  int immediatelyResetTransCount = 0;
};


#endif