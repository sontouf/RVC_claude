# Interaction: UC-005 — tick() (dust boost)

## 맥락·선행 조건

- 세션 Running. `dust=true` 한 tick 이상.

## 시퀀스

```mermaid
sequenceDiagram
  participant Coord as CleaningCoordinator
  participant Sense as ISensorPort
  participant Pow as CleaningPowerPolicy
  participant Act as IActuatorPort
  Coord->>Sense: read()
  Sense-->>Coord: {dust=true, ...}
  Coord->>Pow: nextLevel(reading)
  Pow-->>Coord: Boosted
  Coord->>Act: setPower(Boosted)
  Note over Pow: dustTimer = dustBoostTicks
  loop next ticks
    Coord->>Sense: read()
    Coord->>Pow: nextLevel(reading)
    alt dust=true again
      Pow-->>Coord: Boosted (timer reset)
    else dust=false and timer>0
      Pow-->>Coord: Boosted (timer--)
    else timer == 0
      Pow-->>Coord: Nominal
    end
    Coord->>Act: setPower(<level>)
  end
```

## GRASP / 가시성 메모

- **Information Expert**: `CleaningPowerPolicy`가 dust timer/state의 단일 소유자(SRP).
- **OCP**: 부스트 정책 변경(예: 두 단계 부스트, Boost → Auto)은 `CleaningPowerPolicy` 교체로 대응.
