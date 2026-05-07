// Trace: arch/vnv/traceability-matrix.md — FR-001, FR-003, FR-004, UC-002..004
#pragma once

#include "rvc/domain/i_navigation_policy.hpp"

namespace rvc::domain {

class DefaultNavigationPolicy final : public INavigationPolicy {
public:
    DriveDecision nextDriveCommand(
        const rvc::ports::SensorReading& reading,
        rvc::ports::Phase currentPhase) override;

    EscapeAssist plan_escape_enclosure(
        const rvc::ports::SensorReading& reading) override;
};

}  // namespace rvc::domain
