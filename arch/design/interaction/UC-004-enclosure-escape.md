# Interaction: UC-004 — tick() (triple-blocked enclosure escape)

## 맥락·선행 조건

- 세션 Running. `front=true && left=true && right=true`.
- 단계 전이는 Coordinator가 단일 오케스트레이션으로 소유한다(`reproducibility` RULE §3).

## 시퀀스

```mermaid
sequenceDiagram
  participant Coord as CleaningCoordinator
  participant Sense as ISensorPort
  participant Nav as NavigationPolicy
  participant Act as IActuatorPort
  Coord->>Sense: read()
  Sense-->>Coord: {front=true,left=true,right=true}
  Coord->>Coord: state := Escaping(backoffRemaining = maxBackoffTicks)
  Coord->>Act: drive(Stop)
  Coord->>Act: drive(Backward)
  Note over Coord: --- next tick ---
  Coord->>Sense: read()
  alt left OR right is open  %% FR-004: side-open is the only exit predicate
    Coord->>Nav: plan_escape_enclosure(reading)
    Nav-->>Coord: {turn=Left|Right}
    Coord->>Act: turn(Left|Right)
    Coord->>Coord: state := Driving
    Note over Coord: NO drive(Forward) this tick — next tick reads new heading first
  else both sides still blocked  %% even if front opened, do NOT exit
    alt backoffRemaining > 0
      Coord->>Act: drive(Backward)
      Coord->>Coord: backoffRemaining--
    else
      Coord->>Act: drive(Stop)  %% safety cap; retry next tick
    end
  end
```

## GRASP / 가시성 메모

- **Controller**: 후진 → 회전 → (다음 tick에서) 전진의 단계 전이는 `CleaningCoordinator` 가 소유(SSoT).
- **Exit predicate (FR-004)**: escape를 종료하는 조건은 **L 또는 R 이 열림**. 전방만 열린 상태는 종료 조건이 아니다 — 전진을 재개하면 같은 함정으로 다시 진입하기 때문.
- **No-forward-after-backoff**: 회전한 tick에는 전진 명령을 발신하지 않는다. 다음 tick에서 새 헤딩 기준으로 정상 driving 사이클이 forward를 결정한다.
- **Pure Fabrication**: `NavigationPolicy::plan_escape_enclosure`는 보조 helper(스텁/정책 보조). 코디네이터 전체 시퀀스를 대체하지 않는다.
- **Safety**: `backoffRemaining` 0 도달 시 `drive(Stop)` + 다음 tick 재시도. `maxBackoffTicks==0` 이면 후진 자체를 시도하지 않는다.
