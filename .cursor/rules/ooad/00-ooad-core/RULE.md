---
description: OOAD 흐름 + SOLID + FR/NFR, C++ GTest(단위→통합) 및 GUI 시스템 테스트, GitHub Actions 순차 CI/CD, 정적 분석 보관.
alwaysApply: true
---

# OOAD · 요구사항 · 구현·검증 (프로젝트 전역)

본 RULE은 **arch-with-ai** 스타일의 **세부 에이전트 명세**(`rules/ooad/*/RULE.md`) 위에서 동작하는 **최상위 정책**이다. 모든 슬래시 명령은 해당 주제 RULE의 **입력/출력·절차·체크포인트**를 따른다.

## LLM 교체·재현성

- **`reproducibility` RULE** (`rules/ooad/reproducibility/RULE.md`, `alwaysApply: true`) — 제품 불변조건·결정적 정책·기술 스택·DoD·금지 사항. **모델을 바꿔도** 이 RULE은 동일하게 로드된다.
- **부트스트랩**: 슬래시 명령 **`/ooad/bootstrap-context`** — 영문 요구·비기능·검증을 한 블록으로 복사해 사용자 메시지에 붙이면, 세션 간 **컨텍스트 드리프트**를 줄인다.
- 전체 파이프라인: **`/ooad/workflow-pipeline`** (문서 상단에 재현성 지시 포함).

## 분석/설계 (교재 OOAD)

- **OOA**: 유스케이스, **도메인 모델(개념적)**, **SSD** (`:System` 경계).
- **OOD**: **시퀀스(상호작용)**, **DCD**, (선택) 패키지/레이어.
- **OOI**: GRASP·가시성과 함께 **SOLID**를 준수해 **C++** 로 반영한다(아래 SOLID 절 참고).
- UML 스케치는 문서에 **Mermaid 블록**으로 둘 수 있다(도구 없이 재현 가능한 예시는 주제별 RULE 참고).

## SOLID (객체지향 설계 필수)

설계·코드 리뷰에서 다음을 **위반하지 않도록** 책임·추상화·의존 방향을 맞춘다.

- **S**ingle Responsibility: 클래스/모듈은 **변경 이유가 하나**가 되도록 책임을 제한한다.
- **O**pen/Closed: 확장에 열리고 **기존 코드 수정은 최소**가 되도록 추상화·다형성을 쓴다.
- **L**iskov Substitution: **서브타입**은 기대 계약을 깨지 않고 **대체 가능**해야 한다.
- **I**nterface Segregation: 클라이언트가 **쓰지 않는 연산**에 의존하지 않게 인터페이스를 쪼갠다.
- **D**ependency Inversion: 고수준 정책이 저수준 세부에 직접 의존하지 않게 **추상(인터페이스)** 에 의존한다.

## 추적성 (Traceability) — 규범 단일 표

- **RTM**: `arch/vnv/traceability-matrix.md` — FR/NFR/UC ↔ SSD ↔ 구현 식별자 ↔ GTest ↔ 시스템 JSON `trace` 필드 ↔ 슬래시 명령·RULE 단계.
- FR·UC·NFR·코드·테스트·시나리오를 바꿀 때 **동일 PR/작업 안에서** RTM(및 `implementation-mapping.md` 필요 시)을 갱신한다.
- 재구현 시 **`/ooad/workflow-pipeline`** 순서와 RTM·`fr-nfr.md`·UC 상세만으로도 **동일 행동에 수렴**하도록 유지한다.

## 소프트웨어 공학 요구사항 (FR / NFR)

- **기능 요구사항(FR)**: 유스케이스·시스템 연산·인터페이스와 **추적 가능**하게 연결한다 (`arch/requirements/fr-nfr.md`, **`arch/vnv/traceability-matrix.md` 필수**).
- **비기능 요구사항(NFR) / 품질**: 성능, 보안, 사용성, 신뢰성, 유지보수성, 테스트 용이성 등 **측정 가능한 기준**이 있으면 명시한다.
- OOA 산출물이 FR의 근거가 되고, NFR은 **아키텍처·테스트·CI 정책**과 충돌 없는지 검토한다(명령 `specify-fr-nfr`).

## 구현 필수: C++

- 언어는 **C++** 고정. 빌드·의존성은 저장소에 명시(예: CMake). 세부는 `implementation` RULE.

## 단위·통합 검증: Google Test (GTest)

