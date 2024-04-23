#ifndef __VARIABLES_STORAGE_H__
#define __VARIABLES_STORAGE_H__

#include <Arduino.h>
#include "temperature_switch.h"

//
// EEPROM の I2C アドレス
//
#define EEPROM_I2C_ADDRESS  0x50

////////////////////////////////////////////////////////
// 設定の保存
////////////////////////////////////////////////////////
// NOTE: メイン側の処理と区別するためにクラス化したが、処理方法はグローバル変数を使っていた時と変わりない。

class VariablesStorage {
public:
  // コンストラクタ
  VariablesStorage(TemperatureSwitchBag* plTSB, TSVariables* plTSV) {
    this->pTSB = plTSB;
    this->pTSV = plTSV;
  }

public:
  // 保存する
  void save();
  // 読み込む
  void load();

private:
  // EEPROMに書き込む
  void i2cEepromWriteByte(unsigned int eeaddress, byte data);
  // EEPROMから読み込む
  byte i2cEepromReadByte(unsigned int eeaddress);

private:
  // 設定データ
  TemperatureSwitchBag* pTSB = NULL;
  // 変数
  TSVariables* pTSV = NULL;
};

#endif