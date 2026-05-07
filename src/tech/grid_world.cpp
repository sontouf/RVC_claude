// Trace: arch/vnv/traceability-matrix.md — technical layer
#include "rvc/tech/grid_world.hpp"

#include <sstream>
#include <string>
#include <vector>

namespace rvc::tech {

using rvc::ports::CleaningPowerLevel;
using rvc::ports::Direction;
using rvc::ports::DriveCommand;
using rvc::ports::Heading;
using rvc::ports::SensorReading;

namespace {

Heading parseHeading(const std::string& s, bool& ok) {
    ok = true;
    if (s == "N") return Heading::North;
    if (s == "E") return Heading::East;
    if (s == "S") return Heading::South;
    if (s == "W") return Heading::West;
    ok = false;
    return Heading::North;
}

// Forward delta for a heading (dx, dy) where +y points South.
void delta(Heading h, int& dx, int& dy) noexcept {
    dx = 0;
    dy = 0;
    switch (h) {
        case Heading::North: dy = -1; break;
        case Heading::East:  dx = 1;  break;
        case Heading::South: dy = 1;  break;
        case Heading::West:  dx = -1; break;
    }
}

Heading rotateLeft(Heading h) noexcept {
    switch (h) {
        case Heading::North: return Heading::West;
        case Heading::West:  return Heading::South;
        case Heading::South: return Heading::East;
        case Heading::East:  return Heading::North;
    }
    return h;
}

Heading rotateRight(Heading h) noexcept {
    switch (h) {
        case Heading::North: return Heading::East;
        case Heading::East:  return Heading::South;
        case Heading::South: return Heading::West;
        case Heading::West:  return Heading::North;
    }
    return h;
}

}  // namespace

bool GridWorld::loadFromStream(std::istream& in, std::string& error) {
    *this = GridWorld{};

    std::string line;
    bool inWalls = false;
    bool inDust = false;
    int wallRow = 0;
    int dustRow = 0;
    bool haveSize = false;

    auto resizeMaps = [&]() {
        const std::size_t n = static_cast<std::size_t>(width_) * height_;
        walls_.assign(n, 0);
        dust_.assign(n, 0);
        visited_.assign(n, 0);
    };

    while (std::getline(in, line)) {
        // strip trailing \r
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
            line.pop_back();
        }
        if (line.empty()) continue;

        if (inWalls) {
            if (wallRow >= height_) {
                error = "too many wall rows";
                return false;
            }
            if (static_cast<int>(line.size()) < width_) {
                error = "wall row " + std::to_string(wallRow) +
                        " shorter than width";
                return false;
            }
            for (int x = 0; x < width_; ++x) {
                walls_[wallRow * width_ + x] = (line[x] == '#') ? 1 : 0;
            }
            ++wallRow;
            if (wallRow == height_) inWalls = false;
            continue;
        }
        if (inDust) {
            if (dustRow >= height_) {
                error = "too many dust rows";
                return false;
            }
            if (static_cast<int>(line.size()) < width_) {
                error = "dust row " + std::to_string(dustRow) +
                        " shorter than width";
                return false;
            }
            for (int x = 0; x < width_; ++x) {
                dust_[dustRow * width_ + x] = (line[x] == '*') ? 1 : 0;
            }
            ++dustRow;
            if (dustRow == height_) inDust = false;
            continue;
        }

        std::istringstream iss(line);
        std::string head;
        iss >> head;
        if (head == "width") {
            iss >> width_;
        } else if (head == "height") {
            iss >> height_;
            if (width_ > 0 && height_ > 0 && !haveSize) {
                resizeMaps();
                haveSize = true;
            }
        } else if (head == "robot") {
            std::string hs;
            iss >> rx_ >> ry_ >> hs;
            bool ok = false;
            heading_ = parseHeading(hs, ok);
            if (!ok) {
                error = "invalid heading: " + hs;
                return false;
            }
        } else if (head == "config") {
            std::string key;
            int v = 0;
            iss >> key >> v;
            if (key == "dust_boost_ticks") config_.dustBoostTicks = v;
            else if (key == "max_backoff_ticks") config_.maxBackoffTicks = v;
            else { error = "unknown config key: " + key; return false; }
        } else if (head == "max_ticks") {
            iss >> config_.maxTicks;
        } else if (head == "walls") {
            if (!haveSize) { error = "walls before size"; return false; }
            inWalls = true;
            wallRow = 0;
        } else if (head == "dust") {
            if (!haveSize) { error = "dust before size"; return false; }
            inDust = true;
            dustRow = 0;
        } else if (head == "end") {
            break;
        } else if (head[0] == '#') {
            // comment line
        } else {
            error = "unknown directive: " + head;
            return false;
        }
    }

