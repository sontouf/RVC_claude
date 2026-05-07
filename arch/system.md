# RVC Cleaning Controller — 시스템 정의

## 시스템 개요

### 이름 / 목적 / 풀고자 하는 문제

- **이름**: RVC Cleaning Controller (Robot Vacuum Cleaner — automatic cleaning software controller).
- **목적**: 가정 내 표면을 자동으로 청소·걸레질하는 RVC의 **소프트웨어 컨트롤러**를 정의·구현·검증한다. 본 프로젝트는 **자동 청소 기능 SW**에만 집중하며, 하드웨어 저수준 제어 상세 설계·구현은 비범위로 둔다.
- **풀고자 하는 문제**: 센서 입력(전·좌·우 장애물, 먼지)을 바탕으로 결정적이고 재현 가능한 주행·청소 의사결정을 한다. 동일 시나리오에서 동일한 결과가 나오도록 정책을 고정한다.

## 범위

### 포함 (In Scope)

- 자동 청소 SW 컨트롤러: 전진 청소, 부분 장애물 회피, 삼면 막힘 탈출, 먼지 감지 시 흡입 부스트.
- 격자 기반 시뮬레이터(`rvc_sim`)와 JSON 시나리오 기반 시스템 테스트 하네스.
- C++17 + CMake 빌드, Google Test 단위·통합, Python 시뮬 러너.

### 제외 (Out of Scope)

- HW 저수준 모터/PWM/IMU 드라이버 상세 설계·구현.
- 추가 센서 종류(LiDAR/카메라/ToF)의 상세 모델 및 융합.
- 모바일 앱 통신·클라우드 연동.
- 한 자리 회전 청소(spot circling)·머신러닝 기반 경로 학습.

### 근거

- 사용자 Preliminary requirements가 “automatic cleaning function”에만 초점을 맞추도록 명시(`bootstrap-context`).
- `reproducibility` RULE §2가 위 범위·비범위를 그대로 고정한다.

## 액터·이해관계자

| 액터 | 설명 |
|------|------|
| Owner (User) | RVC를 켜서 자동 청소 세션을 시작·정지하는 가정 사용자. |
| Cleaning Surface | 청소 대상 표면. 시스템은 이를 격자 셀로 추상화한다. |
| Sensor Subsystem (블랙박스 외부) | 전·좌·우 장애물 신호와 먼지 감지 신호를 제공한다(`ISensorPort`로 추상화). |
| Actuator Subsystem (블랙박스 외부) | 주행(전·후·좌·우 회전) 명령과 청소 파워 설정을 수행한다(`IActuatorPort`로 추상화). |
| Test Engineer | 시뮬레이터·시스템 테스트로 정책을 검증한다. |
| CI System (GitHub Actions) | build → unit → integration → system 순차 검증을 자동화한다. |

## 시스템 경계

```mermaid
flowchart LR
  Owner((Owner))
  subgraph SYS["RVC Cleaning Controller (SW only)"]
    APP[CleaningCoordinator]
    NAVP[NavigationPolicy]
    POWP[CleaningPowerPolicy]
  end
  Sensors[Sensor Subsystem<br/>front/left/right/dust]
  Actuators[Actuator Subsystem<br/>drive/turn/power]
  Sim[Simulator + GUI/Headless<br/>rvc_sim]

  Owner -->|start/stop session| SYS
  Sensors -->|ISensorPort.read()| SYS
  SYS -->|IActuatorPort.drive/turn/setPower| Actuators
  SYS -.tick().- Sim
  Sim -->|JSON scenarios| SYS
```

## 제약·가정

- **언어/스택 고정** (자세한 측정·게이트는 `requirements/fr-nfr.md`):
  - C++17, CMake.
  - 단위·통합: Google Test (`rvc_unit_tests`, `rvc_integration_tests`).
  - 시스템: 비-GTest. JSON 시나리오 + 헤드리스 시뮬 + Python 러너 (`system_tests/run_all.py`).
- **결정적 정책**:
  - 부분 장애물(전방 막힘): 한쪽만 열려 있으면 그쪽으로 회피, **양쪽 모두 열려 있으면 좌측 우선** (FR-003 / UC-003).
  - 삼면 막힘(전·좌·우): `CleaningCoordinator`가 후진 → 측면 통로 발생 시 회전 → 전진 청소 재개를 단일 오케스트레이션으로 수행 (FR-004 / UC-004). `NavigationPolicy::plan_escape_enclosure`는 **보조 스텁**이며 코디네이터의 시퀀스를 대체하지 않는다.
  - 먼지 감지: `ControllerConfig::dustBoostTicks`로 정의된 **시간 한도** 동안 청소 파워를 한 단계 부스트 후 자동 복귀 (FR-005 / UC-005).
- **HW 블랙박스 가정**: 센서/액추에이터의 물리적 응답 시간은 제어 주기(`tick`) 단위로 추상화되며, 본 SW는 한 tick 안에 결정·명령을 산출한다.
- **추적성 규범 단일 표**: `arch/vnv/traceability-matrix.md`. ID 체계는 FR-001~005, UC-001~005, NFR-* 접두를 유지한다(`reproducibility` RULE §1).

## 향후 확장 (현 버전 비필수)

- LiDAR/ToF 등 **추가 센서** 융합·정밀 매핑.
- **한 지점 집중 청소**(spot circling) 시나리오.
- **모바일 앱**으로의 원격 제어/상태 보고.
- **머신러닝**으로 가구 배치 학습·경로 최적화.

> 위 항목은 본 버전에서 **필수 구현이 아니다**(`reproducibility` RULE §2). FR/NFR/UC에는 “out of scope (future)”로만 표시하고 코드/테스트로 강제하지 않는다.

## 체크포인트

- [x] 시스템 경계가 명확하다 (SW 컨트롤러 vs 외부 센서/액추에이터/HW 드라이버).
- [x] 범위가 명확하다 (자동 청소 SW만; HW 상세·추가 센서 등은 future).
- [x] 제약이 식별되었다 (C++17/CMake/GTest/JSON 시나리오/SOLID/CI 순서).
- [x] 결정적 정책이 본문에 명시되었다 (FR-003 양쪽 열림 시 Left 우선, FR-004 코디네이터 단일 오케스트레이션).
