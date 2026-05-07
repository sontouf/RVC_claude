// Trace: arch/vnv/traceability-matrix.md §4
// Covers: technical layer (UC-002..005 simulation harness)
#include <gtest/gtest.h>

#include <sstream>

#include "rvc/tech/grid_world.hpp"

using rvc::ports::Direction;
using rvc::ports::DriveCommand;
using rvc::ports::Heading;
using rvc::ports::CleaningPowerLevel;
using rvc::tech::GridWorld;

namespace {
const char* k5x5Open =
    "width 5\n"
    "height 5\n"
    "robot 2 2 N\n"
    "max_ticks 10\n"
    "config dust_boost_ticks 5\n"
    "config max_backoff_ticks 4\n"
    "walls\n"
    "#####\n"
    "#...#\n"
    "#...#\n"
    "#...#\n"
    "#####\n"
    "dust\n"
    ".....\n"
    ".....\n"
    ".....\n"
    ".....\n"
    ".....\n"
    "end\n";
}  // namespace

TEST(GridWorld, LoadsBasicScenario) {
    GridWorld w;
    std::istringstream iss(k5x5Open);
    std::string err;
    ASSERT_TRUE(w.loadFromStream(iss, err)) << err;
    EXPECT_EQ(w.width(), 5);
    EXPECT_EQ(w.height(), 5);
    EXPECT_EQ(w.robotX(), 2);
    EXPECT_EQ(w.robotY(), 2);
    EXPECT_EQ(w.heading(), Heading::North);
    EXPECT_EQ(w.totalCells(), 9u);  // 3x3 inner open cells
}

TEST(GridWorld, SensorReadingSeesNorthWallFromCornerCells) {
    GridWorld w;
    std::istringstream iss(k5x5Open);
    std::string err;
    ASSERT_TRUE(w.loadFromStream(iss, err));
    auto r = w.sensorReading();
    EXPECT_FALSE(r.front);
    EXPECT_FALSE(r.left);
    EXPECT_FALSE(r.right);

    // Move robot to (1,1), facing N. Now left wall and front wall.
    // We achieve this via reload.
    const char* edge =
        "width 5\nheight 5\nrobot 1 1 N\nmax_ticks 10\n"
        "config dust_boost_ticks 5\nconfig max_backoff_ticks 4\n"
        "walls\n#####\n#...#\n#...#\n#...#\n#####\n"
        "dust\n.....\n.....\n.....\n.....\n.....\nend\n";
    GridWorld w2;
    std::istringstream iss2(edge);
    ASSERT_TRUE(w2.loadFromStream(iss2, err));
    auto r2 = w2.sensorReading();
    EXPECT_TRUE(r2.front);
    EXPECT_TRUE(r2.left);
    EXPECT_FALSE(r2.right);
}

TEST(GridWorld, ApplyDriveForwardMovesRobotAndCleansCell) {
    GridWorld w;
    std::istringstream iss(k5x5Open);
    std::string err;
    ASSERT_TRUE(w.loadFromStream(iss, err));
    w.applyPower(CleaningPowerLevel::Nominal);
    EXPECT_FALSE(w.applyDrive(DriveCommand::Forward));
    EXPECT_EQ(w.robotX(), 2);
    EXPECT_EQ(w.robotY(), 1);
    w.markCurrentCellCleanedIfPowered();
    EXPECT_GE(w.cleanedCells(), 1u);
}

TEST(GridWorld, ApplyDriveIntoWallCountsCollision) {
    const char* tight =
        "width 3\nheight 3\nrobot 1 1 N\nmax_ticks 10\n"
        "config dust_boost_ticks 5\nconfig max_backoff_ticks 4\n"
        "walls\n###\n#.#\n###\ndust\n...\n...\n...\nend\n";
    GridWorld w;
    std::istringstream iss(tight);
    std::string err;
    ASSERT_TRUE(w.loadFromStream(iss, err));
    EXPECT_TRUE(w.applyDrive(DriveCommand::Forward));
    EXPECT_EQ(w.collisions(), 1u);
    EXPECT_EQ(w.robotX(), 1);
    EXPECT_EQ(w.robotY(), 1);
}

TEST(GridWorld, ApplyTurnRotatesHeading) {
    GridWorld w;
    std::istringstream iss(k5x5Open);
    std::string err;
    ASSERT_TRUE(w.loadFromStream(iss, err));
    w.applyTurn(Direction::Right);
    EXPECT_EQ(w.heading(), Heading::East);
    w.applyTurn(Direction::Right);
    EXPECT_EQ(w.heading(), Heading::South);
    w.applyTurn(Direction::Left);
    EXPECT_EQ(w.heading(), Heading::East);
}
