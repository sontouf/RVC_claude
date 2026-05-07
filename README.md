# RVC Cleaning Controller

자동 청소 RVC(Robot Vacuum Cleaner)의 **소프트웨어 컨트롤러**. 본 저장소는 [`/ooad/workflow-pipeline`](.cursor/commands/ooad/workflow-pipeline.md) 순서를 따라 **Vision → FR/NFR → OOA → OOD → C++ 구현 → GTest 단위·통합 → 시뮬레이터 시스템 테스트(≥30) → GitHub Actions CI** 순으로 구성되어 있다. 모델 교체에도 동일 산출이 나오도록 [`reproducibility` RULE](.cursor/rules/ooad/reproducibility/RULE.md)을 단일 근거로 둔다.

## 결정적 정책 요약

- 회피·탈출이 아니면 **전진하면서 청소** (FR-001 / UC-002).
- 전방 장애물 + 측면 한 쪽 열림 → 그쪽으로 회전.
  **양쪽 모두 열려 있으면 좌측 우선** (FR-003 / UC-003 deterministic).
- 전·좌·우 동시 막힘 → `CleaningCoordinator`가 후진 → 측면 열리면 회전 → 전진 청소 재개를 단일 오케스트레이션으로 수행 (FR-004 / UC-004).
- 먼지 감지 시 청소 파워를 한 단계 부스트 후 `dustBoostTicks` 만료 시 자동 복귀, 부스트 중 재감지 시 타이머 리셋(debounce) (FR-005 / UC-005).

자세한 사양은 [`arch/system.md`](arch/system.md), 추적은 [`arch/vnv/traceability-matrix.md`](arch/vnv/traceability-matrix.md).

## 디렉터리 구조

```
arch/                 # 비전·요구·OOA·OOD·V&V 산출물 (단일 근거)
  system.md
  requirements/fr-nfr.md
  usecases.md, usecase/UC-00*.md
  domain/model.md
  ssd/UC-*.md
  design/{class-diagram, packages, implementation-mapping}.md
  design/interaction/UC-*.md
  vnv/{traceability-matrix, system-tests}.md
include/rvc/...       # 헤더 (ports/domain/app/tech 레이어)
src/...               # 구현 (.cpp)
tools/rvc_sim.cpp     # 헤드리스 시뮬레이터 진입점
tests/                # GTest unit + integration
system_tests/
  maps/ST-*.json      # ≥30 JSON 시나리오, `trace` 배열 포함
  run_all.py          # 비-GTest 시스템 테스트 러너
.github/workflows/ci.yml
```

레이어 의존: `tools → app → domain → ports`, `tools → tech → ports`. `domain`/`app`은 `tech`(Grid 어댑터)에 의존하지 않는다 (DIP, [`packages.md`](arch/design/packages.md)).

## 빌드 (로컬)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Unit + integration (GTest)
./build/tests/rvc_unit_tests
./build/tests/rvc_integration_tests

# System tests (비-GTest, Python + 시뮬 바이너리)
python3 system_tests/run_all.py --sim ./build/rvc_sim --check-determinism
```

`cmake --build` 는 `FetchContent`로 GoogleTest v1.14.0을 가져온다. 인터넷이 없으면 `-DRVC_BUILD_TESTS=OFF`로 코어/시뮬만 빌드 가능.

## GUI 시뮬레이터 (저작·디버깅용)

직접 맵을 그려 가며 실행해 보고 싶으면 [`tools/rvc_gui.py`](tools/rvc_gui.py)를 사용한다. Python 표준 라이브러리(Tkinter)만 쓰므로 추가 설치가 필요 없다.

```powershell
python tools\rvc_gui.py                              # 빈 7x7 맵으로 시작
python tools\rvc_gui.py system_tests\maps\ST-014.json  # 기존 시나리오 열기
```

좌클릭=칠하기, 우클릭=지우기, 도구는 우측 패널에서 `Wall / Floor / Dust / Robot` 중 선택. `Start / Pause / Step / Reset` 버튼으로 틱 단위 시뮬레이션, `Save JSON ...`으로 `system_tests/maps/`에 그대로 저장 가능. 컨트롤러 동작은 `system_tests/_py_emulator.py`(C++ `rvc_sim`을 1:1 미러링)를 그대로 사용하므로, 여기서 PASS한 맵은 그대로 CI의 C++ 시스템 테스트로 추가할 수 있다.

> 주의: GUI는 **저작·디버깅 보조 도구**다. CI/RTM의 권위 있는 시스템 테스트는 항상 컴파일된 C++ `rvc_sim` 바이너리로 실행된다 (NFR-CI-001).

## CI/CD (GitHub Actions)

[.github/workflows/ci.yml](.github/workflows/ci.yml)는 `build → unit_tests → integration_tests → system_tests` 순으로 `needs:`가 연결되어 있어 앞 단계 실패 시 이후 단계가 실행되지 않는다 (NFR-CI-001).

## OOAD 슬래시 명령

본 프로젝트는 Cursor의 `/ooad/*` 명령 모음을 동봉한다(`.cursor/commands/ooad/`). 진입점은 [`/ooad/workflow-pipeline`](.cursor/commands/ooad/workflow-pipeline.md), 점검은 [`/ooad/review-phase-checkpoints`](.cursor/commands/ooad/review-phase-checkpoints.md). 모델·세션 교체 시 [`/ooad/bootstrap-context`](.cursor/commands/ooad/bootstrap-context.md) 블록을 메시지에 함께 붙인다.

## 라이선스 / 기여

학습/연구 목적의 하네스 엔지니어링 예제 저장소. 외부 의존: GoogleTest (FetchContent), CMake, Python ≥3.10, C++17 컴파일러.
