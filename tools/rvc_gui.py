#!/usr/bin/env python3
"""Tkinter GUI: design RVC scenarios visually and run them step-by-step.

Reuses the controller logic from `system_tests/_py_emulator.py` so the
behavior is identical to the Python emulator (which mirrors the C++
`rvc_sim` 1:1). This GUI is a *debugging / authoring* aid; the
authoritative system tests still run the compiled C++ binary in CI.

Run from the repo root:

    python tools/rvc_gui.py
    python tools/rvc_gui.py system_tests/maps/ST-014.json
"""

from __future__ import annotations

import json
import sys
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, ttk

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "system_tests"))

import _py_emulator as emu  # noqa: E402  (path injected above)


HEADINGS = ("N", "E", "S", "W")
DEFAULT_W, DEFAULT_H = 7, 7
MIN_DIM, MAX_DIM = 3, 30
CELL_PX = 36

TOOL_WALL = "Wall"
TOOL_FLOOR = "Floor"
TOOL_DUST = "Dust"
TOOL_ROBOT = "Robot"

COLOR_BG = "#1e1e2e"
COLOR_GRID = "#3a3a4a"
COLOR_FLOOR = "#2a2a3a"
COLOR_WALL = "#6c6c7a"
COLOR_DUST = "#e6c84d"
COLOR_VISITED = "#3b5a78"
COLOR_CLEANED = "#3aa66e"
COLOR_ROBOT = "#ff6b6b"
COLOR_ROBOT_OUTLINE = "#ffffff"


# -----------------------------------------------------------------------------
# Editable scenario model (separate from runtime state so Reset returns here).
# -----------------------------------------------------------------------------
class Scenario:
    def __init__(self, width=DEFAULT_W, height=DEFAULT_H):
        self.width = width
        self.height = height
        self.walls = [[False] * width for _ in range(height)]
        self.dust = [[False] * width for _ in range(height)]
        for x in range(width):
            self.walls[0][x] = True
            self.walls[height - 1][x] = True
        for y in range(height):
            self.walls[y][0] = True
            self.walls[y][width - 1] = True
        self.robot_x = 1
        self.robot_y = 1
        self.robot_heading = "N"
        self.dust_boost_ticks = 5
        self.max_backoff_ticks = 4
        self.max_ticks = 60

    def resize(self, width, height):
        old_w, old_h = self.width, self.height
        old_walls, old_dust = self.walls, self.dust
        self.width = width
        self.height = height
        self.walls = [[False] * width for _ in range(height)]
        self.dust = [[False] * width for _ in range(height)]
        for y in range(height):
            for x in range(width):
                if x == 0 or y == 0 or x == width - 1 or y == height - 1:
                    self.walls[y][x] = True
                elif y < old_h and x < old_w:
                    self.walls[y][x] = old_walls[y][x]
                    self.dust[y][x] = old_dust[y][x]
        self.robot_x = min(self.robot_x, width - 2)
        self.robot_y = min(self.robot_y, height - 2)
        self.robot_x = max(self.robot_x, 1)
        self.robot_y = max(self.robot_y, 1)
        if self.walls[self.robot_y][self.robot_x]:
            self.robot_x, self.robot_y = 1, 1
            self.walls[1][1] = False

    def to_dict(self, sid="ST-CUSTOM", title="Custom scenario", typ="positive"):
        return {
            "id": sid,
            "title": title,
            "type": typ,
            "trace": [],
            "scenario": {
                "width": self.width,
                "height": self.height,
                "robot": {"x": self.robot_x, "y": self.robot_y, "heading": self.robot_heading},
                "config": {
                    "dust_boost_ticks": self.dust_boost_ticks,
                    "max_backoff_ticks": self.max_backoff_ticks,
                },
                "max_ticks": self.max_ticks,
                "walls": ["".join("#" if c else "." for c in row) for row in self.walls],
                "dust": ["".join("*" if c else "." for c in row) for row in self.dust],
            },
            "expect": {},
        }

    @classmethod
    def from_dict(cls, scn):
        s = scn["scenario"]
        obj = cls(s["width"], s["height"])
        obj.walls = [[c == "#" for c in row] for row in s["walls"]]
        obj.dust = [[c == "*" for c in row] for row in s["dust"]]
        obj.robot_x = s["robot"]["x"]
        obj.robot_y = s["robot"]["y"]
        obj.robot_heading = s["robot"]["heading"]
        cfg = s.get("config", {})
        obj.dust_boost_ticks = cfg.get("dust_boost_ticks", 5)
        obj.max_backoff_ticks = cfg.get("max_backoff_ticks", 4)
        obj.max_ticks = s.get("max_ticks", 60)
        return obj


