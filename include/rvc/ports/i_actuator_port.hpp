// Trace: arch/vnv/traceability-matrix.md — UC-001..005
#pragma once

#include "rvc/ports/enums.hpp"

namespace rvc::ports {

class IActuatorPort {
public:
    virtual ~IActuatorPort() = default;
    virtual void drive(DriveCommand cmd) = 0;
    virtual void turn(Direction dir) = 0;
    virtual void setPower(CleaningPowerLevel level) = 0;
};

}  // namespace rvc::ports
