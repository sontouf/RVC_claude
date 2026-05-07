// Trace: arch/vnv/traceability-matrix.md — FR-001, FR-003, FR-004, UC-002..004
#include "rvc/domain/default_navigation_policy.hpp"

namespace rvc::domain {

using rvc::ports::DriveCommand;
using rvc::ports::Phase;
using rvc::ports::SensorReading;
using rvc::ports::TurnCommand;

namespace {

// Deterministic side selection per FR-003:
//   - if exactly one side is open, choose that side
//   - if both sides open, prefer Left
TurnCommand chooseSide(const SensorReading& r) noexcept {
    const bool leftOpen = !r.left;
    const bool rightOpen = !r.right;
    if (leftOpen && !rightOpen) return TurnCommand::Left;
    if (!leftOpen && rightOpen) return TurnCommand::Right;
    if (leftOpen && rightOpen) return TurnCommand::Left;  // FR-003 deterministic
    return TurnCommand::None;  // both blocked → not handled here (escape)
}

}  // namespace

DriveDecision DefaultNavigationPolicy::nextDriveCommand(
    const SensorReading& reading,
    Phase /*currentPhase*/) {
    DriveDecision d{};
    if (!reading.front) {
        // No front obstacle → continue forward (FR-001).
        d.drive = DriveCommand::Forward;
        d.turn = TurnCommand::None;
        return d;
    }

    if (reading.front && reading.left && reading.right) {
        // Triple blocked is owned by Coordinator's escape orchestration.
        // Return a defensive Stop here; Coordinator must short-circuit.
        d.drive = DriveCommand::Stop;
        d.turn = TurnCommand::None;
        return d;
    }

    // Partial block (FR-003).
    d.drive = DriveCommand::Stop;
    d.turn = chooseSide(reading);
    return d;
}

EscapeAssist DefaultNavigationPolicy::plan_escape_enclosure(
    const SensorReading& reading) {
    EscapeAssist a{};
    // FR-004: the Coordinator only calls this on escape exit, which per its
    // predicate (L or R open) guarantees at least one side is open. We always
    // commit to a side turn (left preference, FR-003) so the next driving
    // tick does NOT advance back into the trap we just escaped from. If both
    // sides happen to be blocked (defensive path), return None — the
    // Coordinator will keep the robot in Escaping.
    a.turn = chooseSide(reading);
    return a;
}

}  // namespace rvc::domain
