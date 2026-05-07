---
description: LLM 교체·세션 초기화에도 동일 산출이 나오도록 규범·금지·완료 정의를 고정한다. RVC 그리드 컨트롤러 하네스.
alwaysApply: true
---

# 재현성 (Reproducibility) — 모델 무관 에이전트 규범

같은 저장소·같은 슬래시 명령을 쓸 때 **큰 LLM을 바꿔도** 절차·산출물 형태·검증 게이트가 크게 흔들리지 않게 한다.

## 1. 단일 근거 원칙 (Single source of truth)

- 기능·비기능·추적의 **규범 순서**: `arch/system.md` → `arch/requirements/fr-nfr.md` → `arch/usecase/UC-*.md` → `arch/vnv/traceability-matrix.md` → 구현·테스트.
- 위 파일이 **없으면** 만들되, **이미 있으면 먼저 읽고** 그 구조·ID 체계를 **임의로 바꾸지 않는다** (FR-001~005, UC-001~005, NFR 접두 유지).
- 사용자가 요구하지 않는 한 **새 요구 ID를 발명하거나** 기존 ID 의미를 뒤집지 않는다.

## 2. RVC 제품 불변조건 (Product invariants — 영문 규범, 요약)

아래는 **비전/FR을 쓰거나 코드를 쓸 때 반드시 반영**한다. HW 저수준 제어는 **비범위**.

- Automatic cleaning / mopping on household surfaces; **focus only on automatic cleaning SW**.
- Default: **go straight forward while cleaning** when not in avoidance/escape.
- **Obstacle (partial)**: stop cleaning motion → **turn aside** (left or right per policy) → resume forward cleaning.
- **Triple blocked (front + left + right)**: **move backward** (policy may repeat until a lateral opening exists) → turn aside → forward cleaning.
- **Dust**: **increase cleaning power for a bounded duration** (timer/debounce per `ControllerConfig` / UC-005).
- **Out of scope for now** (mention in `arch/system.md` under future/extended only; do not implement as required behavior): extra sensors detail, spot circling, mobile app comms, ML.

## 3. 구현 결정성 (Deterministic policy — 모호할 때 이것을 택한다)

- **Navigation (partial avoidance, front blocked)**: 한쪽만 열림 → 그쪽; **양쪽 열림 → Left 우선** (FR-003 / UC-003).
- **Enclosure escape**: 삼면 막힘 후 후진 루프는 **`CleaningCoordinator` 단일 오케스트레이션**; `NavigationPolicy::plan_escape_enclosure` 는 **탈출 시퀀스를 대체하지 않는다** (스텁·정책 보조만).
- **시스템 테스트**: **GTest 아님**; **시뮬레이터(JSON 맵) + 스크립트** (`system_tests/run_all.py` 패턴).

## 4. 기술 스택 고정 (바꾸려면 사용자 승인)

| 영역 | 고정 |
|------|------|
| 언어 | C++17 |
| 빌드 | CMake |
| 단위·통합 | Google Test, 타겟 분리 (`rvc_unit_tests`, `rvc_integration_tests` 등 저장소 관례) |
| 시스템 | 시뮬 바이너리 + JSON 시나리오 + **헤드리스** 러너 |
| 추적 | `arch/vnv/traceability-matrix.md`, 시나리오 `trace` 배열, 테스트 `// Trace:` / `// Covers:` |

## 5. 완료 정의 (Definition of Done — 구현 변경 후)

에이전트는 **가능하면** 다음을 **직접 실행**해 실패 시 수정한다 (환경에 도구가 없으면 한계를 짧게 보고).

1. CMake 구성·빌드 (Release/Debug 팀 관례).
2. `rvc_unit_tests` → `rvc_integration_tests` 순.
3. `python system_tests/run_all.py --sim <path-to-rvc_sim>` (시나리오 개수·전역 DisplayState 정책은 `arch/vnv/system-tests.md` / `run_all.py` 준수).

## 6. 금지·경고 (모델 붕괴 방지)

- **`arch/` 없이** “요구사항을 내가 요약했으니 건너뛴다” — 금지. `workflow-pipeline` 순서를 역행하지 않는다.
- 시스템 테스트를 **GTest로 대체** — 금지.
- FR/UC 번호 체계를 **다른 과제 표기**(예: 별도 팀의 UC-03 의미)와 **섞어 쓰지 않는다** — `arch/usecases.md` Name 과 통일.
- 불확실하면 **가정으로 채우지 말고** 사용자에게 질문한 뒤 `arch/` 에 기록한다.

## 7. 슬래시 명령과의 관계

- 전체 순서: **`/ooad/workflow-pipeline`**.
- 단계 품질: **`/ooad/review-phase-checkpoints`**.
- 본 RULE은 **모든 OOAD 작업**에 우선 적용되며, 주제별 RULE(`unit-test`, `cicd`, …)과 **충돌 시 본 절의 제품 불변조건·기술 스택**이 이긴다.
