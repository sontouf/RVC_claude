// Trace: arch/vnv/traceability-matrix.md §4
// Covers: FR-005 UC-005
#include <gtest/gtest.h>

#include "rvc/domain/default_cleaning_power_policy.hpp"

using rvc::domain::DefaultCleaningPowerPolicy;
using rvc::ports::CleaningPowerLevel;
using rvc::ports::SensorReading;

namespace {
SensorReading dustOn() { SensorReading r{}; r.dust = true; return r; }
SensorReading dustOff() { SensorReading r{}; r.dust = false; return r; }
}  // namespace

TEST(DefaultCleaningPowerPolicy, NoDust_ReturnsNominal) {
    DefaultCleaningPowerPolicy p(5);
    EXPECT_EQ(p.nextLevel(dustOff()), CleaningPowerLevel::Nominal);
}

TEST(DefaultCleaningPowerPolicy, DustOnce_BoostsForBoundedTicks) {
    DefaultCleaningPowerPolicy p(3);
    EXPECT_EQ(p.nextLevel(dustOn()),  CleaningPowerLevel::Boosted);
    EXPECT_EQ(p.remainingBoostTicks(), 3);
    EXPECT_EQ(p.nextLevel(dustOff()), CleaningPowerLevel::Boosted);
    EXPECT_EQ(p.nextLevel(dustOff()), CleaningPowerLevel::Boosted);
    EXPECT_EQ(p.nextLevel(dustOff()), CleaningPowerLevel::Nominal);
}

TEST(DefaultCleaningPowerPolicy, DustExtended_TimerResetsOnRedetection) {
    DefaultCleaningPowerPolicy p(2);
    EXPECT_EQ(p.nextLevel(dustOn()),  CleaningPowerLevel::Boosted);
    EXPECT_EQ(p.nextLevel(dustOff()), CleaningPowerLevel::Boosted);
    // Re-detection at the last boost tick should extend.
    EXPECT_EQ(p.nextLevel(dustOn()),  CleaningPowerLevel::Boosted);
    EXPECT_EQ(p.remainingBoostTicks(), 2);
    EXPECT_EQ(p.nextLevel(dustOff()), CleaningPowerLevel::Boosted);
    EXPECT_EQ(p.nextLevel(dustOff()), CleaningPowerLevel::Nominal);
}

TEST(DefaultCleaningPowerPolicy, ZeroOrNegativeBoostTicks_AlwaysNominal) {
    DefaultCleaningPowerPolicy p(0);
    EXPECT_EQ(p.nextLevel(dustOn()),  CleaningPowerLevel::Nominal);
    EXPECT_EQ(p.nextLevel(dustOff()), CleaningPowerLevel::Nominal);

    DefaultCleaningPowerPolicy q(-3);
    EXPECT_EQ(q.nextLevel(dustOn()),  CleaningPowerLevel::Nominal);
}

TEST(DefaultCleaningPowerPolicy, Reset_ClearsTimer) {
    DefaultCleaningPowerPolicy p(4);
    EXPECT_EQ(p.nextLevel(dustOn()), CleaningPowerLevel::Boosted);
    p.reset();
    EXPECT_EQ(p.remainingBoostTicks(), 0);
    EXPECT_EQ(p.nextLevel(dustOff()), CleaningPowerLevel::Nominal);
}
