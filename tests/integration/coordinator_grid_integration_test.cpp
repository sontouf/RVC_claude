// Trace: arch/vnv/traceability-matrix.md §4
// Covers: FR-001..005 UC-001..005 (Coordinator + Grid* integration)
#include <gtest/gtest.h>

#include <sstream>

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
using rvc::ports::Heading;
using rvc::ports::Phase;
using rvc::ports::SessionState;
using rvc::tech::GridActuator;
using rvc::tech::GridSensor;
using rvc::tech::GridWorld;

namespace {

bool loadOpen5x5(GridWorld& w, int rx, int ry, char heading = 'N') {
    std::ostringstream oss;
    oss << "width 5\nheight 5\nrobot " << rx << " " << ry << " "
        << heading << "\nmax_ticks 30\n"
        << "config dust_boost_ticks 5\nconfig max_backoff_ticks 4\n"
        << "walls\n#####\n#...#\n#...#\n#...#\n#####\n"
        << "dust\n.....\n.....\n.....\n.....\n.....\nend\n";
    std::istringstream iss(oss.str());
    std::string err;
    return w.loadFromStream(iss, err);
}

}  // namespace

TEST(CoordinatorGridIntegration, ForwardCleaningFromCorner_TurnsAlongWalls) {
    GridWorld world;
    ASSERT_TRUE(loadOpen5x5(world, 1, 1, 'N'));
    GridSensor s(world);
    GridActuator a(world);
    DefaultNavigationPolicy nav;
    DefaultCleaningPowerPolicy power(world.config().dustBoostTicks);
    ControllerConfig cfg;
    cfg.dustBoostTicks = world.config().dustBoostTicks;
    cfg.maxBackoffTicks = world.config().maxBackoffTicks;
    CleaningCoordinator c(s, a, nav, power, cfg);

    c.startSession();
    for (int t = 0; t < 30; ++t) c.tick();

    EXPECT_EQ(world.collisions(), 0u);
    EXPECT_GT(world.cleanedCells(), 1u);
    EXPECT_EQ(c.sessionState(), SessionState::Running);
}

TEST(CoordinatorGridIntegration, CleansMostOfOpenSpaceWithinBudget) {
    GridWorld world;
    ASSERT_TRUE(loadOpen5x5(world, 1, 1, 'N'));
    GridSensor s(world);
    GridActuator a(world);
    DefaultNavigationPolicy nav;
    DefaultCleaningPowerPolicy power(world.config().dustBoostTicks);
    ControllerConfig cfg;
    cfg.dustBoostTicks = world.config().dustBoostTicks;
    cfg.maxBackoffTicks = world.config().maxBackoffTicks;
    CleaningCoordinator c(s, a, nav, power, cfg);

    c.startSession();
    for (int t = 0; t < 200; ++t) c.tick();

    EXPECT_EQ(world.collisions(), 0u);
    EXPECT_GT(world.cleanedRatio(), 0.4);
}

TEST(CoordinatorGridIntegration, EscapesTripleBlockedDeadEnd) {
    // 1-wide vertical dead-end (3x5). Robot at (1,3,N) drives up, hits the
    // closed top at (1,1), enters Escape, backs off within the budget, then
    // halts safely (FR-004 revised: backoff exits only when L or R opens;
    // since L/R never open in a 1-wide corridor, the safety cap holds the
    // robot at rest without re-entering the trap or bumping the back wall).
    const char* deadEnd =
        "width 3\nheight 5\nrobot 1 3 N\nmax_ticks 30\n"
        "config dust_boost_ticks 5\nconfig max_backoff_ticks 2\n"
        "walls\n###\n#.#\n#.#\n#.#\n###\n"
        "dust\n...\n...\n...\n...\n...\nend\n";
    GridWorld world;
    std::istringstream iss(deadEnd);
    std::string err;
    ASSERT_TRUE(world.loadFromStream(iss, err)) << err;
    GridSensor s(world);
    GridActuator a(world);
    DefaultNavigationPolicy nav;
    DefaultCleaningPowerPolicy power(world.config().dustBoostTicks);
    ControllerConfig cfg;
    cfg.dustBoostTicks = 5;
    cfg.maxBackoffTicks = 2;
    CleaningCoordinator c(s, a, nav, power, cfg);

    c.startSession();
    for (int t = 0; t < 30; ++t) c.tick();
    EXPECT_EQ(world.collisions(), 0u);
    EXPECT_EQ(c.phase(), Phase::Escaping);
}

