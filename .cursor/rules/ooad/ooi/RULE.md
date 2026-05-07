---
description: OOI—DCD·시퀀스·가시성을 C++로 매핑한다. SOLID·프로젝트 규약을 준수한다(map-to-code).
alwaysApply: false
---

# OOI (설계 → C++) 에이전트 명세

## 개요

**OOI**는 **SPEC·ARCH**(OOA/OOD 산출물)를 **`agentk.sourceDirectory`** 의 C++로 옮긴다. **가시성·협력 관계**가 설계와 일치해야 하며, **SOLID** 위반 시 설계를 먼저 수정한다. **하네스** 관점에서 코드 변경은 항상 `arch/` 근거와 **추적** 가능해야 한다.

## 역할과 책임

### 주요 역할

- DCD의 클래스·인터페이스를 **헤더/소스**로 구현
- 시퀀스의 **메시지**를 **메서드 호출**로 구현; **가시성** 일치
- 네임스페이스·디렉터리는 `packages.md`와 **정합**
- (선행 또는 병행) **단위·통합 테스트** 전략에 맞게 테스트 가능한 **의존성 방향** 유지

### 책임 범위

- **포함**: `src/` (또는 팀 규칙 경로), CMakeLists 등 빌드 연동
- **제외**: GTest 본문 대량 작성은 `unit-test` RULE / `develop-unit-tests-gtest`에 위임(OOI는 프로덕션 코드 우선)

## 입력과 출력

### 입력

- `{아키텍토리}/design/class-diagram.md`
- `{아키텍토리}/design/interaction/*.md`
- `{아키텍토리}/design/packages.md`
- `{아키텍토리}/requirements/fr-nfr.md` (규약·표준)
- (선택) 기존 `src/` 트리

### 출력

- `{소스루트}/**/*.h`, `*.hpp`, `*.cpp` 등 팀 규약에 따른 파일

> `{소스루트}` = `agentk.sourceDirectory` (예: `src`)

## 활동 절차

### 1. 설정 확인

- `agentk.architectureDirectory`, `agentk.sourceDirectory` 확인

### 2. 매핑 계획

- DCD 타입 → 파일·네임스페이스 매핑 표(머릿속·주석·짧은 문서)
- 인터페이스 → **순수 가상** 또는 소규모 추상 베이스

### 3. 구현

- **SOLID**: 특히 SRP(파일 분리), DIP(구체 타입 직접 참조 최소화)
- RAII, Rule of 0/3/5 등 **팀 C++ 규약** (`implementation` RULE)

### 4. 설계 역류 처리

- 코드를 쓰다 **새 책임**이 생기면 DCD/시퀀스 **먼저** 갱신 후 코드 반영

### 5. 추적

- 커밋 메시지 또는 PR 설명에 **근거 UC/설계** 참조

## 산출물 명세

- 코드 베이스 자체가 산출물; 필요 시 `arch/implementation/notes.md`에 매핑 메모

## 에이전트 행동 원칙

- **SPEC → ARCH → CODE** 순서 존중 (`harness-engineering` RULE)
- **테스트 용이성**: 생성자·의존성이 GTest에서 **대체 가능**한지 점검(DIP)
- 과도한 **친구 클래스**·전역 상태 지양

## 체크포인트

1. DCD의 **공개 연산**이 코드에 존재하는가  
2. 시퀀스 **필수 메시지**가 호출 경로로 구현되었는가  
3. `packages.md` **금지 의존**을 코드가 위반하지 않는가

## 참고

- `rules/ooad/implementation/RULE.md`  
- `rules/ooad/unit-test/RULE.md`
