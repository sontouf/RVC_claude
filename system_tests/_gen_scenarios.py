#!/usr/bin/env python3
"""Generate the canonical RVC system test scenarios into ``maps/``.

Run once and commit the resulting ``ST-*.json`` files. Re-run if scenarios
need to change. Hand-edits to the JSON files are fine; this generator is a
convenience to keep the catalog reproducible.

Trace: arch/vnv/traceability-matrix.md §5; arch/vnv/system-tests.md.
"""

from __future__ import annotations

import json
from pathlib import Path

OUT = Path(__file__).resolve().parent / "maps"
OUT.mkdir(exist_ok=True)


def std_walls(w: int, h: int):
    rows = []
    for y in range(h):
        if y == 0 or y == h - 1:
            rows.append("#" * w)
        else:
            rows.append("#" + "." * (w - 2) + "#")
    return rows


def empty_dust(w: int, h: int):
    return ["." * w for _ in range(h)]


def with_dust_at(dust_grid, points):
    rows = [list(r) for r in dust_grid]
    for x, y in points:
        rows[y][x] = "*"
    return ["".join(r) for r in rows]


def make(scn_id, title, scn_type, trace, scenario, expect):
    return {
        "id": scn_id,
        "title": title,
        "type": scn_type,
        "trace": trace,
        "scenario": scenario,
        "expect": expect,
    }


def write(obj):
    p = OUT / f"{obj['id']}.json"
    p.write_text(json.dumps(obj, indent=2) + "\n", encoding="utf-8")


def base_scn(w, h, rx, ry, heading, *, dust_boost_ticks=5, max_backoff_ticks=4,
             max_ticks=20, walls=None, dust_points=()):
    if walls is None:
        walls = std_walls(w, h)
    dust = with_dust_at(empty_dust(w, h), list(dust_points))
    return {
        "width": w,
        "height": h,
        "robot": {"x": rx, "y": ry, "heading": heading},
        "config": {
            "dust_boost_ticks": dust_boost_ticks,
            "max_backoff_ticks": max_backoff_ticks,
        },
        "max_ticks": max_ticks,
        "walls": walls,
        "dust": dust,
    }


# =============================================================================
# Positive scenarios (22)
# =============================================================================

write(make("ST-001", "Forward cleaning in open 5x5",
           "positive", ["FR-001", "UC-002"],
           base_scn(5, 5, 1, 1, "N", max_ticks=20),
           {"no_collisions": True, "min_cleaned_cells": 2,
            "session_running": True}))

write(make("ST-002", "Forward cleaning starts from center of 5x5 facing E",
           "positive", ["FR-001", "UC-002"],
           base_scn(5, 5, 2, 2, "E", max_ticks=30),
           {"no_collisions": True, "min_cleaned_cells": 4,
            "session_running": True}))

write(make("ST-003", "Forward cleaning in larger 7x7 open grid",
           "positive", ["FR-001", "UC-002", "NFR-PERF-001"],
           base_scn(7, 7, 3, 3, "N", max_ticks=80),
           {"no_collisions": True, "min_cleaned_ratio": 0.40,
            "session_running": True}))

write(make("ST-004", "FR-003 deterministic: both sides open prefers Left",
           "positive", ["FR-003", "UC-003", "NFR-DET-001"],
           base_scn(5, 5, 1, 2, "W", max_ticks=2),
           {"no_collisions": True, "final_x": 1, "final_y": 3,
            "final_heading": "S", "session_running": True}))

write(make("ST-005", "Front+Left blocked turns Right",
           "positive", ["FR-003", "UC-003"],
           base_scn(5, 5, 1, 1, "N", max_ticks=2),
           {"no_collisions": True, "final_x": 2, "final_y": 1,
            "final_heading": "E", "session_running": True}))

write(make("ST-006", "Front+Right blocked turns Left",
           "positive", ["FR-003", "UC-003"],
           base_scn(5, 5, 3, 1, "N", max_ticks=2),
           {"no_collisions": True, "final_x": 2, "final_y": 1,
            "final_heading": "W", "session_running": True}))

