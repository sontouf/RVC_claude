// Trace: arch/vnv/traceability-matrix.md — FR-001, FR-003, FR-004, UC-002..004
#pragma once

#include "rvc/ports/enums.hpp"

namespace rvc::domain {

struct DriveDecision {
    rvc::ports::DriveCommand drive{rvc::ports::DriveCommand::Forward};
    rvc::ports::TurnCommand turn{rvc::ports::TurnCommand::None};
};

struct EscapeAssist {
    rvc::ports::TurnCommand turn{rvc::ports::TurnCommand::None};
};

}  // namespace rvc::domain
