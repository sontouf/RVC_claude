#!/usr/bin/env python3
"""RVC system test runner.

Discovers all JSON scenario files in ``system_tests/maps/``, converts each to
the simple text format consumed by ``rvc_sim``, runs the simulator, parses the
machine-readable result block from stdout, and evaluates the scenario's
``expect`` block.

Trace: arch/vnv/traceability-matrix.md §5 (system tests are non-GTest).
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent
MAPS_DIR = ROOT / "maps"


def scenario_to_sim_text(scn: dict) -> str:
    s = scn["scenario"]
    lines = []
    lines.append(f"width {s['width']}")
    lines.append(f"height {s['height']}")
    r = s["robot"]
    lines.append(f"robot {r['x']} {r['y']} {r['heading']}")
    cfg = s.get("config", {})
    lines.append(f"config dust_boost_ticks {cfg.get('dust_boost_ticks', 5)}")
    lines.append(f"config max_backoff_ticks {cfg.get('max_backoff_ticks', 4)}")
    lines.append(f"max_ticks {s.get('max_ticks', 30)}")
    lines.append("walls")
    for row in s["walls"]:
        lines.append(row)
    lines.append("dust")
    for row in s["dust"]:
        lines.append(row)
    lines.append("end")
    return "\n".join(lines) + "\n"


def parse_sim_output(stdout: str) -> dict:
    res = {}
    in_block = False
    for line in stdout.splitlines():
        line = line.strip()
        if line == "=== rvc_sim result ===":
            in_block = True
            continue
        if line == "=== rvc_sim end ===":
            in_block = False
            continue
        if not in_block or not line:
            continue
        parts = line.split(maxsplit=1)
        if len(parts) != 2:
            continue
        key, val = parts
        res[key] = val
    return res


def evaluate_expect(expect: dict, result: dict) -> tuple[bool, list[str]]:
    failures: list[str] = []

    def fail(msg: str) -> None:
        failures.append(msg)

    if "no_collisions" in expect and bool(expect["no_collisions"]):
        if int(result.get("collisions", "0")) != 0:
            fail(f"expected no_collisions, got {result.get('collisions')}")
    if "max_collisions" in expect:
        if int(result.get("collisions", "0")) > int(expect["max_collisions"]):
            fail(
                f"collisions {result.get('collisions')} exceeds "
                f"max_collisions {expect['max_collisions']}"
            )
    if "min_cleaned_cells" in expect:
        if int(result.get("cleaned_cells", "0")) < int(expect["min_cleaned_cells"]):
            fail(
                f"cleaned_cells {result.get('cleaned_cells')} < "
                f"min_cleaned_cells {expect['min_cleaned_cells']}"
            )
    if "max_cleaned_cells" in expect:
        if int(result.get("cleaned_cells", "0")) > int(expect["max_cleaned_cells"]):
            fail(
                f"cleaned_cells {result.get('cleaned_cells')} > "
                f"max_cleaned_cells {expect['max_cleaned_cells']}"
            )
    if "min_cleaned_ratio" in expect:
        if float(result.get("cleaned_ratio", "0")) < float(expect["min_cleaned_ratio"]):
            fail(
                f"cleaned_ratio {result.get('cleaned_ratio')} < "
                f"{expect['min_cleaned_ratio']}"
            )
    if "session_running" in expect:
        want = "Running" if expect["session_running"] else "Stopped"
        if result.get("session_state") != want:
            fail(f"expected session_state={want}, got {result.get('session_state')}")
    if "last_power" in expect:
        if result.get("last_power") != expect["last_power"]:
            fail(
                f"expected last_power={expect['last_power']}, got "
                f"{result.get('last_power')}"
            )
    if "final_x" in expect and int(result.get("final_x", "-1")) != int(expect["final_x"]):
        fail(f"final_x {result.get('final_x')} != {expect['final_x']}")
    if "final_y" in expect and int(result.get("final_y", "-1")) != int(expect["final_y"]):
        fail(f"final_y {result.get('final_y')} != {expect['final_y']}")
    if "final_heading" in expect and result.get("final_heading") != expect["final_heading"]:
        fail(f"final_heading {result.get('final_heading')} != {expect['final_heading']}")
    if "min_total_ticks" in expect:
        if int(result.get("total_ticks", "0")) < int(expect["min_total_ticks"]):
            fail(
                f"total_ticks {result.get('total_ticks')} < "
                f"{expect['min_total_ticks']}"
            )
    if "max_total_ticks" in expect:
        if int(result.get("total_ticks", "0")) > int(expect["max_total_ticks"]):
            fail(
                f"total_ticks {result.get('total_ticks')} > "
                f"{expect['max_total_ticks']}"
            )

    return (len(failures) == 0, failures)


def _strip_scenario_path(stdout: str) -> str:
    # The simulator prints `scenario <path>` once. The path varies across
    # tempfile invocations even for identical scenarios, so we strip that line
    # before comparing two runs for determinism (NFR-DET-001).
    return "\n".join(
        line for line in stdout.splitlines() if not line.startswith("scenario ")
    )


def determinism_check(sim: Path, scenario_text: str) -> tuple[bool, str]:
    # Use the same scenario file for both runs to keep all path-dependent
    # output identical.
    with tempfile.NamedTemporaryFile(
        "w", suffix=".scn.txt", delete=False, encoding="utf-8"
    ) as tf:
        tf.write(scenario_text)
        tmp_path = tf.name
    try:
        cp1 = subprocess.run(
            [str(sim), "--scenario", tmp_path],
            capture_output=True, text=True, timeout=60,
        )
        cp2 = subprocess.run(
            [str(sim), "--scenario", tmp_path],
            capture_output=True, text=True, timeout=60,
        )
    finally:
        try:
            os.unlink(tmp_path)
        except OSError:
            pass
    if _strip_scenario_path(cp1.stdout) == _strip_scenario_path(cp2.stdout):
        return True, ""
    return False, "two runs produced different stdout (NFR-DET-001 violation)"


def run_sim(sim: Path, scenario_text: str) -> subprocess.CompletedProcess:
    with tempfile.NamedTemporaryFile(
        "w", suffix=".scn.txt", delete=False, encoding="utf-8"
    ) as tf:
        tf.write(scenario_text)
        tmp_path = tf.name
    try:
        return subprocess.run(
            [str(sim), "--scenario", tmp_path],
            capture_output=True,
            text=True,
            timeout=60,
        )
    finally:
        try:
            os.unlink(tmp_path)
        except OSError:
            pass


def main() -> int:
    ap = argparse.ArgumentParser(description="RVC system tests runner")
    ap.add_argument("--sim", required=True, help="path to rvc_sim binary")
    ap.add_argument("--maps", default=str(MAPS_DIR), help="directory of *.json scenarios")
    ap.add_argument("--filter", default=None, help="substring filter on scenario id")
    ap.add_argument("--check-determinism", action="store_true",
                    help="also re-run each scenario to check determinism")
    args = ap.parse_args()

    sim = Path(args.sim).resolve()
    if not sim.exists():
        print(f"ERROR: sim binary not found: {sim}", file=sys.stderr)
        return 2

    maps_dir = Path(args.maps)
    if not maps_dir.exists():
        print(f"ERROR: maps dir not found: {maps_dir}", file=sys.stderr)
        return 2

    scenarios = sorted(maps_dir.glob("*.json"))
    if args.filter:
        scenarios = [p for p in scenarios if args.filter in p.stem]
    if not scenarios:
        print("ERROR: no scenarios found", file=sys.stderr)
        return 2

    pos_total = neg_total = 0
    pos_pass = neg_pass = 0
    fails: list[str] = []

    for path in scenarios:
        with open(path, "r", encoding="utf-8") as f:
            scn = json.load(f)
        scn_id = scn.get("id", path.stem)
        scn_type = scn.get("type", "positive")
        if scn_type == "positive":
            pos_total += 1
        else:
            neg_total += 1
        scn_text = scenario_to_sim_text(scn)
        cp = run_sim(sim, scn_text)
        if cp.returncode != 0:
            fails.append(f"{scn_id}: sim exited {cp.returncode}; stderr={cp.stderr.strip()}")
            print(f"FAIL {scn_id}: sim exited {cp.returncode}")
            continue
        result = parse_sim_output(cp.stdout)
        ok, errs = evaluate_expect(scn.get("expect", {}), result)
        if ok and args.check_determinism:
            det_ok, det_msg = determinism_check(sim, scn_text)
            if not det_ok:
                ok = False
                errs.append(det_msg)
        if ok:
            if scn_type == "positive":
                pos_pass += 1
            else:
                neg_pass += 1
            print(f"PASS {scn_id} ({scn_type})")
        else:
            fails.append(f"{scn_id}: {'; '.join(errs)}")
            print(f"FAIL {scn_id}: {'; '.join(errs)}")

    total = pos_total + neg_total
    passed = pos_pass + neg_pass
    print()
    print(f"--- summary ---")
    print(f"positive: {pos_pass}/{pos_total}")
    print(f"negative: {neg_pass}/{neg_total}")
    print(f"total   : {passed}/{total}")
    if total < 30:
        print(f"WARNING: only {total} scenarios discovered; NFR-SYS-001 requires >=30",
              file=sys.stderr)
    if fails:
        print()
        print("--- failures ---")
        for f in fails:
            print(f)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