    if (!haveSize || width_ <= 0 || height_ <= 0) {
        error = "missing or invalid size";
        return false;
    }
    if (rx_ < 0 || rx_ >= width_ || ry_ < 0 || ry_ >= height_) {
        error = "robot out of bounds";
        return false;
    }
    if (wallAt(rx_, ry_)) {
        error = "robot on wall";
        return false;
    }

    totalOpen_ = 0;
    for (std::size_t i = 0; i < walls_.size(); ++i) {
        if (!walls_[i]) ++totalOpen_;
    }

    // Mark starting cell visited; it will become "cleaned" once power is on.
    visited_[ry_ * width_ + rx_] = 1;
    return true;
}

bool GridWorld::wallAt(int x, int y) const noexcept {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return true;
    return walls_[y * width_ + x] != 0;
}

bool GridWorld::isOpen(int x, int y) const noexcept {
    return !wallAt(x, y);
}

SensorReading GridWorld::sensorReading() const {
    int dx = 0, dy = 0;
    delta(heading_, dx, dy);
    SensorReading r{};
    r.front = wallAt(rx_ + dx, ry_ + dy);

    const Heading lh = rotateLeft(heading_);
    int ldx = 0, ldy = 0;
    delta(lh, ldx, ldy);
    r.left = wallAt(rx_ + ldx, ry_ + ldy);

    const Heading rh = rotateRight(heading_);
    int rdx = 0, rdy = 0;
    delta(rh, rdx, rdy);
    r.right = wallAt(rx_ + rdx, ry_ + rdy);

    r.dust = (dust_[ry_ * width_ + rx_] != 0);
    return r;
}

bool GridWorld::applyDrive(DriveCommand cmd) {
    int dx = 0, dy = 0;
    delta(heading_, dx, dy);
    int nx = rx_, ny = ry_;
    bool moved = false;
    switch (cmd) {
        case DriveCommand::Forward:
            nx = rx_ + dx; ny = ry_ + dy; moved = true; break;
        case DriveCommand::Backward:
            nx = rx_ - dx; ny = ry_ - dy; moved = true; break;
        case DriveCommand::Stop:
        default:
            return false;
    }
    if (!moved) return false;
    if (wallAt(nx, ny)) {
        ++collisions_;
        return true;  // collision, stay in place
    }
    rx_ = nx;
    ry_ = ny;
    const std::size_t idx =
        static_cast<std::size_t>(ry_) * width_ + static_cast<std::size_t>(rx_);
    if (visited_[idx] == 0) {
        visited_[idx] = 1;
    }
    return false;
}

void GridWorld::applyTurn(Direction dir) {
    heading_ = (dir == Direction::Left) ? rotateLeft(heading_)
                                        : rotateRight(heading_);
}

void GridWorld::applyPower(CleaningPowerLevel level) {
    lastPower_ = level;
    if (level != CleaningPowerLevel::Off) {
        // Cleans the current cell once on first power-on at this cell.
        markCurrentCellCleanedIfPowered();
    }
}

void GridWorld::markCurrentCellCleanedIfPowered() {
    if (lastPower_ == CleaningPowerLevel::Off) return;
    const std::size_t idx =
        static_cast<std::size_t>(ry_) * width_ + static_cast<std::size_t>(rx_);
    // visited_ doubles as "cleaned" once power has been applied. Dust on the
    // cell is left in place so the controller's sensor can still report it on
    // a subsequent tick (system tests rely on that ordering).
    if (visited_[idx] != 2) {
        visited_[idx] = 2;
        ++cleaned_;
    }
}

double GridWorld::cleanedRatio() const noexcept {
    if (totalOpen_ == 0) return 0.0;
    return static_cast<double>(cleaned_) / static_cast<double>(totalOpen_);
}

}  // namespace rvc::tech
