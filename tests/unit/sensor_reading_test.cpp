// Trace: arch/vnv/traceability-matrix.md §4
// Covers: FR-003 UC-003, FR-004 UC-004
#include <gtest/gtest.h>

#include "rvc/ports/sensor_reading.hpp"

using rvc::ports::SensorReading;
using rvc::ports::partialFrontBlocked;
using rvc::ports::tripleBlocked;

TEST(SensorReadingHelpers, TripleBlocked_DetectsFrontLeftRight) {
    SensorReading r{};
    r.front = true; r.left = true; r.right = true;
    EXPECT_TRUE(tripleBlocked(r));
    EXPECT_FALSE(partialFrontBlocked(r));

    r.right = false;
    EXPECT_FALSE(tripleBlocked(r));
    EXPECT_TRUE(partialFrontBlocked(r));
}

TEST(SensorReadingHelpers, NoFront_NoneOfTheTriggers) {
    SensorReading r{};
    r.front = false; r.left = true; r.right = true;
    EXPECT_FALSE(tripleBlocked(r));
    EXPECT_FALSE(partialFrontBlocked(r));
}
