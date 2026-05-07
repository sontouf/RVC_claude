---
description: C++ 구현 규약—표준·디렉터리·CMake·헤더/소스·SOLID·폴더와 설계 정합(implementation).
alwaysApply: false
---

# implementation (C++ 구현 규약) 에이전트 명세

## 개요

프로젝트 **C++ 구현 표준**을 고정해 에이전트·개발자·CI가 동일한 전제를 쓰게 한다. **OOI** 단계의 세부 관용(표준 버전, 스마트 포인터, 예외 정책 등)을 문서화한다.

## 역할과 책임

### 주요 역할

- C++ 표준·컴파일러·플랫폼 명시
- 디렉터리·네임스페이스·CMake 타겟 규칙
- 헤더/소스 분리, public API 노출 원칙
- **SOLID**를 코드 레벨에서 해석(SRP=파일/타입, DIP=DI·인터페이스)
- **테스트 코드** 위치·타겟(`tests/` 등)과 프로덕션 분리

### 책임 범위

- **포함**: `README` 절 또는 `arch/implementation/cpp-conventions.md` 작성·유지
- **제외**: FR/NFR 전체, 유스케이스 본문

## 입력과 출력

### 입력

- `{아키텍토리}/design/packages.md`
- `{아키텍토리}/requirements/fr-nfr.md`
- 팀 합의·기존 저장소 관례

### 출력

- `arch/implementation/cpp-conventions.md` (권장) 또는 루트 `CONTRIBUTING`/README 섹션

## 활동 절차

### 1. 최소 명세 목록

- C++ 표준 (예: **C++17/20**)
- 컴파일러·OS (MSVC, Clang, GCC)
- 빌드: **CMake** 최소 버전, `target` 구조
- 의존성: 허용·금지 서드파티
- **예외** 사용 여부, `noexcept` 정책
- **스마트 포인터**·소유권
- 로깅·assert

### 2. 디렉터리와 DCD·패키지 정합

- 폴더 = 패키지/레이어의 **1:1 또는 명시적 매핑 표**

### 3. 테스트 타겟

- `unit` / `integration` CMake 타겟 이름·접두사 규칙

## 산출물 명세 — `cpp-conventions.md` 스켈레톤

```markdown
# C++ 구현 규약

## 도구 체인
## 디렉터리·네임스페이스
## 헤더/소스·API
## 메모리·예외
## SOLID·리뷰 체크리스트
## 테스트 타겟 명명
```

## 에이전트 행동 원칙

- **짧은 규약**: 과도한 스타일 논쟁 대신 **검증 가능한 규칙**
- **CI와 연동**: 포맷터·경고 레벨은 파이프라인과 일치

## 체크포인트 (assign-worker·module 아이디어 정렬)

1. 폴더 구조가 `packages.md`·**DCD**와 대응하는가  
2. **역할 분리**가 디렉터리/타겟 수준에서 드러나는가  
3. 신규 모듈 추가 시 **규약 문서** 갱신 여부

## 테스트 코드 배치

- GTest 바이너리는 `tests/` 또는 `test/` 등 **한 곳**에 모으고 CMake/GoogleTest 타겟으로 빌드한다.
