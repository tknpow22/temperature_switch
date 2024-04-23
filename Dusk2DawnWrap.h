#ifndef __DUSK_2_DAWN_WRAP_H__
#define __DUSK_2_DAWN_WRAP_H__

#include <Dusk2Dawn.h>
#include "temperature_switch.h"

////////////////////////////////////////////////////////
// 日の出・日の入り取得
////////////////////////////////////////////////////////

class Dusk2DawnWrap {
public:
  // コンストラクタ
  Dusk2DawnWrap() {}

public:
  // 初期化
  void init(LatLngBag* platlngBag);
  // リセット
  void reset(LatLngBag* platlngBag);

public:
  // 日の出時刻(分)を得る
  int getSunriseTime(int year, int month, int day);
  // 日の入り時刻(分)を得る
  int getSunsetTime(int year, int month, int day);

private:
  // 日の出・日の入り時刻キャッシュ
  int sunriseTime = -1;
  int sunsetTime = -1;
  // 日の出・日の入り時刻キャッシュに保持している日の出・日の入りの年月日
  unsigned long sunriseYMD = 0;
  unsigned long sunsetYMD = 0;

  // 日の出・日の入りを取得するためのクラスライブラリ
  Dusk2Dawn* pDusk2Dawn = NULL;
};

#endif
