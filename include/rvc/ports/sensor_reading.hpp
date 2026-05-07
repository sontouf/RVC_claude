// Trace: arch/vnv/traceability-matrix.md — FR-001..005, UC-002..005
#pragma once

namespace rvc::ports {

struct SensorReading {
    bool front{false};
    bool left{false};
    bool right{false};
    bool dust{false};
};

constexpr bool tripleBlocked(const SensorReading& r) noexcept {
    return r.front && r.left && r.right;
}

constexpr bool partialFrontBlocked(const SensorReading& r) noexcept {
    // front blocked but at least one side open
    return r.front && !(r.left && r.right);
}

}  // namespace rvc::ports
