# Traceability Matrix (RTM) — RVC Cleaning Controller

본 표는 FR/NFR/UC ↔ SSD ↔ 구현 식별자 ↔ GTest ↔ 시스템 JSON `trace` 필드 ↔ 슬래시 명령·RULE 단계의 **단일 근거(SSoT)**다 (`reproducibility` RULE §1, `00-ooad-core` RULE §추적성). 코드·시나리오·문서 변경 시 **동일 PR**에서 본 표를 갱신한다.

## §1. ID 체계 (불변)

- **FR-001 ~ FR-005**, **UC-001 ~ UC-005**, **NFR-{DET,PERF,MAINT,TEST,SYS,CI,SAFE,REPRO}-001**.
- ID 의미는 임의로 뒤집지 않는다 (`reproducibility` RULE §1).

## §2. UC ↔ FR ↔ 핵심 구현 ↔ SSD/Interaction

| UC | FR | 핵심 구현 (헤더 / 메서드) | SSD / Interaction 문서 |
|----|----|----------------------------|-----------------------|
| UC-001 Start/Stop Session | FR-002 (+ FR-001 baseline) | `rvc::app::CleaningCoordinator::startSession()`, `::stopSession()` (`include/rvc/app/cleaning_coordinator.hpp`, `src/app/cleaning_coordinator.cpp`) | `arch/ssd/UC-001-main-success.md`, `arch/design/interaction/UC-001-start-stop.md` |
| UC-002 Forward Cleaning | FR-001 | `CleaningCoordinator::tick()` Driving 분기, `DefaultNavigationPolicy::nextDriveCommand` (no front) | `arch/ssd/UC-002-forward-cleaning.md`, `arch/design/interaction/UC-002-forward.md` |
| UC-003 Avoid Partial Obstacle | FR-003 | `CleaningCoordinator::tick()` partial 분기, `DefaultNavigationPolicy::nextDriveCommand` (front, ≥1 side open) — **양쪽 열림 시 Left 우선** | `arch/ssd/UC-003-partial-obstacle.md`, `arch/design/interaction/UC-003-partial-avoid.md` |
| UC-004 Escape Triple-Blocked | FR-004 | `CleaningCoordinator::tick()` Escaping 단계 (단일 오케스트레이션), `DefaultNavigationPolicy::plan_escape_enclosure` (보조) | `arch/ssd/UC-004-enclosure-escape.md`, `arch/design/interaction/UC-004-enclosure-escape.md` |
| UC-005 Boost Power on Dust | FR-005 | `DefaultCleaningPowerPolicy::nextLevel` 타이머/디바운스, `CleaningCoordinator::tick()` setPower 호출 | `arch/ssd/UC-005-dust-boost.md`, `arch/design/interaction/UC-005-dust-boost.md` |

`Trace:` 주석이 적용된 핵심 헤더/cpp:
- `include/rvc/app/cleaning_coordinator.hpp`, `src/app/cleaning_coordinator.cpp`
- `include/rvc/domain/default_navigation_policy.hpp`, `src/domain/default_navigation_policy.cpp`
- `include/rvc/domain/default_cleaning_power_policy.hpp`, `src/domain/default_cleaning_power_policy.cpp`
- `include/rvc/ports/*.hpp`, `include/rvc/tech/*.hpp`, `src/tech/grid_world.cpp`
- `tools/rvc_sim.cpp`

## §3. NFR ↔ 검증 위치

| NFR | 검증 자리 |
|-----|-----------|
| NFR-DET-001 (Determinism) | `tests/integration/coordinator_grid_integration_test.cpp::DeterministicAcrossTwoRuns`, `system_tests/run_all.py --check-determinism`, ST-018, ST-022, ST-030. |
| NFR-PERF-001 (≤1 s / 1k tick) | ST-022 (200 tick smoke), ST-030 (500 tick), CI `system_tests` 시간 한도. |
| NFR-MAINT-001 (SOLID) | DCD `arch/design/class-diagram.md` SOLID 점검 절, `arch/design/packages.md` 의존 방향, `archive-static-analysis-report` 단계. |
| NFR-TEST-001 (Coverage ≥80% line) | CI `unit_tests` + `integration_tests` 커버리지 아티팩트 (gcovr). |
| NFR-SYS-001 (≥30 sim cases pos/neg) | `arch/vnv/system-tests.md`, `system_tests/maps/*.json` 31개. |
| NFR-CI-001 (build → unit → integration → system) | `.github/workflows/ci.yml` `needs:` 그래프. |
| NFR-SAFE-001 (Stopped → no actuator) | `tests/unit/cleaning_coordinator_test.cpp::SessionStopped_NoActuatorCalls`, ST-007/015/023/024/029. |
| NFR-REPRO-001 (LLM 교체 재현성) | 본 RTM + `reproducibility` RULE + `bootstrap-context` 컨텍스트 번들. |

## §4. GTest ↔ FR/UC

