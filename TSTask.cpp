#include "TSTask.h"

////////////////////////////////////////////////////////
// モード毎の動作など
////////////////////////////////////////////////////////

//------------------------------------------------------
// 準備処理(一連の処理の前に呼び出すこと)
//------------------------------------------------------

void TSTask::prepare()
{
  // 現在時刻を設定する
  this->currentTime = this->pTSV->tm.Hour * 60 + this->pTSV->tm.Minute;
  this->currentSecTime = ((long)this->currentTime * 60) + this->pTSV->tm.Second;

  // タクトスイッチの状態を得る
  this->autoModeSwt = digitalRead(AUTO_MODE_PIN);
  this->tempDownSwt = digitalRead(TEMP_DOWN_PIN);
  this->tempUpSwt = digitalRead(TEMP_UP_PIN);
  this->setModeSwt = digitalRead(SETTING_MODE_PIN);
  this->finishSetModeSwt = digitalRead(FINISH_SETTING_MODE_PIN);
}

//------------------------------------------------------
// タクトスイッチの処理
//------------------------------------------------------

void TSTask::processSwt()
{
  //
  // タクトスイッチの状態に応じて処理を行う
  //

  if (this->autoModeSwt == BUTTON_ON) {
    ++this->setSettingModeTransCount;
  } else if (this->autoModeSwt == BUTTON_OFF) {
    this->setSettingModeTransCount = 0;
  }

  if (this->finishSetModeSwt == BUTTON_ON) {
    ++this->immediatelyResetTransCount;
  } else if (this->finishSetModeSwt == BUTTON_OFF) {
    this->immediatelyResetTransCount = 0;
  }

  if (this->pTSV->itfcMode == AUTO_MODE && 10 < this->setSettingModeTransCount) {
    this->setMode(TIME_SETTING_MODE);

    this->cancelReset();

    this->pTSV->setTimeModeKind = MIN_TIME_SETTING_MODE_KIND;
    this->setSettingModeTransCount = 0;
    
    this->pTSV->setTm = this->pTSV->tm;
    this->pTSV->setTimeOk = false;

  } else if (this->pTSV->itfcMode == TIME_SETTING_MODE && 10 < this->setSettingModeTransCount) {

    this->setMode(SERVO_SETTING_MODE);

    this->cancelReset();

    this->pTSV->setServoParamModeKind = MIN_SERVO_SETTING_MODE_KIND;
    this->setSettingModeTransCount = 0;

  } else if ((this->pTSV->itfcMode == AUTO_MODE || this->pTSV->itfcMode == MANUAL_MODE) && 10 < this->immediatelyResetTransCount) {
    this->setReset();
    this->immediatelyResetTransCount = 0;
  } else if (this->pTSV->itfcMode == MANUAL_MODE && this->autoModeSwt == BUTTON_ON) {
    this->setMode(AUTO_MODE);
    this->pStorage->save();
  } else if (this->pTSV->itfcMode == AUTO_MODE && (this->tempDownSwt == BUTTON_ON || this->tempUpSwt == BUTTON_ON)) {
    this->setMode(MANUAL_MODE);
  } else if ((this->pTSV->itfcMode == AUTO_MODE || this->pTSV->itfcMode == SETTING_MODE) && this->setModeSwt == BUTTON_ON) {
    this->setMode(SETTING_MODE);

    this->cancelReset();

    if (this->autoModeSwt == BUTTON_ON) {
      --this->pTSV->setModeKind;
    } else {
      ++this->pTSV->setModeKind;
    }

    if (this->pTSV->setModeKind < MIN_SETTING_MODE_KIND) {
      this->pTSV->setModeKind = MAX_SETTING_MODE_KIND;
    } else if (MAX_SETTING_MODE_KIND < this->pTSV->setModeKind) {
      this->pTSV->setModeKind = MIN_SETTING_MODE_KIND;
    }
  } else if (this->pTSV->itfcMode == TIME_SETTING_MODE && this->setModeSwt == BUTTON_ON) {
    
    if (this->autoModeSwt == BUTTON_ON) {
      --this->pTSV->setTimeModeKind;
    } else {
      ++this->pTSV->setTimeModeKind;
    }
    
    if (this->pTSV->setTimeModeKind < MIN_TIME_SETTING_MODE_KIND) {
      this->pTSV->setTimeModeKind = MAX_TIME_SETTING_MODE_KIND;
    } else if (MAX_TIME_SETTING_MODE_KIND < this->pTSV->setTimeModeKind) {
      this->pTSV->setTimeModeKind = MIN_TIME_SETTING_MODE_KIND;
    }

  } else if (this->pTSV->itfcMode == SERVO_SETTING_MODE && this->setModeSwt == BUTTON_ON) {
    
    if (this->autoModeSwt == BUTTON_ON) {
      --this->pTSV->setServoParamModeKind;
    } else {
      ++this->pTSV->setServoParamModeKind;
    }
    
    if (this->pTSV->setServoParamModeKind < MIN_SERVO_SETTING_MODE_KIND) {
      this->pTSV->setServoParamModeKind = MAX_SERVO_SETTING_MODE_KIND;
    } else if (MAX_SERVO_SETTING_MODE_KIND < this->pTSV->setServoParamModeKind) {
      this->pTSV->setServoParamModeKind = MIN_SERVO_SETTING_MODE_KIND;
    }

  } else if ((this->pTSV->itfcMode == SETTING_MODE || this->pTSV->itfcMode == TIME_SETTING_MODE || this->pTSV->itfcMode == SERVO_SETTING_MODE) && this->finishSetModeSwt == BUTTON_ON) {
    if (this->pTSV->itfcMode == SETTING_MODE) {
      this->pTSV->setModeKind = SET_UNDEFINED;
      this->pDusk2DawnWrap->reset(&this->pTSB->latlngBag);        
      this->pStorage->save();
    } else if (this->pTSV->itfcMode == TIME_SETTING_MODE) {
      this->pTSV->setTimeModeKind = SET_TIME_UNDEFINED;

      if (this->pTSV->setTimeOk) {
        this->pRtc->write(this->pTSV->setTm);
        this->pRtc->read(this->pTSV->tm);
      }
    } else if (this->pTSV->itfcMode == SERVO_SETTING_MODE) {
      this->pTSV->setServoParamModeKind = SET_SERVO_PARAM_UNDEFINED;

      this->pStorage->save();
    }

    this->setMode(AUTO_MODE);
    this->pServo->reset(); // 内部変数をリセットしてサーボモーターを動かす
  }
}

