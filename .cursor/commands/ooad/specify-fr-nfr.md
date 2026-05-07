---
description: arch/requirements/fr-nfr.md에 FR·NFR(품질)을 정리하고 UC·테스트·CI 정책과 추적·모순 검토를 수행한다. (규칙 ooad/requirements)
---

# specify-fr-nfr

## 산출물

- `arch/requirements/fr-nfr.md` 갱신 (FR/NFR 표, UC 추적 표)

## 추적성 (필수)

- **`rules/ooad/reproducibility/RULE.md`** 의 제품 불변조건·미래 요구(비구현) 분리와 모순 없게 작성한다.
- FR·NFR ID를 **추가·변경·폐기**할 때 **`arch/vnv/traceability-matrix.md`** 의 §2·§3·§5·§6 을 **동일 작업**에서 맞춘다.
- UC 번호·이름이 바뀌면 `arch/usecases.md` 및 각 `arch/usecase/UC-nnn.md` 와 RTM을 함께 갱신한다.

## 절차 요약

- `arch/system.md` 와 모순 없는지 검토.
- NFR은 측정·검증 수단(`system-tests.md`, CI job, GTest)과 짝을 이루게 서술.
