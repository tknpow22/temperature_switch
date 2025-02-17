#ifndef __TEMPERATURE_SWITCH_H__
#define __TEMPERATURE_SWITCH_H__

#include <TimeLib.h>

// 緯度・経度の最小・最大
// NOTE: 整数部と小数部を別々に設定する関係からチェックが面倒なので、1つ大きい・小さい値を下限・上限とする。
// 緯度の最小・最大(-90 - 90)
// 整数部
#define MIN_LAT_IPART  -89
#define MAX_LAT_IPART  89
// 小数部
#define MIN_LAT_DPART  0
#define MAX_LAT_DPART  99

// 経度の最小・最大(-180 - 180)
// 整数部
#define MIN_LNG_IPART  -179
#define MAX_LNG_IPART  179
// 小数部
#define MIN_LNG_DPART  0
#define MAX_LNG_DPART  99

// 稼働地点の緯度・経度(デフォルト)
////#define DEFAULT_LATITUDE  33.5087   // 緯度
////#define DEFAULT_LONGITUDE 133.8416  // 経度
#define DEFAULT_LATITUDE_IPART  33
#define DEFAULT_LATITUDE_DPART1 50
#define DEFAULT_LATITUDE_DPART2 87

#define DEFAULT_LONGITUDE_IPART   133
#define DEFAULT_LONGITUDE_DPART1  84
#define DEFAULT_LONGITUDE_DPART2  16

// ボタンの状態
// プルアップ接続なので、押されると 0、離すと 1
#define BUTTON_ON LOW
#define BUTTON_OFF  HIGH

//
// 温度設定
//

// 温度設定の最小・最大: 可動領域が 180 度より小さくなる値を指定すること
#define MIN_TEMPERATURE 12
#define MAX_TEMPERATURE 33

//
// 温度指定
//

// 日の出から処理開始までの時間(分)の最小・最大
#define MIN_AM_START_SRATIME  0
#define MAX_AM_START_SRATIME  (3 * 60)

// 午前の初期温度の最小・最大
#define MIN_AM_START_TEMPERATURE MIN_TEMPERATURE
#define MAX_AM_START_TEMPERATURE MAX_TEMPERATURE

// 午前の最終温度の最小・最大
#define MIN_AM_END_TEMPERATURE MIN_TEMPERATURE
#define MAX_AM_END_TEMPERATURE MAX_TEMPERATURE

// 処理開始から午前の最終温度に達するまでの時間(分)の最小・最大
#define MIN_AM_END_TEMPERATURE_TIME  (0)
#define MAX_AM_END_TEMPERATURE_TIME  (6 * 60)

// 午後の初期温度の最小・最大
#define MIN_PM_START_TEMPERATURE MIN_TEMPERATURE
#define MAX_PM_START_TEMPERATURE MAX_TEMPERATURE

// 午後の最終温度の最小・最大
#define MIN_PM_END_TEMPERATURE MIN_TEMPERATURE
#define MAX_PM_END_TEMPERATURE MAX_TEMPERATURE

// 午後温度の開始時刻(分)の最小・最大
#define MIN_PM_START_TIME  (10 * 60)
#define MAX_PM_START_TIME  (16 * 60)

// 午後の処理終了から日の入りまでの時間(分)の最小・最大
#define MIN_PM_END_SSBTIME  (0)
#define MAX_PM_END_SSBTIME  (3 * 60)

//
// リセットパラメータ
//


// リセットパターン
#define RESET_NONE  0 // リセットしない
#define RESET_FULL  1 // 終日
#define RESET_DAY   2 // 日の出から日の入りまで

// リセットパターンの最小・最大
#define MIN_RESET_PATTERN  RESET_NONE
#define MAX_RESET_PATTERN  RESET_DAY

// リセットから除外する時刻の最小・最大
#define MIN_RESET_EXCLUSION_HOUR  0
#define MAX_RESET_EXCLUSION_HOUR  24

// リセットを行う時間間隔(時)の最小・最大
#define MIN_RESET_INTERVAL_HOUR 1
#define MAX_RESET_INTERVAL_HOUR 9

// リセットする時間(分)の最小・最大
#define MIN_RESET_MINUTES  1
#define MAX_RESET_MINUTES  9

//
// モード
//
#define AUTO_MODE 0   // 自動
#define MANUAL_MODE 1 // 手動
#define SETTING_MODE  2   // 設定
#define TIME_SETTING_MODE 3 // 時刻設定
#define SERVO_SETTING_MODE 4  // サーボ設定

//
// 設定変更時
//

// 設定種別
#define SET_UNDEFINED (-1)  // 未定義
#define SET_AM_START_TEMPERATURE 0 // 午前の初期温度
#define SET_AM_END_TEMPERATURE 1 // 午前の最終温度
#define SET_PM_START_TEMPERATURE  2 // 午後の初期温度
#define SET_PM_END_TEMPERATURE  3 // 午後の最終温度
#define SET_AM_START_SRATIME  4 // 日の出から処理開始までの時間(分)
#define SET_AM_END_TEMPERATURE_TIME  5 // 処理開始から午前の最終温度に達するまでの時間(分)
#define SET_PM_START_TIME 6 // 午後温度の開始時刻(分)
#define SET_PM_END_SSBTIME  7 // 処理終了から日の入りまでの時間(分)
#define SET_RESET_PATTERN  8 // リセットパターン
#define SET_RESET_INTERVAL_HOUR 9  // リセットを行う時間間隔(時)
#define SET_RESET_MINUTES 10  // リセットする時間(分)
#define SET_RESET_EXCLUSION_HOUR_START  11   // リセットから除外する時刻の開始(時)
#define SET_RESET_EXCLUSION_HOUR_END  12   // リセットから除外する時刻の終了(時)
#define SET_LAT_IPART  13 // 緯度(整数部)
#define SET_LAT_DPART1  14 // 緯度(少数部1)
#define SET_LAT_DPART2  15 // 緯度(少数部2)
#define SET_LNG_IPART  16 // 経度(整数部)
#define SET_LNG_DPART1  17 // 経度(少数部1)
#define SET_LNG_DPART2  18 // 経度(少数部2)

