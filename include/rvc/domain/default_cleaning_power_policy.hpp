// Trace: arch/vnv/traceability-matrix.md — FR-005, UC-005
#pragma once

#include "rvc/domain/i_cleaning_power_policy.hpp"

namespace rvc::domain {

class DefaultCleaningPowerPolicy final : public ICleaningPowerPolicy {
public:
    explicit DefaultCleaningPowerPolicy(int dustBoostTicks) noexcept;

    rvc::ports::CleaningPowerLevel nextLevel(
        const rvc::ports::SensorReading& reading) override;

    void reset() override;

    int remainingBoostTicks() const noexcept { return timerRemaining_; }

private:
    int dustBoostTicks_{0};
    int timerRemaining_{0};
};

}  // namespace rvc::domain
