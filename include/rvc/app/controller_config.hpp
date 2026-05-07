// Trace: arch/vnv/traceability-matrix.md — UC-001, UC-004, UC-005
#pragma once

#include <cstdint>

namespace rvc::app {

struct ControllerConfig {
    int dustBoostTicks{5};
    int maxBackoffTicks{4};
    std::uint64_t seed{0};

    static ControllerConfig defaults() {
        return ControllerConfig{};
    }
};

}  // namespace rvc::app
