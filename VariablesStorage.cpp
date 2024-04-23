#include <Wire.h>
#include "VariablesStorage.h"

//------------------------------------------------------
// 保存する
//------------------------------------------------------

void VariablesStorage::save()
{
  this->pTSB->isManualMode = (this->pTSV->mode == MANUAL_MODE);
  if (this->pTSB->isManualMode) {
    this->pTSB->manualTemperature = this->pTSV->temperature;
  }

  byte* plTsb = (byte*) this->pTSB;
  for (int i = 0; i < sizeof(*this->pTSB); ++i) {
    this->i2cEepromWriteByte(i, plTsb[i]);
    delay(5);
  }
}

//------------------------------------------------------
// 読み込む
//------------------------------------------------------

void VariablesStorage::load()
{
  byte* plTsb = (byte*) this->pTSB;
  for (int i = 0; i < sizeof(*this->pTSB); ++i) {
    plTsb[i] = this->i2cEepromReadByte(i);
  }

  if (this->pTSB->typeBegin != TSB_TYPE_BEGIN || this->pTSB->typeEnd != TSB_TYPE_END || this->pTSB->typeVersion != TSB_TYPE_VERSION) {
    this->pTSB->typeBegin = TSB_TYPE_BEGIN;
    this->pTSB->typeVersion = TSB_TYPE_VERSION;
    //
    this->pTSB->amStartSRATime = (2 * 60);
    this->pTSB->amEndTemperatureTime = (2 * 60);
    this->pTSB->amStartTemperature = 20;
    this->pTSB->amEndTemperature = 28;
    this->pTSB->angleCorrection = 0;
    this->pTSB->pmPlusTempretureTime = (12 * 60);
    this->pTSB->pmPlusTempreture = MIN_PM_PLUS_TEMPERATURE;
    this->pTSB->pmPlusTempreture2SSBTime = (1 * 60);
    this->pTSB->pmPlusTempreture2 = MIN_PM_PLUS_TEMPERATURE2;
    //
    this->pTSB->isManualMode = (this->pTSV->mode == MANUAL_MODE);
    this->pTSB->manualTemperature = this->pTSV->temperature;
    //
    this->pTSB->typeEnd = TSB_TYPE_END;
  } else {
    if (this->pTSB->isManualMode) {
      this->pTSV->mode = MANUAL_MODE;
      this->pTSV->temperature = this->pTSB->manualTemperature;
    }
  }
}

//------------------------------------------------------
// EEPROM に書き込む
// from https://playground.arduino.cc/Code/I2CEEPROM/
//------------------------------------------------------

void VariablesStorage::i2cEepromWriteByte(unsigned int eeaddress, byte data)
{
  Wire.beginTransmission(EEPROM_I2C_ADDRESS);
  Wire.write((int)(eeaddress >> 8)); // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.write(data);
  Wire.endTransmission();
}

//------------------------------------------------------
// EEPROM から読み込む
// from https://playground.arduino.cc/Code/I2CEEPROM/
//------------------------------------------------------

byte VariablesStorage::i2cEepromReadByte(unsigned int eeaddress)
{
  byte rdata = 0xFF;
  Wire.beginTransmission(EEPROM_I2C_ADDRESS);
  Wire.write((int)(eeaddress >> 8)); // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
  Wire.requestFrom(EEPROM_I2C_ADDRESS, 1);
  if (Wire.available()) {
    rdata = Wire.read();
  }
  return rdata;
}
