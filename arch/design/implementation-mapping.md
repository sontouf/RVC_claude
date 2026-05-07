# SSD 연산 ↔ C++ 매핑 — implementation-mapping

본 문서는 SSD/시퀀스에서 등장한 연산을 실제 C++ 식별자(헤더/메서드)로 1:1 매핑한다. 새 연산 추가 시 본 표와 RTM §2·§7을 동일 작업에서 갱신한다.

## SSD 시스템 연산 → C++ 메서드

| SSD 연산 | C++ 식별자 | 헤더 |
|----------|------------|------|
| `startSession()` | `rvc::app::CleaningCoordinator::startSession()` | `include/rvc/app/cleaning_coordinator.hpp` |
| `stopSession()` | `rvc::app::CleaningCoordinator::stopSession()` | 〃 |
| `tick()` | `rvc::app::CleaningCoordinator::tick()` | 〃 |
| `sensors.read()` | `rvc::ports::ISensorPort::read()` → `rvc::tech::GridSensor::read()` | `include/rvc/ports/i_sensor_port.hpp`, `include/rvc/tech/grid_sensor.hpp` |
| `actuator.drive(cmd)` | `rvc::ports::IActuatorPort::drive(DriveCommand)` → `rvc::tech::GridActuator::drive(...)` | `include/rvc/ports/i_actuator_port.hpp`, `include/rvc/tech/grid_actuator.hpp` |
| `actuator.turn(dir)` | `IActuatorPort::turn(Direction)` → `GridActuator::turn(...)` | 〃 |
| `actuator.setPower(level)` | `IActuatorPort::setPower(CleaningPowerLevel)` → `GridActuator::setPower(...)` | 〃 |
| `nav.nextDriveCommand(reading,state)` | `rvc::domain::INavigationPolicy::nextDriveCommand(...)` → `DefaultNavigationPolicy::nextDriveCommand(...)` | `include/rvc/domain/i_navigation_policy.hpp`, `include/rvc/domain/default_navigation_policy.hpp` |
| `nav.plan_escape_enclosure(reading)` | `INavigationPolicy::plan_escape_enclosure(...)` → `DefaultNavigationPolicy::plan_escape_enclosure(...)` | 〃 |
| `pow.nextLevel(reading)` | `rvc::domain::ICleaningPowerPolicy::nextLevel(...)` → `DefaultCleaningPowerPolicy::nextLevel(...)` | `include/rvc/domain/i_cleaning_power_policy.hpp`, `include/rvc/domain/default_cleaning_power_policy.hpp` |
| `pow.reset()` | `ICleaningPowerPolicy::reset()` → `DefaultCleaningPowerPolicy::reset()` | 〃 |

## value types

| 도메인 개념 | C++ 식별자 | 헤더 |
|-------------|------------|------|
| `SensorReading` | `rvc::ports::SensorReading` (POD struct) | `include/rvc/ports/sensor_reading.hpp` |
| `DriveDecision` | `rvc::domain::DriveDecision` | `include/rvc/domain/decisions.hpp` |
| `EscapeAssist` | `rvc::domain::EscapeAssist` | 〃 |
| `ControllerConfig` | `rvc::app::ControllerConfig` | `include/rvc/app/controller_config.hpp` |
| `CoordinatorState` | `rvc::app::CoordinatorState` (private nested or detail) | `src/app/cleaning_coordinator.cpp` (private) |
| `DriveCommand`, `TurnCommand`, `Direction`, `CleaningPowerLevel`, `SessionState`, `Phase` | `rvc::ports::*` enums | `include/rvc/ports/enums.hpp` |

## 정책 ↔ 코디네이터 책임 분담

- **단계 전이 (Driving / Avoiding / Escaping)**: `CleaningCoordinator::tick()` 내부 결정. `Phase` 는 코디네이터 사적(state)임.
- **결정적 분기 (좌 우선)**: `DefaultNavigationPolicy::nextDriveCommand`에서 enforced.
- **부스트 타이머**: `DefaultCleaningPowerPolicy` 내부 `timerRemaining`.
- **삼면 막힘 후진/회전 시퀀스**: `CleaningCoordinator` 가 SSoT. `plan_escape_enclosure`는 한 줄짜리 보조(turn 결정)만.

## 추적 주석 컨벤션

각 핵심 헤더/cpp 파일 상단에는 다음 주석 블록을 둔다:

```cpp
// Trace: arch/vnv/traceability-matrix.md — FR-00x, UC-00y
```

GTest 파일은 `// Trace:` + `// Covers:` 두 줄(아래 unit-test RULE 참조).
