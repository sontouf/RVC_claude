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
  alt at least one side open
    Coord->>Nav: plan_escape_enclosure(reading)
    Nav-->>Coord: {turn=Left|Right}
    Coord->>Act: turn(Left|Right)
    Coord->>Coord: state := Driving (next tick → Forward)
  else still triple blocked
    Coord->>Act: drive(Backward)
    Coord->>Coord: backoffRemaining--
  end
```

## GRASP / 가시성 메모

- **Controller**: 후진 → 회전 → 전진의 단계 전이는 `CleaningCoordinator` 가 소유(SSoT).
- **Pure Fabrication**: `NavigationPolicy::plan_escape_enclosure`는 보조 helper(스텁/정책 보조). 코디네이터 전체 시퀀스를 대체하지 않는다.
- **Safety**: `backoffRemaining` 0 도달 시 `drive(Stop)` + 다음 tick 재시도(또는 향후 owner notify).