//------------------------------------------------------
// モード毎の処理
//------------------------------------------------------

void TSTask::processMode()
{
  if (this->pTSV->itfcMode == SERVO_SETTING_MODE) {
    // サーボ設定中は自動・手動の処理が走るとまずいので、
    // なにもさせない。
  } else {
    if (this->pTSB->actMode == AUTO_MODE) {
      // 自動
      this->processAutoMode();

    } else if (this->pTSB->actMode == MANUAL_MODE) {
      // 手動
      this->processManualMode();
    }
  }
    
  if (this->pTSV->itfcMode == SETTING_MODE) {
    // 設定
    this->processSettingMode();
  } else if (this->pTSV->itfcMode == TIME_SETTING_MODE) {
    // 時刻設定
    this->processTimeSettingMode();
  } else if (this->pTSV->itfcMode == SERVO_SETTING_MODE) {
    // サーボ設定
    this->processServoSettingMode();
  }
}

//------------------------------------------------------
// 温度を設定する
//------------------------------------------------------

void TSTask::setTemperature()
{
  if (this->pTSV->itfcMode != SERVO_SETTING_MODE) {
    int temperature = (0 <= this->pTSV->resetStartSecTime) ? MAX_TEMPERATURE : this->pTSB->temperature;
    this->pServo->setTemperatureWithCache(temperature, this->pTSB->minTemperaturePulse, this->pTSB->maxTemperaturePulse);
  }
}

