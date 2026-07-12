#include "hr_algo.h"

// 센터드 이동평균을 빼서 베이스라인(DC + 저주파 흔들림)을 제거한다.
// 창 폭 w는 최저 심박 주기의 약 절반 → 심박 성분은 보존, 그보다 느린 드리프트만 깎임.
static void detrend(const float* x, size_t n, float* out, int w) {
  int half = w / 2;
  for (size_t i = 0; i < n; i++) {
    int lo = (int)i - half; if (lo < 0) lo = 0;
    int hi = (int)i + half; if (hi > (int)n - 1) hi = (int)n - 1;
    float sum = 0.0f;
    for (int j = lo; j <= hi; j++) sum += x[j];
    out[i] = x[i] - sum / (float)(hi - lo + 1);
  }
}

HrResult estimateHeartRate(const float* x, size_t n, const HrConfig& cfg) {
  HrResult r{false, 0.0f, 0.0f};
  if (n < 16) return r;

  // 탐색할 lag 범위: BPM ↔ lag 변환은 lag = 60*fs/BPM
  int minLag = (int)(60.0f * cfg.fs / cfg.maxBpm);  // 빠른 심박 = 짧은 lag
  int maxLag = (int)(60.0f * cfg.fs / cfg.minBpm + 0.9999f);
  if (minLag < 2) minLag = 2;
  if (maxLag > (int)n - 2) maxLag = (int)n - 2;
  if (maxLag <= minLag) return r;

  // 베이스라인 제거: 최저 심박(40bpm=1.5s=37샘플) 주기의 절반 창
  static float y[512];
  if (n > 512) n = 512;
  int w = (int)(cfg.fs * 0.75f);           // ~0.75초 창
  detrend(x, n, y, w);

  // r0 = 신호 에너지(정규화 분모)
  float r0 = 0.0f;
  for (size_t i = 0; i < n; i++) r0 += y[i] * y[i];
  if (r0 <= 1e-9f) return r;               // 완전 평탄 → 신호 없음

  // 각 lag에서 자기상관 → 정규화 → 최대 탐색
  int   bestLag = -1;
  float bestVal = -1e30f;
  float rPrev = 0, rCur = 0, rNext = 0;    // 파라볼릭 보간용 이웃값
  for (int lag = minLag; lag <= maxLag; lag++) {
    float acc = 0.0f;
    for (size_t i = 0; i + lag < n; i++) acc += y[i] * y[i + lag];
    float norm = acc / r0;
    if (norm > bestVal) { bestVal = norm; bestLag = lag; }
  }
  if (bestLag < 0) return r;

  // 최적 lag 주변 3점으로 파라볼릭 보간 → 서브샘플 정밀도
  auto acorr = [&](int lag) -> float {
    if (lag < 1 || lag > (int)n - 2) return 0.0f;
    float acc = 0.0f;
    for (size_t i = 0; i + lag < n; i++) acc += y[i] * y[i + lag];
    return acc / r0;
  };
  rPrev = acorr(bestLag - 1);
  rCur  = bestVal;
  rNext = acorr(bestLag + 1);
  float denom = (rPrev - 2.0f * rCur + rNext);
  float delta = 0.0f;
  if (denom != 0.0f) delta = 0.5f * (rPrev - rNext) / denom;  // [-0.5,0.5]
  if (delta > 0.5f) delta = 0.5f;
  if (delta < -0.5f) delta = -0.5f;

  float lag = (float)bestLag + delta;
  r.bpm   = 60.0f * cfg.fs / lag;
  r.sqi   = bestVal;
  r.valid = (bestVal >= cfg.sqiMin);
  return r;
}
