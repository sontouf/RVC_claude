// Trace: arch/vnv/traceability-matrix.md §4
// Covers: FR-001 UC-002, FR-003 UC-003, FR-004 UC-004
#include <gtest/gtest.h>

#include "rvc/domain/default_navigation_policy.hpp"
#include "rvc/ports/sensor_reading.hpp"

using rvc::domain::DefaultNavigationPolicy;
using rvc::ports::DriveCommand;
using rvc::ports::Phase;
using rvc::ports::SensorReading;
using rvc::ports::TurnCommand;

TEST(DefaultNavigationPolicy, NoObstacle_GoesForward) {
    DefaultNavigationPolicy nav;
    SensorReading r{};
    auto d = nav.nextDriveCommand(r, Phase::Driving);
    EXPECT_EQ(d.drive, DriveCommand::Forward);
    EXPECT_EQ(d.turn, TurnCommand::None);
}

TEST(DefaultNavigationPolicy, FrontBlocked_LeftOnlyOpen_TurnsRight) {
    // front=true, left=true (blocked), right=false (open) → turn Right.
    DefaultNavigationPolicy nav;
    SensorReading r{};
    r.front = true; r.left = true; r.right = false;
    auto d = nav.nextDriveCommand(r, Phase::Driving);
    EXPECT_EQ(d.drive, DriveCommand::Stop);
    EXPECT_EQ(d.turn, TurnCommand::Right);
}

TEST(DefaultNavigationPolicy, FrontBlocked_RightOnlyOpen_TurnsLeft) {
    DefaultNavigationPolicy nav;
    SensorReading r{};
    r.front = true; r.left = false; r.right = true;
    auto d = nav.nextDriveCommand(r, Phase::Driving);
    EXPECT_EQ(d.drive, DriveCommand::Stop);
    EXPECT_EQ(d.turn, TurnCommand::Left);
}

TEST(DefaultNavigationPolicy, FrontBlocked_BothOpen_PrefersLeft) {
    // FR-003 deterministic: left preferred when both sides open.
    DefaultNavigationPolicy nav;
    SensorReading r{};
    r.front = true; r.left = false; r.right = false;
    auto d = nav.nextDriveCommand(r, Phase::Driving);
    EXPECT_EQ(d.drive, DriveCommand::Stop);
    EXPECT_EQ(d.turn, TurnCommand::Left);
}

TEST(DefaultNavigationPolicy, TripleBlocked_ReturnsStopForCoordinator) {
    // Coordinator owns the escape orchestration; policy must not pretend.
    DefaultNavigationPolicy nav;
    SensorReading r{};
    r.front = true; r.left = true; r.right = true;
    auto d = nav.nextDriveCommand(r, Phase::Driving);
    EXPECT_EQ(d.drive, DriveCommand::Stop);
    EXPECT_EQ(d.turn, TurnCommand::None);
}

TEST(DefaultNavigationPolicy, EscapeAssist_BothOpen_PrefersLeft) {
    DefaultNavigationPolicy nav;
    SensorReading r{};
    r.front = false; r.left = false; r.right = false;
    auto a = nav.plan_escape_enclosure(r);
    EXPECT_EQ(a.turn, TurnCommand::None);

    r.left = true;  // only right open
    a = nav.plan_escape_enclosure(r);
    EXPECT_EQ(a.turn, TurnCommand::Right);

    r.left = false; r.right = true;  // only left open
    a = nav.plan_escape_enclosure(r);
    EXPECT_EQ(a.turn, TurnCommand::Left);

    r.left = false; r.right = false;  // both open after backoff
    a = nav.plan_escape_enclosure(r);
    EXPECT_EQ(a.turn, TurnCommand::None);  // no turn — keep heading

    r.front = true; r.left = false; r.right = false;
    a = nav.plan_escape_enclosure(r);
    EXPECT_EQ(a.turn, TurnCommand::Left);  // FR-003 deterministic
}
