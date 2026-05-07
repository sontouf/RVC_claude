#!/usr/bin/env python3
"""Python emulator of the C++ RVC controller for offline scenario debugging.

Mirrors `src/app/cleaning_coordinator.cpp`, `src/domain/*.cpp`, and
`src/tech/grid_world.cpp` exactly. Use it to iterate on scenario expectations
without rebuilding the C++ project.
"""

from __future__ import annotations

import json
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

ROOT = Path(__file__).resolve().parent
MAPS = ROOT / "maps"


# --- enums -------------------------------------------------------------------
DRIVE_STOP, DRIVE_FORWARD, DRIVE_BACKWARD = "Stop", "Forward", "Backward"
TURN_NONE, TURN_LEFT, TURN_RIGHT = "None", "Left", "Right"
DIR_LEFT, DIR_RIGHT = "Left", "Right"
POW_OFF, POW_NOMINAL, POW_BOOSTED = "Off", "Nominal", "Boosted"
SESSION_STOPPED, SESSION_RUNNING = "Stopped", "Running"
PHASE_DRIVING, PHASE_AVOIDING, PHASE_ESCAPING = "Driving", "Avoiding", "Escaping"
HEAD_N, HEAD_E, HEAD_S, HEAD_W = "N", "E", "S", "W"


def heading_delta(h):
    return {HEAD_N: (0, -1), HEAD_E: (1, 0), HEAD_S: (0, 1), HEAD_W: (-1, 0)}[h]


def rotate_left(h):
    return {HEAD_N: HEAD_W, HEAD_W: HEAD_S, HEAD_S: HEAD_E, HEAD_E: HEAD_N}[h]


def rotate_right(h):
    return {HEAD_N: HEAD_E, HEAD_E: HEAD_S, HEAD_S: HEAD_W, HEAD_W: HEAD_N}[h]


@dataclass
class GridWorld:
    width: int = 0
    height: int = 0
    rx: int = 0
    ry: int = 0
    heading: str = HEAD_N
    walls: list = field(default_factory=list)  # [bool] flat
    dust: list = field(default_factory=list)
    visited: list = field(default_factory=list)  # 0=unvisited 1=visited 2=cleaned
    last_power: str = POW_OFF
    collisions: int = 0
    cleaned_cells: int = 0
    total_open: int = 0

    @classmethod
    def from_scenario_dict(cls, scn: dict):
        s = scn["scenario"]
        w, h = s["width"], s["height"]
        walls = []
        for y in range(h):
            row = s["walls"][y]
            for x in range(w):
                walls.append(row[x] == "#")
        dust = []
        for y in range(h):
            row = s["dust"][y]
            for x in range(w):
                dust.append(row[x] == "*")
        gw = cls(width=w, height=h, rx=s["robot"]["x"], ry=s["robot"]["y"],
                 heading=s["robot"]["heading"], walls=walls, dust=dust,
                 visited=[0] * (w * h))
        gw.visited[gw.ry * w + gw.rx] = 1
        gw.total_open = sum(1 for k in walls if not k)
        return gw

    def wall_at(self, x, y):
        if x < 0 or y < 0 or x >= self.width or y >= self.height:
            return True
        return self.walls[y * self.width + x]

    def sensor_reading(self):
        dx, dy = heading_delta(self.heading)
        front = self.wall_at(self.rx + dx, self.ry + dy)
        ldx, ldy = heading_delta(rotate_left(self.heading))
        left = self.wall_at(self.rx + ldx, self.ry + ldy)
        rdx, rdy = heading_delta(rotate_right(self.heading))
        right = self.wall_at(self.rx + rdx, self.ry + rdy)
        d = bool(self.dust[self.ry * self.width + self.rx])
        return {"front": front, "left": left, "right": right, "dust": d}

    def apply_drive(self, cmd):
        dx, dy = heading_delta(self.heading)
        if cmd == DRIVE_FORWARD:
            nx, ny = self.rx + dx, self.ry + dy
        elif cmd == DRIVE_BACKWARD:
            nx, ny = self.rx - dx, self.ry - dy
        else:
            return False
        if self.wall_at(nx, ny):
            self.collisions += 1
            return True
        self.rx, self.ry = nx, ny
        idx = self.ry * self.width + self.rx
        if self.visited[idx] != 2:
            self.visited[idx] = 1 if self.visited[idx] == 0 else self.visited[idx]
        self.mark_current_cell_cleaned_if_powered()
        return False

    def apply_turn(self, dr):
        self.heading = rotate_left(self.heading) if dr == DIR_LEFT else rotate_right(self.heading)

    def apply_power(self, level):
        self.last_power = level
        if level != POW_OFF:
            self.mark_current_cell_cleaned_if_powered()

    def mark_current_cell_cleaned_if_powered(self):
        if self.last_power == POW_OFF:
            return
        idx = self.ry * self.width + self.rx
        if self.visited[idx] != 2:
            self.visited[idx] = 2
            self.cleaned_cells += 1

    def cleaned_ratio(self):
        return 0.0 if self.total_open == 0 else self.cleaned_cells / self.total_open


