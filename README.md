# Mesh Vitals — off-grid health telemetry you can actually trust

*Meshtastic Build-Off 2026 entry.*

A DIY health band and a Meshtastic health node that relay **quality-gated vital
signs** (heart rate, SpO2) over LoRa — for hikers, search-and-rescue, and anyone
outside cellular coverage.

## Why

Meshtastic already has health telemetry (MAX30102 heart rate / SpO2), but the
stock pipeline uses the Maxim reference beat-detector, which happily reports
confident-looking **wrong** BPM under motion or poor skin contact. On a mesh,
a wrong vital is worse than no vital: it wastes shared airtime and can mislead
the person watching the dashboard.

We replace it with an **autocorrelation estimator + signal-quality index (SQI)**:
readings that don't carry enough periodicity are suppressed instead of sent.
No reading beats a wrong reading.

## What's here

| Path | What |
|---|---|
| `hr-quality/` | The estimator as portable C++ + a synthetic-PPG host test suite (8/8: 50–150 BPM within ±1, low-perfusion cases pass, pure noise auto-rejected) |
| `patches/` | Changes against upstream [meshtastic/firmware](https://github.com/meshtastic/firmware): `HrSqiEstimator.h`, SQI gating in `MAX30102Sensor.cpp`, and a fix for an upstream bug where invalid SpO2 was still flagged `has_spO2 = true` |
| `docs/` | Build log, field-test results *(TODO: hardware arriving)* |

## Hardware

- 2× [XIAO nRF52840 & Wio-SX1262 Kit for Meshtastic](https://www.seeedstudio.com/XIAO-nRF52840-Wio-SX1262-Kit-for-Meshtastic-p-6400.html) — health node + relay
- MAX30102 PPG sensor on the health node's I²C (`seeed_xiao_nrf52840_kit_i2c` variant)
- L76K GNSS — position + vitals in one telemetry stream
- Companion project: a from-scratch [DIY fitness band](https://github.com/) *(link TBD)* built on XIAO nRF52840 Sense Plus — same estimator, BLE Heart Rate Service

## Build

```bash
git clone --depth 1 https://github.com/meshtastic/firmware meshtastic-firmware
./patches/apply.sh
cd meshtastic-firmware && pio run -e seeed_xiao_nrf52840_kit_i2c_health
```

Note: health telemetry is compiled out of every stock Meshtastic build (a global
`-DMESHTASTIC_EXCLUDE_HEALTH_TELEMETRY=1` in `platformio.ini`). Our patch adds a
`seeed_xiao_nrf52840_kit_i2c_health` env that re-enables it for this kit —
flash the resulting UF2 by double-tapping reset and dragging it onto the drive.

Host-test the algorithm without any hardware:

```bash
cd hr-quality && c++ -std=c++17 -O2 test_hr_algo.cpp hr_algo.cpp -o test_hr && ./test_hr
```

## Status

- [x] Estimator designed + validated on synthetic PPG (host tests, 8/8)
- [x] Ported into Meshtastic `MAX30102Sensor` with SQI gating (+ SpO2 validity bugfix)
- [x] Firmware builds for `seeed_xiao_nrf52840_kit_i2c_health` — estimator verified present in the ELF, UF2 ready to flash
- [ ] Hardware bring-up (parts ordered, ETA late July)
- [ ] Real-wrist validation & threshold tuning
- [ ] Field test: vitals over multi-hop mesh, range numbers
- [ ] Upstream PR to meshtastic/firmware
