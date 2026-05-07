// Trace: arch/vnv/traceability-matrix.md — FR-005, UC-005
#pragma once

#include "rvc/ports/enums.hpp"
#include "rvc/ports/sensor_reading.hpp"

namespace rvc::domain {

class ICleaningPowerPolicy {
public:
    virtual ~ICleaningPowerPolicy() = default;

    virtual rvc::ports::CleaningPowerLevel nextLevel(
        const rvc::ports::SensorReading& reading) = 0;

    virtual void reset() = 0;
};

}  // namespace rvc::domain
