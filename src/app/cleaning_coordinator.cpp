// Trace: arch/vnv/traceability-matrix.md — UC-001..005, FR-001..005
//
// Coordinator owns the full Avoiding/Escaping orchestration (reproducibility
// RULE §3): triple-blocked → Stop+Backward this tick, then on the next tick
// either turn (if a side opens) or back off again until maxBackoffTicks. The
// NavigationPolicy is consulted only for a single-turn helper.
#include "rvc/app/cleaning_coordinator.hpp"

namespace rvc::app {

using rvc::domain::DriveDecision;
using rvc::domain::EscapeAssist;
using rvc::ports::CleaningPowerLevel;
using rvc::ports::Direction;
using rvc::ports::DriveCommand;
using rvc::ports::Phase;
using rvc::ports::SensorReading;
using rvc::ports::SessionState;
using rvc::ports::TurnCommand;
using rvc::ports::tripleBlocked;

namespace {

Direction toDirection(TurnCommand t) noexcept {
    return t == TurnCommand::Right ? Direction::Right : Direction::Left;
}

}  // namespace

CleaningCoordinator::CleaningCoordinator(
    rvc::ports::ISensorPort& sensor,
    rvc::ports::IActuatorPort& actuator,
    rvc::domain::INavigationPolicy& nav,
    rvc::domain::ICleaningPowerPolicy& power,
    ControllerConfig config) noexcept
    : sensor_(sensor),
      actuator_(actuator),
      nav_(nav),
      power_(power),
      config_(config) {}

void CleaningCoordinator::startSession() {
    if (sessionState_ == SessionState::Running) {
        return;  // idempotent (UC-001 A1)
    }
    power_.reset();
    sessionState_ = SessionState::Running;
    phase_ = Phase::Driving;
    backoffRemaining_ = 0;
    tickCount_ = 0;
    // Power is enabled here so the robot's starting cell counts as cleaned;
    // the first drive command is issued during the next tick(), after the
    // first sensor read (avoids "blind" forward motion before sensing).
    actuator_.setPower(CleaningPowerLevel::Nominal);
}

void CleaningCoordinator::stopSession() {
    if (sessionState_ == SessionState::Stopped) {
        return;  // idempotent (UC-001 A2)
    }
    actuator_.drive(DriveCommand::Stop);
    actuator_.setPower(CleaningPowerLevel::Off);
    power_.reset();
    sessionState_ = SessionState::Stopped;
    phase_ = Phase::Driving;
    backoffRemaining_ = 0;
}

void CleaningCoordinator::tick() {
    if (sessionState_ != SessionState::Running) {
        // NFR-SAFE-001: no actuator calls when stopped.
        return;
    }

    ++tickCount_;
    const SensorReading reading = sensor_.read();

    // Power policy is independent of navigation (UC-005).
    const CleaningPowerLevel level = power_.nextLevel(reading);
    actuator_.setPower(level);

    // Phase transition is fully owned here.
    if (phase_ == Phase::Escaping) {
        // We backed off last tick; check if a side is open now.
        if (!tripleBlocked(reading)) {
            const EscapeAssist assist = nav_.plan_escape_enclosure(reading);
            if (assist.turn != TurnCommand::None) {
                actuator_.turn(toDirection(assist.turn));
            }
            phase_ = Phase::Driving;
            backoffRemaining_ = 0;
            // Forward will resume next tick when no obstacle is reported.
            return;
        }
        // Still triple-blocked → keep backing off until budget exhausted.
        if (backoffRemaining_ > 0) {
            --backoffRemaining_;
            actuator_.drive(DriveCommand::Backward);
        } else {
            // Safety: stop and stay in Escaping; will retry next tick.
            actuator_.drive(DriveCommand::Stop);
        }
        return;
    }

    if (tripleBlocked(reading)) {
        // Enter escape: stop, and (if configured) backward this tick.
        phase_ = Phase::Escaping;
        backoffRemaining_ = config_.maxBackoffTicks > 0
                                ? config_.maxBackoffTicks - 1
                                : 0;
        actuator_.drive(DriveCommand::Stop);
        if (config_.maxBackoffTicks > 0) {
            actuator_.drive(DriveCommand::Backward);
        }
        return;
    }

    // Driving / Avoiding: delegate to navigation policy.
    const DriveDecision d = nav_.nextDriveCommand(reading, phase_);
    if (d.turn != TurnCommand::None) {
        phase_ = Phase::Avoiding;
        actuator_.drive(DriveCommand::Stop);
        actuator_.turn(toDirection(d.turn));
        // Next tick will read sensors again and resume forward (FR-001).
        return;
    }

    phase_ = Phase::Driving;
    actuator_.drive(d.drive);
}

}  // namespace rvc::app
