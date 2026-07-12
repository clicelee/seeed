// 손목용 심박 추정 (S4.5) — 이식 가능한 순수 C++ (Arduino 의존성 없음)
// band.ino에 그대로 넣을 수 있고, 호스트에서 합성 PPG로 단위 테스트 가능.
//
// 파이프라인: 8초 윈도우(fs=25Hz) 수집 → 베이스라인 제거(센터드 MA)
//   → 자기상관(autocorrelation) → 최대 상관 lag = 주기 → BPM
//   → 정규화 상관값 = 신호품질지표(SQI). SQI 낮으면 "측정 불가".
//
// 자기상관을 쓰는 이유: FFT보다 코드가 작고(임베디드 친화), 백색잡음에 강하며
// 라이브러리가 필요 없음. 손목 PPG처럼 파형이 흐릴 때도 "주기성"은 살아있음.

#ifndef HR_ALGO_H
#define HR_ALGO_H

#include <stdint.h>
#include <stddef.h>

struct HrConfig {
  float fs        = 25.0f;  // 샘플레이트 Hz (MAX30102 duty-cycle과 맞춤)
  float windowSec = 8.0f;   // 분석 윈도우 길이(초)
  float minBpm    = 40.0f;  // 탐색 하한
  float maxBpm    = 220.0f; // 탐색 상한
  float sqiMin    = 0.30f;  // 이 값 미만이면 유효하지 않은 측정으로 간주
};

struct HrResult {
  bool  valid;   // SQI가 임계값 이상인가
  float bpm;     // 추정 심박 (valid==false면 무의미)
  float sqi;     // 0..1, 신호 품질(자기상관 정규화 피크)
};

// x: 길이 n의 raw PPG 샘플(예: MAX30102 IR). n은 대략 fs*windowSec.
// 반환: 추정 결과. 순수 함수라 상태 없음(테스트·재현 쉬움).
HrResult estimateHeartRate(const float* x, size_t n, const HrConfig& cfg);

#endif
