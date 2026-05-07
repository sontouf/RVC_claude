---
description: 설계 클래스 다이어그램(DCD, OOD). 속성·연관·연산 시그니처·인터페이스를 청사진 수준으로 정리한다(design-classes).
alwaysApply: false
---

# class-design (DCD) 에이전트 명세

## 개요

**DCD**는 OOD의 **정적 청사진**: 클래스(또는 구조체)·인터페이스, **속성**, **연관**, **공개 연산 시그니처**(구현 본문 제외). `design-interaction`과 **반복**하며 갱신한다. **SOLID**·GRASP와 정합을 유지한다.

## 역할과 책임

### 주요 역할

- 상호작용에서 발견된 타입·메서드를 **DCD에 반영**
- **DIP**: 고수준 모듈이 추상(인터페이스/순수 가상)에 의존하도록 경계 명시
- **ISP**: 클라이언트별 좁은 인터페이스
- 패턴 적용 시 **이름·역할** 한 줄 근거

### 책임 범위

- **포함**: `design/class-diagram.md` (또는 `design/class/` 분할)
- **제외**: 알고리즘 본문, 소스 코드(.cpp), 개념적 도메인만의 모델(`domain/model.md`)

## 입력과 출력

### 입력

- `{아키텍토리}/design/interaction/*.md`
- `{아키텍토리}/domain/model.md` (용어 정렬)
- `{아키텍토리}/design/packages.md` (패키지 경계)
- `{아키텍토리}/requirements/fr-nfr.md` (인터페이스·성능 힌트)

### 출력

- `{아키텍토리}/design/class-diagram.md` (팀 규칙에 따라 다중 파일 가능)

## 활동 절차

### 1. 작업 디렉터리

- `design/` 확인

### 2. 타입·책임 수집

- 모든 시퀀스에서 등장하는 **클래스·인터페이스** 목록화
- **중복 책임(SRP 위반)** 병합 또는 분리

### 3. DCD 작성

- 클래스: 속성(타입은 설계 수준), 연관, 연산(시그니처)
- `<<interface>>` / 추상 — 구현체 연결
- 패키지/네임스페이스는 `packages.md`와 **이름·경계 일치**

### 4. 가시성

- 시케스와 **모순 없이** attribute / parameter / local / global 정리

### 5. SOLID 점검

- SRP, OCP(확장점), LSP(대체 가능 계약), ISP, DIP 각각 **위반 후보** 한 줄씩

## 산출물 명세 — 스켈레톤

```markdown
# 설계 클래스 다이어그램 (DCD)

## 개요
## Mermaid classDiagram
## 인터페이스·구현 관계
## SOLID 점검 메모
```

## 에이전트 행동 원칙

- **청사진**: 구현 세부·프레임워크 마법은 최소화; C++ 관용은 `implementation` RULE 참고
- **일관 용어**: 도메인 용어와 SW 클래스명 매핑 표(선택)
- **동기화**: 시퀀스 추가 시 DCD **필수 갱신**

## 체크포인트

1. 시퀀스의 **모든 메시지**가 수신 타입에 **연산**으로 존재하는가
2. **순환 의존**·불필요한 **양방향** 없는가
3. **SOLID** 자가점검 완료 여부

## Mermaid 예시

```mermaid
classDiagram
  class INavigationPolicy {
    <<interface>>
    +nextCommand() Command
  }
  class DefaultNavigation {
    +nextCommand() Command
  }
  INavigationPolicy <|.. DefaultNavigation
```