#define MIN_SETTING_MODE_KIND SET_AM_START_TEMPERATURE  // 設定種別の最小値
#define MAX_SETTING_MODE_KIND SET_LNG_DPART2  // 設定種別の最大値

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

#define MIN_TIME_SETTING_MODE_KIND  SET_TIME_YEAR // 時刻設定種別の最小値
#define MAX_TIME_SETTING_MODE_KIND  SET_TIME_OK // 時刻設定種別の最大値

//
// サーボ設定
//

// サーボモーターのパルス範囲
// NOTE: FS5103B のデータシートだと 600 - 2400 なのだが、少し行き過ぎるため上を少なめにした
#define MIN_SERVO_PULSE 600
#define MAX_SERVO_PULSE 2275

// サーボ設定での調整範囲
// MIN_SERVO_PULSE + SERVO_PULSE_ADJUSTMENT_RANGE < MAX_SERVO_PULSE - SERVO_PULSE_ADJUSTMENT_RANGE
// となる値を設定すること
#define SERVO_PULSE_ADJUSTMENT_RANGE 250

// サーボ設定種別
#define SET_SERVO_PARAM_UNDEFINED  (-1)  // 未定義
#define SET_SERVO_PARAM_MIN_TEMPERATURE_PULSE  0 // 温度設定の最小時のパルス値
#define SET_SERVO_PARAM_MAX_TEMPERATURE_PULSE  1 // 温度設定の最大時のパルス値

#define MIN_SERVO_SETTING_MODE_KIND SET_SERVO_PARAM_MIN_TEMPERATURE_PULSE // サーボ設定種別の最小値
#define MAX_SERVO_SETTING_MODE_KIND SET_SERVO_PARAM_MAX_TEMPERATURE_PULSE // サーボ設定種別の最大値

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
#define SETTING_MODE_PIN  8

// 設定モード終了ピン
#define FINISH_SETTING_MODE_PIN 12

//
// 設定データ
//

#define TSB_TYPE_BEGIN  'T'
#define TSB_TYPE_END  'S'
#define TSB_TYPE_VERSION  8

// 緯度・経度保存用
struct LatLngBag {
  // 小数表現 III.DDdd を格納する
  // 緯度
  int latitudeIPart;  // III
  int latitudeDPart1; // DD
  int latitudeDPart2; // dd
  // 経度
  int longitudeIPart;
  int longitudeDPart1;
  int longitudeDPart2;
};

// リセットパラメータ
struct ResetParam {
  int resetPattern; // リセットパターン
  int intervalHour; // リセットを行う時間間隔(時)
  int resetMinutes; // リセットする時間(分)
  int exclusionHourStart; // リセットから除外する時刻の開始(時)
  int exclusionHourEnd; // リセットから除外する時刻の終了(時)
};

//
// 設定保存用
//
struct TemperatureSwitchBag {
  char typeBegin;
  int typeVersion;
  // 午前の処理
  int amStartSRATime; // 日の出から処理開始までの時間(分)  
  int amEndTemperatureTime;  // 処理開始から午前の最終温度に達するまでの時間(分)
  int amStartTemperature;  // 午前の初期温度
  int amEndTemperature;  // 午前の最終温度
  // 午後の処理
  int pmStartTime; // 午後温度の開始時刻(分)
  int pmEndSSBTime; // 午後の処理終了から日の入りまでの時間(分)
  int pmStartTemperature;  // 午後の初期温度
  int pmEndTemperature;  // 午後の最終温度
  //
  int minTemperaturePulse;  // 温度設定の最小時のパルス値
  int maxTemperaturePulse;  // 温度設定の最大時のパルス値
  // 現在の設定を覚える
  int actMode;  // 動作モード(AUTO_MODE、MANUAL_MODE のいずれか)
  int temperature;  // 現在の温度設定(MANUAL_MODE の時のみ有効)
  // 緯度・経度
  LatLngBag latlngBag;
  // リセットパラメータ
  ResetParam resetParam;
  //
  char typeEnd;
};

//
// 変数
//
struct TSVariables {
  int itfcMode = AUTO_MODE; // インターフェースモード
  tmElements_t tm; // 現在時刻
  int setModeKind = SET_UNDEFINED;  // 設定種別
  int setTimeModeKind = SET_TIME_UNDEFINED;  // 時刻設定種別
  int setServoParamModeKind = SET_SERVO_PARAM_UNDEFINED;  // サーボ設定種別
  int sunriseTime = -1;  // 日の出時刻
  int sunsetTime = -1;  // 日の入り時刻
  // リセット機能用
  long resetStartSecTime = -1;  // リセットの開始時刻(秒)
  long resetEndSecTime = -1;    // リセットの終了時刻(秒)
  // 時刻設定用変数
  tmElements_t setTm;
  bool setTimeOk = false;
};

#endif