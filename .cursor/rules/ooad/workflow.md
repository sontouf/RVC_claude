# 워크플로 (OOAD + FR/NFR + C++/검증 + CI/CD)

산출물 루트: `agentk.architectureDirectory`(예: `arch`). 코드: `agentk.sourceDirectory`(예: `src`).

각 OOAD·V&V 주제는 **arch-with-ai**의 AgentK RULE과 동일하게 **개요 → 역할(포함/제외) → 입력·출력 → 활동 절차 → 산출물 스켈레톤 → 행동 원칙 → 체크포인트** 구조를 갖춘 `rules/ooad/<주제>/RULE.md`로 정의되어 있다.

## 하네스 엔지니어링 자료와의 정렬

Cho, «하네스 엔지니어링»·*AI Agent Harness*에 나오는 **맥락 모델**, **c/i/o/f**, **SPEC→ARCH→CODE**, **테스트 하네스**, **단계별 체크포인트**는 `rules/ooad/harness-engineering/RULE.md`에 정리되어 있고, `vision`·`usecase`·`domain`·`requirements`·`unit-test` 등 RULE에 교재식 체크리스트가 보강되어 있다. 단계 산출 후 **`/ooad/review-phase-checkpoints`** 로 점검한다. **전체 명령 순서**는 **`/ooad/workflow-pipeline`** 한 번에 볼 수 있다.

**LLM 교체 시**: `rules/ooad/reproducibility/RULE.md` + 사용자가 **`/ooad/bootstrap-context`** 블록을 메시지에 포함 — 컨텍스트 드리프트 완화.

## 1. 비전·요구

1. `arch/system.md` — 범위·맥락
2. `arch/requirements/fr-nfr.md` — **FR / NFR** 목록, 측정 기준, 유스케이스 추적
3. 품질(NFR)이 테스트·CI·GUI 요구와 맞는지 점검 (`specify-fr-nfr`)

## 2. OOA

4. `arch/usecases.md`, `arch/usecase/UC-nnn.md`
5. `arch/domain/model.md`
6. `arch/ssd/` — SSD (Mermaid `sequenceDiagram` 권장)

## 3. OOD

7. `arch/design/interaction/` — 시퀀스
8. `arch/design/class-diagram.md` — DCD (**SOLID** 반영)
9. (선택) `arch/design/packages.md`

## 4. 구현 (C++ · OOI · SOLID)

10. `src/` 등에 C++ 구현 — **SOLID** 준수(전역 RULE 참고)
11. **`arch/design/implementation-mapping.md`**, **`arch/vnv/traceability-matrix.md` §2·§7**, 소스 `// Trace:` 주석 정합

## 5. 자동 테스트 (GTest)

12. **단위 테스트** — focal·stub/driver·커버리지 (`develop-unit-tests-gtest`)
13. **통합 테스트** — 모듈 결합(GTest, 단위 다음 단계), **`traceability-matrix.md` §4** 및 테스트 파일 `// Covers:` 갱신

## 6. 시스템 테스트 (비-GTest, GUI·시뮬레이터)

14. `arch/vnv/system-tests.md` — **≥30** 케이스, pos/neg (`plan-system-tests-gui`). CI에서는 **통합 테스트 통과 후** 실행.
15. **`arch/vnv/traceability-matrix.md`** — FR/UC/NFR ↔ 구현 ↔ GTest ↔ `system_tests/maps/*.json` **`trace`** 필드 유지.

## 7. CI/CD

16. `.github/workflows/` — **빌드 → 단위 → 통합 → 시스템** 순 (`configure-cicd-github-actions`)

## 8. 정적 분석 (시스템 테스트 통과 후)

17. 전체 코드 정적 분석, **`reports/static-analysis/`** 에 결과 저장 (`archive-static-analysis-report`)

## UML · Mermaid

- 전용 UML 도구를 쓰지 않을 때는 문서에 **Mermaid** 블록으로 동일한 뷰를 남긴다. 문법 예시는 `rules/ooad/*/RULE.md` 참고.
