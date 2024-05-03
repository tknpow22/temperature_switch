#include "TSTask.h"

////////////////////////////////////////////////////////
// モード毎の動作など
////////////////////////////////////////////////////////

//------------------------------------------------------
// タクトスイッチの処理
//------------------------------------------------------

void TSTask::processSwt()
{
  if (this->autoModeSwt == BUTTON_ON) {
    ++this->setTimeModeTransCount;
  } else if (this->autoModeSwt == BUTTON_OFF) {
    this->setTimeModeTransCount = 0;
  }

  if (this->pTSV->mode == AUTO_MODE && 10 < this->setTimeModeTransCount) {
    this->pTSV->mode = SET_TIME_MODE;
    this->pTSV->setTimeModeKind = MIN_SET_TIME_MODE_KIND;
    this->setTimeModeTransCount = 0;
    this->pTSV->setTm = this->pTSV->tm;
    this->pTSV->setTimeOk = false;
  } else if (this->pTSV->mode == MANUAL_MODE && this->autoModeSwt == BUTTON_ON) {
    this->pTSV->mode = AUTO_MODE;
    this->pStorage->save();
  } else if (this->pTSV->mode == AUTO_MODE && (this->tempDownSwt == BUTTON_ON || this->tempUpSwt == BUTTON_ON)) {
    this->pTSV->mode = MANUAL_MODE;
  } else if ((this->pTSV->mode == AUTO_MODE || this->pTSV->mode == SET_MODE) && this->setModeSwt == BUTTON_ON) {
    this->pTSV->mode = SET_MODE;

    if (this->autoModeSwt == BUTTON_ON) {
      --this->pTSV->setModeKind;
    } else {
      ++this->pTSV->setModeKind;
    }

    if (this->pTSV->setModeKind < MIN_SET_MODE_KIND) {
      this->pTSV->setModeKind = MAX_SET_MODE_KIND;
    } else if (MAX_SET_MODE_KIND < this->pTSV->setModeKind) {
      this->pTSV->setModeKind = MIN_SET_MODE_KIND;
    }
  } else if (this->pTSV->mode == SET_TIME_MODE && this->setModeSwt == BUTTON_ON) {
    
    if (this->autoModeSwt == BUTTON_ON) {
      --this->pTSV->setTimeModeKind;
    } else {
      ++this->pTSV->setTimeModeKind;
    }
    
    if (this->pTSV->setTimeModeKind < MIN_SET_TIME_MODE_KIND) {
      this->pTSV->setTimeModeKind = MAX_SET_TIME_MODE_KIND;
    } else if (MAX_SET_TIME_MODE_KIND < this->pTSV->setTimeModeKind) {
      this->pTSV->setTimeModeKind = MIN_SET_TIME_MODE_KIND;
    }

  } else if ((this->pTSV->mode == SET_MODE || this->pTSV->mode == SET_TIME_MODE) && this->finishSetModeSwt == BUTTON_ON) {
    if (this->pTSV->mode == SET_MODE) {
      this->pTSV->setModeKind = SET_UNDEFINED;
      this->pDusk2DawnWrap->reset(&this->pTSB->latlngBag);        
      this->pStorage->save();
    } else if (this->pTSV->mode == SET_TIME_MODE) {
      this->pTSV->setTimeModeKind = SET_TIME_UNDEFINED;
      if (this->pTSV->setTimeOk) {
        this->pRtc->write(this->pTSV->setTm);
        this->pRtc->read(this->pTSV->tm);
      }
    }

    this->pTSV->mode = AUTO_MODE;
    this->pServo->reset(); // 内部変数をリセットしてサーボモーターを動かす
  }
}

//------------------------------------------------------
// モード毎の処理
//------------------------------------------------------

void TSTask::processMode()
{
  if (this->pTSV->mode == AUTO_MODE) {
    // 自動
    this->autoMode();

  } else if (this->pTSV->mode == MANUAL_MODE) {
    // 手動
    this->manualMode();

  } else if (this->pTSV->mode == SET_MODE) {
    // 設定
    this->setMode();

  } else if (this->pTSV->mode == SET_TIME_MODE) {
    // 時刻設定
    this->setTimeMode();
  }
}

