#include <Arduino.h>
#include "Dusk2DawnWrap.h"

////////////////////////////////////////////////////////
// 日の出・日の入り取得
////////////////////////////////////////////////////////

//------------------------------------------------------
// 初期化
//------------------------------------------------------

void Dusk2DawnWrap::init(LatLngBag* platlngBag)
{
  this->reset(platlngBag);
}

//------------------------------------------------------
// リセット
//------------------------------------------------------

void Dusk2DawnWrap::reset(LatLngBag* platlngBag)
{
  if (this->pDusk2Dawn != NULL) {
    delete this->pDusk2Dawn;    
  }

  float latitude = 0;
  float longitude = 0;

  if (0 <= platlngBag->latitudeIPart) {
    latitude = (float)platlngBag->latitudeIPart + ((float)platlngBag->latitudeDPart1 * 0.01) + ((float)platlngBag->latitudeDPart2 * 0.0001);
  } else {
    latitude = (float)platlngBag->latitudeIPart - ((float)platlngBag->latitudeDPart1 * 0.01) - ((float)platlngBag->latitudeDPart2 * 0.0001);
  }

  if (0 <= platlngBag->longitudeIPart) {
    longitude = (float)platlngBag->longitudeIPart + ((float)platlngBag->longitudeDPart1 * 0.01) + ((float)platlngBag->longitudeDPart2 * 0.0001);
  } else {
    longitude = (float)platlngBag->longitudeIPart - ((float)platlngBag->longitudeDPart1 * 0.01) - ((float)platlngBag->longitudeDPart2 * 0.0001);
  }

  this->pDusk2Dawn = new Dusk2Dawn(latitude, longitude, 9/*timezone*/);

  this->sunriseTime = -1;
  this->sunriseYMD = 0;

  this->sunsetTime = -1;
  this->sunsetYMD = 0;
}

//------------------------------------------------------
// 日の出時刻(分)を得る
//------------------------------------------------------

int Dusk2DawnWrap::getSunriseTime(int year, int month, int day)
{
  unsigned long ymd = ((unsigned long)year * 10000) + ((unsigned long)month * 100) + (unsigned long)day;

  if (this->sunriseYMD == ymd && 0 <= this->sunriseTime) {
    return this->sunriseTime;
  }

  int lsunriseTime = this->pDusk2Dawn->sunrise(year, month, day, false/*サマータイム*/);
  this->sunriseTime = lsunriseTime;
  this->sunriseYMD = ymd;

  return lsunriseTime;
}

//------------------------------------------------------
// 日の入り時刻(分)を得る
//------------------------------------------------------

int Dusk2DawnWrap::getSunsetTime(int year, int month, int day)
{
  unsigned long ymd = ((unsigned long)year * 10000) + ((unsigned long)month * 100) + (unsigned long)day;

  if (this->sunsetYMD == ymd && 0 <= this->sunsetTime) {
    return this->sunsetTime;
  }

  int lsunsetTime = this->pDusk2Dawn->sunset(year, month, day, false/*サマータイム*/);
  this->sunsetTime = lsunsetTime;
  this->sunsetYMD = ymd;

  return lsunsetTime;
}
