#include <Arduino.h>
#include "TemperatureSwitch.h"
#include "MyServo.h"

////////////////////////////////////////////////////////
// サーボ制御
////////////////////////////////////////////////////////

//------------------------------------------------------
// 初期設定を行う
//------------------------------------------------------

uint8_t MyServo::attach(int pin)
{
  return this->servo.attach(pin, MIN_SERVO_PULSE, MAX_SERVO_PULSE);
}

//------------------------------------------------------
// 温度を設定する(キャッシュあり)
//------------------------------------------------------

void MyServo::setTemperatureWithCache(int temperature, int minTemperaturePulse, int maxTemperaturePulse)
{
  if (this->prevTemperature != temperature) {
    this->setTemperature(temperature, minTemperaturePulse, maxTemperaturePulse);
    this->prevTemperature = temperature;
  }
}

//------------------------------------------------------
// 温度を設定する
//------------------------------------------------------

void MyServo::setTemperature(int temperature, int minTemperaturePulse, int maxTemperaturePulse)
{
  double pulsePerTemp = (double) (maxTemperaturePulse - minTemperaturePulse) / (double) (MAX_TEMPERATURE - MIN_TEMPERATURE);

  // NOTE: @DEBUG
  // {
  //   int nPulsePerTemp = round(pulsePerTemp);
  //   Serial.println(nPulsePerTemp);
  // }

  double pulse = minTemperaturePulse + (pulsePerTemp * (temperature - MIN_TEMPERATURE));

  int nPulse = round(pulse);

  nPulse = min(nPulse, MAX_SERVO_PULSE);
  nPulse = max(nPulse, MIN_SERVO_PULSE);
  //Serial.println(nPulse);
  this->servo.writeMicroseconds(nPulse);

  delay(10);
}

//------------------------------------------------------
// 内部変数等のリセットを行う
//------------------------------------------------------

void MyServo::reset()
{
  this->prevTemperature = 0;
}