@dataclass
class CleaningPowerPolicy:
    dust_boost_ticks: int
    timer: int = 0

    def next_level(self, reading):
        if self.dust_boost_ticks <= 0:
            return POW_NOMINAL
        if reading["dust"]:
            self.timer = self.dust_boost_ticks
            return POW_BOOSTED
        if self.timer > 0:
            self.timer -= 1
            return POW_BOOSTED if self.timer > 0 else POW_NOMINAL
        return POW_NOMINAL

    def reset(self):
        self.timer = 0


def navigation_choose_side(reading):
    left_open = not reading["left"]
    right_open = not reading["right"]
    if left_open and not right_open:
        return TURN_LEFT
    if not left_open and right_open:
        return TURN_RIGHT
    if left_open and right_open:
        return TURN_LEFT  # FR-003 deterministic
    return TURN_NONE


def navigation_next_drive(reading):
    if not reading["front"]:
        return (DRIVE_FORWARD, TURN_NONE)
    if reading["front"] and reading["left"] and reading["right"]:
        return (DRIVE_STOP, TURN_NONE)
    return (DRIVE_STOP, navigation_choose_side(reading))


def navigation_plan_escape(reading):
    # FR-004: helper is invoked only on escape exit (Coordinator's predicate
    # guarantees L or R is open). Always pick a side (left preference, FR-003)
    # so the next tick does not re-enter the trap.
    return navigation_choose_side(reading)


def to_direction(t):
    return DIR_RIGHT if t == TURN_RIGHT else DIR_LEFT


@dataclass
class Coordinator:
    world: GridWorld
    power: CleaningPowerPolicy
    max_backoff_ticks: int = 4
    session_state: str = SESSION_STOPPED
    phase: str = PHASE_DRIVING
    backoff_remaining: int = 0
    tick_count: int = 0

    def start_session(self):
        if self.session_state == SESSION_RUNNING:
            return
        self.power.reset()
        self.session_state = SESSION_RUNNING
        self.phase = PHASE_DRIVING
        self.backoff_remaining = 0
        self.tick_count = 0
        self.world.apply_power(POW_NOMINAL)

    def stop_session(self):
        if self.session_state == SESSION_STOPPED:
            return
        self.world.apply_drive(DRIVE_STOP)
        self.world.apply_power(POW_OFF)
        self.power.reset()
        self.session_state = SESSION_STOPPED
        self.phase = PHASE_DRIVING

    def tick(self):
        if self.session_state != SESSION_RUNNING:
            return
        self.tick_count += 1
        reading = self.world.sensor_reading()
        level = self.power.next_level(reading)
        self.world.apply_power(level)

        if self.phase == PHASE_ESCAPING:
            # FR-004: exit predicate is "L or R open". Front-only-open is NOT
            # an exit because resuming forward would re-enter the same trap.
            side_open = (not reading["left"]) or (not reading["right"])
            if side_open:
                turn = navigation_plan_escape(reading)
                if turn != TURN_NONE:
                    self.world.apply_turn(to_direction(turn))
                self.phase = PHASE_DRIVING
                self.backoff_remaining = 0
                return
            if self.backoff_remaining > 0:
                self.backoff_remaining -= 1
                self.world.apply_drive(DRIVE_BACKWARD)
            else:
                self.world.apply_drive(DRIVE_STOP)
            return

        triple = reading["front"] and reading["left"] and reading["right"]
        if triple:
            self.phase = PHASE_ESCAPING
            self.backoff_remaining = self.max_backoff_ticks - 1 if self.max_backoff_ticks > 0 else 0
            self.world.apply_drive(DRIVE_STOP)
            if self.max_backoff_ticks > 0:
                self.world.apply_drive(DRIVE_BACKWARD)
            return

        drive, turn = navigation_next_drive(reading)
        if turn != TURN_NONE:
            self.phase = PHASE_AVOIDING
            self.world.apply_drive(DRIVE_STOP)
            self.world.apply_turn(to_direction(turn))
            return
        self.phase = PHASE_DRIVING
        self.world.apply_drive(drive)


