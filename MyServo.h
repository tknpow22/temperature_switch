#ifndef __MY_SERVO_H__
#define __MY_SERVO_H__

#include <Servo.h>
//#include <VarSpeedServo.h>  // NOTE: 回転速度が早いと感じたら利用すること
#include "TemperatureSwitch.h"

////////////////////////////////////////////////////////
// サーボ制御
////////////////////////////////////////////////////////

class MyServo {
public:
  // コンストラクタ
  MyServo() {}

public:
  // 初期設定を行う
  uint8_t attach(int pin);
  // 温度を設定する(キャッシュあり)
  void setTemperatureWithCache(int temperature, int minTemperaturePulse, int maxTemperaturePulse);
  // 温度を設定する
  void setTemperature(int temperature, int minTemperaturePulse, int maxTemperaturePulse);
  // 内部変数等のリセットを行う
  void reset();

private:
  // 直前の温度設定を保持する
  int prevTemperature = 0;

  // サーボモータは FS5103B を使っている
  //VarSpeedServo servo;   // NOTE: 回転速度が早いと感じたら利用すること
  Servo servo;
};

#endif