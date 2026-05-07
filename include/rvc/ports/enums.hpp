// Trace: arch/vnv/traceability-matrix.md — FR-001..005, UC-001..005
#pragma once

namespace rvc::ports {

enum class DriveCommand {
    Stop,
    Forward,
    Backward
};

enum class TurnCommand {
    None,
    Left,
    Right
};

enum class Direction {
    Left,
    Right
};

enum class CleaningPowerLevel {
    Off,
    Nominal,
    Boosted
};

enum class SessionState {
    Stopped,
    Running
};

enum class Phase {
    Driving,
    Avoiding,
    Escaping
};

enum class Heading {
    North,
    East,
    South,
    West
};

}  // namespace rvc::ports