//------------------------------------------------------
// 自動モードの処理
//------------------------------------------------------

void TSTask::processAutoMode()
{
  // pTSV->sunriseTime および pTSV->sunsetTime には日の出時刻および日の入り時刻が、深夜 0 時からの時間(分)で入っている。
  // NOTE: 取得に失敗している場合は共にマイナスの値が設定される。

  // 午前の温度処理
  this->processAmTask();

  // 午後の温度処理
  this->processPmTask();

  // リセットの処理
  this->processResetTask();

  // リセットのチェック処理
  this->processResetCheck();
}

//------------------------------------------------------
// 午前の温度処理
//------------------------------------------------------

void TSTask::processAmTask()
{
  if (0 <= this->pTSV->sunriseTime) {

    int startTime = this->pTSV->sunriseTime + this->pTSB->amStartSRATime;
    int endTime = startTime + this->pTSB->amEndTemperatureTime;

    if (this->currentTime < this->pTSV->sunriseTime) {
      // 日の出前は午後の最終温度を設定する
      this->pTSB->temperature = this->pTSB->pmEndTemperature;
    } else if (this->pTSV->sunriseTime <= this->currentTime && this->currentTime < startTime) {
      // 日の出から処理開始までの期間は午前の初期温度を設定する
      this->pTSB->temperature = this->pTSB->amStartTemperature;
    } else {
      this->pTSB->temperature = this->processTemperature(
        startTime,
        this->pTSB->amStartTemperature,
        endTime,
        this->pTSB->amEndTemperature
      );
    }
  }
}

//------------------------------------------------------
// 午後の温度処理
//------------------------------------------------------

void TSTask::processPmTask()
{
  if (0 <= this->pTSV->sunsetTime) {

    if (this->currentTime < this->pTSB->pmStartTime) {
      return;
    }

    int startTime = this->pTSB->pmStartTime;
    int endTime = this->pTSV->sunsetTime - this->pTSB->pmEndSSBTime;

    if (endTime < startTime) {
      // 午後の開始時刻が日の入り時刻を超えている場合は、午後の最終温度を設定する
      this->pTSB->temperature = this->pTSB->pmEndTemperature;
    } else {
      this->pTSB->temperature = this->processTemperature(
          startTime,
          this->pTSB->pmStartTemperature,
          endTime,
          this->pTSB->pmEndTemperature
        );
    }
  }
}

//------------------------------------------------------
// 温度処理
//------------------------------------------------------

int TSTask::processTemperature(int startTime, int startTemperature, int endTime, int endTemperature)
{
  long startSecTime = (long)startTime * 60;
  long endSecTime = (long)endTime * 60;

  int temperature = startTemperature;
  int addTemp = 0;
  int tempSpan = 0;

  long secTimeSpan = endSecTime - startSecTime;

  if (this->currentSecTime < startSecTime || endSecTime <= this->currentSecTime) {
    return endTemperature;
  }

  if (startTemperature <= endTemperature) {
    tempSpan = endTemperature - startTemperature;
    addTemp = 1;
  } else {
    tempSpan = startTemperature - endTemperature;
    addTemp = -1;
  }
  if (tempSpan == 0) {
    return endTemperature;
  }

  long secTimeRange = secTimeSpan / (long)tempSpan;
  if (secTimeRange != 0) {
    for (long secTime = startSecTime; secTime <= endSecTime; ) {
      long nextSecTime = secTime + secTimeRange;
      if (secTime <= this->currentSecTime && this->currentSecTime < nextSecTime) {
        break;
      }
      secTime = nextSecTime;
      temperature += addTemp;
    }
  } else {
    temperature = endTemperature;
  }

  if (endSecTime <= this->currentSecTime) {
    temperature = endTemperature;
  }

  return temperature;
}