# Vertical dead-end corridor (3x6) closed at top.
deadend_walls = ["###", "#.#", "#.#", "#.#", "#.#", "###"]
write(make("ST-007", "Triple-blocked dead-end safely backs off (oscillation)",
           "positive", ["FR-004", "UC-004", "NFR-SAFE-001"],
           {
               "width": 3, "height": 6, "robot": {"x": 1, "y": 1, "heading": "N"},
               "config": {"dust_boost_ticks": 5, "max_backoff_ticks": 4},
               "max_ticks": 30, "walls": deadend_walls,
               "dust": empty_dust(3, 6),
           },
           {"no_collisions": True, "session_running": True,
            "max_total_ticks": 30}))

# Corridor with side opening: at row 1 the column 2 is wall → escape via right.
write(make("ST-008", "Triple-blocked then side opens after backoff",
           "positive", ["FR-004", "UC-004"],
           {
               "width": 5, "height": 5,
               "robot": {"x": 1, "y": 1, "heading": "N"},
               "config": {"dust_boost_ticks": 5, "max_backoff_ticks": 4},
               "max_ticks": 30,
               "walls": ["#####", "#.#.#", "#...#", "#...#", "#####"],
               "dust": empty_dust(5, 5),
           },
           {"no_collisions": True, "min_cleaned_cells": 2,
            "session_running": True}))

write(make("ST-009", "Dust cell triggers boost mid-traverse",
           "positive", ["FR-005", "UC-005"],
           base_scn(5, 5, 1, 1, "E", dust_boost_ticks=3, max_ticks=2,
                    dust_points=[(2, 1)]),
           {"no_collisions": True, "last_power": "Boosted",
            "final_x": 3, "final_y": 1, "final_heading": "E",
            "session_running": True}))

write(make("ST-010", "Dust boost auto-returns to Nominal after timer",
           "positive", ["FR-005", "UC-005"],
           base_scn(5, 5, 1, 1, "E", dust_boost_ticks=2, max_ticks=4,
                    dust_points=[(2, 1)]),
           {"no_collisions": True, "last_power": "Nominal",
            "session_running": True}))

write(make("ST-011", "Long boost window keeps power Boosted at end",
           "positive", ["FR-005", "UC-005"],
           base_scn(5, 5, 1, 1, "E", dust_boost_ticks=10, max_ticks=4,
                    dust_points=[(2, 1)]),
           {"no_collisions": True, "last_power": "Boosted",
            "session_running": True}))

write(make("ST-012", "No dust ever - power stays Nominal",
           "positive", ["FR-001", "UC-002", "FR-005", "UC-005"],
           base_scn(5, 5, 1, 1, "N", dust_boost_ticks=5, max_ticks=20),
           {"no_collisions": True, "last_power": "Nominal",
            "session_running": True}))

# Internal-block 5x5: pillar at (2,2)
write(make("ST-013", "Navigates around a single internal obstacle",
           "positive", ["FR-001", "FR-003", "UC-002", "UC-003"],
           {
               "width": 5, "height": 5,
               "robot": {"x": 1, "y": 1, "heading": "N"},
               "config": {"dust_boost_ticks": 5, "max_backoff_ticks": 4},
               "max_ticks": 60,
               "walls": ["#####", "#...#", "#.#.#", "#...#", "#####"],
               "dust": empty_dust(5, 5),
           },
           {"no_collisions": True, "min_cleaned_cells": 3,
            "session_running": True}))

# 7x5 corridor with an inner block.
write(make("ST-014", "Navigates a corridor with a center block",
           "positive", ["FR-001", "FR-003", "UC-002", "UC-003"],
           {
               "width": 7, "height": 5,
               "robot": {"x": 1, "y": 2, "heading": "E"},
               "config": {"dust_boost_ticks": 5, "max_backoff_ticks": 4},
               "max_ticks": 80,
               "walls": ["#######", "#.....#", "#.###.#", "#.....#", "#######"],
               "dust": empty_dust(7, 5),
           },
           {"no_collisions": True, "min_cleaned_cells": 4,
            "session_running": True}))

