// Trace: arch/vnv/traceability-matrix.md — UC-001..005, FR-001..005
#pragma once

#include <cstdint>

#include "rvc/app/controller_config.hpp"
#include "rvc/domain/i_cleaning_power_policy.hpp"
#include "rvc/domain/i_navigation_policy.hpp"
#include "rvc/ports/enums.hpp"
#include "rvc/ports/i_actuator_port.hpp"
#include "rvc/ports/i_sensor_port.hpp"

namespace rvc::app {

class CleaningCoordinator {
public:
    CleaningCoordinator(
        rvc::ports::ISensorPort& sensor,
        rvc::ports::IActuatorPort& actuator,
        rvc::domain::INavigationPolicy& nav,
        rvc::domain::ICleaningPowerPolicy& power,
        ControllerConfig config) noexcept;

    void startSession();
    void stopSession();
    void tick();

    rvc::ports::SessionState sessionState() const noexcept { return sessionState_; }
    rvc::ports::Phase phase() const noexcept { return phase_; }
    int backoffRemaining() const noexcept { return backoffRemaining_; }
    std::uint64_t tickCount() const noexcept { return tickCount_; }

private:
    rvc::ports::ISensorPort& sensor_;
    rvc::ports::IActuatorPort& actuator_;
    rvc::domain::INavigationPolicy& nav_;
    rvc::domain::ICleaningPowerPolicy& power_;
    ControllerConfig config_;

    rvc::ports::SessionState sessionState_{rvc::ports::SessionState::Stopped};
    rvc::ports::Phase phase_{rvc::ports::Phase::Driving};
    int backoffRemaining_{0};
    std::uint64_t tickCount_{0};
};

}  // namespace rvc::app
