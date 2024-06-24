#include <Wire.h>
#include "VariablesStorage.h"

////////////////////////////////////////////////////////
// 設定の保存
////////////////////////////////////////////////////////

//------------------------------------------------------
// 保存する
//------------------------------------------------------

void VariablesStorage::save()
{
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
    this->pTSB->amEndTemperature = 27;
    //
    this->pTSB->pmStartTime = (13 * 60);
    this->pTSB->pmEndSSBTime = (1 * 60);
    this->pTSB->pmStartTemperature = 28;
    this->pTSB->pmEndTemperature = 28;
    //
    this->pTSB->angleCorrection = 0;
    //
    this->pTSB->actMode = AUTO_MODE;
    this->pTSB->temperature = MAX_TEMPERATURE;
    //
    this->pTSB->latlngBag.latitudeIPart = DEFAULT_LATITUDE_IPART;
    this->pTSB->latlngBag.latitudeDPart1 = DEFAULT_LATITUDE_DPART1;
    this->pTSB->latlngBag.latitudeDPart2 = DEFAULT_LATITUDE_DPART2;

    this->pTSB->latlngBag.longitudeIPart = DEFAULT_LONGITUDE_IPART;
    this->pTSB->latlngBag.longitudeDPart1 = DEFAULT_LONGITUDE_DPART1;
    this->pTSB->latlngBag.longitudeDPart2 = DEFAULT_LONGITUDE_DPART2;
    //
    this->pTSB->resetParam.resetPattern = RESET_NONE;
    this->pTSB->resetParam.intervalHour = 3;
    this->pTSB->resetParam.resetMinutes = 3;
    this->pTSB->resetParam.exclusionHourStart = 0;
    this->pTSB->resetParam.exclusionHourEnd = 0;
    //
    this->pTSB->typeEnd = TSB_TYPE_END;
  } else {
    this->pTSV->itfcMode = this->pTSB->actMode;
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
