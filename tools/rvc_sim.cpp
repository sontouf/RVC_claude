// Trace: arch/vnv/traceability-matrix.md — simulator entry point (NFR-SYS-001)
//
// rvc_sim — headless simulator entry point used by system tests.
//
// Usage:
//   rvc_sim --scenario <path-to-scenario.txt>
//
// Reads a simple text scenario (see GridWorld::loadFromStream), runs the
// CleaningCoordinator for max_ticks, and emits a deterministic key=value
// result block on stdout for the Python runner to evaluate against the
// scenario JSON's `expect:` block.
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#include "rvc/app/cleaning_coordinator.hpp"
#include "rvc/app/controller_config.hpp"
#include "rvc/domain/default_cleaning_power_policy.hpp"
#include "rvc/domain/default_navigation_policy.hpp"
#include "rvc/tech/grid_actuator.hpp"
#include "rvc/tech/grid_sensor.hpp"
#include "rvc/tech/grid_world.hpp"

using rvc::app::CleaningCoordinator;
using rvc::app::ControllerConfig;
using rvc::domain::DefaultCleaningPowerPolicy;
using rvc::domain::DefaultNavigationPolicy;
using rvc::ports::CleaningPowerLevel;
using rvc::ports::Heading;
using rvc::ports::SessionState;
using rvc::tech::GridActuator;
using rvc::tech::GridSensor;
using rvc::tech::GridWorld;

namespace {

const char* headingName(Heading h) {
    switch (h) {
        case Heading::North: return "N";
        case Heading::East:  return "E";
        case Heading::South: return "S";
        case Heading::West:  return "W";
    }
    return "?";
}

const char* powerName(CleaningPowerLevel p) {
    switch (p) {
        case CleaningPowerLevel::Off:     return "Off";
        case CleaningPowerLevel::Nominal: return "Nominal";
        case CleaningPowerLevel::Boosted: return "Boosted";
    }
    return "?";
}

const char* sessionName(SessionState s) {
    return s == SessionState::Running ? "Running" : "Stopped";
}

int usage(const char* prog) {
    std::fprintf(stderr, "Usage: %s --scenario <path>\n", prog);
    return 2;
}

}  // namespace

int main(int argc, char** argv) {
    std::string scenarioPath;
    bool stopAtEnd = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--scenario") == 0 && i + 1 < argc) {
            scenarioPath = argv[++i];
        } else if (std::strcmp(argv[i], "--stop-at-end") == 0) {
            stopAtEnd = true;
        } else if (std::strcmp(argv[i], "--help") == 0 ||
                   std::strcmp(argv[i], "-h") == 0) {
            return usage(argv[0]);
        } else {
            std::fprintf(stderr, "rvc_sim: unknown arg: %s\n", argv[i]);
            return usage(argv[0]);
        }
    }
    if (scenarioPath.empty()) {
        return usage(argv[0]);
    }

    std::ifstream fin(scenarioPath);
    if (!fin) {
        std::fprintf(stderr, "rvc_sim: cannot open: %s\n", scenarioPath.c_str());
        return 3;
    }

    GridWorld world;
    std::string err;
    if (!world.loadFromStream(fin, err)) {
        std::fprintf(stderr, "rvc_sim: scenario error: %s\n", err.c_str());
        return 4;
    }

    GridSensor sensor(world);
    GridActuator actuator(world);
    DefaultNavigationPolicy nav;
    DefaultCleaningPowerPolicy power(world.config().dustBoostTicks);
    ControllerConfig cfg;
    cfg.dustBoostTicks = world.config().dustBoostTicks;
    cfg.maxBackoffTicks = world.config().maxBackoffTicks;

    CleaningCoordinator controller(sensor, actuator, nav, power, cfg);
    controller.startSession();

    const int maxTicks = world.config().maxTicks;
    int t = 0;
    for (; t < maxTicks; ++t) {
        controller.tick();
    }

    if (stopAtEnd) {
        controller.stopSession();
    }

    // Emit machine-readable result block.
    std::printf("=== rvc_sim result ===\n");
    std::printf("scenario %s\n", scenarioPath.c_str());
    std::printf("total_ticks %d\n", t);
    std::printf("collisions %llu\n",
                static_cast<unsigned long long>(world.collisions()));
    std::printf("cleaned_cells %zu\n", world.cleanedCells());
    std::printf("total_open_cells %zu\n", world.totalCells());
    std::printf("cleaned_ratio %.6f\n", world.cleanedRatio());
    std::printf("session_state %s\n", sessionName(controller.sessionState()));
    std::printf("phase %d\n", static_cast<int>(controller.phase()));
    std::printf("backoff_remaining %d\n", controller.backoffRemaining());
    std::printf("final_x %d\n", world.robotX());
    std::printf("final_y %d\n", world.robotY());
    std::printf("final_heading %s\n", headingName(world.heading()));
    std::printf("last_power %s\n", powerName(world.lastPower()));
    std::printf("=== rvc_sim end ===\n");
    return 0;
}