# Long 1xN corridor — robot bumps top, oscillates safely
write(make("ST-015", "Long 1-wide corridor with closed top oscillates safely",
           "positive", ["FR-004", "UC-004"],
           {
               "width": 3, "height": 8,
               "robot": {"x": 1, "y": 6, "heading": "N"},
               "config": {"dust_boost_ticks": 5, "max_backoff_ticks": 4},
               "max_ticks": 40,
               "walls": ["###", "#.#", "#.#", "#.#", "#.#", "#.#", "#.#", "###"],
               "dust": empty_dust(3, 8),
           },
           {"no_collisions": True, "session_running": True}))

# U-shape
write(make("ST-016", "U-shaped layout still cleans without collisions",
           "positive", ["FR-001", "FR-003", "UC-002", "UC-003"],
           {
               "width": 5, "height": 5,
               "robot": {"x": 1, "y": 3, "heading": "E"},
               "config": {"dust_boost_ticks": 5, "max_backoff_ticks": 4},
               "max_ticks": 60,
               "walls": ["#####", "#.#.#", "#.#.#", "#...#", "#####"],
               "dust": empty_dust(5, 5),
           },
           {"no_collisions": True, "min_cleaned_cells": 3,
            "session_running": True}))

# Small room with single inner wall column
write(make("ST-017", "Small room with inner wall column",
           "positive", ["FR-001", "UC-002"],
           {
               "width": 5, "height": 5,
               "robot": {"x": 3, "y": 2, "heading": "W"},
               "config": {"dust_boost_ticks": 5, "max_backoff_ticks": 4},
               "max_ticks": 40,
               "walls": ["#####", "#...#", "#.#.#", "#...#", "#####"],
               "dust": empty_dust(5, 5),
           },
           {"no_collisions": True, "min_cleaned_cells": 2,
            "session_running": True}))

# Determinism positive — open grid, plenty of ticks
write(make("ST-018", "Determinism over 100 ticks in open 5x5",
           "positive", ["FR-001", "UC-002", "NFR-DET-001"],
           base_scn(5, 5, 2, 2, "E", max_ticks=100),
           {"no_collisions": True, "session_running": True}))

write(make("ST-019", "Heading East baseline cleaning",
           "positive", ["FR-001", "UC-002"],
           base_scn(5, 5, 1, 1, "E", max_ticks=30),
           {"no_collisions": True, "min_cleaned_cells": 3,
            "session_running": True}))

write(make("ST-020", "Heading South baseline cleaning",
           "positive", ["FR-001", "UC-002"],
           base_scn(5, 5, 3, 1, "S", max_ticks=30),
           {"no_collisions": True, "min_cleaned_cells": 3,
            "session_running": True}))

write(make("ST-021", "Heading West baseline cleaning",
           "positive", ["FR-001", "UC-002"],
           base_scn(5, 5, 3, 3, "W", max_ticks=30),
           {"no_collisions": True, "min_cleaned_cells": 3,
            "session_running": True}))

# Larger 9x9 stress
write(make("ST-022", "Long-run cleaning in 9x9 open grid",
           "positive", ["FR-001", "UC-002", "NFR-PERF-001", "NFR-DET-001"],
           base_scn(9, 9, 4, 4, "N", max_ticks=200),
           {"no_collisions": True, "min_cleaned_ratio": 0.30,
            "session_running": True}))


# =============================================================================
# Negative / edge scenarios (>=8)
# =============================================================================

# Fully enclosed single cell: 3x3 with center open.
write(make("ST-023", "Fully enclosed 1x1 cell - bounded collisions, then safe stop",
           "negative", ["FR-004", "UC-004", "NFR-SAFE-001"],
           {
               "width": 3, "height": 3,
               "robot": {"x": 1, "y": 1, "heading": "N"},
               "config": {"dust_boost_ticks": 5, "max_backoff_ticks": 2},
               "max_ticks": 10,
               "walls": ["###", "#.#", "###"],
               "dust": empty_dust(3, 3),
           },
           {"max_collisions": 2, "session_running": True}))