| 테스트 파일 | 커버 (Covers) | 비고 |
|-------------|---------------|------|
| `tests/unit/navigation_policy_test.cpp` | FR-001 UC-002, FR-003 UC-003, FR-004 UC-004 | 정책 단위 |
| `tests/unit/cleaning_power_policy_test.cpp` | FR-005 UC-005 | 부스트 타이머 |
| `tests/unit/cleaning_coordinator_test.cpp` | FR-001..005 UC-001..005, NFR-SAFE-001 | Coordinator 단위 (stub sensor/actuator) |
| `tests/unit/grid_world_test.cpp` | technical 어댑터 | 시뮬 입력/센서 정합 |
| `tests/unit/sensor_reading_test.cpp` | FR-003 UC-003, FR-004 UC-004 | helpers |
| `tests/integration/coordinator_grid_integration_test.cpp` | FR-001..005 UC-001..005, NFR-DET-001 | 실제 Grid + Coordinator |
| `tests/integration/scenario_smoke_test.cpp` | 시뮬 하네스 smoke | malformed 입력 등 |

## §5. 시스템 시나리오 ↔ FR/UC/NFR

| ST-id | 유형 | trace |
|-------|------|-------|
| ST-001 | positive | FR-001, UC-002 |
| ST-002 | positive | FR-001, UC-002 |
| ST-003 | positive | FR-001, UC-002, NFR-PERF-001 |
| ST-004 | positive | FR-003, UC-003, NFR-DET-001 |
| ST-005 | positive | FR-003, UC-003 |
| ST-006 | positive | FR-003, UC-003 |
| ST-007 | positive | FR-004, UC-004, NFR-SAFE-001 |
| ST-008 | positive | FR-004, UC-004 |
| ST-009 | positive | FR-005, UC-005 |
| ST-010 | positive | FR-005, UC-005 |
| ST-011 | positive | FR-005, UC-005 |
| ST-012 | positive | FR-001, UC-002, FR-005, UC-005 |
| ST-013 | positive | FR-001, FR-003, UC-002, UC-003 |
| ST-014 | positive | FR-001, FR-003, UC-002, UC-003 |
| ST-015 | positive | FR-004, UC-004 |
| ST-016 | positive | FR-001, FR-003, UC-002, UC-003 |
| ST-017 | positive | FR-001, UC-002 |
| ST-018 | positive | FR-001, UC-002, NFR-DET-001 |
| ST-019 | positive | FR-001, UC-002 |
| ST-020 | positive | FR-001, UC-002 |
| ST-021 | positive | FR-001, UC-002 |
| ST-022 | positive | FR-001, UC-002, NFR-PERF-001, NFR-DET-001 |
| ST-023 | negative | FR-004, UC-004, NFR-SAFE-001 |
| ST-024 | negative | FR-004, UC-004, NFR-SAFE-001 |
| ST-025 | negative | FR-005, UC-005 |
| ST-026 | negative | FR-005, UC-005 |
| ST-027 | negative | FR-005, UC-005 |
| ST-028 | negative | FR-001, FR-002, UC-001, UC-002 |
| ST-029 | negative | FR-002, UC-001, NFR-SAFE-001 |
| ST-030 | negative | FR-004, UC-004, NFR-DET-001, NFR-PERF-001 |
| ST-031 | negative | FR-003, FR-004, UC-003, UC-004, NFR-SAFE-001 |

본 §5는 `system_tests/maps/ST-*.json` 의 `trace` 배열과 **bit-identical**이어야 한다 (RULE: `system-test`).

## §6. NFR ↔ FR

| NFR | 영향 받는 FR |
|-----|--------------|
| NFR-DET-001 | FR-001, FR-003, FR-004, FR-005 |
| NFR-PERF-001 | FR-001 |
| NFR-MAINT-001 | 전 FR (구조 제약) |
| NFR-TEST-001 | 전 FR (검증) |
| NFR-SYS-001 | 전 FR (시스템 검증) |
| NFR-CI-001 | 전 FR (검증 파이프라인) |
| NFR-SAFE-001 | FR-002, FR-004 |
| NFR-REPRO-001 | 전 FR/UC (메타) |

## §7. 핵심 구현 ↔ 슬래시 명령·RULE 단계

| 구현 식별자 | 명령/RULE 단계 |
|--------------|----------------|
| `CleaningCoordinator` | `/ooad/map-to-code` → `arch/design/implementation-mapping.md`, RULE `ooi`/`implementation` |
| `DefaultNavigationPolicy` | 〃 |
| `DefaultCleaningPowerPolicy` | 〃 |
| `GridWorld` / `GridSensor` / `GridActuator` | 〃 (technical 레이어, `arch/design/packages.md`) |
| `rvc_sim` | `/ooad/plan-system-tests-gui` (시뮬 + JSON 시나리오) |
| `tests/unit/*`, `tests/integration/*` | `/ooad/develop-unit-tests-gtest`, RULE `unit-test` |
| `system_tests/run_all.py`, `maps/*.json` | RULE `system-test`, NFR-SYS-001 |
| `.github/workflows/ci.yml` | `/ooad/configure-cicd-github-actions`, RULE `cicd`, NFR-CI-001 |
| `reports/static-analysis/` | `/ooad/archive-static-analysis-report`, RULE `static-analysis` |