# -----------------------------------------------------------------------------
# GUI
# -----------------------------------------------------------------------------
class RvcGui:
    def __init__(self, root: tk.Tk, initial_path: Path | None = None):
        self.root = root
        self.root.title("RVC Simulator (Tkinter) - design & run")
        self.root.configure(bg=COLOR_BG)

        self.scenario = Scenario()
        self.world: emu.GridWorld | None = None
        self.coord: emu.Coordinator | None = None
        self.tool = tk.StringVar(value=TOOL_WALL)
        self.heading_var = tk.StringVar(value="N")
        self.running = False
        self.tick_ms = tk.IntVar(value=200)

        self._build_layout()
        if initial_path and initial_path.exists():
            self._load_path(initial_path)
        self._reset_runtime()
        self._redraw()

    # ---- layout -----------------------------------------------------------
    def _build_layout(self):
        outer = tk.Frame(self.root, bg=COLOR_BG)
        outer.pack(fill="both", expand=True)

        self.canvas = tk.Canvas(outer, bg=COLOR_BG, highlightthickness=0)
        self.canvas.pack(side="left", fill="both", expand=True, padx=8, pady=8)
        self.canvas.bind("<Button-1>", self._on_click)
        self.canvas.bind("<B1-Motion>", self._on_drag)
        self.canvas.bind("<Button-3>", self._on_right_click)
        self.canvas.bind("<Configure>", lambda _e: self._redraw())

        side = tk.Frame(outer, bg=COLOR_BG, padx=8, pady=8)
        side.pack(side="right", fill="y")

        self._build_tools(side)
        self._build_grid_settings(side)
        self._build_robot_settings(side)
        self._build_config(side)
        self._build_run_controls(side)
        self._build_status(side)
        self._build_io(side)

    def _section(self, parent, label):
        frame = tk.LabelFrame(parent, text=label, fg="white", bg=COLOR_BG,
                              padx=6, pady=6, bd=1, relief="groove")
        frame.pack(fill="x", pady=4)
        return frame

    def _build_tools(self, parent):
        frame = self._section(parent, "Paint tool (left-click=paint, right-click=erase)")
        for tool in (TOOL_WALL, TOOL_FLOOR, TOOL_DUST, TOOL_ROBOT):
            tk.Radiobutton(frame, text=tool, variable=self.tool, value=tool,
                           bg=COLOR_BG, fg="white", selectcolor=COLOR_GRID,
                           activebackground=COLOR_BG, activeforeground="white").pack(anchor="w")

    def _build_grid_settings(self, parent):
        frame = self._section(parent, "Grid size")
        self.width_var = tk.IntVar(value=self.scenario.width)
        self.height_var = tk.IntVar(value=self.scenario.height)
        row = tk.Frame(frame, bg=COLOR_BG); row.pack(fill="x")
        tk.Label(row, text="W", bg=COLOR_BG, fg="white").pack(side="left")
        tk.Spinbox(row, from_=MIN_DIM, to=MAX_DIM, width=4, textvariable=self.width_var).pack(side="left", padx=4)
        tk.Label(row, text="H", bg=COLOR_BG, fg="white").pack(side="left")
        tk.Spinbox(row, from_=MIN_DIM, to=MAX_DIM, width=4, textvariable=self.height_var).pack(side="left", padx=4)
        tk.Button(frame, text="Apply size", command=self._apply_resize).pack(fill="x", pady=2)
        tk.Button(frame, text="Clear interior", command=self._clear_interior).pack(fill="x", pady=2)

    def _build_robot_settings(self, parent):
        frame = self._section(parent, "Robot heading (used when placing)")
        for h in HEADINGS:
            tk.Radiobutton(frame, text=h, variable=self.heading_var, value=h,
                           bg=COLOR_BG, fg="white", selectcolor=COLOR_GRID,
                           activebackground=COLOR_BG, activeforeground="white").pack(side="left")

    def _build_config(self, parent):
        frame = self._section(parent, "Controller config")
        self.boost_var = tk.IntVar(value=self.scenario.dust_boost_ticks)
        self.backoff_var = tk.IntVar(value=self.scenario.max_backoff_ticks)
        self.maxticks_var = tk.IntVar(value=self.scenario.max_ticks)
        for label, var in (("dust_boost_ticks", self.boost_var),
                           ("max_backoff_ticks", self.backoff_var),
                           ("max_ticks", self.maxticks_var)):
            row = tk.Frame(frame, bg=COLOR_BG); row.pack(fill="x", pady=1)
            tk.Label(row, text=label, bg=COLOR_BG, fg="white", width=18, anchor="w").pack(side="left")
            tk.Spinbox(row, from_=0, to=10000, width=8, textvariable=var).pack(side="left")

    def _build_run_controls(self, parent):
        frame = self._section(parent, "Simulation")
        row = tk.Frame(frame, bg=COLOR_BG); row.pack(fill="x")
        tk.Button(row, text="Start", command=self._start).pack(side="left", expand=True, fill="x", padx=2)
        tk.Button(row, text="Pause", command=self._pause).pack(side="left", expand=True, fill="x", padx=2)
        tk.Button(row, text="Step", command=self._step_once).pack(side="left", expand=True, fill="x", padx=2)
        tk.Button(row, text="Reset", command=self._reset_clicked).pack(side="left", expand=True, fill="x", padx=2)
        row2 = tk.Frame(frame, bg=COLOR_BG); row2.pack(fill="x", pady=(4, 0))
        tk.Label(row2, text="ms/tick", bg=COLOR_BG, fg="white").pack(side="left")
        tk.Scale(row2, from_=20, to=800, orient="horizontal", variable=self.tick_ms,
                 bg=COLOR_BG, fg="white", troughcolor=COLOR_GRID, highlightthickness=0,
                 length=180).pack(side="left", fill="x", expand=True, padx=4)

    def _build_status(self, parent):
        frame = self._section(parent, "Status")
        self.status_text = tk.StringVar(value="(idle)")
        tk.Label(frame, textvariable=self.status_text, bg=COLOR_BG, fg="white",
                 justify="left", anchor="w").pack(fill="x")

    def _build_io(self, parent):
        frame = self._section(parent, "Scenario file")
        tk.Button(frame, text="Load JSON ...", command=self._load).pack(fill="x", pady=2)
        tk.Button(frame, text="Save JSON ...", command=self._save).pack(fill="x", pady=2)

    # ---- canvas drawing ---------------------------------------------------
    def _cell_size(self):
        cw = max(1, self.canvas.winfo_width())
        ch = max(1, self.canvas.winfo_height())
        return min(cw // self.scenario.width, ch // self.scenario.height, CELL_PX * 2)

    def _redraw(self):
        c = self.canvas
        c.delete("all")
        size = self._cell_size()
        if size < 4:
            return
        sc = self.scenario
        for y in range(sc.height):
            for x in range(sc.width):
                color = COLOR_WALL if sc.walls[y][x] else COLOR_FLOOR
                if not sc.walls[y][x] and self.world is not None:
                    v = self.world.visited[y * sc.width + x]
                    if v == 2:
                        color = COLOR_CLEANED
                    elif v == 1:
                        color = COLOR_VISITED
                c.create_rectangle(x * size, y * size, (x + 1) * size, (y + 1) * size,
                                   fill=color, outline=COLOR_GRID)
                if sc.dust[y][x] and not sc.walls[y][x]:
                    if self.world is not None:
                        idx = y * sc.width + x
                        if self.world.dust[idx]:
                            self._draw_dust(c, x, y, size)
                    else:
                        self._draw_dust(c, x, y, size)
        if self.world is not None:
            self._draw_robot(c, self.world.rx, self.world.ry, self.world.heading, size)
        else:
            self._draw_robot(c, sc.robot_x, sc.robot_y, sc.robot_heading, size)
        self._update_status()

    def _draw_dust(self, c, x, y, size):
        cx = x * size + size / 2
        cy = y * size + size / 2
        r = size * 0.18
        c.create_oval(cx - r, cy - r, cx + r, cy + r, fill=COLOR_DUST, outline="")

    def _draw_robot(self, c, x, y, heading, size):
        cx = x * size + size / 2
        cy = y * size + size / 2
        r = size * 0.38
        if heading == "N":
            pts = [cx, cy - r, cx + r, cy + r * 0.8, cx - r, cy + r * 0.8]
        elif heading == "S":
            pts = [cx, cy + r, cx + r, cy - r * 0.8, cx - r, cy - r * 0.8]
        elif heading == "E":
            pts = [cx + r, cy, cx - r * 0.8, cy + r, cx - r * 0.8, cy - r]
        else:
            pts = [cx - r, cy, cx + r * 0.8, cy + r, cx + r * 0.8, cy - r]
        c.create_polygon(*pts, fill=COLOR_ROBOT, outline=COLOR_ROBOT_OUTLINE, width=2)

    # ---- input handlers ---------------------------------------------------
    def _xy_from_event(self, event):
        size = self._cell_size()
        if size < 1:
            return None
        x = event.x // size
        y = event.y // size
        if 0 <= x < self.scenario.width and 0 <= y < self.scenario.height:
            return int(x), int(y)
        return None

    def _on_click(self, event):
        if self.world is not None:
            return
        xy = self._xy_from_event(event)
        if xy is None:
            return
        self._paint(xy[0], xy[1], erase=False)

    def _on_drag(self, event):
        if self.world is not None:
            return
        xy = self._xy_from_event(event)
        if xy is None:
            return
        self._paint(xy[0], xy[1], erase=False, drag=True)

    def _on_right_click(self, event):
        if self.world is not None:
            return
        xy = self._xy_from_event(event)
        if xy is None:
            return
        self._paint(xy[0], xy[1], erase=True)

    def _paint(self, x, y, erase, drag=False):
        sc = self.scenario
        tool = self.tool.get()
        if tool == TOOL_ROBOT and not erase and not drag:
            if not sc.walls[y][x]:
                sc.robot_x, sc.robot_y = x, y
                sc.robot_heading = self.heading_var.get()
        elif tool == TOOL_WALL:
            if x == sc.robot_x and y == sc.robot_y and not erase:
                return
            sc.walls[y][x] = not erase
            if not erase:
                sc.dust[y][x] = False
        elif tool == TOOL_FLOOR:
            sc.walls[y][x] = False
        elif tool == TOOL_DUST:
            if not sc.walls[y][x]:
                sc.dust[y][x] = not erase
        self._redraw()

    # ---- size / clear -----------------------------------------------------
    def _apply_resize(self):
        try:
            w = int(self.width_var.get())
            h = int(self.height_var.get())
        except (tk.TclError, ValueError):
            return
        w = max(MIN_DIM, min(MAX_DIM, w))
        h = max(MIN_DIM, min(MAX_DIM, h))
        self.scenario.resize(w, h)
        self._reset_runtime()
        self._redraw()

    def _clear_interior(self):
        sc = self.scenario
        for y in range(1, sc.height - 1):
            for x in range(1, sc.width - 1):
                sc.walls[y][x] = False
                sc.dust[y][x] = False
        self._reset_runtime()
        self._redraw()

    # ---- runtime ----------------------------------------------------------
    def _sync_config_into_scenario(self):
        try:
            self.scenario.dust_boost_ticks = int(self.boost_var.get())
            self.scenario.max_backoff_ticks = int(self.backoff_var.get())
            self.scenario.max_ticks = int(self.maxticks_var.get())
        except (tk.TclError, ValueError):
            pass

    def _reset_runtime(self):
        self.running = False
        self.world = None
        self.coord = None

    def _build_runtime(self):
        self._sync_config_into_scenario()
        scn = self.scenario.to_dict()
        self.world = emu.GridWorld.from_scenario_dict(scn)
        self.coord = emu.Coordinator(
            world=self.world,
            power=emu.CleaningPowerPolicy(dust_boost_ticks=self.scenario.dust_boost_ticks),
            max_backoff_ticks=self.scenario.max_backoff_ticks,
        )
        self.coord.start_session()

    def _start(self):
        if self.world is None:
            self._build_runtime()
        if self.running:
            return
        self.running = True
        self._tick_loop()

    def _pause(self):
        self.running = False

    def _step_once(self):
        if self.world is None:
            self._build_runtime()
        self.running = False
        self._do_one_tick()
        self._redraw()

    def _reset_clicked(self):
        self._reset_runtime()
        self._redraw()

    def _do_one_tick(self):
        if self.coord is None:
            return False
        if self.coord.tick_count >= self.scenario.max_ticks:
            return False
        self.coord.tick()
        return True

    def _tick_loop(self):
        if not self.running:
            return
        more = self._do_one_tick()
        self._redraw()
        if not more:
            self.running = False
            return
        self.root.after(max(20, int(self.tick_ms.get())), self._tick_loop)

    def _update_status(self):
        if self.world is None or self.coord is None:
            self.status_text.set("(edit mode) click cells to paint walls / dust / robot. Press Start.")
            return
        w = self.world
        c = self.coord
        ratio = w.cleaned_ratio()
        self.status_text.set(
            f"session={c.session_state}  phase={c.phase}\n"
            f"tick={c.tick_count}/{self.scenario.max_ticks}\n"
            f"pose=({w.rx},{w.ry},{w.heading})  power={w.last_power}\n"
            f"cleaned={w.cleaned_cells}  visited(blue)={sum(1 for v in w.visited if v == 1)}\n"
            f"collisions={w.collisions}  cleaned_ratio={ratio:.2f}"
        )

    # ---- I/O --------------------------------------------------------------
    def _load(self):
        initial = ROOT / "system_tests" / "maps"
        path = filedialog.askopenfilename(
            title="Load scenario JSON",
            initialdir=str(initial) if initial.exists() else str(ROOT),
            filetypes=[("JSON", "*.json"), ("All", "*.*")],
        )
        if not path:
            return
        self._load_path(Path(path))
        self._reset_runtime()
        self._redraw()

    def _load_path(self, path: Path):
        try:
            scn = json.loads(path.read_text(encoding="utf-8"))
            self.scenario = Scenario.from_dict(scn)
        except Exception as exc:
            messagebox.showerror("Load failed", f"{path}\n\n{exc}")
            return
        self.width_var.set(self.scenario.width)
        self.height_var.set(self.scenario.height)
        self.boost_var.set(self.scenario.dust_boost_ticks)
        self.backoff_var.set(self.scenario.max_backoff_ticks)
        self.maxticks_var.set(self.scenario.max_ticks)
        self.heading_var.set(self.scenario.robot_heading)
        self.root.title(f"RVC Simulator (Tkinter) - {path.name}")

    def _save(self):
        self._sync_config_into_scenario()
        path = filedialog.asksaveasfilename(
            title="Save scenario JSON",
            initialdir=str(ROOT / "system_tests" / "maps"),
            defaultextension=".json",
            filetypes=[("JSON", "*.json")],
        )
        if not path:
            return
        sid = Path(path).stem
        data = self.scenario.to_dict(sid=sid, title=f"GUI-authored {sid}")
        Path(path).write_text(json.dumps(data, indent=2), encoding="utf-8")
        messagebox.showinfo("Saved", f"Wrote {path}")


def main():
    initial = None
    if len(sys.argv) > 1:
        initial = Path(sys.argv[1]).resolve()
    root = tk.Tk()
    try:
        ttk.Style().theme_use("clam")
    except tk.TclError:
        pass
    root.geometry("1100x720")
    RvcGui(root, initial_path=initial)
    root.mainloop()


if __name__ == "__main__":
    main()
