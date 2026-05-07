// Trace: arch/vnv/traceability-matrix.md §4
// Covers: simulator harness smoke (used by system_tests/run_all.py)
#include <gtest/gtest.h>

#include <sstream>

#include "rvc/tech/grid_world.hpp"

TEST(ScenarioSmoke, MalformedScenarioYieldsError) {
    rvc::tech::GridWorld w;
    std::istringstream iss("width 5\nheight 5\nrobot 99 99 N\n"
                            "walls\n#####\n#...#\n#...#\n#...#\n#####\n"
                            "dust\n.....\n.....\n.....\n.....\n.....\nend\n");
    std::string err;
    EXPECT_FALSE(w.loadFromStream(iss, err));
    EXPECT_FALSE(err.empty());
}

TEST(ScenarioSmoke, EmptyStreamYieldsError) {
    rvc::tech::GridWorld w;
    std::istringstream iss("");
    std::string err;
    EXPECT_FALSE(w.loadFromStream(iss, err));
}