write(make("ST-024", "max_backoff_ticks=0 - no backward attempts means no collisions",
           "negative", ["FR-004", "UC-004", "NFR-SAFE-001"],
           {
               "width": 3, "height": 3,
               "robot": {"x": 1, "y": 1, "heading": "N"},
               "config": {"dust_boost_ticks": 5, "max_backoff_ticks": 0},
               "max_ticks": 10,
               "walls": ["###", "#.#", "###"],
               "dust": empty_dust(3, 3),
           },
           {"no_collisions": True, "session_running": True}))

write(make("ST-025", "dust_boost_ticks=0 disables boost even with dust present",
           "negative", ["FR-005", "UC-005"],
           base_scn(5, 5, 1, 1, "E", dust_boost_ticks=0, max_ticks=4,
                    dust_points=[(2, 1)]),
           {"no_collisions": True, "last_power": "Nominal",
            "session_running": True}))

write(make("ST-026", "Robot starts on dust cell - boost activates on tick 1",
           "negative", ["FR-005", "UC-005"],
           base_scn(5, 5, 1, 1, "E", dust_boost_ticks=4, max_ticks=1,
                    dust_points=[(1, 1)]),
           {"no_collisions": True, "last_power": "Boosted",
            "session_running": True}))

# Dust on every reachable cell with long boost ticks → power stays Boosted
heavy_dust_points = [(x, y) for x in range(1, 4) for y in range(1, 4)]
write(make("ST-027", "Dust on every reachable cell keeps power Boosted",
           "negative", ["FR-005", "UC-005"],
           base_scn(5, 5, 1, 1, "E", dust_boost_ticks=10, max_ticks=20,
                    dust_points=heavy_dust_points),
           {"no_collisions": True, "last_power": "Boosted",
            "session_running": True}))

# 1-tick session — verifies controller starts and emits one decision
write(make("ST-028", "Single-tick session emits one decision and remains running",
           "negative", ["FR-001", "FR-002", "UC-001", "UC-002"],
           base_scn(5, 5, 2, 2, "N", max_ticks=1),
           {"no_collisions": True, "session_running": True,
            "max_total_ticks": 1, "min_total_ticks": 1}))

# Zero-tick session — start with no ticks
write(make("ST-029", "Zero-tick session keeps robot stationary",
           "negative", ["FR-002", "UC-001", "NFR-SAFE-001"],
           base_scn(5, 5, 2, 2, "N", max_ticks=0),
           {"no_collisions": True, "session_running": True,
            "final_x": 2, "final_y": 2, "final_heading": "N"}))

# Long oscillation in dead-end — determinism stress, no collisions
write(make("ST-030", "Long oscillation in vertical dead-end remains safe",
           "negative", ["FR-004", "UC-004", "NFR-DET-001", "NFR-PERF-001"],
           {
               "width": 3, "height": 5,
               "robot": {"x": 1, "y": 1, "heading": "N"},
               "config": {"dust_boost_ticks": 5, "max_backoff_ticks": 4},
               "max_ticks": 500,
               "walls": ["###", "#.#", "#.#", "#.#", "###"],
               "dust": empty_dust(3, 5),
           },
           {"no_collisions": True, "session_running": True,
            "min_total_ticks": 500, "max_total_ticks": 500}))

# Maze with multiple triple-block traps
write(make("ST-031", "Maze with multiple triple-block dead-ends - safe and deterministic",
           "negative", ["FR-003", "FR-004", "UC-003", "UC-004", "NFR-SAFE-001"],
           {
               "width": 7, "height": 7,
               "robot": {"x": 1, "y": 5, "heading": "N"},
               "config": {"dust_boost_ticks": 5, "max_backoff_ticks": 4},
               "max_ticks": 200,
               "walls": [
                   "#######",
                   "#.....#",
                   "#.###.#",
                   "#...#.#",
                   "###.#.#",
                   "#.....#",
                   "#######",
               ],
               "dust": empty_dust(7, 7),
           },
           {"no_collisions": True, "session_running": True}))


print(f"Wrote {len(list(OUT.glob('ST-*.json')))} scenarios into {OUT}")
