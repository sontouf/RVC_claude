// Trace: arch/vnv/traceability-matrix.md — FR-005, UC-005
#include "rvc/domain/default_cleaning_power_policy.hpp"

namespace rvc::domain {

using rvc::ports::CleaningPowerLevel;
using rvc::ports::SensorReading;

DefaultCleaningPowerPolicy::DefaultCleaningPowerPolicy(int dustBoostTicks) noexcept
    : dustBoostTicks_(dustBoostTicks > 0 ? dustBoostTicks : 0),
      timerRemaining_(0) {}

CleaningPowerLevel DefaultCleaningPowerPolicy::nextLevel(
    const SensorReading& reading) {
    if (dustBoostTicks_ <= 0) {
        return CleaningPowerLevel::Nominal;
    }

    if (reading.dust) {
        timerRemaining_ = dustBoostTicks_;  // start or extend (debounce)
        return CleaningPowerLevel::Boosted;
    }

    if (timerRemaining_ > 0) {
        --timerRemaining_;
        return timerRemaining_ > 0 ? CleaningPowerLevel::Boosted
                                   : CleaningPowerLevel::Nominal;
    }

    return CleaningPowerLevel::Nominal;
}

void DefaultCleaningPowerPolicy::reset() {
    timerRemaining_ = 0;
}

}  // namespace rvc::domain
