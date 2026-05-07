# 시스템 테스트 카탈로그 — RVC Cleaning Controller

## 환경·버전

- 시뮬레이터: `rvc_sim` (CMake target). 헤드리스 실행, JSON `system_tests/maps/*.json` 시나리오를 Python 러너가 텍스트 형식으로 변환해 stdin 표준 입력 대신 파일 인자로 전달한다.
- 러너: `system_tests/run_all.py` (Python ≥ 3.10).
- 결정성 게이트: 동일 시나리오 2회 실행 시 stdout 동등성(`--check-determinism`).
- 본 문서는 **GTest를 사용하지 않는다**(`reproducibility` RULE §3, `system-test` RULE).

## 실행 방법

```bash
# 1) 빌드 (Release)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target rvc_sim

# 2) 시스템 테스트 실행
python system_tests/run_all.py --sim build/rvc_sim

# 3) (선택) 결정성 게이트
python system_tests/run_all.py --sim build/rvc_sim --check-determinism
```

CI에서는 `system_tests` job 이 `integration_tests` 통과 후에 위 절차를 실행한다.

## 케이스 카탈로그

| ID | 제목 | 유형 | 전제 | 단계 요약 | GUI/관측 증거 | 합격 기준 |
|----|------|------|------|-----------|---------------|-----------|
| ST-001 | Forward cleaning in open 5x5 | positive | 5x5 open, robot (1,1) N | 20 tick 자유 청소 | stdout: `collisions=0`, `cleaned_cells>=2` | `no_collisions`, `session_running` |
| ST-002 | Forward cleaning starts from center facing E | positive | 5x5 open, robot (2,2) E | 30 tick | `cleaned_cells>=4` | 동일 |
| ST-003 | Forward cleaning in larger 7x7 grid | positive | 7x7 open | 80 tick | `cleaned_ratio>=0.40` | NFR-PERF-001 |
| ST-004 | Both sides open prefers Left (FR-003) | positive | 5x5, (1,2) W, max=2 | turn → forward | final (1,3,S) | 결정적 ID |
| ST-005 | Front+Left blocked turns Right | positive | 5x5, (1,1) N, max=2 | turn(Right) → forward | final (2,1,E) | 〃 |
| ST-006 | Front+Right blocked turns Left | positive | 5x5, (3,1) N, max=2 | turn(Left) → forward | final (2,1,W) | 〃 |
| ST-007 | Triple-blocked dead-end backs off within budget then halts | positive | 3x6 1-wide corridor, max_backoff=2 | 후진 → 한도 도달 → safety stop | `collisions=0`, robot halts inside corridor | FR-004 revised, 안전 |
| ST-008 | Triple → side opens → resume cleaning | positive | 5x5 + side gate | 30 tick | `cleaned_cells>=2` | UC-004 |
| ST-009 | Dust cell triggers boost | positive | dust at (2,1), boost=3, max=2 | tick2: setPower(Boosted) | `last_power=Boosted` | UC-005 |
| ST-010 | Dust boost auto-returns to Nominal | positive | boost=2, max=4 | 시간 만료 | `last_power=Nominal` | UC-005 |
| ST-011 | Long boost window keeps Boosted at end | positive | boost=10, max=4 | 부스트 유지 | `last_power=Boosted` | UC-005 |
| ST-012 | No dust ever — power stays Nominal | positive | 5x5 no dust | 20 tick | `last_power=Nominal` | NFR-DET-001 |
| ST-013 | Around a single internal obstacle | positive | 5x5 with pillar (2,2) | 60 tick | `cleaned_cells>=3` | 정상 항행 |
| ST-014 | Corridor with center block | positive | 7x5 with inner block | 80 tick | `cleaned_cells>=4` | 〃 |
| ST-015 | Long 1-wide corridor closed at top | positive | 3x8 corridor | 40 tick 오실레이션 | `collisions=0` | 안전 |
| ST-016 | U-shaped layout | positive | 5x5 U | 60 tick | `cleaned_cells>=3` | 정상 항행 |
| ST-017 | Small room with inner column | positive | 5x5 column | 40 tick | `cleaned_cells>=2` | 〃 |
| ST-018 | Determinism over 100 ticks | positive | 5x5 open | 100 tick | `--check-determinism` 시 stdout 동등 | NFR-DET-001 |
| ST-019 | Heading East baseline | positive | 5x5 open, (1,1) E | 30 tick | `cleaned_cells>=3` | 〃 |
| ST-020 | Heading South baseline | positive | 5x5 open, (3,1) S | 30 tick | 〃 | 〃 |
| ST-021 | Heading West baseline | positive | 5x5 open, (3,3) W | 30 tick | 〃 | 〃 |
| ST-022 | Long-run 9x9 cleaning | positive | 9x9 open | 200 tick | `cleaned_ratio>=0.30` | NFR-PERF-001 |
| ST-023 | Fully enclosed 1x1 — bounded collisions | negative | 3x3 닫힘, backoff=2 | 충돌 한도 안에서 안전 정지 | `max_collisions<=2` | NFR-SAFE-001 |
| ST-024 | max_backoff=0 — no backward attempt | negative | 동 enclosure, backoff=0 | 후진 안 함 | `no_collisions` | NFR-SAFE-001 |
| ST-025 | dust_boost=0 disables boost | negative | dust + boost=0 | 부스트 미발동 | `last_power=Nominal` | UC-005 |
| ST-026 | Robot starts on dust cell | negative | dust at start | tick1 boost | `last_power=Boosted` | UC-005 |
| ST-027 | Dust everywhere — power stays Boosted | negative | dust on every reachable cell | 20 tick | `last_power=Boosted` | UC-005 |
| ST-028 | Single-tick session | negative | max=1 | 한 번의 결정 | `total_ticks=1`, running | UC-001 |
| ST-029 | Zero-tick session | negative | max=0 | 무이동 | start 위치 유지 | NFR-SAFE-001 |
| ST-030 | Long-run safety: dead-end stays halted (FR-004 revised) | negative | 3x5 dead-end, max_backoff=2, 500 tick | 후진 한도 후 정지 유지 | `no_collisions`, 결정적 | NFR-DET-001, FR-004 revised |
| ST-031 | Maze with multiple dead-ends | negative | 7x7 maze | 200 tick | `no_collisions` | UC-003/004 |
| ST-032 | Backoff continues while only front opens; exits only when a side opens | positive | 5x5 pocket + bottom corridor, (1,1) E, max_backoff=4 | F만 열린 중간 cell에서도 backoff 지속 → R 열리는 cell에서 turn(R) → 다음 tick부터 정상 driving | final pose `(1,3,S)`, `cleaned_cells>=5`, `no_collisions` | FR-004 revised, UC-004, NFR-SAFE-001 |

총 **32** 케이스 (positive 23, negative 9). NFR-SYS-001 (≥30 / pos·neg) 충족.

## 결함 보고 절차

1. `run_all.py` 출력에서 FAIL 라인을 그대로 인용한다 (예: `FAIL ST-009: expected last_power=Boosted, got Nominal`).
2. 해당 시나리오 JSON과 시뮬 stdout을 빌드 아티팩트(`build/system_tests.log`)에 첨부.
3. `arch/vnv/traceability-matrix.md` §5 의 ST-id ↔ FR/UC 매핑이 변경되면 같은 PR에서 갱신한다.
4. 가능하면 GTest 단위·통합 단계에서 회귀 케이스를 추가한다 (RTM §4).

## 체크포인트

- [x] **≥30** 케이스, positive 23 / negative 9 균형 (총 32).
- [x] **GTest 미사용** (Python + 시뮬 바이너리).
- [x] CI 순서: `system_tests` needs `integration_tests` (`.github/workflows/ci.yml`).
- [x] 각 시나리오에 `trace` 배열이 RTM §5와 동기화.