//------------------------------------------------------
// 自動モードの処理
//------------------------------------------------------

void TSTask::autoMode()
{
  int currentTime = this->pTSV->tm.Hour * 60 + this->pTSV->tm.Minute;

  // pTSV->sunriseTime および pTSV->sunsetTime には日の出時刻および日の入り時刻が、深夜 0 時からの時間(分)で入っている。
  // NOTE: 取得に失敗している場合は共にマイナスの値が設定される。

  // 午前の温度処理
  this->amTask(currentTime);

  // 午後の温度処理
  this->pmTask(currentTime);

  // 午後の温度処理2
  this->pmTask2(currentTime);

  // リセットの処理
  this->resetTask(currentTime);
}

//------------------------------------------------------
// 午前の温度処理
//------------------------------------------------------

void TSTask::amTask(int currentTime)
{
  if (0 <= this->pTSV->sunriseTime) {

    int startTime = this->pTSV->sunriseTime + this->pTSB->amStartSRATime;
    int endTime = startTime + this->pTSB->amEndTemperatureTime;

    if (currentTime < this->pTSV->sunriseTime) {
      this->pTSV->temperature = this->pTSB->amEndTemperature;
    } else if (this->pTSV->sunriseTime <= currentTime && currentTime < startTime) {
      // 朝すぐの時間帯は温度があまり上がらず、かつ換気したいため、ある程度初期温度時間を長くとるために
      // 日の出から処理開始までの期間は初期温度を設定する
      this->pTSV->temperature = this->pTSB->amStartTemperature;
    } else {
      int timeSpan = endTime - startTime; // NOTE: 現在の仕様では this->pTSB->amEndTemperatureTime に等しい
      int tempSpan = this->pTSB->amEndTemperature - this->pTSB->amStartTemperature;
      if (0 < tempSpan) {
        int timeRange = timeSpan / tempSpan;
        int timeStep = startTime;
        int setTemperature = this->pTSB->amStartTemperature;

        while (setTemperature <= this->pTSB->amEndTemperature) {
          if (timeStep <= currentTime && currentTime < timeStep + timeRange) {
            this->pTSV->temperature = setTemperature;
            break;
          }
          timeStep += timeRange;
          ++setTemperature;
        }
        if (this->pTSB->amEndTemperature < setTemperature) {
          this->pTSV->temperature = this->pTSB->amEndTemperature;
        }
      } else /* tempSpan == 0 */ {
        this->pTSV->temperature = this->pTSB->amEndTemperature;
      }
    }
  }
}

//------------------------------------------------------
// 午後の温度処理
//------------------------------------------------------

void TSTask::pmTask(int currentTime)
{
  if ((currentTime < this->pTSV->sunriseTime || this->pTSB->pmPlusTempretureTime <= currentTime) && 0 < this->pTSB->pmPlusTempreture) {
    // 日の出前または午後温度の開始時刻(分)を過ぎた場合
    int plusTempreture = this->pTSV->temperature + this->pTSB->pmPlusTempreture;
    if (MAX_TEMPERATURE < plusTempreture) {
      plusTempreture = MAX_TEMPERATURE;
    }
    this->pTSV->temperature = plusTempreture;
  }
}

//------------------------------------------------------
// 午後の温度処理2
//------------------------------------------------------

void TSTask::pmTask2(int currentTime)
{
  if (0 <= this->pTSV->sunsetTime) {
    int plusStartTime = this->pTSV->sunsetTime - this->pTSB->pmPlusTempreture2SSBTime;
    if ((currentTime < this->pTSV->sunriseTime || plusStartTime <= currentTime) && 0 < this->pTSB->pmPlusTempreture2) {
      // 日の出前または午後温度2を開始する日の入り前の時間(分)を過ぎた場合
      int plusTempreture = this->pTSV->temperature + this->pTSB->pmPlusTempreture2;
      if (MAX_TEMPERATURE < plusTempreture) {
        plusTempreture = MAX_TEMPERATURE;
      }
      this->pTSV->temperature = plusTempreture;
    }
  }
}

