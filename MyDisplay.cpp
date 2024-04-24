#include <Arduino.h>
#include "temperature_switch.h"
#include "MyDisplay.h"

////////////////////////////////////////////////////////
// ディスプレイ
////////////////////////////////////////////////////////

//------------------------------------------------------
// 初期化する
//------------------------------------------------------

void MyDisplay::init()
{
  this->lcd.init(); 
  this->lcd.backlight();
}

//------------------------------------------------------
// 表示する
//------------------------------------------------------

void MyDisplay::print()
{
  this->createPrintable();

  int startVirRow = 0;
  if (this->pTSV->mode == SET_MODE) {

    if (SET_LNG_IPART <= this->pTSV->setModeKind) {
      startVirRow = 3;
    } else if (SET_LAT_IPART <= this->pTSV->setModeKind) {
      startVirRow = 2;
    } else if (SET_ANGLE_CORRECTION <= this->pTSV->setModeKind) {
      startVirRow = 1;
    }
  }

  this->printDisplay(startVirRow);
}

//------------------------------------------------------
// 表示文字列を作成する
//------------------------------------------------------

void MyDisplay::createPrintable()
{
  if (this->pTSV->mode != SET_TIME_MODE) {
    createNormalPrintable();
  } else {
    createSetTimeModePrintable();
  }
}

//------------------------------------------------------
// 通常時の表示文字列を作成する
//------------------------------------------------------

void MyDisplay::createNormalPrintable()
{
  // 例: "2023/11/13 15:15 A29"
  sprintf(this->displayLines[0], "%04d/%02d/%02d %02d:%02d %c%02d",
      this->pTSV->tm.Year + 1970,
      this->pTSV->tm.Month,
      this->pTSV->tm.Day,
      this->pTSV->tm.Hour,
      this->pTSV->tm.Minute,
      (this->pTSV->mode == AUTO_MODE) ? 'A' : (this->pTSV->mode == SET_MODE) ? 'X' : 'M',
      this->pTSV->temperature
    );

  // 例: "R0603 S1719 S11 E32 "
  sprintf(this->displayLines[1], "R%02d%02d S%02d%02d %c%02d %c%02d ",
      (0 <= this->pTSV->sunriseTime) ? (this->pTSV->sunriseTime / 60) : 99,
      (0 <= this->pTSV->sunriseTime) ? (this->pTSV->sunriseTime % 60) : 99,
      (0 <= this->pTSV->sunsetTime) ? (this->pTSV->sunsetTime / 60) : 99,
      (0 <= this->pTSV->sunsetTime) ? (this->pTSV->sunsetTime % 60) : 99,
      (this->pTSV->mode == SET_MODE && this->pTSV->setModeKind == SET_AM_START_TEMPERATURE) ? 's' : 'S',
      this->pTSB->amStartTemperature,
      (this->pTSV->mode == SET_MODE && this->pTSV->setModeKind == SET_AM_END_TEMPERATURE) ? 'e' : 'E',
      this->pTSB->amEndTemperature
    );

  // 例: "P1229A0 B14-1539A0  "
  {
    int plusStartTime2 = this->pTSV->sunsetTime - this->pTSB->pmPlusTempreture2SSBTime;
    sprintf(this->displayLines[2], "%c%02d%02d%c%01d %c%01d%01d-%02d%02d%c%01d  ",
        (this->pTSV->mode == SET_MODE && this->pTSV->setModeKind == SET_PLUS_PM_TEMPERATURE_TIME) ? 'p' : 'P',
        this->pTSB->pmPlusTempretureTime / 60,
        this->pTSB->pmPlusTempretureTime % 60,
        (this->pTSV->mode == SET_MODE && this->pTSV->setModeKind == SET_PLUS_PM_TEMPERATURE) ? 'a' : 'A',
        this->pTSB->pmPlusTempreture,
        (this->pTSV->mode == SET_MODE && this->pTSV->setModeKind == SET_PLUS_END_TEMPERATURE_SSBTIME2) ? 'b' : 'B',
        this->pTSB->pmPlusTempreture2SSBTime / 60,
        (this->pTSB->pmPlusTempreture2SSBTime % 60) / 10,
        plusStartTime2 / 60,
        plusStartTime2 % 60,
        (this->pTSV->mode == SET_MODE && this->pTSV->setModeKind == SET_PLUS_END_TEMPERATURE2) ? 'a' : 'A',
        this->pTSB->pmPlusTempreture2
      );
  }

  // 例: "S13-0753 E33-0933   "
  {
    int startTime = this->pTSV->sunriseTime + this->pTSB->amStartSRATime;
    int endTime = startTime + this->pTSB->amEndTemperatureTime;
    sprintf(this->displayLines[3], "%c%01d%01d-%02d%02d %c%01d%01d-%02d%02d   ",
        (this->pTSV->mode == SET_MODE && this->pTSV->setModeKind == SET_AM_START_SRATIME) ? 's' : 'S',
        this->pTSB->amStartSRATime / 60,
        (this->pTSB->amStartSRATime % 60) / 10,
        startTime / 60,
        startTime % 60,
        (this->pTSV->mode == SET_MODE && this->pTSV->setModeKind == SET_AM_END_TEMPERATURE_TIME) ? 'e' : 'E',
        this->pTSB->amEndTemperatureTime / 60,
        (this->pTSB->amEndTemperatureTime % 60) / 10,
        endTime / 60,
        endTime % 60
      );
  }

  // 例: "C00                 "
  {
    sprintf(this->displayLines[4], "%c%02d                 ",
        (this->pTSV->mode == SET_MODE && this->pTSV->setModeKind == SET_ANGLE_CORRECTION) ? 'c' : 'C',
        this->pTSB->angleCorrection
      );
  }

  // 例: "LAT IXXXX.D YY YY   "
  {
    sprintf(this->displayLines[5], "LAT %c%4d.%c%c%02d%c%02d   ",
        (this->pTSV->mode == SET_MODE && this->pTSV->setModeKind == SET_LAT_IPART) ? 'i' : 'I',
        this->pTSB->latlngBag.latitudeIPart,
        (this->pTSV->mode == SET_MODE && (this->pTSV->setModeKind == SET_LAT_DPART1 || this->pTSV->setModeKind == SET_LAT_DPART2)) ? 'd' : 'D',
        (this->pTSV->mode == SET_MODE && this->pTSV->setModeKind == SET_LAT_DPART1) ? '>' : ' ',
        this->pTSB->latlngBag.latitudeDPart1,
        (this->pTSV->mode == SET_MODE && this->pTSV->setModeKind == SET_LAT_DPART2) ? '>' : ' ',
        this->pTSB->latlngBag.latitudeDPart2
      );
  }

  // 例: "LNG IXXXX.D YY YY   "
  {
    sprintf(this->displayLines[6], "LNG %c%4d.%c%c%02d%c%02d   ",
        (this->pTSV->mode == SET_MODE && this->pTSV->setModeKind == SET_LNG_IPART) ? 'i' : 'I',
        this->pTSB->latlngBag.longitudeIPart,
        (this->pTSV->mode == SET_MODE && (this->pTSV->setModeKind == SET_LNG_DPART1 || this->pTSV->setModeKind == SET_LNG_DPART2)) ? 'd' : 'D',
        (this->pTSV->mode == SET_MODE && this->pTSV->setModeKind == SET_LNG_DPART1) ? '>' : ' ',
        this->pTSB->latlngBag.longitudeDPart1,
        (this->pTSV->mode == SET_MODE && this->pTSV->setModeKind == SET_LNG_DPART2) ? '>' : ' ',
        this->pTSB->latlngBag.longitudeDPart2
      );
  }
}

