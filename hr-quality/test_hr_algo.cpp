// 합성 PPG로 estimateHeartRate() 검증 (하드웨어 없이).
// 빌드·실행:  c++ -std=c++17 -O2 test_hr_algo.cpp hr_algo.cpp -o test_hr && ./test_hr
//
// 합성 신호 = 심박 파형(기본파 + 2차 고조파, 실제 PPG의 dicrotic notch 흉내)
//   + 베이스라인 흔들림(저주파 드리프트) + 백색잡음.
// "손가락 없음" 케이스는 잡음만 → SQI가 낮아 valid=false 여야 통과.

#include "hr_algo.h"
#include <cstdio>
#include <cmath>
#include <random>

static const float FS = 25.0f;
static const int   N  = (int)(FS * 4);  // 8초 = 200 샘플

// 결정론적 가우시안 잡음(시드 고정 → 테스트 재현 가능)
static std::mt19937 rng(12345);
static std::normal_distribution<float> gauss(0.0f, 1.0f);

// trueBpm 심박의 합성 PPG 생성. noiseAmp/wanderAmp는 신호 대비 배율.
static void makePpg(float* out, int n, float trueBpm, float noiseAmp, float wanderAmp) {
  float f = trueBpm / 60.0f;  // Hz
  for (int i = 0; i < n; i++) {
    float t = i / FS;
    float beat = std::sin(2 * M_PI * f * t)
               + 0.35f * std::sin(2 * M_PI * 2 * f * t + 0.5f);  // 2차 고조파
    float wander = wanderAmp * std::sin(2 * M_PI * 0.1f * t);     // 0.1Hz 드리프트
    float noise  = noiseAmp * gauss(rng);
    out[i] = 2000.0f + 100.0f * (beat + wander) + 100.0f * noise; // MAX30102 IR 스케일 흉내
  }
}

static int passCount = 0, failCount = 0;

static void check(const char* name, bool cond, const char* detail) {
  printf("  [%s] %s %s\n", cond ? "PASS" : "FAIL", name, detail);
  cond ? passCount++ : failCount++;
}

static void scenarioBpm(const char* name, float trueBpm, float noiseAmp, float wanderAmp, float tolBpm) {
  static float ppg[512];
  makePpg(ppg, N, trueBpm, noiseAmp, wanderAmp);
  HrConfig cfg;
  HrResult r = estimateHeartRate(ppg, N, cfg);
  char buf[160];
  snprintf(buf, sizeof(buf), "true=%.0f got=%.1f sqi=%.2f (tol=%.0f)",
           trueBpm, r.bpm, r.sqi, tolBpm);
  check(name, r.valid && std::fabs(r.bpm - trueBpm) <= tolBpm, buf);
}

int main() {
  printf("손목 심박 알고리즘 테스트 (fs=%.0fHz, window=4s (Meshtastic MAX30102_BUFFER_LEN=100 @ 25Hz))\n", FS);

  printf("\n정상 범위(깨끗~중간 잡음):\n");
  scenarioBpm("resting 50bpm",   50,  0.10f, 0.5f, 3.0f);
  scenarioBpm("resting 62bpm",   62,  0.15f, 0.8f, 3.0f);
  scenarioBpm("normal 75bpm",    75,  0.20f, 1.0f, 3.0f);
  scenarioBpm("active 110bpm",  110,  0.25f, 1.0f, 4.0f);
  scenarioBpm("exercise 150bpm",150,  0.30f, 1.0f, 5.0f);

  printf("\n낮은 관류(손목 = 신호 약함, 잡음 큼):\n");
  scenarioBpm("weak 68bpm",      68,  0.60f, 1.5f, 5.0f);
  scenarioBpm("weak 95bpm",      95,  0.60f, 1.5f, 5.0f);

  printf("\n손가락/손목 없음(잡음만 → valid=false 여야 함):\n");
  {
    static float noise[512];
    makePpg(noise, N, 0, 0, 0);
    for (int i = 0; i < N; i++) noise[i] = 2000.0f + 100.0f * gauss(rng); // 순수 잡음
    HrResult r = estimateHeartRate(noise, N, HrConfig{});
    char buf[128];
    snprintf(buf, sizeof(buf), "valid=%d sqi=%.2f (임계 0.30)", r.valid, r.sqi);
    check("no-signal rejected", !r.valid, buf);
  }

  printf("\n결과: %d 통과, %d 실패\n", passCount, failCount);
  return failCount == 0 ? 0 : 1;
}