//------------------------------------------------------
// リセットの処理
//------------------------------------------------------

void TSTask::resetTask(int currentTime)
{
  this->pTSV->bWhileReset = false;

  if (this->pTSB->resetParam.resetPattern == RESET_NONE) {
    // リセットしない
    return;
  }

  if (this->pTSV->sunriseTime < 0 || this->pTSV->sunsetTime < 0) {
    // NOTE: リセットパターンが終日の場合でも日の出時刻および日の入り時刻を取得できていない時は処理しない
    return;
  }

  int startHour = 0;
  int endHour = 24;

  if (this->pTSB->resetParam.resetPattern == RESET_FULL) {
    //startHour = 0;
    //endHour = 24;
  } else if (this->pTSB->resetParam.resetPattern == RESET_DAY) {
    startHour = (this->pTSV->sunriseTime + 30) / 60;  // 30分で切り捨てまたは切り上げする
    endHour = this->pTSV->sunsetTime / 60;
  }

  int currentHour = currentTime / 60;

  if (currentHour < startHour || endHour < currentHour) {
    // 現在時が開始時前または開始時後
    return;
  }

  // 除外の処理
  if (this->pTSB->resetParam.exclusionHourEnd <= this->pTSB->resetParam.exclusionHourStart) {
    // 除外時の開始と終了が等しいか正しくない場合はなにもしない
  } else if (this->pTSB->resetParam.exclusionHourStart <= currentHour && currentHour <= this->pTSB->resetParam.exclusionHourEnd) {
    // 除外時中
    return;
  }

  int evalHour = currentHour - startHour; // 評価用時
  int currentMinute = currentTime % 60;

  if ((evalHour % this->pTSB->resetParam.intervalHour) == 0
    && /*0 <= currentMinute &&*/ currentMinute < this->pTSB->resetParam.resetMinutes) {
    // 処理開始時(分は切り捨て)を基準としてリセットを行う時間間隔(時)で、正時にリセットする時間(分)だけリセットする
    if (this->pTSB->resetParam.resetPattern == RESET_DAY && 0 == evalHour) {
      // リセットパターンが日の出から日の入りまでの場合、日の出直後にはリセットしない
      return;
    }
    this->pTSV->temperature = MAX_TEMPERATURE;
    this->pTSV->bWhileReset = true;
  }
}

//------------------------------------------------------
// 手動モードの処理
//------------------------------------------------------

void TSTask::manualMode()
{
  if (this->tempDownSwt == BUTTON_ON) {
    // 温度設定を下げる
    this->pTSV->temperature = this->decValue(this->pTSV->temperature, MIN_TEMPERATURE);

  } else if (this->tempUpSwt == BUTTON_ON) {
    // 温度設定を上げる
    this->pTSV->temperature = this->incValue(this->pTSV->temperature, MAX_TEMPERATURE);
  }

  this->pStorage->save();
}

//------------------------------------------------------
// 設定モードの処理
//------------------------------------------------------

