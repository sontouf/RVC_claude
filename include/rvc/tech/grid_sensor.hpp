// Trace: arch/vnv/traceability-matrix.md — technical layer
#pragma once

#include "rvc/ports/i_sensor_port.hpp"
#include "rvc/tech/grid_world.hpp"

namespace rvc::tech {

class GridSensor final : public rvc::ports::ISensorPort {
public:
    explicit GridSensor(GridWorld& world) noexcept : world_(world) {}
    rvc::ports::SensorReading read() override { return world_.sensorReading(); }

private:
    GridWorld& world_;
};

}  // namespace rvc::tech