def evaluate(expect, w: GridWorld, c: Coordinator, total_ticks: int):
    fails = []

    def f(m):
        fails.append(m)

    if expect.get("no_collisions") and w.collisions != 0:
        f(f"expected no_collisions, got {w.collisions}")
    if "max_collisions" in expect and w.collisions > expect["max_collisions"]:
        f(f"collisions {w.collisions} > {expect['max_collisions']}")
    if "min_cleaned_cells" in expect and w.cleaned_cells < expect["min_cleaned_cells"]:
        f(f"cleaned_cells {w.cleaned_cells} < {expect['min_cleaned_cells']}")
    if "max_cleaned_cells" in expect and w.cleaned_cells > expect["max_cleaned_cells"]:
        f(f"cleaned_cells {w.cleaned_cells} > {expect['max_cleaned_cells']}")
    if "min_cleaned_ratio" in expect and w.cleaned_ratio() < expect["min_cleaned_ratio"]:
        f(f"cleaned_ratio {w.cleaned_ratio():.3f} < {expect['min_cleaned_ratio']}")
    if "session_running" in expect:
        want = SESSION_RUNNING if expect["session_running"] else SESSION_STOPPED
        if c.session_state != want:
            f(f"session_state {c.session_state} != {want}")
    if "last_power" in expect and w.last_power != expect["last_power"]:
        f(f"last_power {w.last_power} != {expect['last_power']}")
    if "final_x" in expect and w.rx != expect["final_x"]:
        f(f"final_x {w.rx} != {expect['final_x']}")
    if "final_y" in expect and w.ry != expect["final_y"]:
        f(f"final_y {w.ry} != {expect['final_y']}")
    if "final_heading" in expect and w.heading != expect["final_heading"]:
        f(f"final_heading {w.heading} != {expect['final_heading']}")
    if "min_total_ticks" in expect and total_ticks < expect["min_total_ticks"]:
        f(f"total_ticks {total_ticks} < {expect['min_total_ticks']}")
    if "max_total_ticks" in expect and total_ticks > expect["max_total_ticks"]:
        f(f"total_ticks {total_ticks} > {expect['max_total_ticks']}")
    return fails


def run_one(scn):
    s = scn["scenario"]
    cfg = s.get("config", {})
    boost = cfg.get("dust_boost_ticks", 5)
    backoff = cfg.get("max_backoff_ticks", 4)
    max_ticks = s.get("max_ticks", 30)
    w = GridWorld.from_scenario_dict(scn)
    p = CleaningPowerPolicy(dust_boost_ticks=boost)
    c = Coordinator(world=w, power=p, max_backoff_ticks=backoff)
    c.start_session()
    for _ in range(max_ticks):
        c.tick()
    fails = evaluate(scn.get("expect", {}), w, c, c.tick_count)
    return fails, w, c


def main():
    paths = sorted(MAPS.glob("ST-*.json"))
    n_pass = n_fail = 0
    for p in paths:
        scn = json.loads(p.read_text(encoding="utf-8"))
        fails, w, c = run_one(scn)
        if not fails:
            print(f"PASS {scn['id']} ({scn.get('type','?')})  cleaned={w.cleaned_cells} "
                  f"coll={w.collisions} pose=({w.rx},{w.ry},{w.heading}) "
                  f"power={w.last_power} ticks={c.tick_count}")
            n_pass += 1
        else:
            print(f"FAIL {scn['id']} ({scn.get('type','?')})  cleaned={w.cleaned_cells} "
                  f"coll={w.collisions} pose=({w.rx},{w.ry},{w.heading}) "
                  f"power={w.last_power} ticks={c.tick_count}")
            for fl in fails:
                print(f"   - {fl}")
            n_fail += 1
    print()
    print(f"summary: {n_pass} pass, {n_fail} fail (total {len(paths)})")
    return 0 if n_fail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