void TSTask::setMode()
{
  if (this->tempDownSwt == BUTTON_ON) {
    if (this->pTSV->setModeKind == SET_AM_START_SRATIME) {
      this->pTSB->amStartSRATime = this->decValue(this->pTSB->amStartSRATime, MIN_AM_START_SRATIME, 10);
    } else if (this->pTSV->setModeKind == SET_AM_END_TEMPERATURE_TIME) {
      this->pTSB->amEndTemperatureTime = this->decValue(this->pTSB->amEndTemperatureTime, MIN_AM_END_TEMPERATURE_TIME, 10);
    } else if (this->pTSV->setModeKind == SET_AM_START_TEMPERATURE) {
      this->pTSB->amStartTemperature = this->decValue(this->pTSB->amStartTemperature, MIN_AM_START_TEMPERATURE);
    } else if (this->pTSV->setModeKind == SET_AM_END_TEMPERATURE) {
      this->pTSB->amEndTemperature = this->decValue(this->pTSB->amEndTemperature, MIN_AM_END_TEMPERATURE);

      // 午前の最終温度設定が午前の初期温度設定より小さくなった場合、午前の初期温度設定を減らす
      if (this->pTSB->amEndTemperature < this->pTSB->amStartTemperature) {
        this->pTSB->amStartTemperature = this->decValue(this->pTSB->amStartTemperature, MIN_AM_START_TEMPERATURE);
      }

    } else if (this->pTSV->setModeKind == SET_ANGLE_CORRECTION) {
      this->pTSB->angleCorrection = this->decValue(this->pTSB->angleCorrection, MIN_ANGLE_CORRECTION);
    } else if (this->pTSV->setModeKind == SET_PLUS_PM_TEMPERATURE_TIME) {
      this->pTSB->pmPlusTempretureTime = this->decValue(this->pTSB->pmPlusTempretureTime, MIN_PM_PLUS_TEMPERATURE_TIME, 10);
    } else if (this->pTSV->setModeKind == SET_PLUS_PM_TEMPERATURE) {
      this->pTSB->pmPlusTempreture = this->decValue(this->pTSB->pmPlusTempreture, MIN_PM_PLUS_TEMPERATURE);
    } else if (this->pTSV->setModeKind == SET_PLUS_END_TEMPERATURE_SSBTIME2) {
      this->pTSB->pmPlusTempreture2SSBTime = this->decValue(this->pTSB->pmPlusTempreture2SSBTime, MIN_PM_PLUS_TEMPERATURE2_SSBTIME, 10);
    } else if (this->pTSV->setModeKind == SET_PLUS_END_TEMPERATURE2) {
      this->pTSB->pmPlusTempreture2 = this->decValue(this->pTSB->pmPlusTempreture2, MIN_PM_PLUS_TEMPERATURE2);
    } else if (this->pTSV->setModeKind == SET_LAT_IPART) {
      this->pTSB->latlngBag.latitudeIPart = this->decValue(this->pTSB->latlngBag.latitudeIPart, MIN_LAT_IPART);
    } else if (this->pTSV->setModeKind == SET_LAT_DPART1) {
      this->pTSB->latlngBag.latitudeDPart1 = this->decValue(this->pTSB->latlngBag.latitudeDPart1, MIN_LAT_DPART);
    } else if (this->pTSV->setModeKind == SET_LAT_DPART2) {
      this->pTSB->latlngBag.latitudeDPart2 = this->decValue(this->pTSB->latlngBag.latitudeDPart2, MIN_LAT_DPART);
    } else if (this->pTSV->setModeKind == SET_LNG_IPART) {
      this->pTSB->latlngBag.longitudeIPart = this->decValue(this->pTSB->latlngBag.longitudeIPart, MIN_LNG_IPART);
    } else if (this->pTSV->setModeKind == SET_LNG_DPART1) {
      this->pTSB->latlngBag.longitudeDPart1 = this->decValue(this->pTSB->latlngBag.longitudeDPart1, MIN_LNG_DPART);
    } else if (this->pTSV->setModeKind == SET_LNG_DPART2) {
      this->pTSB->latlngBag.longitudeDPart2 = this->decValue(this->pTSB->latlngBag.longitudeDPart2, MIN_LNG_DPART);
    } else if (this->pTSV->setModeKind == SET_RESET_PATTERN) {
      this->pTSB->resetParam.resetPattern = this->decValue(this->pTSB->resetParam.resetPattern, MIN_RESET_PATTERN);
    } else if (this->pTSV->setModeKind == SET_RESET_INTERVAL_HOUR) {
      this->pTSB->resetParam.intervalHour = this->decValue(this->pTSB->resetParam.intervalHour, MIN_RESET_INTERVAL_HOUR);
    } else if (this->pTSV->setModeKind == SET_RESET_MINUTES) {
      this->pTSB->resetParam.resetMinutes = this->decValue(this->pTSB->resetParam.resetMinutes, MIN_RESET_MINUTES);
    } else if (this->pTSV->setModeKind == SET_RESET_EXCLUSION_HOUR_START) {
      this->pTSB->resetParam.exclusionHourStart = this->decValue(this->pTSB->resetParam.exclusionHourStart, MIN_RESET_EXCLUSION_HOUR);
    } else if (this->pTSV->setModeKind == SET_RESET_EXCLUSION_HOUR_END) {
      this->pTSB->resetParam.exclusionHourEnd = this->decValue(this->pTSB->resetParam.exclusionHourEnd, MIN_RESET_EXCLUSION_HOUR);

      // リセットから除外する時刻の終了が開始より小さくなった場合、開始時刻を減らす
      if (this->pTSB->resetParam.exclusionHourEnd < this->pTSB->resetParam.exclusionHourStart) {
        this->pTSB->resetParam.exclusionHourStart = this->decValue(this->pTSB->resetParam.exclusionHourStart, MIN_RESET_EXCLUSION_HOUR);
      }

    }

  } else if (this->tempUpSwt == BUTTON_ON) {
    if (this->pTSV->setModeKind == SET_AM_START_SRATIME) {
      this->pTSB->amStartSRATime = this->incValue(this->pTSB->amStartSRATime, MAX_AM_START_SRATIME, 10);
    } else if (this->pTSV->setModeKind == SET_AM_END_TEMPERATURE_TIME) {
      this->pTSB->amEndTemperatureTime = this->incValue(this->pTSB->amEndTemperatureTime, MAX_AM_END_TEMPERATURE_TIME, 10);
    } else if (this->pTSV->setModeKind == SET_AM_START_TEMPERATURE) {
      this->pTSB->amStartTemperature = this->incValue(this->pTSB->amStartTemperature, MAX_AM_START_TEMPERATURE);

      if (this->pTSB->amEndTemperature < this->pTSB->amStartTemperature) {
        // 午前の初期温度設定が午前の最終温度設定より大きくなった場合、午前の最終温度設定を増やす
        this->pTSB->amEndTemperature = this->incValue(this->pTSB->amEndTemperature, MAX_AM_END_TEMPERATURE);
      }

    } else if (this->pTSV->setModeKind == SET_AM_END_TEMPERATURE) {
      this->pTSB->amEndTemperature = this->incValue(this->pTSB->amEndTemperature, MAX_AM_END_TEMPERATURE);
    } else if (this->pTSV->setModeKind == SET_ANGLE_CORRECTION) {
      this->pTSB->angleCorrection = this->incValue(this->pTSB->angleCorrection, MAX_ANGLE_CORRECTION);
    } else if (this->pTSV->setModeKind == SET_PLUS_PM_TEMPERATURE_TIME) {
      this->pTSB->pmPlusTempretureTime = this->incValue(this->pTSB->pmPlusTempretureTime, MAX_PM_PLUS_TEMPERATURE_TIME, 10);
    } else if (this->pTSV->setModeKind == SET_PLUS_PM_TEMPERATURE) {
      this->pTSB->pmPlusTempreture = this->incValue(this->pTSB->pmPlusTempreture, MAX_PM_PLUS_TEMPERATURE);
    } else if (this->pTSV->setModeKind == SET_PLUS_END_TEMPERATURE_SSBTIME2) {
      this->pTSB->pmPlusTempreture2SSBTime = this->incValue(this->pTSB->pmPlusTempreture2SSBTime, MAX_PM_PLUS_TEMPERATURE2_SSBTIME, 10);
    } else if (this->pTSV->setModeKind == SET_PLUS_END_TEMPERATURE2) {
      this->pTSB->pmPlusTempreture2 = this->incValue(this->pTSB->pmPlusTempreture2, MAX_PM_PLUS_TEMPERATURE2);
    } else if (this->pTSV->setModeKind == SET_LAT_IPART) {
      this->pTSB->latlngBag.latitudeIPart = this->incValue(this->pTSB->latlngBag.latitudeIPart, MAX_LAT_IPART);
    } else if (this->pTSV->setModeKind == SET_LAT_DPART1) {
      this->pTSB->latlngBag.latitudeDPart1 = this->incValue(this->pTSB->latlngBag.latitudeDPart1, MAX_LAT_DPART);
    } else if (this->pTSV->setModeKind == SET_LAT_DPART2) {
      this->pTSB->latlngBag.latitudeDPart2 = this->incValue(this->pTSB->latlngBag.latitudeDPart2, MAX_LAT_DPART);
    } else if (this->pTSV->setModeKind == SET_LNG_IPART) {
      this->pTSB->latlngBag.longitudeIPart = this->incValue(this->pTSB->latlngBag.longitudeIPart, MAX_LNG_IPART);
    } else if (this->pTSV->setModeKind == SET_LNG_DPART1) {
      this->pTSB->latlngBag.longitudeDPart1 = this->incValue(this->pTSB->latlngBag.longitudeDPart1, MAX_LNG_DPART);
    } else if (this->pTSV->setModeKind == SET_LNG_DPART2) {
      this->pTSB->latlngBag.longitudeDPart2 = this->incValue(this->pTSB->latlngBag.longitudeDPart2, MAX_LNG_DPART);
    } else if (this->pTSV->setModeKind == SET_RESET_PATTERN) {
      this->pTSB->resetParam.resetPattern = this->incValue(this->pTSB->resetParam.resetPattern, MAX_RESET_PATTERN);
    } else if (this->pTSV->setModeKind == SET_RESET_INTERVAL_HOUR) {
      this->pTSB->resetParam.intervalHour = this->incValue(this->pTSB->resetParam.intervalHour, MAX_RESET_INTERVAL_HOUR);
    } else if (this->pTSV->setModeKind == SET_RESET_MINUTES) {
      this->pTSB->resetParam.resetMinutes = this->incValue(this->pTSB->resetParam.resetMinutes, MAX_RESET_MINUTES);
    } else if (this->pTSV->setModeKind == SET_RESET_EXCLUSION_HOUR_START) {
      this->pTSB->resetParam.exclusionHourStart = this->incValue(this->pTSB->resetParam.exclusionHourStart, MAX_RESET_EXCLUSION_HOUR);

      if (this->pTSB->resetParam.exclusionHourEnd < this->pTSB->resetParam.exclusionHourStart) {
        // リセットから除外する時刻の開始が終了より大きくなった場合、終了時刻を増やす
        this->pTSB->resetParam.exclusionHourEnd = this->incValue(this->pTSB->resetParam.exclusionHourEnd, MAX_RESET_EXCLUSION_HOUR);
      }

    } else if (this->pTSV->setModeKind == SET_RESET_EXCLUSION_HOUR_END) {
      this->pTSB->resetParam.exclusionHourEnd = this->incValue(this->pTSB->resetParam.exclusionHourEnd, MAX_RESET_EXCLUSION_HOUR);
    }
  }
}

