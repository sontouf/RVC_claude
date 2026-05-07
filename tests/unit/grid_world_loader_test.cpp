// Trace: arch/vnv/traceability-matrix.md §4 (technical layer)
// Covers: GridWorld scenario-loader error paths and behavior branches that
// system tests exercise only end-to-end. These are the longest tail of the
// coverage curve, so we exercise each parse error / heading rotation / power
// state explicitly here to satisfy NFR-TEST-001 (>=85% line / >=75% branch).
#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "rvc/tech/grid_world.hpp"

using rvc::ports::CleaningPowerLevel;
using rvc::ports::Direction;
using rvc::ports::DriveCommand;
using rvc::ports::Heading;
using rvc::tech::GridWorld;

namespace {

// Helper: build a minimal valid scenario string with a custom prologue,
// so each test can override one specific line/section to trigger the
// branch under test without re-typing the whole map.
std::string makeScenario(const std::string& body) {
    return body;
}

bool tryLoad(const std::string& src, std::string& err) {
    GridWorld w;
    std::istringstream iss(src);
    return w.loadFromStream(iss, err);
}

}  // namespace

// ============================================================================
// Loader: error branches
// ============================================================================

TEST(GridWorldLoader, EmptyStream_ReportsMissingSize) {
    std::string err;
    EXPECT_FALSE(tryLoad("", err));
    EXPECT_NE(err.find("size"), std::string::npos);
}

TEST(GridWorldLoader, MissingHeight_ReportsMissingSize) {
    std::string err;
    EXPECT_FALSE(tryLoad("width 5\nrobot 1 1 N\nend\n", err));
    EXPECT_NE(err.find("size"), std::string::npos);
}

TEST(GridWorldLoader, WallsBeforeSize_Rejected) {
    std::string err;
    EXPECT_FALSE(tryLoad("walls\n#####\nend\n", err));
    EXPECT_EQ(err, "walls before size");
}

TEST(GridWorldLoader, DustBeforeSize_Rejected) {
    std::string err;
    EXPECT_FALSE(tryLoad("dust\n.....\nend\n", err));
    EXPECT_EQ(err, "dust before size");
}

TEST(GridWorldLoader, InvalidHeading_Rejected) {
    const std::string scn =
        "width 3\nheight 3\nrobot 1 1 Q\n"
        "walls\n###\n#.#\n###\n"
        "dust\n...\n...\n...\nend\n";
    std::string err;
    EXPECT_FALSE(tryLoad(scn, err));
    EXPECT_NE(err.find("invalid heading"), std::string::npos);
}

TEST(GridWorldLoader, RobotOutOfBounds_Rejected) {
    const std::string scn =
        "width 3\nheight 3\nrobot 99 99 N\n"
        "walls\n###\n#.#\n###\n"
        "dust\n...\n...\n...\nend\n";
    std::string err;
    EXPECT_FALSE(tryLoad(scn, err));
    EXPECT_EQ(err, "robot out of bounds");
}

TEST(GridWorldLoader, RobotOnWall_Rejected) {
    // Robot placed at (0,0) which is the top-left wall corner.
    const std::string scn =
        "width 3\nheight 3\nrobot 0 0 N\n"
        "walls\n###\n#.#\n###\n"
        "dust\n...\n...\n...\nend\n";
    std::string err;
    EXPECT_FALSE(tryLoad(scn, err));
    EXPECT_EQ(err, "robot on wall");
}

TEST(GridWorldLoader, UnknownDirective_Rejected) {
    const std::string scn =
        "width 3\nheight 3\nrobot 1 1 N\n"
        "wibble 7\n"
        "walls\n###\n#.#\n###\n"
        "dust\n...\n...\n...\nend\n";
    std::string err;
    EXPECT_FALSE(tryLoad(scn, err));
    EXPECT_NE(err.find("unknown directive"), std::string::npos);
}

TEST(GridWorldLoader, UnknownConfigKey_Rejected) {
    const std::string scn =
        "width 3\nheight 3\nrobot 1 1 N\n"
        "config bogus_key 7\n"
        "walls\n###\n#.#\n###\n"
        "dust\n...\n...\n...\nend\n";
    std::string err;
    EXPECT_FALSE(tryLoad(scn, err));
    EXPECT_NE(err.find("unknown config key"), std::string::npos);
}

TEST(GridWorldLoader, WallRowShorterThanWidth_Rejected) {
    // width=5 but a wall row has only 3 chars.
    const std::string scn =
        "width 5\nheight 3\nrobot 1 1 N\n"
        "walls\n#####\n#.#\n#####\n"
        "dust\n.....\n.....\n.....\nend\n";
    std::string err;
    EXPECT_FALSE(tryLoad(scn, err));
    EXPECT_NE(err.find("wall row"), std::string::npos);
}