//------------------------------------------------------
// リセットの処理
//------------------------------------------------------

void TSTask::processResetTask()
{
  if (0 <= this->pTSV->resetStartSecTime) {
    // リセット開始時刻が設定されている場合は処理しない
    return;
  }

  if (this->pTSB->resetParam.resetPattern == RESET_NONE) {
    // リセットしないときは処理しない
    return;
  }

  if (this->pTSV->sunriseTime < 0 || this->pTSV->sunsetTime < 0) {
    // NOTE: 日の出時刻または日の入り時刻を取得できていない時は処理しない
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

  int currentHour = this->pTSV->tm.Hour;

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
  int currentMinute = this->pTSV->tm.Minute;
  int currentSecond = this->pTSV->tm.Second;

  if (this->pTSB->resetParam.resetPattern == RESET_DAY && 0 == evalHour) {
    // リセットパターンが日の出から日の入りまでの場合、日の出直後にはリセットしない
    return;
  }

  if ((evalHour % this->pTSB->resetParam.intervalHour) == 0 && currentMinute == 0 && currentSecond == 0) {
    // 処理開始時(分は切り捨て)を基準としてリセットを行う時間間隔(時)で、正時にリセットを開始する
    this->setReset();
  }
}

//------------------------------------------------------
// リセットのチェック処理
//------------------------------------------------------

void TSTask::processResetCheck()
{
  if (0 <= this->pTSV->resetStartSecTime) {
    if (this->pTSV->resetStartSecTime <= this->currentSecTime && this->currentSecTime < this->pTSV->resetEndSecTime) {
      // リセット中
    } else {
      this->cancelReset();
    }
  }
}

//------------------------------------------------------
// リセットの設定処理
//------------------------------------------------------

void TSTask::setReset()
{
  this->pTSV->resetStartSecTime = this->currentSecTime;
  this->pTSV->resetEndSecTime = this->currentSecTime + (this->pTSB->resetParam.resetMinutes * 60);
}

//------------------------------------------------------
// リセットのキャンセル処理
//------------------------------------------------------

void TSTask::cancelReset()
{
  this->pTSV->resetStartSecTime = -1;
  this->pTSV->resetEndSecTime = -1;
}

//------------------------------------------------------
// 手動モードの処理
//------------------------------------------------------

void TSTask::processManualMode()
{
  if (this->tempDownSwt == BUTTON_ON) {
    // 温度設定を下げる
    this->pTSB->temperature = this->decValue(this->pTSB->temperature, MIN_TEMPERATURE);
    this->pStorage->save();
  } else if (this->tempUpSwt == BUTTON_ON) {
    // 温度設定を上げる
    this->pTSB->temperature = this->incValue(this->pTSB->temperature, MAX_TEMPERATURE);
    this->pStorage->save();
  }

  if (this->setModeSwt == BUTTON_ON) {
    this->cancelReset();
  }

  // リセットのチェック処理
  this->processResetCheck();
}

//------------------------------------------------------
// 設定モードの処理
//------------------------------------------------------

void TSTask::processSettingMode()
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
    } else if (this->pTSV->setModeKind == SET_PM_START_TIME) {
      this->pTSB->pmStartTime = this->decValue(this->pTSB->pmStartTime, MIN_PM_START_TIME, 10);
    } else if (this->pTSV->setModeKind == SET_PM_END_SSBTIME) {
      this->pTSB->pmEndSSBTime = this->decValue(this->pTSB->pmEndSSBTime, MIN_PM_END_SSBTIME, 10);
    } else if (this->pTSV->setModeKind == SET_PM_START_TEMPERATURE) {
      this->pTSB->pmStartTemperature = this->decValue(this->pTSB->pmStartTemperature, MIN_PM_START_TEMPERATURE);
    } else if (this->pTSV->setModeKind == SET_PM_END_TEMPERATURE) {
      this->pTSB->pmEndTemperature = this->decValue(this->pTSB->pmEndTemperature, MIN_PM_END_TEMPERATURE);
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
    } else if (this->pTSV->setModeKind == SET_AM_END_TEMPERATURE) {
      this->pTSB->amEndTemperature = this->incValue(this->pTSB->amEndTemperature, MAX_AM_END_TEMPERATURE);
    } else if (this->pTSV->setModeKind == SET_PM_START_TIME) {
      this->pTSB->pmStartTime = this->incValue(this->pTSB->pmStartTime, MAX_PM_START_TIME, 10);
    } else if (this->pTSV->setModeKind == SET_PM_END_SSBTIME) {
      this->pTSB->pmEndSSBTime = this->incValue(this->pTSB->pmEndSSBTime, MAX_PM_END_SSBTIME, 10);
    } else if (this->pTSV->setModeKind == SET_PM_START_TEMPERATURE) {
      this->pTSB->pmStartTemperature = this->incValue(this->pTSB->pmStartTemperature, MAX_PM_START_TEMPERATURE);
    } else if (this->pTSV->setModeKind == SET_PM_END_TEMPERATURE) {
      this->pTSB->pmEndTemperature = this->incValue(this->pTSB->pmEndTemperature, MAX_PM_END_TEMPERATURE);
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

void TSTask::processTimeSettingMode()
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
// サーボ設定の処理
//------------------------------------------------------

void TSTask::processServoSettingMode()
{
  if (this->tempDownSwt == BUTTON_ON) {
    if (this->pTSV->setServoParamModeKind == SET_SERVO_PARAM_MIN_TEMPERATURE_PULSE) {
      this->pTSB->minTemperaturePulse = this->decValue(this->pTSB->minTemperaturePulse, MIN_SERVO_PULSE);
    } else if (this->pTSV->setServoParamModeKind == SET_SERVO_PARAM_MAX_TEMPERATURE_PULSE) {
      this->pTSB->maxTemperaturePulse = this->decValue(this->pTSB->maxTemperaturePulse, MAX_SERVO_PULSE - SERVO_PULSE_ADJUSTMENT_RANGE);
    }
  } else if (this->tempUpSwt == BUTTON_ON) {
    if (this->pTSV->setServoParamModeKind == SET_SERVO_PARAM_MIN_TEMPERATURE_PULSE) {
      this->pTSB->minTemperaturePulse = this->incValue(this->pTSB->minTemperaturePulse, MIN_SERVO_PULSE + SERVO_PULSE_ADJUSTMENT_RANGE);
    } else if (this->pTSV->setServoParamModeKind == SET_SERVO_PARAM_MAX_TEMPERATURE_PULSE) {
      this->pTSB->maxTemperaturePulse = this->incValue(this->pTSB->maxTemperaturePulse, MAX_SERVO_PULSE);
    }
  }

  if (this->pTSV->setServoParamModeKind == SET_SERVO_PARAM_MIN_TEMPERATURE_PULSE) {
    this->pServo->setTemperature(MIN_TEMPERATURE, this->pTSB->minTemperaturePulse, this->pTSB->maxTemperaturePulse);
  } else if (this->pTSV->setServoParamModeKind == SET_SERVO_PARAM_MAX_TEMPERATURE_PULSE) {
    this->pServo->setTemperature(MAX_TEMPERATURE, this->pTSB->minTemperaturePulse, this->pTSB->maxTemperaturePulse);
  }
}

//------------------------------------------------------
// モードの設定
//------------------------------------------------------

void TSTask::setMode(int mode)
{
  this->pTSV->itfcMode = mode;
  switch (mode) {
    case AUTO_MODE:
    case MANUAL_MODE:
      this->pTSB->actMode = mode;
      break;
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
