#include <Arduino.h>
#include "TemperatureSwitch.h"
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
  if (this->pTSV->itfcMode == SETTING_MODE) {
    if (SET_LNG_IPART <= this->pTSV->setModeKind) {
      startVirRow = 4;
    } else if (SET_LAT_IPART <= this->pTSV->setModeKind) {
      startVirRow = 3;
    } else if (SET_RESET_PATTERN <= this->pTSV->setModeKind) {
      startVirRow = 2;
    } else if (SET_PM_START_TIME <= this->pTSV->setModeKind) {
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
  if (this->pTSV->itfcMode == TIME_SETTING_MODE) {
    createSetTimeModePrintable();
  } else if (this->pTSV->itfcMode == SERVO_SETTING_MODE) {
    createServoSettingModePrintable();
  } else {
    createNormalPrintable();
  }
}

//------------------------------------------------------
// 通常時の表示文字列を作成する
//------------------------------------------------------

void MyDisplay::createNormalPrintable()
{
  char resetChr = ' ';
  if (0 <= this->pTSV->resetStartSecTime) {
    resetChr = 'R';
  } else {
    if (this->pTSB->actMode == AUTO_MODE && this->pTSB->resetParam.resetPattern != RESET_NONE) {
      resetChr = '@';
    }
  }

  char modeChr = 'A';
  if (this->pTSV->itfcMode == AUTO_MODE) {
    modeChr = 'A';
  } else if (this->pTSV->itfcMode == MANUAL_MODE) {
    modeChr = 'M';
  } else {
    modeChr = 'X';
  }

  // 例: "2023/11/13 15:15@A29"
  sprintf(this->displayLines[0], "%04d/%02d/%02d %02d:%02d%c%c%02d",
      this->pTSV->tm.Year + 1970,
      this->pTSV->tm.Month,
      this->pTSV->tm.Day,
      this->pTSV->tm.Hour,
      this->pTSV->tm.Minute,
      resetChr,
      modeChr,
      this->pTSB->temperature
    );

  // 例: "SR 06:03 SS 17:19   "
  sprintf(this->displayLines[1], "SR %02d:%02d SS %02d:%02d   ",
      (0 <= this->pTSV->sunriseTime) ? (this->pTSV->sunriseTime / 60) : 99,
      (0 <= this->pTSV->sunriseTime) ? (this->pTSV->sunriseTime % 60) : 99,
      (0 <= this->pTSV->sunsetTime) ? (this->pTSV->sunsetTime / 60) : 99,
      (0 <= this->pTSV->sunsetTime) ? (this->pTSV->sunsetTime % 60) : 99
    );

  // 例: "AS18 E27 PS28 E25   "
  sprintf(this->displayLines[2], "A%c%02d %c%02d P%c%02d %c%02d   ",
      (this->pTSV->itfcMode == SETTING_MODE && this->pTSV->setModeKind == SET_AM_START_TEMPERATURE) ? 's' : 'S',
      this->pTSB->amStartTemperature,
      (this->pTSV->itfcMode == SETTING_MODE && this->pTSV->setModeKind == SET_AM_END_TEMPERATURE) ? 'e' : 'E',
      this->pTSB->amEndTemperature,
      (this->pTSV->itfcMode == SETTING_MODE && this->pTSV->setModeKind == SET_PM_START_TEMPERATURE) ? 's' : 'S',
      this->pTSB->pmStartTemperature,
      (this->pTSV->itfcMode == SETTING_MODE && this->pTSV->setModeKind == SET_PM_END_TEMPERATURE) ? 'e' : 'E',
      this->pTSB->pmEndTemperature
    );

  // 例: "AS13-07:53 E33-09:33"
  {
    int startTime = this->pTSV->sunriseTime + this->pTSB->amStartSRATime;
    int endTime = startTime + this->pTSB->amEndTemperatureTime;
    sprintf(this->displayLines[3], "A%c%01d%01d-%02d:%02d %c%01d%01d-%02d:%02d",
        (this->pTSV->itfcMode == SETTING_MODE && this->pTSV->setModeKind == SET_AM_START_SRATIME) ? 's' : 'S',
        this->pTSB->amStartSRATime / 60,
        (this->pTSB->amStartSRATime % 60) / 10,
        startTime / 60,
        startTime % 60,
        (this->pTSV->itfcMode == SETTING_MODE && this->pTSV->setModeKind == SET_AM_END_TEMPERATURE_TIME) ? 'e' : 'E',
        this->pTSB->amEndTemperatureTime / 60,
        (this->pTSB->amEndTemperatureTime % 60) / 10,
        endTime / 60,
        endTime % 60
      );
  }

  // 例: "PS12:29 E11-17:33   "
  {
    int pmEndTime = this->pTSV->sunsetTime - this->pTSB->pmEndSSBTime;
    sprintf(this->displayLines[4], "P%c%02d:%02d %c%01d%01d-%02d:%02d   ",
        (this->pTSV->itfcMode == SETTING_MODE && this->pTSV->setModeKind == SET_PM_START_TIME) ? 's' : 'S',
        this->pTSB->pmStartTime / 60,
        this->pTSB->pmStartTime % 60,
        (this->pTSV->itfcMode == SETTING_MODE && this->pTSV->setModeKind == SET_PM_END_SSBTIME) ? 'e' : 'E',
        this->pTSB->pmEndSSBTime / 60,
        (this->pTSB->pmEndSSBTime % 60) / 10,
        pmEndTime / 60,
        pmEndTime % 60
      );
  }


  // 例: "RY H1 M1 S00 E00    "
  {
    char resetPatternChr = 'N';
    if (this->pTSB->resetParam.resetPattern == RESET_NONE) {
      //resetPatternChr = 'N';
    } else if (this->pTSB->resetParam.resetPattern == RESET_FULL) {
      resetPatternChr = 'F';
    } else /*if (this->pTSB->resetParam.resetPattern == RESET_DAY)*/ {
      resetPatternChr = 'D';
    }

    sprintf(this->displayLines[5], "%c%c %c%1d %c%1d %c%02d %c%02d    ",
        (this->pTSV->itfcMode == SETTING_MODE && this->pTSV->setModeKind == SET_RESET_PATTERN) ? 'r' : 'R',
        resetPatternChr,
        (this->pTSV->itfcMode == SETTING_MODE && this->pTSV->setModeKind == SET_RESET_INTERVAL_HOUR) ? 'h' : 'H',
        this->pTSB->resetParam.intervalHour,
        (this->pTSV->itfcMode == SETTING_MODE && this->pTSV->setModeKind == SET_RESET_MINUTES) ? 'm' : 'M',
        this->pTSB->resetParam.resetMinutes,
        (this->pTSV->itfcMode == SETTING_MODE && this->pTSV->setModeKind == SET_RESET_EXCLUSION_HOUR_START) ? 's' : 'S',
        this->pTSB->resetParam.exclusionHourStart,
        (this->pTSV->itfcMode == SETTING_MODE && this->pTSV->setModeKind == SET_RESET_EXCLUSION_HOUR_END) ? 'e' : 'E',
        this->pTSB->resetParam.exclusionHourEnd
      );
  }

  // 例: "LAT IXXXX.D YY YY   "
  {
    sprintf(this->displayLines[6], "LAT %c%4d.%c%c%02d%c%02d   ",
        (this->pTSV->itfcMode == SETTING_MODE && this->pTSV->setModeKind == SET_LAT_IPART) ? 'i' : 'I',
        this->pTSB->latlngBag.latitudeIPart,
        (this->pTSV->itfcMode == SETTING_MODE && (this->pTSV->setModeKind == SET_LAT_DPART1 || this->pTSV->setModeKind == SET_LAT_DPART2)) ? 'd' : 'D',
        (this->pTSV->itfcMode == SETTING_MODE && this->pTSV->setModeKind == SET_LAT_DPART1) ? '>' : ' ',
        this->pTSB->latlngBag.latitudeDPart1,
        (this->pTSV->itfcMode == SETTING_MODE && this->pTSV->setModeKind == SET_LAT_DPART2) ? '>' : ' ',
        this->pTSB->latlngBag.latitudeDPart2
      );
  }

  // 例: "LNG IXXXX.D YY YY   "
  {
    sprintf(this->displayLines[7], "LNG %c%4d.%c%c%02d%c%02d   ",
        (this->pTSV->itfcMode == SETTING_MODE && this->pTSV->setModeKind == SET_LNG_IPART) ? 'i' : 'I',
        this->pTSB->latlngBag.longitudeIPart,
        (this->pTSV->itfcMode == SETTING_MODE && (this->pTSV->setModeKind == SET_LNG_DPART1 || this->pTSV->setModeKind == SET_LNG_DPART2)) ? 'd' : 'D',
        (this->pTSV->itfcMode == SETTING_MODE && this->pTSV->setModeKind == SET_LNG_DPART1) ? '>' : ' ',
        this->pTSB->latlngBag.longitudeDPart1,
        (this->pTSV->itfcMode == SETTING_MODE && this->pTSV->setModeKind == SET_LNG_DPART2) ? '>' : ' ',
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
// サーボ設定時の表示文字列を作成する
//------------------------------------------------------

void MyDisplay::createServoSettingModePrintable()
{
  // 例: "T12: >0600          "
  sprintf(this->displayLines[0], "T%02d: %c%04d          ",
      MIN_TEMPERATURE,
      (this->pTSV->setServoParamModeKind == SET_SERVO_PARAM_MIN_TEMPERATURE_PULSE) ? '>' : ' ',
      this->pTSB->minTemperaturePulse
    );

  // 例: "T33: >2275          "
  sprintf(this->displayLines[1], "T%02d: %c%04d          ",
      MAX_TEMPERATURE,
      (this->pTSV->setServoParamModeKind == SET_SERVO_PARAM_MAX_TEMPERATURE_PULSE) ? '>' : ' ',
      this->pTSB->maxTemperaturePulse
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

