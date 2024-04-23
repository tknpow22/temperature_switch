#ifndef __MY_DISPLAY_H__
#define __MY_DISPLAY_H__

#include <TimeLib.h>
#include <LiquidCrystal_I2C.h>
#include "temperature_switch.h"

//
// ディスプレイの I2C アドレス
//
#define DISPLAY_I2C_ADDRESS 0x27

//
// 表示領域の列・行の定義
//
#define DISPLAY_COLS 20
#define DISPLAY_COLS_BUFFER_SIZE (DISPLAY_COLS + 5) // プログラミングのバグで壊しそうなので念のため余分にとってある
#define DISPLAY_ROWS 4
#define DISPLAY_VIR_ROWS  5 // バッファ上の表示行

////////////////////////////////////////////////////////
// ディスプレイ
////////////////////////////////////////////////////////
// NOTE: メイン側の処理と区別するためにクラス化したが、処理方法はグローバル変数を使っていた時と変わりない。

class MyDisplay {
public:
  // コンストラクタ
  MyDisplay(TemperatureSwitchBag* plTSB, TSVariables* plTSV) :
    lcd(DISPLAY_I2C_ADDRESS/*I2Cスーレブアドレス*/, DISPLAY_COLS/*COLS*/, DISPLAY_ROWS/*ROWS*/)
  {
    this->pTSB = plTSB;
    this->pTSV = plTSV;
  }

public:
  // 初期化する
  void init();
  // 表示する
  void print();

private:
  // 表示文字列を作成する
  void createPrintable();
  // 通常時の表示文字列を作成する
  void createNormalPrintable();
  // 時刻設定時の表示文字列を作成する
  void createSetTimeModePrintable();
  // ディスプレイに表示する
  void printDisplay(int startRow);

private:
  // 設定データ
  TemperatureSwitchBag* pTSB = NULL;
  // 変数
  TSVariables* pTSV = NULL;
  // 表示文字列を格納する
  char displayLines[DISPLAY_VIR_ROWS][DISPLAY_COLS_BUFFER_SIZE];
  // 20 x 4 LCD を使用する
  LiquidCrystal_I2C lcd;
};

#endif
