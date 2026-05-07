---
description: Cho Yong Jin «하네스 엔지니어링» 자료와 현재 OOAD·V&V 명령을 맞춘 메타 규칙—맥락 모델, c/i/o/f, SPEC→ARCH→CODE, 테스트 하네스, 단계별 체크포인트.
alwaysApply: true
---

# 설계·구현 하네스 (PDF «AI Agent Harness / 구조 설계» 정렬)

교재의 **에이전트 하네스**는 모델만이 아니라 **맥락·도구·복구·상태·메모리**를 감싼 **아키텍처**다. 우리 프로젝트에서는 그에 대응하는 **산출물·검증 층**을 `arch/`, 테스트, CI, `reports/`에 둔다.

## 맥락 모델 (Context Model)

PDF는 LLM·에이전트가 **전체 시스템 맥락** 없이 국소적으로 코드를 내면 비용·불일치·기술 부채가 난다고 본다. 이 저장소에서는 **`arch/**가 그 맥락 모델**이다: 비전·FR/NFR·유스케이스·도메인·SSD·DCD·패키지·V&V 계획이 한 데 모여야 “다음에 무엇을 바꿀지”가 일관된다.

## 활동 단위: c / i / o / f

각 명령(슬래시 커맨드)을 실행할 때 문서·채팅에 아래를 **명시적으로** 적어 재현성을 높인다.

| 기호 | 의미 | 우리 프로젝트에서의 예 |
|------|------|------------------------|
| **c** (context) | 참조할 맥락 | `arch/system.md`, `arch/requirements/fr-nfr.md`, 관련 UC·SSD |
| **i** (input) | 이번 작업 입력 | 사용자 요청, 수정할 파일, 이전 산출물 ID |
| **o** (output) | 기대 산출물 | 예: `arch/usecase/UC-003.md`, `src/…`, CI YAML |
| **f** (feedback) | 검증·승인 | 체크포인트(아래·각 주제 RULE), 리뷰어, CI 결과 |

## SPEC → ARCH → CODE

PDF의 **설계(ARCH)를 먼저 두고 CODE를 생성·변경**하는 흐름을 따른다.

- **SPEC**: 요구·유스케이스·NFR·도메인·SSD·상호작용·DCD 등 `arch/` 문서.
- **ARCH**: 패키지/레이어·모듈 경계(`design-packages`, 클래스 다이어그램).
- **CODE**: `agentk.sourceDirectory`의 C++ 및 테스트.

코드만 바꿀 때도 **어느 SPEC/ARCH 근거로 바꿨는지** 한 줄이라도 추적 가능하게 남긴다(커밋 메시지·주석·`arch/` 갱신).

## 소프트웨어 테스트 하네스 (Test Harness)

PDF 정의: *구성 요소를 독립적으로 테스트하기 위한 **도구·소프트웨어·데이터·환경**의 집합*. 단위/통합 RULE의 **GTest + stub/driver + 픽스처 + CI 환경**이 여기에 해당한다.

## 품질·NFR (교재 활동3과의 연결)

교재의 **Utility Tree·품질 시나리오·측정 가능성**은 본 프로젝트에서는 **`specify-fr-nfr`** 과 NFR 절(요구 RULE)로 **경량 반영**한다. 대규모 후보 구조·ATAM 전 과정은 필수 아니어도, **NFR은 측정·검증 방법과 연결**되어야 한다.

## 인지·신뢰 (비판적 시각)

하네스가 정교해질수록 **자동화 편향**(출력을 무비판 수용) 위험이 있다고 PDF는 경고한다. **체크포인트·인간 리뷰**를 건너뛰지 않는다.

## PDF 활동 ↔ 현재 명령 (참조 표)

| 교재 활동·에이전트 축 | 대응 명령 (ooad/) |
|----------------------|-------------------|
| 시스템 정의 (`system.md`, 경계·제약) | `define-vision`, (`specify-fr-nfr` 제약 반영) |
| 비즈니스·드라이버 (`business.md`) | `specify-fr-nfr` 내 비즈·우선순위 한 절로 흡수 가능 |
| 기능: 유스케이스 추출·명세 | `extract-usecases`, `specify-usecase` |
| 기능: 도메인 | `model-domain` (+ 선택 `arch/domain/UC-nnn.md`) |
| 품질 선정 (시나리오·NFR·QA) | `specify-fr-nfr` |
| 패키지·레이어 | `design-packages` |
| 코드·테스트·설정 | `map-to-code`, `develop-unit-tests-gtest` |
| 시스템 테스트(비 GTest) | `plan-system-tests-gui` |
| CI·관측 가능성 | `configure-cicd-github-actions` |
| 정적 분석 보관 | `archive-static-analysis-report` |
| 산출물 대비 **체크포인트 검토** | `review-phase-checkpoints` |

교재의 **후보 구조 평가·배포·모듈 통합·거대 `architecture.md`** 전용 AgentK 커맨드는 현재 OOAD·C++ 과제 범위에 **필수로 두지 않았다**. 필요 시 `arch/design/architecture-summary.md` 한 장으로 **경량 명세**를 추가할 수 있다.

## 하네스 기반 작업 절차 (권장)

슬래시 명령을 실행할 때마다 아래를 **순서대로** 적용한다.

1. **c**: 이번 작업이 참조해야 할 `arch/` 파일·UC ID를 나열한다.  
2. **i**: 사용자 요청·수정 대상·가설을 명확히 한다.  
3. **o**: 만들거나 갱신할 산출물 경로를 주제 RULE의 **출력**과 맞춘다.  
4. 수행: 해당 `rules/ooad/<주제>/RULE.md`의 **활동 절차**를 따른다.  
5. **f**: 주제 RULE **체크포인트**와 `review-phase-checkpoints`로 스스로 검토한다.

## 에이전트 행동 원칙 (하네스)

- **맥락 우선**: `arch/`를 읽기 전에 코드만 수정하지 않는다(SPEC→ARCH→CODE).  
- **복구**: 산출물이 깨졌다면 이전 단계 문서를 **소스 오브 트루스**로 삼고 맞춘다.  
- **상태**: 대화가 길어지면 “현재 UC·파일·미완료 체크포인트”를 한 블록으로 요약한다.  
- **외부 메모리**: 지속 규칙은 RULE에 두고, 일회성은 `arch/` 문서에 남긴다.

## 산출물 체인 (요약)

`system.md` → `usecases.md` → `usecase/UC-nnn.md` → `domain/model.md` → `ssd/` → `design/interaction/` → `design/class-diagram.md` → `design/packages.md` → `src/` + `tests/` → CI → `reports/static-analysis/`

필요 시 `requirements/fr-nfr.md`는 **초기에** 두고 이후 단계에서 지속 갱신한다.

## 참고

- 세부 절차·스켈레톤: 각 주제 `RULE.md` (arch-with-ai `system-definer` 등과 동일한 **명세 구조**).
- 워크플로 요약: `rules/ooad/workflow.md`.
