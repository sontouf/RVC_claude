// Trace: arch/vnv/traceability-matrix.md — technical layer
#pragma once

#include "rvc/ports/i_actuator_port.hpp"
#include "rvc/tech/grid_world.hpp"

namespace rvc::tech {

class GridActuator final : public rvc::ports::IActuatorPort {
public:
    explicit GridActuator(GridWorld& world) noexcept : world_(world) {}

    void drive(rvc::ports::DriveCommand cmd) override {
        world_.applyDrive(cmd);
        world_.markCurrentCellCleanedIfPowered();
    }
    void turn(rvc::ports::Direction dir) override { world_.applyTurn(dir); }
    void setPower(rvc::ports::CleaningPowerLevel level) override {
        world_.applyPower(level);
    }

private:
    GridWorld& world_;
};

}  // namespace rvc::tech
