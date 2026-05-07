---
description: 워크플로/재구현 시 에이전트와 사용자가 동일 컨텍스트를 공유하기 위한 붙여넣기 번들. LLM 교체 시 함께 제공 권장.
---

# OOAD 부트스트랩 컨텍스트 (복사용)

`/ooad/workflow-pipeline` 실행 시 **아래 블록을 사용자 메시지에 함께 붙여 넣는다** (또는 에이전트가 요청 시 사용자에게 요청한다).

## A. 제품 한 줄·범위

- RVC **소프트웨어 컨트롤러**만: 자동 청소·걸레(행동 규칙). **HW 제어 상세 설계·구현은 비범위.**

## B. Preliminary requirements (영문 그대로 유지 권장)

- An RVC automatically cleans and mops household surface.
- It goes straight forward while cleaning.
- If its sensors found an obstacle, it stops cleaning, turns aside left or right, and goes forward with cleaning.
- If there are obstacles in both front, left and right, it move backward and turn aside left or right, and goes forward.
- If it detects dust, power up the cleaning for a while.
- We do not consider the detail design and implementation on HW controls.
- We only focus on the automatic cleaning function.

## C. Future / extended (문서에만 명시, 필수 구현 아님)

- Add or sensors; circulate one spot; mobile app; ML — `arch/system.md` Future 절.

## D. 비기능·검증 (이 저장소 관례)

- **SOLID** 준수.
- **CI/CD** (GitHub Actions): 빌드 → **GTest 단위** → **GTest 통합** → **시뮬레이터 시스템 테스트(비-GTest)** → (정책) 정적 분석 보관.
- 시스템 테스트 **≥30** 시나리오, pos/neg, 규범·`trace` 는 `arch/vnv/traceability-matrix.md`.

## E. 결정적 정책 (에이전트가 임의로 바꾸지 말 것)

- 전방 막힘·**양쪽 통로**: **좌회피** (FR-003).
- 삼면 막힘 탈출: **코디네이터**가 후진·회전·재전진 오케스트레이션 (`traceability-matrix.md`, UC-004).

## F. 완료 시 검증 (가능 시 실행)

- `cmake --build` 후 `rvc_unit_tests`, `rvc_integration_tests`, `python system_tests/run_all.py --sim <rvc_sim>`.

---

**지시:** 에이전트는 위 A–F를 **요구사항의 최소 공통 분모**로 삼고, 상세 ID·SSD·코드는 저장소 `arch/` 및 RULE `reproducibility`를 따른다.