- **GTest**로 **단위 테스트**와 **통합 테스트**를 수행한다( **시스템 테스트는 GTest 사용 안 함** ).
- **단위**: 한 클래스·focal 연산 위주, **stub/fake**로 격리·빠른 피드백.
- **통합**: 여러 컴포넌트·모듈의 **실제 결합**을 검증(필요 시 테스트 더블 최소화). 팀은 CMake 타겟 등으로 `unit` / `integration` 실행을 **구분**한다.
- **focal method**(검증 대상 핵심 연산)마다 **충분한** 케이스(정상·경계·오류·전제 위반 등)를 설계한다.
- **Driver / Stub / Fake**: 외부·미구현·느린 의존성은 테스트 더블로 분리하고, focal이 격리되도록 한다.
- **커버리지**: 라인/브랜치(또는 팀 합의 지표)를 **측정**하고 CI에서 리포트·임계값을 정책으로 둔다.

## 시스템 테스트 (시뮬레이터 + GUI)

- 실제 **시스템 테스트**는 팀원별 **시뮬레이터**를 활용한다. 본 프로젝트에서는 **GUI**에서 동작이 **육안으로 확인** 가능해야 합니다.
- **시스템 테스트 케이스 ≥ 30건**. **긍정(positive)·부정(negative)** 시나리오를 **모두** 포함한다.
- 시스템 테스트는 **GTest가 아닌** 방식(수동 시나리오·GUI 자동화·별도 스크립트 등)으로 정의·실행한다. 근거·절차는 `arch/vnv/system-tests.md`(또는 동등 문서)에 둔다.

## CI/CD (GitHub Actions)

- **GitHub Actions**로 **빌드**, (합의 시) **배포**, **운영** 파이프라인을 정의한다.
- **검증 job 순서(필수)**: **① 빌드 → ② 단위 테스트(GTest) → ③ 통합 테스트(GTest) → ④ 시스템 테스트**(비-GTest·GUI·시뮬레이터). 앞 단계 실패 시 이후 단계는 수행하지 않는다.
- **커버리지**는 단위·통합 실행 후 수집·게이트(팀 정책)를 둔다.

## 정적 분석

- **시스템 테스트를 통과한 전체 시스템 코드**에 대해 **정적 분석**을 수행한다.
- **위반 수정은 필수 아님**이나, **수행 결과(로그·SARIF·리포트)**는 저장소 내 **`reports/static-analysis/`** (또는 팀 합의 경로)에 **버전·일자와 함께 보관**한다.

## 경로 (설정 연동)

- 설계·요구·V&V 문서: `agentk.architectureDirectory`(예: `arch`).
- 소스: `agentk.sourceDirectory`(예: `src`). 테스트 코드 경로는 팀 규칙에 맞게 `tests/`, `test/` 등으로 명시한다.

## 공통 에이전트 행동 원칙 (arch-with-ai 정렬)

- **작업 디렉토리**: 항상 `agentk.architectureDirectory` 확인; 사용자가 지정한 경로 우선.
- **선행 조건**: `system.md` 없이 유스케이스 추출 금지 등 **의존 단계** 준수.
- **문서 정합**: 기존 산출물과 충돌 시 사용자 확인.
- **단계 집중**: 각 명령은 해당 RULE의 **포함/제외** 범위를 넘지 않음.
- **하네스**: 맥락·c/i/o/f, SPEC→ARCH→CODE, 자동화 편향 경계 — `harness-engineering` RULE 참고.
- **점검**: 산출 후 `review-phase-checkpoints`로 단계 체크리스트 검토 가능.

## 명령 ↔ RULE 색인 (빠른 참조)

| 명령 | 주 RULE |
|------|---------|
| `define-vision` | `vision` |
| `extract-usecases`, `specify-usecase` | `usecase` |
| `specify-fr-nfr` | `requirements` |
| `model-domain` | `domain` |
| `model-ssd` | `ssd` |
| `design-interaction` | `interaction` |
| `design-classes` | `class-design` |
| `design-packages` | `packages` |
| `map-to-code` | `ooi` + `implementation` |
| `develop-unit-tests-gtest` | `unit-test` |
| `plan-system-tests-gui` | `system-test` |
| `configure-cicd-github-actions` | `cicd` |
| `archive-static-analysis-report` | `static-analysis` |
| `review-phase-checkpoints` | 각 RULE **체크포인트** 절 |
| `workflow-pipeline` | **전체 명령 실행 순서** 요약(이 표와 동등); **RTM 갱신 단계 포함** |
| `bootstrap-context` | **재현용** 영문 요구·비기능 블록 (사용자 메시지에 붙여넣기) |

**RTM(규범 추적 표)**: `arch/vnv/traceability-matrix.md` — 명령 순서만으로 재구현 시에도 FR/UC/NFR·코드·테스트·시나리오를 교차 확인한다.
