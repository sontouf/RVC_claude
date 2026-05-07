// Trace: arch/vnv/traceability-matrix.md — UC-002..005
#pragma once

#include "rvc/ports/sensor_reading.hpp"

namespace rvc::ports {

class ISensorPort {
public:
    virtual ~ISensorPort() = default;
    virtual SensorReading read() = 0;
};

}  // namespace rvc::ports
