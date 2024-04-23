#include "Dusk2DawnWrap.h"

////////////////////////////////////////////////////////
// 日の出・日の入り取得
////////////////////////////////////////////////////////

//------------------------------------------------------
// 日の出時刻(分)を得る
//------------------------------------------------------

int Dusk2DawnWrap::getSunriseTime(int year, int month, int day)
{
  unsigned long ymd = ((unsigned long)year * 10000) + ((unsigned long)month * 100) + (unsigned long)day;

  if (this->sunriseYMD == ymd && 0 <= this->sunriseTime) {
    return this->sunriseTime;
  }

  int lsunriseTime = this->dusk2Dawn.sunrise(year, month, day, false/*サマータイム*/);
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

  int lsunsetTime = this->dusk2Dawn.sunset(year, month, day, false/*サマータイム*/);
  this->sunsetTime = lsunsetTime;
  this->sunsetYMD = ymd;

  return lsunsetTime;
}