//------------------------------------------------------
// 時刻設定時の表示文字列を作成する
//------------------------------------------------------

void MyDisplay::createSetTimeModePrintable()
{
  // 例: ">2023>11>13>18>18>18"
  sprintf(this->displayLines[0], "%c%04d%c%02d%c%02d%c%02d%c%02d%c%02d",
      (this->pTSV->setTimeModeKind == SET_TIME_YEAR) ? '>' : ' ',
      this->pTSV->setTm.Year + 1970,
      (this->pTSV->setTimeModeKind == SET_TIME_MONTH) ? '>' : ' ',
      this->pTSV->setTm.Month,
      (this->pTSV->setTimeModeKind == SET_TIME_DAY) ? '>' : ' ',
      this->pTSV->setTm.Day,
      (this->pTSV->setTimeModeKind == SET_TIME_HOUR) ? '>' : ' ',
      this->pTSV->setTm.Hour,
      (this->pTSV->setTimeModeKind == SET_TIME_MINUTE) ? '>' : ' ',
      this->pTSV->setTm.Minute,
      (this->pTSV->setTimeModeKind == SET_TIME_SECOND) ? '>' : ' ',
      this->pTSV->setTm.Second
    );

  // 例: ">N                  "
  sprintf(this->displayLines[1], "%c%c                  ",
      (this->pTSV->setTimeModeKind == SET_TIME_OK) ? '>' : ' ',
      this->pTSV->setTimeOk ? 'Y' : 'N'
    );

  // 例: "                    "
  sprintf(this->displayLines[2], "                    ");

  // 例: "                    "
  sprintf(this->displayLines[3], "                    ");
}

//------------------------------------------------------
// ディスプレイに表示する
//------------------------------------------------------

void MyDisplay::printDisplay(int startVirRow)
{
  int row = startVirRow;
  for (int i = 0; i < DISPLAY_ROWS; ++i) {
    if (memcmp(this->displayLines[row], this->prevDisplayLines[i], DISPLAY_COLS) != 0) {
      this->lcd.setCursor(0, i);
      this->lcd.print(this->displayLines[row]);
      strncpy(this->prevDisplayLines[i], this->displayLines[row], DISPLAY_COLS + 1);
    }
    ++row;
  }
}

