---
description: OOAD OOI — SPEC·DCD·시퀀스 근거로 SOLID 준수 C++를 agentk.sourceDirectory에 매핑한다. 하네스 PDF의 SPEC→ARCH→CODE 흐름 준수. (규칙 ooad/ooi, implementation, harness-engineering)
---

# map-to-code — 구현 매핑과 추적성

## 목표

`arch/design/class-diagram.md`, `arch/design/interaction/`, `arch/design/packages.md`, `arch/design/implementation-mapping.md` 및 유스케이스·SSD를 **근거**로 `include/`·`src/`·`tools/` 에 C++ 를 반영한다.

## 추적성 (필수)

1. 구현 전 **`rules/ooad/reproducibility/RULE.md`** 제품 불변조건·스택을 위반하지 않는지 확인한다.
2. 구현 전 **`arch/vnv/traceability-matrix.md`** §2 의 해당 FR/UC 행을 읽고, 바꾸는 **클래스·함수**가 그 열과 일치하는지 확인한다.
3. 구현 후 **`arch/design/implementation-mapping.md`** 의 SSD 연산 ↔ C++ 표를 갱신한다 (새 시스템 연산·포트 메서드 추가 시).
4. 중요 구현 파일(코디네이터·정책·포트·시뮬 진입점) 헤더 또는 익명 네임스페이스 직후에 블록 주석:
   ```cpp
   // Trace: arch/vnv/traceability-matrix.md — FR-00x, UC-00y (해당 ID)
   ```
5. 동일 작업에서 **`traceability-matrix.md`** §2·§7 의 “핵심 구현” 열이 **실제 식별자**와 어긋나면 수정한다.

## 절차 요약

- DIP: 고수준(`CleaningCoordinator`, `*Policy`)는 `ISensorPort`/`IActuatorPort` 에만 의존.
- 격자 어댑터(`GridSensor`/`GridActuator`/`GridWorld`)는 technical 레이어에 유지.
- UC-004 탈출 오케스트레이션은 **`CleaningCoordinator`** 가 담당(RTM §2 주석 참고).

## 완료 조건

- 빌드 타겟 `rvc_core`, `rvc_sim`(옵션), GTest 타겟과 링크된다.
- **`/ooad/workflow-pipeline`** 4단계 직후 RTM §2 점검이 끝났다.