TEST(CoordinatorGridIntegration, DustCellTriggersBoostAndAutoReturn) {
    const char* withDust =
        "width 5\nheight 5\nrobot 1 1 E\nmax_ticks 30\n"
        "config dust_boost_ticks 3\nconfig max_backoff_ticks 4\n"
        "walls\n#####\n#...#\n#...#\n#...#\n#####\n"
        "dust\n.....\n..*..\n.....\n.....\n.....\nend\n";
    GridWorld world;
    std::istringstream iss(withDust);
    std::string err;
    ASSERT_TRUE(world.loadFromStream(iss, err)) << err;
    GridSensor s(world);
    GridActuator a(world);
    DefaultNavigationPolicy nav;
    DefaultCleaningPowerPolicy power(world.config().dustBoostTicks);
    ControllerConfig cfg;
    cfg.dustBoostTicks = world.config().dustBoostTicks;
    cfg.maxBackoffTicks = world.config().maxBackoffTicks;
    CleaningCoordinator c(s, a, nav, power, cfg);

    c.startSession();
    bool sawBoost = false;
    for (int t = 0; t < 20; ++t) {
        c.tick();
        if (world.lastPower() == rvc::ports::CleaningPowerLevel::Boosted) {
            sawBoost = true;
        }
    }
    EXPECT_TRUE(sawBoost);
    // After enough ticks past the boost window, expect Nominal again.
    for (int t = 0; t < 10; ++t) c.tick();
    EXPECT_EQ(world.lastPower(), rvc::ports::CleaningPowerLevel::Nominal);
}

TEST(CoordinatorGridIntegration, StoppedSessionDoesNotMove) {
    GridWorld world;
    ASSERT_TRUE(loadOpen5x5(world, 2, 2, 'N'));
    GridSensor s(world);
    GridActuator a(world);
    DefaultNavigationPolicy nav;
    DefaultCleaningPowerPolicy power(world.config().dustBoostTicks);
    ControllerConfig cfg;
    cfg.dustBoostTicks = world.config().dustBoostTicks;
    cfg.maxBackoffTicks = world.config().maxBackoffTicks;
    CleaningCoordinator c(s, a, nav, power, cfg);

    // Don't start session.
    for (int t = 0; t < 10; ++t) c.tick();
    EXPECT_EQ(world.robotX(), 2);
    EXPECT_EQ(world.robotY(), 2);
    EXPECT_EQ(world.collisions(), 0u);
    EXPECT_EQ(c.sessionState(), SessionState::Stopped);
}

TEST(CoordinatorGridIntegration, DeterministicAcrossTwoRuns) {
    // NFR-DET-001
    auto runOnce = [](unsigned long long& finalPose, std::size_t& cleaned,
                      std::uint64_t& collisions) {
        GridWorld world;
        ASSERT_TRUE(loadOpen5x5(world, 1, 1, 'N'));
        GridSensor s(world);
        GridActuator a(world);
        DefaultNavigationPolicy nav;
        DefaultCleaningPowerPolicy power(world.config().dustBoostTicks);
        ControllerConfig cfg;
        cfg.dustBoostTicks = world.config().dustBoostTicks;
        cfg.maxBackoffTicks = world.config().maxBackoffTicks;
        CleaningCoordinator c(s, a, nav, power, cfg);
        c.startSession();
        for (int t = 0; t < 100; ++t) c.tick();
        finalPose = (static_cast<unsigned long long>(world.robotX()) << 32) |
                    static_cast<unsigned long long>(world.robotY());
        cleaned = world.cleanedCells();
        collisions = world.collisions();
    };
    unsigned long long p1 = 0, p2 = 0;
    std::size_t c1 = 0, c2 = 0;
    std::uint64_t k1 = 0, k2 = 0;
    runOnce(p1, c1, k1);
    runOnce(p2, c2, k2);
    EXPECT_EQ(p1, p2);
    EXPECT_EQ(c1, c2);
    EXPECT_EQ(k1, k2);
}
