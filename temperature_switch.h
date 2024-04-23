#ifndef __TEMPERATURE_SWITCH_H__
#define __TEMPERATURE_SWITCH_H__

#include <TimeLib.h>

// 稼働地点の緯度・経度
#define Longitude 133.8416  // 経度
#define Latitude  33.5087   // 緯度

// ボタンの状態
// プルアップ接続なので、押されると 0、離すと 1
#define BUTTON_ON LOW
#define BUTTON_OFF  HIGH

//
// 温度設定
//

// 温度設定の最小・最大
#define MIN_TEMPERATURE 11
#define MAX_TEMPERATURE 34

// 角度補正の最小・最大
#define MIN_ANGLE_CORRECTION  -9
#define MAX_ANGLE_CORRECTION  9

//
// 温度指定
//

// 日の出から処理開始までの時間(分)の最小・最大
#define MIN_AM_START_SRATIME  0
#define MAX_AM_START_SRATIME  (3 * 60)

// 午前の初期温度の最小・最大
#define MIN_AM_START_TEMPERATURE 14
#define MAX_AM_START_TEMPERATURE 28

// 午前の最終温度の最小・最大
#define MIN_AM_END_TEMPERATURE 16
#define MAX_AM_END_TEMPERATURE MAX_TEMPERATURE

// 処理開始から午前の最終温度に達するまでの時間(分)の最小・最大
#define MIN_AM_END_TEMPERATURE_TIME  (0)
#define MAX_AM_END_TEMPERATURE_TIME  (6 * 60)

// 午後温度で追加する温度の最小・最大
#define MIN_PM_PLUS_TEMPERATURE  0
#define MAX_PM_PLUS_TEMPERATURE  5

// 午後温度2で追加する温度の最小・最大
#define MIN_PM_PLUS_TEMPERATURE2  0
#define MAX_PM_PLUS_TEMPERATURE2  5

// 午後温度の開始時刻(分)の最小・最大
#define MIN_PM_PLUS_TEMPERATURE_TIME  (10 * 60)
#define MAX_PM_PLUS_TEMPERATURE_TIME  (19 * 60)

// 午後温度2を開始する日の入り前の時間(分)の最小・最大
#define MIN_PM_PLUS_TEMPERATURE2_SSBTIME  (0)
#define MAX_PM_PLUS_TEMPERATURE2_SSBTIME  (5 * 60)

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
// ピン設定
//

// サーボモーター
// 制御ピン
#define SERVO_PIN  10

// タクトスイッチ
// 使用ピン 2,4,7,8,12

// 自動モードピン - 長押しで年月日時刻設定へ遷移する
#define AUTO_MODE_PIN 2

// 温度設定ダウンピン
#define TEMP_DOWN_PIN 4

// 温度設定アップピン
#define TEMP_UP_PIN 7

// 設定モードピン
#define SET_MODE_PIN  8

// 設定モード終了ピン
#define FINISH_SET_MODE_PIN 12

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