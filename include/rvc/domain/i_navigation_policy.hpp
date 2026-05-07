// Trace: arch/vnv/traceability-matrix.md — FR-001, FR-003, FR-004, UC-002..004
#pragma once

#include "rvc/domain/decisions.hpp"
#include "rvc/ports/enums.hpp"
#include "rvc/ports/sensor_reading.hpp"

namespace rvc::domain {

class INavigationPolicy {
public:
    virtual ~INavigationPolicy() = default;

    // Decides next drive/turn command for a non-escape phase.
    // Caller (Coordinator) owns escape-phase orchestration; this method
    // is consulted only during Driving/Avoiding phases.
    virtual DriveDecision nextDriveCommand(
        const rvc::ports::SensorReading& reading,
        rvc::ports::Phase currentPhase) = 0;

    // Auxiliary helper: given a post-backoff sensor reading, suggest a
    // single-turn command toward an opening. Does NOT replace the
    // Coordinator's escape sequence (reproducibility RULE §3).
    virtual EscapeAssist plan_escape_enclosure(
        const rvc::ports::SensorReading& reading) = 0;
};

}  // namespace rvc::domain
