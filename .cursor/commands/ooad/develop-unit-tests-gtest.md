---
description: GTest로 단위 테스트와 통합 테스트를 구현한다. 단위(격리·stub) 후 통합(모듈 결합), 커버리지·CI 순서는 cicd RULE. 시스템 테스트 제외. (규칙 ooad/unit-test)
---

# develop-unit-tests-gtest — GTest + RTM

## 목표

`tests/unit/`, `tests/integration/` 에 Google Test 를 추가·갱신한다. **시스템 테스트(`system_tests/`)는 제외.**

## 추적성 (필수)

- **`rules/ooad/reproducibility/RULE.md`**: 시스템 테스트는 GTest 금지, 스택 고정 등을 위반하지 않는다.
- **`arch/vnv/traceability-matrix.md` §4** 를 연다. 다루는 FR/UC 가 표에 없으면 행을 추가한다.
- 각 `*_test.cpp` **파일 최상단**에 주석:
   ```cpp
   // Trace: arch/vnv/traceability-matrix.md §4
   // Covers: FR-00x UC-00y [, FR-00z …]
   ```
- focal 메서드·정책 분기가 요구와 1:1 대응하는지 확인; 불일치 시 **먼저** `fr-nfr.md` / UC 문서 / RTM 중 어디를 바꿀지 결정한다.

## 절차 요약

- 단위: `NavigationPolicy`, `CleaningPowerPolicy`, `GridWorld`, … — stub/fake 최소화 или 0.
- 통합: `CleaningCoordinator` + 실제 `GridSensor`/`GridActuator` 결합.
- CMake: `rvc_unit_tests`, `rvc_integration_tests` 타겟 유지.

## 완료 조건

- 로컬에서 단위→통합 순 실행 통과.
- RTM §4 및 각 테스트 파일 `// Covers:` 가 일치.