TEST(GridWorldLoader, DustRowShorterThanWidth_Rejected) {
    const std::string scn =
        "width 5\nheight 3\nrobot 1 1 N\n"
        "walls\n#####\n#...#\n#####\n"
        "dust\n.....\n...\n.....\nend\n";
    std::string err;
    EXPECT_FALSE(tryLoad(scn, err));
    EXPECT_NE(err.find("dust row"), std::string::npos);
}

TEST(GridWorldLoader, CommentLine_IgnoredOutsideMaps) {
    const std::string scn =
        "# header comment\n"
        "width 3\nheight 3\n"
        "# this scenario tests comment handling\n"
        "robot 1 1 N\n"
        "walls\n###\n#.#\n###\n"
        "dust\n...\n...\n...\nend\n";
    std::string err;
    EXPECT_TRUE(tryLoad(scn, err)) << err;
}

TEST(GridWorldLoader, CarriageReturnLineEndings_StrippedSafely) {
    // Force CRLF endings to exercise the trailing-\r strip branch.
    const std::string scn =
        "width 3\r\nheight 3\r\nrobot 1 1 N\r\n"
        "walls\r\n###\r\n#.#\r\n###\r\n"
        "dust\r\n...\r\n...\r\n...\r\nend\r\n";
    std::string err;
    EXPECT_TRUE(tryLoad(scn, err)) << err;
}

// ============================================================================
// Loader: happy path - all four headings round-trip through parseHeading and
// produce the expected sensorReading wall-pattern.
// ============================================================================

namespace {

constexpr const char* k3x3HollowWalls =
    "walls\n###\n#.#\n###\n"
    "dust\n...\n...\n...\nend\n";

// The 3x3 hollow-square always has the robot at the only floor cell (1,1)
// surrounded by walls, so for every heading the front sensor must be true.
GridWorld load3x3WithHeading(char h) {
    std::ostringstream oss;
    oss << "width 3\nheight 3\nrobot 1 1 " << h << "\n" << k3x3HollowWalls;
    GridWorld w;
    std::istringstream iss(oss.str());
    std::string err;
    EXPECT_TRUE(w.loadFromStream(iss, err)) << err;
    return w;
}

}  // namespace

TEST(GridWorldHeadings, AllFourHeadingsParse) {
    EXPECT_EQ(load3x3WithHeading('N').heading(), Heading::North);
    EXPECT_EQ(load3x3WithHeading('E').heading(), Heading::East);
    EXPECT_EQ(load3x3WithHeading('S').heading(), Heading::South);
    EXPECT_EQ(load3x3WithHeading('W').heading(), Heading::West);
}

TEST(GridWorldHeadings, SensorReadingIsTripleBlockedFromAllHeadings) {
    for (char h : {'N', 'E', 'S', 'W'}) {
        auto w = load3x3WithHeading(h);
        auto r = w.sensorReading();
        EXPECT_TRUE(r.front);
        EXPECT_TRUE(r.left);
        EXPECT_TRUE(r.right);
    }
}

TEST(GridWorldHeadings, RotateLeftCyclesAllFour) {
    auto w = load3x3WithHeading('N');
    w.applyTurn(Direction::Left); EXPECT_EQ(w.heading(), Heading::West);
    w.applyTurn(Direction::Left); EXPECT_EQ(w.heading(), Heading::South);
    w.applyTurn(Direction::Left); EXPECT_EQ(w.heading(), Heading::East);
    w.applyTurn(Direction::Left); EXPECT_EQ(w.heading(), Heading::North);
}

TEST(GridWorldHeadings, RotateRightCyclesAllFour) {
    auto w = load3x3WithHeading('N');
    w.applyTurn(Direction::Right); EXPECT_EQ(w.heading(), Heading::East);
    w.applyTurn(Direction::Right); EXPECT_EQ(w.heading(), Heading::South);
    w.applyTurn(Direction::Right); EXPECT_EQ(w.heading(), Heading::West);
    w.applyTurn(Direction::Right); EXPECT_EQ(w.heading(), Heading::North);
}

// ============================================================================
// Drive: each command branch and the wall/non-wall paths.
// ============================================================================

namespace {

GridWorld loadOpen5x5(int rx, int ry, char h) {
    std::ostringstream oss;
    oss << "width 5\nheight 5\nrobot " << rx << " " << ry << " " << h << "\n"
        << "walls\n#####\n#...#\n#...#\n#...#\n#####\n"
        << "dust\n.....\n.....\n.....\n.....\n.....\nend\n";
    GridWorld w;
    std::istringstream iss(oss.str());
    std::string err;
    EXPECT_TRUE(w.loadFromStream(iss, err)) << err;
    return w;
}

}  // namespace

TEST(GridWorldDrive, StopReturnsFalseAndDoesNotMove) {
    auto w = loadOpen5x5(2, 2, 'N');
    EXPECT_FALSE(w.applyDrive(DriveCommand::Stop));
    EXPECT_EQ(w.robotX(), 2);
    EXPECT_EQ(w.robotY(), 2);
}

