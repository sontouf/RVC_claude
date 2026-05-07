# SSD: UC-003 — Avoid Partial Obstacle

## 전제

- 세션 Running. `front=true`, 좌·우 중 ≥1쪽 열려 있음.

## 시퀀스

```mermaid
sequenceDiagram
  participant Sensors
  participant Sys as :System
  participant Acts as Actuator Subsystem
  Sys->>Sensors: read()
  Sensors-->>Sys: {front=true, left=?, right=?, dust=?}
  Sys->>Acts: drive(Stop)
  alt left=false (or both open)
    Sys->>Acts: turn(Left)
  else right=false
    Sys->>Acts: turn(Right)
  end
  Note over Sys,Acts: next tick → drive(Forward)
```

## 시스템 연산 요약

| 연산 | 의미 |
|------|------|
| `tick()` | 부분 장애물 시 stop → turn(Left|Right) 한 번 → 다음 tick에서 forward. |
| `turn(direction)` | 양쪽 열림 시 deterministic Left 우선. |
