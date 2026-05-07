// Trace: arch/vnv/traceability-matrix.md §4
// Covers: FR-001..005 UC-001..005, NFR-SAFE-001
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include "rvc/app/cleaning_coordinator.hpp"
#include "rvc/app/controller_config.hpp"
#include "rvc/domain/default_cleaning_power_policy.hpp"
#include "rvc/domain/default_navigation_policy.hpp"
#include "rvc/ports/i_actuator_port.hpp"
#include "rvc/ports/i_sensor_port.hpp"

using rvc::app::CleaningCoordinator;
using rvc::app::ControllerConfig;
using rvc::domain::DefaultCleaningPowerPolicy;
using rvc::domain::DefaultNavigationPolicy;
using rvc::ports::CleaningPowerLevel;
using rvc::ports::Direction;
using rvc::ports::DriveCommand;
using rvc::ports::IActuatorPort;
using rvc::ports::ISensorPort;
using rvc::ports::Phase;
using rvc::ports::SensorReading;
using rvc::ports::SessionState;

namespace {

class StubSensor : public ISensorPort {
public:
    void enqueue(SensorReading r) { queue_.push_back(r); }
    SensorReading read() override {
        if (queue_.empty()) return SensorReading{};
        SensorReading r = queue_.front();
        queue_.erase(queue_.begin());
        last_ = r;
        return r;
    }
    SensorReading lastRead() const { return last_; }

private:
    std::vector<SensorReading> queue_;
    SensorReading last_{};
};

struct LoggedCall {
    enum class Kind { Drive, Turn, Power } kind;
    DriveCommand drive;
    Direction turnDir;
    CleaningPowerLevel power;
};

class RecordingActuator : public IActuatorPort {
public:
    void drive(DriveCommand cmd) override {
        log_.push_back({LoggedCall::Kind::Drive, cmd, Direction::Left,
                        CleaningPowerLevel::Off});
    }
    void turn(Direction dir) override {
        log_.push_back({LoggedCall::Kind::Turn, DriveCommand::Stop, dir,
                        CleaningPowerLevel::Off});
    }
    void setPower(CleaningPowerLevel level) override {
        log_.push_back({LoggedCall::Kind::Power, DriveCommand::Stop,
                        Direction::Left, level});
    }

    const std::vector<LoggedCall>& log() const { return log_; }
    void clear() { log_.clear(); }

    int countDrive(DriveCommand cmd) const {
        int n = 0;
        for (const auto& c : log_) {
            if (c.kind == LoggedCall::Kind::Drive && c.drive == cmd) ++n;
        }
        return n;
    }
    int countTurn(Direction d) const {
        int n = 0;
        for (const auto& c : log_) {
            if (c.kind == LoggedCall::Kind::Turn && c.turnDir == d) ++n;
        }
        return n;
    }
    int countPower(CleaningPowerLevel p) const {
        int n = 0;
        for (const auto& c : log_) {
            if (c.kind == LoggedCall::Kind::Power && c.power == p) ++n;
        }
        return n;
    }

private:
    std::vector<LoggedCall> log_;
};

struct Fixture {
    StubSensor sensor;
    RecordingActuator actuator;
    DefaultNavigationPolicy nav;
    DefaultCleaningPowerPolicy power{4};
    ControllerConfig config{};
    CleaningCoordinator make() {
        return CleaningCoordinator(sensor, actuator, nav, power, config);
    }
};

SensorReading allClear() { return SensorReading{}; }
SensorReading withFlags(bool f, bool l, bool r, bool d) {
    SensorReading s{}; s.front = f; s.left = l; s.right = r; s.dust = d;
    return s;
}

}  // namespace

TEST(CleaningCoordinator, StartSession_TransitionsToRunningAndPowersOn) {
    Fixture fx;
    auto c = fx.make();
    c.startSession();
    EXPECT_EQ(c.sessionState(), SessionState::Running);
    EXPECT_GE(fx.actuator.countPower(CleaningPowerLevel::Nominal), 1);
    // No drive command is issued during start; the first drive is emitted
    // by the next tick() after a sensor read.
    EXPECT_EQ(fx.actuator.countDrive(DriveCommand::Forward), 0);
    EXPECT_EQ(fx.actuator.countDrive(DriveCommand::Backward), 0);
}

TEST(CleaningCoordinator, StopSession_StopsAndPowerOff) {
    Fixture fx;
    auto c = fx.make();
    c.startSession();
    fx.actuator.clear();
    c.stopSession();
    EXPECT_EQ(c.sessionState(), SessionState::Stopped);
    EXPECT_GE(fx.actuator.countDrive(DriveCommand::Stop), 1);
    EXPECT_GE(fx.actuator.countPower(CleaningPowerLevel::Off), 1);
}

TEST(CleaningCoordinator, StartIsIdempotent) {
    Fixture fx;
    auto c = fx.make();
    c.startSession();
    fx.actuator.clear();
    c.startSession();  // second call should not re-emit
    EXPECT_EQ(fx.actuator.log().size(), 0u);
    EXPECT_EQ(c.sessionState(), SessionState::Running);
}

TEST(CleaningCoordinator, SessionStopped_NoActuatorCalls) {
    // NFR-SAFE-001
    Fixture fx;
    auto c = fx.make();
    fx.sensor.enqueue(allClear());
    c.tick();
    EXPECT_EQ(fx.actuator.log().size(), 0u);
    EXPECT_EQ(c.sessionState(), SessionState::Stopped);
}

