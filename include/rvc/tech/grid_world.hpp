// Trace: arch/vnv/traceability-matrix.md — technical layer (UC-002..005 simulation)
#pragma once

#include <cstddef>
#include <cstdint>
#include <istream>
#include <string>
#include <vector>

#include "rvc/ports/enums.hpp"
#include "rvc/ports/sensor_reading.hpp"

namespace rvc::tech {

struct GridConfig {
    int dustBoostTicks{5};
    int maxBackoffTicks{4};
    int maxTicks{100};
};

class GridWorld {
public:
    GridWorld() = default;

    // Load a scenario from a simple text stream. Schema:
    //   width <int>
    //   height <int>
    //   robot <x> <y> <N|E|S|W>
    //   config dust_boost_ticks <int>
    //   config max_backoff_ticks <int>
    //   max_ticks <int>
    //   walls
    //   .....         (height lines, '.'=open, '#'=wall)
    //   dust
    //   .....         (height lines, '.'=clean, '*'=dust)
    //   end
    bool loadFromStream(std::istream& in, std::string& error);

    int width() const noexcept { return width_; }
    int height() const noexcept { return height_; }
    int robotX() const noexcept { return rx_; }
    int robotY() const noexcept { return ry_; }
    rvc::ports::Heading heading() const noexcept { return heading_; }
    GridConfig config() const noexcept { return config_; }

    // Sensor view at current pose.
    rvc::ports::SensorReading sensorReading() const;

    // Returns true if the cell is within bounds AND not a wall.
    bool isOpen(int x, int y) const noexcept;

    // Apply a drive/turn/power command. Returns true if a collision was
    // detected (movement attempted into a wall or out of bounds).
    bool applyDrive(rvc::ports::DriveCommand cmd);
    void applyTurn(rvc::ports::Direction dir);
    void applyPower(rvc::ports::CleaningPowerLevel level);

    // Bookkeeping for verification.
    std::uint64_t collisions() const noexcept { return collisions_; }
    std::size_t cleanedCells() const noexcept { return cleaned_; }
    std::size_t totalCells() const noexcept { return totalOpen_; }
    double cleanedRatio() const noexcept;
    rvc::ports::CleaningPowerLevel lastPower() const noexcept { return lastPower_; }

    // After load, the robot's starting cell is considered "visited" but only
    // counts toward cleaning when the power is at least Nominal at some tick.
    void markCurrentCellCleanedIfPowered();

private:
    bool wallAt(int x, int y) const noexcept;

    int width_{0};
    int height_{0};
    int rx_{0};
    int ry_{0};
    rvc::ports::Heading heading_{rvc::ports::Heading::North};
    GridConfig config_{};

    std::vector<std::uint8_t> walls_;
    std::vector<std::uint8_t> dust_;
    std::vector<std::uint8_t> visited_;

    std::uint64_t collisions_{0};
    std::size_t cleaned_{0};
    std::size_t totalOpen_{0};
    rvc::ports::CleaningPowerLevel lastPower_{rvc::ports::CleaningPowerLevel::Off};
};

}  // namespace rvc::tech
