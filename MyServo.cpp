#include <Arduino.h>
#include "temperature_switch.h"
#include "MyServo.h"

////////////////////////////////////////////////////////
// サーボ制御
////////////////////////////////////////////////////////

//------------------------------------------------------
// 初期設定を行う
//------------------------------------------------------

uint8_t MyServo::attach(int pin)
{
  this->servo.attach(pin, MIN_SERVO_PULSE, MAX_SERVO_PULSE);
}

//------------------------------------------------------
// 温度を設定する
//------------------------------------------------------

void MyServo::setTemperature(int temperature, int angleCorrection)
{
  if (this->prevTemperature != temperature) {
    double pulse = MIN_SERVO_PULSE + (this->PULSE_PER_TEMP * (temperature - MIN_TEMPERATURE));

    // 補正を行う
    pulse += this->PULSE_PER_DEGREE * angleCorrection;

    int nPulse = round(pulse);

    nPulse = min(nPulse, MAX_SERVO_PULSE);
    nPulse = max(nPulse, MIN_SERVO_PULSE);

    //Serial.println(nPulse);
    this->servo.writeMicroseconds(nPulse);
    delay(10);

    this->prevTemperature = temperature;
  }
}

//------------------------------------------------------
// 内部変数等のリセットを行う
//------------------------------------------------------

void MyServo::reset()
{
  this->prevTemperature = 0;
}