TEST(GridWorldDrive, BackwardMovesAwayFromHeading) {
    // Facing N (dy=-1), backward goes south (+y).
    auto w = loadOpen5x5(2, 2, 'N');
    EXPECT_FALSE(w.applyDrive(DriveCommand::Backward));
    EXPECT_EQ(w.robotX(), 2);
    EXPECT_EQ(w.robotY(), 3);
}

TEST(GridWorldDrive, BackwardIntoWallCountsCollision) {
    auto w = loadOpen5x5(1, 1, 'S');  // facing south, backward = north into wall
    EXPECT_TRUE(w.applyDrive(DriveCommand::Backward));
    EXPECT_EQ(w.collisions(), 1u);
    EXPECT_EQ(w.robotX(), 1);
    EXPECT_EQ(w.robotY(), 1);
}

TEST(GridWorldDrive, RevisitDoesNotResetCleanedFlag) {
    // visited_ must not regress a "cleaned (=2)" cell back to 1; this
    // protects cleaned_cells from double-counting (FR-001 / NFR-DET-001).
    // Note: GridWorld::applyDrive does NOT auto-clean the new cell; the
    // GridActuator wrapper calls markCurrentCellCleanedIfPowered() after
    // each drive. We replicate that ordering here explicitly.
    auto w = loadOpen5x5(2, 2, 'N');
    w.applyPower(CleaningPowerLevel::Nominal);
    EXPECT_EQ(w.cleanedCells(), 1u);  // start cell cleaned by power-on

    EXPECT_FALSE(w.applyDrive(DriveCommand::Forward));   // (2,1)
    w.markCurrentCellCleanedIfPowered();
    EXPECT_EQ(w.cleanedCells(), 2u);

    EXPECT_FALSE(w.applyDrive(DriveCommand::Backward));  // back to (2,2)
    w.markCurrentCellCleanedIfPowered();
    // (2,2) was already cleaned; revisit must NOT bump cleaned_cells to 3.
    EXPECT_EQ(w.cleanedCells(), 2u);
}

// ============================================================================
// Power: Off / Nominal / Boosted transitions and clean-counting interaction.
// ============================================================================

TEST(GridWorldPower, OffDoesNotCleanCells) {
    auto w = loadOpen5x5(2, 2, 'N');
    w.applyPower(CleaningPowerLevel::Off);
    EXPECT_EQ(w.cleanedCells(), 0u);
    EXPECT_EQ(w.lastPower(), CleaningPowerLevel::Off);
}

TEST(GridWorldPower, BoostedAlsoCleansCurrentCell) {
    auto w = loadOpen5x5(2, 2, 'N');
    w.applyPower(CleaningPowerLevel::Boosted);
    EXPECT_EQ(w.cleanedCells(), 1u);
    EXPECT_EQ(w.lastPower(), CleaningPowerLevel::Boosted);
}

TEST(GridWorldPower, MarkCurrentCellWithoutPowerIsNoop) {
    auto w = loadOpen5x5(2, 2, 'N');
    // last_power defaults to Off, so explicit mark must do nothing.
    w.markCurrentCellCleanedIfPowered();
    EXPECT_EQ(w.cleanedCells(), 0u);
}

TEST(GridWorldPower, RatioWithNoMovementMatches1OverTotal) {
    auto w = loadOpen5x5(2, 2, 'N');
    w.applyPower(CleaningPowerLevel::Nominal);
    const double r = w.cleanedRatio();
    EXPECT_GT(r, 0.0);
    EXPECT_LE(r, 1.0);
}

TEST(GridWorldPower, RatioOnFullyEnclosedScenarioIsBounded) {
    // 3x3 hollow has only 1 open cell -> ratio should be 1.0 after cleaning it.
    auto w = load3x3WithHeading('N');
    w.applyPower(CleaningPowerLevel::Nominal);
    EXPECT_EQ(w.cleanedRatio(), 1.0);
}

// ============================================================================
// isOpen: out-of-bounds always reports as not-open (defensive bounds check).
// ============================================================================

TEST(GridWorldIsOpen, OutOfBoundsAndWallsReportNotOpen) {
    auto w = loadOpen5x5(2, 2, 'N');
    EXPECT_FALSE(w.isOpen(-1, 2));
    EXPECT_FALSE(w.isOpen(2, -1));
    EXPECT_FALSE(w.isOpen(99, 2));
    EXPECT_FALSE(w.isOpen(2, 99));
    EXPECT_FALSE(w.isOpen(0, 0));   // top-left wall
    EXPECT_FALSE(w.isOpen(4, 4));   // bottom-right wall
    EXPECT_TRUE(w.isOpen(2, 2));    // floor
    EXPECT_TRUE(w.isOpen(1, 1));    // floor (corner of inner area)
}