TEST(CleaningCoordinator, Tick_NoObstacle_DrivesForwardWithNominalPower) {
    Fixture fx;
    auto c = fx.make();
    c.startSession();
    fx.actuator.clear();
    fx.sensor.enqueue(allClear());
    c.tick();
    EXPECT_EQ(fx.actuator.countDrive(DriveCommand::Forward), 1);
    EXPECT_EQ(fx.actuator.countPower(CleaningPowerLevel::Nominal), 1);
    EXPECT_EQ(c.phase(), Phase::Driving);
}

TEST(CleaningCoordinator, Tick_PartialFrontBoth_OpenPrefersLeft) {
    // FR-003 deterministic: both sides open → Left.
    Fixture fx;
    auto c = fx.make();
    c.startSession();
    fx.actuator.clear();
    fx.sensor.enqueue(withFlags(true, false, false, false));
    c.tick();
    EXPECT_EQ(fx.actuator.countDrive(DriveCommand::Stop), 1);
    EXPECT_EQ(fx.actuator.countTurn(Direction::Left), 1);
    EXPECT_EQ(c.phase(), Phase::Avoiding);
}

TEST(CleaningCoordinator, Tick_PartialFrontLeftBlocked_TurnsRight) {
    Fixture fx;
    auto c = fx.make();
    c.startSession();
    fx.actuator.clear();
    fx.sensor.enqueue(withFlags(true, true, false, false));
    c.tick();
    EXPECT_EQ(fx.actuator.countTurn(Direction::Right), 1);
}

TEST(CleaningCoordinator, Tick_TripleBlocked_StopsThenBackward) {
    // UC-004 enter escape.
    Fixture fx;
    auto c = fx.make();
    c.startSession();
    fx.actuator.clear();
    fx.sensor.enqueue(withFlags(true, true, true, false));
    c.tick();
    EXPECT_EQ(fx.actuator.countDrive(DriveCommand::Stop), 1);
    EXPECT_EQ(fx.actuator.countDrive(DriveCommand::Backward), 1);
    EXPECT_EQ(c.phase(), Phase::Escaping);
}

TEST(CleaningCoordinator, Tick_EscapeThenSideOpens_TurnsAndExitsEscape) {
    Fixture fx;
    auto c = fx.make();
    c.startSession();
    fx.actuator.clear();

    // tick 1: triple blocked → enter escape, backward.
    fx.sensor.enqueue(withFlags(true, true, true, false));
    c.tick();
    EXPECT_EQ(c.phase(), Phase::Escaping);

    // tick 2: only left blocked → escape assist returns Right, exit escape.
    fx.actuator.clear();
    fx.sensor.enqueue(withFlags(true, true, false, false));
    c.tick();
    EXPECT_EQ(fx.actuator.countTurn(Direction::Right), 1);
    EXPECT_EQ(c.phase(), Phase::Driving);
    EXPECT_EQ(c.backoffRemaining(), 0);
}

TEST(CleaningCoordinator, Tick_EscapeStillBlocked_KeepsBackingOffWithinBudget) {
    Fixture fx;
    fx.config.maxBackoffTicks = 3;
    auto c = fx.make();
    c.startSession();
    fx.actuator.clear();

    fx.sensor.enqueue(withFlags(true, true, true, false));
    c.tick();
    EXPECT_EQ(c.backoffRemaining(), 2);  // 3 - 1 (consumed by initial backward)

    fx.actuator.clear();
    fx.sensor.enqueue(withFlags(true, true, true, false));
    c.tick();
    EXPECT_EQ(fx.actuator.countDrive(DriveCommand::Backward), 1);
    EXPECT_EQ(c.backoffRemaining(), 1);

    fx.actuator.clear();
    fx.sensor.enqueue(withFlags(true, true, true, false));
    c.tick();
    EXPECT_EQ(fx.actuator.countDrive(DriveCommand::Backward), 1);
    EXPECT_EQ(c.backoffRemaining(), 0);

    // Budget exhausted → safety stop while still escaping.
    fx.actuator.clear();
    fx.sensor.enqueue(withFlags(true, true, true, false));
    c.tick();
    EXPECT_EQ(fx.actuator.countDrive(DriveCommand::Stop), 1);
    EXPECT_EQ(c.phase(), Phase::Escaping);
}

TEST(CleaningCoordinator, Tick_DustDetected_BoostsPowerForBoundedDuration) {
    Fixture fx;
    fx.config.dustBoostTicks = 2;
    fx.power = DefaultCleaningPowerPolicy(2);
    auto c = fx.make();
    c.startSession();
    fx.actuator.clear();

    fx.sensor.enqueue(withFlags(false, false, false, true));
    c.tick();
    EXPECT_EQ(fx.actuator.countPower(CleaningPowerLevel::Boosted), 1);

    fx.actuator.clear();
    fx.sensor.enqueue(allClear());
    c.tick();
    EXPECT_EQ(fx.actuator.countPower(CleaningPowerLevel::Boosted), 1);

    fx.actuator.clear();
    fx.sensor.enqueue(allClear());
    c.tick();
    EXPECT_EQ(fx.actuator.countPower(CleaningPowerLevel::Nominal), 1);
}