//------------------------------------------------------
// 時刻設定モードの処理
//------------------------------------------------------

void TSTask::setTimeMode()
{
  if (this->tempDownSwt == BUTTON_ON) {
    if (this->pTSV->setTimeModeKind == SET_TIME_YEAR) {
      this->pTSV->setTm.Year = this->decValue(this->pTSV->setTm.Year, 2023 - 1970);
    } else if (this->pTSV->setTimeModeKind == SET_TIME_MONTH) {
      this->pTSV->setTm.Month = this->decValue(this->pTSV->setTm.Month, 1);
      int days = this->getMonthsDays(this->pTSV->setTm.Year + 1970, this->pTSV->setTm.Month);
      if (days < this->pTSV->setTm.Day) {
        this->pTSV->setTm.Day = days;
      }
    } else if (this->pTSV->setTimeModeKind == SET_TIME_DAY) {
      this->pTSV->setTm.Day = this->decValue(this->pTSV->setTm.Day, 1);
    } else if (this->pTSV->setTimeModeKind == SET_TIME_HOUR) {
      this->pTSV->setTm.Hour = this->decValue(this->pTSV->setTm.Hour, 0); // Hour 以降は unsigned なので注意
    } else if (this->pTSV->setTimeModeKind == SET_TIME_MINUTE) {
      this->pTSV->setTm.Minute = this->decValue(this->pTSV->setTm.Minute, 0);
    } else if (this->pTSV->setTimeModeKind == SET_TIME_SECOND) {
      this->pTSV->setTm.Second = this->decValue(this->pTSV->setTm.Second, 0);
    } else if (this->pTSV->setTimeModeKind == SET_TIME_OK) {
      this->pTSV->setTimeOk = !this->pTSV->setTimeOk;
    }
  } else if (this->tempUpSwt == BUTTON_ON) {
    if (this->pTSV->setTimeModeKind == SET_TIME_YEAR) {
      this->pTSV->setTm.Year = this->incValue(this->pTSV->setTm.Year, 2050 - 1970);
    } else if (this->pTSV->setTimeModeKind == SET_TIME_MONTH) {
      this->pTSV->setTm.Month = this->incValue(this->pTSV->setTm.Month, 12);
      int days = this->getMonthsDays(this->pTSV->setTm.Year + 1970, this->pTSV->setTm.Month);
      if (days < this->pTSV->setTm.Day) {
        this->pTSV->setTm.Day = days;
      }
    } else if (this->pTSV->setTimeModeKind == SET_TIME_DAY) {
      int days = this->getMonthsDays(this->pTSV->setTm.Year + 1970, this->pTSV->setTm.Month);
      this->pTSV->setTm.Day = this->incValue(this->pTSV->setTm.Day, days);
    } else if (this->pTSV->setTimeModeKind == SET_TIME_HOUR) {
      this->pTSV->setTm.Hour = this->incValue(this->pTSV->setTm.Hour, 23);
    } else if (this->pTSV->setTimeModeKind == SET_TIME_MINUTE) {
      this->pTSV->setTm.Minute = this->incValue(this->pTSV->setTm.Minute, 59);
    } else if (this->pTSV->setTimeModeKind == SET_TIME_SECOND) {
      this->pTSV->setTm.Second = this->incValue(this->pTSV->setTm.Second, 59);
    } else if (this->pTSV->setTimeModeKind == SET_TIME_OK) {
      this->pTSV->setTimeOk = !this->pTSV->setTimeOk;
    }
  }
}

//------------------------------------------------------
// 指定月の日数を返す
//------------------------------------------------------

int TSTask::getMonthsDays(int year, int month)
{
  int days = 0;

  switch (month) {
    case 1: case 3: case 5: case 7: case 8: case 10: case 12:
      days =  31;
      break;
    case 2:
      {
        bool isLeapYear = false;
        if ((year % 4) == 0) {
          isLeapYear = true;
        }
        if ((year % 100) == 0) {
          isLeapYear = false;
        }
        if ((year % 400) == 0) {
          isLeapYear = true;
        }

        days = (isLeapYear) ? 29 : 28;
      }
      break;
    default:  // case 4: case 6: case 9: case 11:
      days = 30;
      break;
  }
  return days;
}

//------------------------------------------------------
// 値を増やして返す
//------------------------------------------------------

int TSTask::incValue(int orgVal, int maxValue, int addVal = 1)
{
  int result = orgVal + addVal;
  if (maxValue < result) {
    result = maxValue;
  }
  return result;
}

//------------------------------------------------------
// 値を減らして返す
//------------------------------------------------------

int TSTask::decValue(int orgVal, int minValue, int subVal = 1)
{
  int result = orgVal - subVal;
  if (result < minValue) {
    result = minValue;
  }
  return result;
}
