#ifndef __TEMPERATURE_SWITCH_H__
#define __TEMPERATURE_SWITCH_H__

#include <TimeLib.h>

//
// 温度設定
//

// 温度設定の最小・最大
#define MIN_TEMPERATURE 11
#define MAX_TEMPERATURE 34

//
// モード
//
#define AUTO_MODE 0   // 自動
#define MANUAL_MODE 1 // 手動
#define SET_MODE  2   // 設定
#define SET_TIME_MODE 3 // 時刻設定

//
// 設定変更時
//

// 設定種別
#define SET_UNDEFINED (-1)  // 未定義
#define SET_AM_START_TEMPERATURE 0 // 午前の初期温度
#define SET_AM_END_TEMPERATURE 1 // 午前の最終温度
#define SET_PLUS_PM_TEMPERATURE_TIME  2 // 午後温度の開始時刻(分)
#define SET_PLUS_PM_TEMPERATURE  3 // 午後温度での追加温度
#define SET_PLUS_END_TEMPERATURE_SSBTIME2  4 // 午後温度2を開始する日の入り前の時間(分)
#define SET_PLUS_END_TEMPERATURE2  5 // 午後温度2での追加温度
#define SET_AM_START_SRATIME  6 // 日の出から処理開始までの時間(分)
#define SET_AM_END_TEMPERATURE_TIME  7 // 処理開始から午前の最終温度に達するまでの時間(分)
#define SET_ANGLE_CORRECTION  8 // 角度補正

#define MIN_SET_MODE_KIND SET_AM_START_TEMPERATURE  // 設定種別の最小値
#define MAX_SET_MODE_KIND SET_ANGLE_CORRECTION  // 設定種別の最大値

//
// 時刻設定
//

// 時刻設定種別
#define SET_TIME_UNDEFINED  (-1)  // 未定義
#define SET_TIME_YEAR 0 // 年
#define SET_TIME_MONTH  1 // 月
#define SET_TIME_DAY  2 // 日
#define SET_TIME_HOUR 3 // 時
#define SET_TIME_MINUTE 4 // 分
#define SET_TIME_SECOND  5 // 秒
#define SET_TIME_OK 6 // 確認

#define MIN_SET_TIME_MODE_KIND  SET_TIME_YEAR // 時刻設定種別の最小値
#define MAX_SET_TIME_MODE_KIND  SET_TIME_OK // 時刻設定種別の最大値

//
// 設定データ
//

#define TSB_TYPE_BEGIN  'T'
#define TSB_TYPE_END  'S'
#define TSB_TYPE_VERSION  3

struct TemperatureSwitchBag {
  char typeBegin;
  int typeVersion;
  int amStartSRATime; // 日の出から処理開始までの時間(分)  
  int amEndTemperatureTime;  // 処理開始から午前の最終温度に達するまでの時間(分)
  int amStartTemperature;  // 午前の初期温度
  int amEndTemperature;  // 午前の最終温度
  int angleCorrection; // 角度補正
  int pmPlusTempretureTime; // 午後温度の開始時刻(分)
  int pmPlusTempreture; // 午後温度での追加温度
  int pmPlusTempreture2SSBTime;  // 午後温度2を開始する日の入り前の時間(分)
  int pmPlusTempreture2; // 午後温度2での追加温度
  // 現在の設定を覚える
  bool isManualMode; // 手動時か否か
  int manualTemperature;  // 手動時の温度設定
  //
  char typeEnd;
};

//
// 変数
//
struct TSVariables {
  int mode = AUTO_MODE; // モード
  tmElements_t tm; // 現在時刻
  int temperature = MAX_TEMPERATURE;  // 現在の温度設定
  int setModeKind = SET_UNDEFINED;  // 設定種別
  int setTimeModeKind = SET_TIME_UNDEFINED;  // 時刻設定種別
  int sunriseTime = -1;  // 日の出時刻
  int sunsetTime = -1;  // 日の入り時刻
  // 時刻設定用変数
  tmElements_t setTm;
  bool setTimeOk = false;
};

#endif