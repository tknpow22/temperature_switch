#ifndef __MY_SERVO_H__
#define __MY_SERVO_H__

#include <Servo.h>
//#include <VarSpeedServo.h>  // NOTE: 回転速度が早いと感じたら利用すること
#include "temperature_switch.h"

// 温度設定に対応する角度の最小・最大
//#define MIN_TEMPERATURE_ANGLE 0
#define MAX_TEMPERATURE_ANGLE 180

// サーボモーターのパルス範囲
// NOTE: FS5103B のデータシートだと 600 - 2400 なのだが、少し行き過ぎるため上を少なめにした
#define MIN_SERVO_PULSE 600
#define MAX_SERVO_PULSE 2275

////////////////////////////////////////////////////////
// サーボ制御
////////////////////////////////////////////////////////

class MyServo {
public:
  MyServo() {}

public:
  // 初期設定を行う
  uint8_t attach(int pin);
  // 温度を設定する
  void setTemperature(int temperature, int angleCorrection);
  // 内部変数等のリセットを行う
  void reset();

private:
  // パルス値の設定用定数
  const double PULSE_PER_TEMP = (double)(MAX_SERVO_PULSE - MIN_SERVO_PULSE) / (double)(MAX_TEMPERATURE - MIN_TEMPERATURE);
  const double PULSE_PER_DEGREE = (double)(MAX_SERVO_PULSE - MIN_SERVO_PULSE) / (double)MAX_TEMPERATURE_ANGLE;

private:
  // 直前の温度設定を保持する
  int prevTemperature = 0;

  // サーボモータは FS5103B を使っている
  //VarSpeedServo servo;   // NOTE: 回転速度が早いと感じたら利用すること
  Servo servo;
};

#endif