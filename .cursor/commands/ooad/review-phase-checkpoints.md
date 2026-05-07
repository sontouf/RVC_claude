---
description: 하네스 엔지니어(PDF)식 단계 점검—현재 단계 산출물이 체크포인트(완전성·명확성·일관성·측정 가능성)를 통과하는지 검토한다. (규칙 ooad/vision, usecase, domain, requirements, harness-engineering, reproducibility)
---

# review-phase-checkpoints — 재현 가능 점검

에이전트는 **현재 단계**를 식별한 뒤, 아래 표에서 해당 행만 **Yes/No**로 스스로 채점한다. **No**가 있으면 그 항목을 수정한 다음 다음 단계로 간다.

## 공통 (모든 단계)

- [ ] 근거는 **저장소 파일**에 있는가? (빈 일반 지식으로 요구를 대체하지 않았는가)
- [ ] `arch/requirements/fr-nfr.md` 의 FR/NFR ID가 **임의 변경**되지 않았는가?
- [ ] `arch/vnv/traceability-matrix.md` 가 있으면 **같은 PR/작업**에서 함께 갱신되었는가?
- [ ] RULE **`reproducibility`** 의 제품 불변조건·기술 스택과 **모순**되지 않았는가?

## OOA 끝 (`usecases`, `UC-nnn`, `domain`, `ssd`)

- [ ] UC **Name** 이 `usecases.md` 표와 **영문 전체 문장**으로 일치하는가?
- [ ] SSD에 **Mermaid** `sequenceDiagram`(또는 동등 서술)이 있는가?
- [ ] 삼면 막힘 vs 부분 막힘이 **UC-004 vs UC-003** 으로 분리되어 있는가?

## OOD 끝 (`interaction`, `class-diagram`, `packages`)

- [ ] DCD가 **Coordinator vs Policy vs Port vs Grid 기술** 책임을 **SOLID**에 맞게 나누는가?
- [ ] UC-004 탈출 **연속 후진** 등 구현 디테일이 문서·RTM과 **동기**인가?

## 구현·테스트 직후

- [ ] 시스템 테스트가 **GTest가 아닌** 시뮬 경로인가?
- [ ] GTest 파일 상단에 `// Trace:` / `// Covers:` 가 있는가? (신규/변경 파일)
- [ ] 시나리오 JSON에 `trace` 배열이 RTM §5와 **일치**하는가?

## CI

- [ ] Job 순서가 **빌드 → 단위 → 통합 → 시스템** 인가?

---

**지시:** 사용자에게 **현재 단계**와 **No인 항목 목록**을 짧게 보고한 뒤, 수정 액션을 제안한다.
