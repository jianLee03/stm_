import argparse
import math
import queue
import random
import re
import threading
import time
import tkinter as tk
from collections import deque
from dataclasses import dataclass
from tkinter import filedialog, ttk

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    serial = None
    list_ports = None


ADC_MAX = 4095
DEFAULT_BAUD = 9600
REFRESH_MS = 33
STALE_AFTER_SECONDS = 1.5
DEFAULT_HISTORY = 360

LIGHT_WEIGHT = 35
RESISTANCE_WEIGHT = 65
WEIGHT_TOTAL = LIGHT_WEIGHT + RESISTANCE_WEIGHT

FIELD_RE = re.compile(
    r"\b(?P<key>L|R|C|STATUS|OLED)\s*[:=]\s*(?P<value>[A-Za-z0-9_.-]+)",
    re.IGNORECASE,
)

STATUS_COLORS = {
    "SAFE": "#27a56b",
    "WARNING": "#e2a33a",
    "DANGER": "#db4b57",
    "WAIT": "#7d8795",
}


@dataclass
class SensorSample:
    light: int
    resistance: int
    risk: int
    status: str
    oled: str
    raw_line: str
    received_at: float


def clamp(value, low, high):
    return max(low, min(high, value))


def calculate_risk(light, resistance):
    weighted = (light * LIGHT_WEIGHT) + (resistance * RESISTANCE_WEIGHT)
    return int(clamp(round(weighted * 100 / (ADC_MAX * WEIGHT_TOTAL)), 0, 100))


def status_from_risk(risk):
    if risk <= 30:
        return "SAFE"
    if risk < 70:
        return "WARNING"
    return "DANGER"


def parse_sensor_line(line):
    fields = {}
    for match in FIELD_RE.finditer(line):
        fields[match.group("key").upper()] = match.group("value")

    if "L" not in fields or "R" not in fields:
        return None

    try:
        light = clamp(int(fields["L"]), 0, ADC_MAX)
        resistance = clamp(int(fields["R"]), 0, ADC_MAX)
        risk = clamp(int(fields.get("C", calculate_risk(light, resistance))), 0, 100)
    except ValueError:
        return None

    status = fields.get("STATUS", status_from_risk(risk)).upper()
    if status not in {"SAFE", "WARNING", "DANGER"}:
        status = status_from_risk(risk)

    return SensorSample(
        light=light,
        resistance=resistance,
        risk=risk,
        status=status,
        oled=fields.get("OLED", "-"),
        raw_line=line,
        received_at=time.time(),
    )


class SerialReader(threading.Thread):
    def __init__(self, port, baud, events, stop_event):
        super().__init__(daemon=True)
        self.port = port
        self.baud = baud
        self.events = events
        self.stop_event = stop_event
        self.device = None

    def run(self):
        if serial is None:
            self.events.put(
                (
                    "error",
                    "pyserial is not installed. Run: python -m pip install pyserial",
                )
            )
            return

        try:
            self.device = serial.Serial(
                self.port,
                self.baud,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.15,
            )
            self.device.reset_input_buffer()
            self.events.put(("opened", self.port))

            while not self.stop_event.is_set():
                raw = self.device.readline()
                if not raw:
                    continue
                line = raw.decode("utf-8", errors="ignore").strip()
                if line:
                    self.events.put(("line", line))
        except Exception as exc:
            self.events.put(("error", str(exc)))
        finally:
            if self.device is not None:
                try:
                    self.device.close()
                except Exception:
                    pass
            self.events.put(("closed", self.port))


class DemoReader(threading.Thread):
    def __init__(self, events, stop_event):
        super().__init__(daemon=True)
        self.events = events
        self.stop_event = stop_event
        self.phase = 0.0

    def run(self):
        self.events.put(("opened", "DEMO"))
        while not self.stop_event.is_set():
            self.phase += 0.045

            darkness = 0.52 + math.sin(self.phase * 0.28) * 0.38
            darkness += random.uniform(-0.025, 0.025)

            vehicle_wave = (math.sin(self.phase * 0.95) + 1.0) * 0.5
            passing_vehicle = max(0.0, math.sin(self.phase * 1.9)) ** 5
            closeness = (vehicle_wave * 0.55) + (passing_vehicle * 0.45)
            closeness += random.uniform(-0.025, 0.025)

            light = int(clamp(darkness, 0.0, 1.0) * ADC_MAX)
            resistance = int(clamp(closeness, 0.0, 1.0) * ADC_MAX)
            risk = calculate_risk(light, resistance)
            status = status_from_risk(risk)

            self.events.put(
                (
                    "line",
                    f"L={light} R={resistance} C={risk} STATUS={status} OLED=DEMO",
                )
            )
            time.sleep(0.07)
        self.events.put(("closed", "DEMO"))


class RelationGraphApp(tk.Tk):
    def __init__(self, args):
        super().__init__()
        self.args = args
        self.title("Light and Vehicle Relation Graph")
        self.geometry("1160x760")
        self.minsize(980, 640)
        self.configure(bg="#0f1418")

        self.events = queue.Queue()
        self.stop_event = None
        self.reader = None
        self.history = deque(maxlen=args.history)
        self.raw_lines = deque(maxlen=6)
        self.sample = None
        self.last_error = ""
        self.connection = "Idle"

        self.port_var = tk.StringVar(value=args.port or "")
        self.baud_var = tk.StringVar(value=str(args.baud))
        self.connection_var = tk.StringVar(value="Idle")

        self.build_ui()
        self.refresh_ports(select_first=not args.port)

        if args.demo or (not args.port and not args.auto):
            self.after(200, self.start_demo)
        elif args.port or (args.auto and self.port_var.get()):
            self.after(200, self.connect_serial)

        self.after(20, self.process_events)
        self.after(REFRESH_MS, self.draw)
        self.protocol("WM_DELETE_WINDOW", self.on_close)

    def build_ui(self):
        style = ttk.Style()
        try:
            style.theme_use("clam")
        except tk.TclError:
            pass

        style.configure("Toolbar.TFrame", background="#141b21")
        style.configure("Toolbar.TLabel", background="#141b21", foreground="#d7dfe8")
        style.configure("Status.TLabel", background="#141b21", foreground="#9fb0c1")
        style.configure("Toolbar.TButton", padding=(10, 4))

        toolbar = ttk.Frame(self, style="Toolbar.TFrame", padding=(10, 8))
        toolbar.pack(fill=tk.X)

        ttk.Label(toolbar, text="Port", style="Toolbar.TLabel").pack(side=tk.LEFT)
        self.port_combo = ttk.Combobox(toolbar, textvariable=self.port_var, width=12)
        self.port_combo.pack(side=tk.LEFT, padx=(6, 12))

        ttk.Label(toolbar, text="Baud", style="Toolbar.TLabel").pack(side=tk.LEFT)
        ttk.Entry(toolbar, textvariable=self.baud_var, width=8).pack(side=tk.LEFT, padx=(6, 12))

        ttk.Button(toolbar, text="Refresh", command=self.refresh_ports, style="Toolbar.TButton").pack(
            side=tk.LEFT, padx=(0, 6)
        )
        ttk.Button(toolbar, text="Connect", command=self.connect_serial, style="Toolbar.TButton").pack(
            side=tk.LEFT, padx=(0, 6)
        )
        ttk.Button(toolbar, text="Demo", command=self.start_demo, style="Toolbar.TButton").pack(
            side=tk.LEFT, padx=(0, 6)
        )
        ttk.Button(toolbar, text="Stop", command=self.disconnect_reader, style="Toolbar.TButton").pack(
            side=tk.LEFT, padx=(0, 6)
        )
        ttk.Button(toolbar, text="Clear", command=self.clear_history, style="Toolbar.TButton").pack(
            side=tk.LEFT, padx=(0, 6)
        )
        ttk.Button(toolbar, text="CSV", command=self.save_csv, style="Toolbar.TButton").pack(
            side=tk.LEFT, padx=(0, 12)
        )

        ttk.Label(toolbar, textvariable=self.connection_var, style="Status.TLabel").pack(
            side=tk.LEFT, padx=(4, 0)
        )

        self.canvas = tk.Canvas(self, bg="#0f1418", bd=0, highlightthickness=0)
        self.canvas.pack(fill=tk.BOTH, expand=True)

    def refresh_ports(self, select_first=True):
        ports = []
        if list_ports is not None:
            ports = [port.device for port in list_ports.comports()]
        self.port_combo["values"] = ports
        if select_first and ports and not self.port_var.get():
            self.port_var.set(ports[0])

    def connect_serial(self):
        port = self.port_var.get().strip()
        if not port:
            self.last_error = "No COM port selected."
            self.connection_var.set("No COM port")
            return

        try:
            baud = int(self.baud_var.get())
        except ValueError:
            baud = DEFAULT_BAUD
            self.baud_var.set(str(DEFAULT_BAUD))

        self.disconnect_reader()
        self.stop_event = threading.Event()
        self.reader = SerialReader(port, baud, self.events, self.stop_event)
        self.reader.start()
        self.connection_var.set(f"Opening {port}...")

    def start_demo(self):
        self.disconnect_reader()
        self.stop_event = threading.Event()
        self.reader = DemoReader(self.events, self.stop_event)
        self.reader.start()
        self.connection_var.set("Starting demo...")

    def disconnect_reader(self):
        if self.stop_event is not None:
            self.stop_event.set()
        if self.reader is not None and self.reader.is_alive():
            self.reader.join(timeout=0.5)
        self.stop_event = None
        self.reader = None

    def clear_history(self):
        self.history.clear()
        self.raw_lines.clear()
        self.sample = None

    def save_csv(self):
        if not self.history:
            self.last_error = "No samples to save."
            return

        path = filedialog.asksaveasfilename(
            title="Save samples as CSV",
            defaultextension=".csv",
            filetypes=[("CSV files", "*.csv"), ("All files", "*.*")],
        )
        if not path:
            return

        try:
            with open(path, "w", encoding="utf-8", newline="") as file:
                file.write("timestamp,L,R,C,status,oled,raw\n")
                for sample in self.history:
                    raw = sample.raw_line.replace('"', '""')
                    file.write(
                        f"{sample.received_at:.3f},{sample.light},{sample.resistance},"
                        f"{sample.risk},{sample.status},{sample.oled},\"{raw}\"\n"
                    )
            self.last_error = f"Saved {len(self.history)} samples."
        except OSError as exc:
            self.last_error = str(exc)

    def process_events(self):
        while True:
            try:
                kind, payload = self.events.get_nowait()
            except queue.Empty:
                break

            if kind == "opened":
                self.connection = str(payload)
                self.last_error = ""
                self.connection_var.set(f"Connected: {payload}")
            elif kind == "closed":
                self.connection = "Idle"
                if self.reader is None:
                    self.connection_var.set("Stopped")
                else:
                    self.connection_var.set(f"Closed: {payload}")
            elif kind == "error":
                self.last_error = str(payload)
                self.connection_var.set(f"Error: {payload}")
            elif kind == "line":
                self.raw_lines.append(str(payload))
                sample = parse_sensor_line(str(payload))
                if sample is not None:
                    self.sample = sample
                    self.history.append(sample)

        self.after(20, self.process_events)

    def draw(self):
        width = max(1, self.canvas.winfo_width())
        height = max(1, self.canvas.winfo_height())
        self.canvas.delete("all")

        pad = 18
        gap = 14
        status_h = 76
        graph_h = height - status_h - (pad * 2) - (gap * 2)
        top_h = int(graph_h * 0.47)
        bottom_h = graph_h - top_h
        right_w = max(315, int(width * 0.31))
        left_w = width - (pad * 2) - gap - right_w

        self.draw_status_bar(pad, pad, width - pad * 2, status_h)
        self.draw_time_panel(pad, pad + status_h + gap, left_w, top_h)
        self.draw_relation_panel(pad, pad + status_h + gap + top_h + gap, left_w, bottom_h)

        right_x = pad + left_w + gap
        right_y = pad + status_h + gap
        curve_h = (top_h + bottom_h + gap - gap) // 2
        self.draw_distance_curve(right_x, right_y, right_w, curve_h)
        self.draw_light_curve(right_x, right_y + curve_h + gap, right_w, curve_h)

        self.after(REFRESH_MS, self.draw)

    def draw_status_bar(self, x, y, w, h):
        self.round_rect(x, y, x + w, y + h, radius=8, fill="#151c22", outline="#27323c")

        sample = self.sample
        stale = True
        if sample is not None:
            stale = (time.time() - sample.received_at) > STALE_AFTER_SECONDS

        status = "WAIT" if sample is None or stale else sample.status
        color = STATUS_COLORS.get(status, STATUS_COLORS["WAIT"])
        title = "Waiting for L/R data" if sample is None else f"Status {status}"
        if sample is not None and stale:
            title = "Data stale"

        self.canvas.create_oval(x + 18, y + 20, x + 54, y + 56, fill=color, outline="")
        self.canvas.create_text(
            x + 68,
            y + 21,
            text=title,
            anchor="nw",
            fill="#e9eef4",
            font=("Segoe UI", 18, "bold"),
        )

        if sample is None:
            values = "L --   R --   Risk --"
        else:
            values = (
                f"L {sample.light:4d} ({sample.light / ADC_MAX * 100:5.1f}% dark)   "
                f"R {sample.resistance:4d} ({sample.resistance / ADC_MAX * 100:5.1f}% close)   "
                f"Risk {sample.risk:3d}%"
            )
        self.canvas.create_text(
            x + 68,
            y + 49,
            text=values,
            anchor="nw",
            fill="#aebdca",
            font=("Consolas", 11),
        )

        rule = "R rises as vehicle gets closer. L rises as surroundings get darker."
        self.canvas.create_text(
            x + w - 18,
            y + 27,
            text=rule,
            anchor="ne",
            fill="#9fb0c1",
            font=("Segoe UI", 10),
        )
        self.canvas.create_text(
            x + w - 18,
            y + 49,
            text=self.connection_var.get(),
            anchor="ne",
            fill="#7f90a0",
            font=("Segoe UI", 10),
        )

    def draw_time_panel(self, x, y, w, h):
        self.draw_panel(x, y, w, h, "Realtime ADC trend", "L: bright -> dark    R: far -> close")
        gx, gy, gw, gh = x + 52, y + 48, w - 76, h - 82
        self.draw_grid(gx, gy, gw, gh, x_ticks=6, y_ticks=4)

        self.canvas.create_text(gx - 8, gy, text="4095", anchor="ne", fill="#768695", font=("Consolas", 9))
        self.canvas.create_text(
            gx - 8, gy + gh, text="0", anchor="ne", fill="#768695", font=("Consolas", 9)
        )

        samples = list(self.history)
        if len(samples) < 2:
            self.draw_empty_text(gx, gy, gw, gh, "Waiting for samples")
            self.draw_legend(x + w - 168, y + 20)
            return

        self.plot_line(
            gx,
            gy,
            gw,
            gh,
            [sample.light for sample in samples],
            "#58b7ff",
            width=2,
        )
        self.plot_line(
            gx,
            gy,
            gw,
            gh,
            [sample.resistance for sample in samples],
            "#ffb347",
            width=2,
        )

        self.draw_legend(x + w - 168, y + 20)
        last = samples[-1]
        self.canvas.create_text(
            gx,
            y + h - 22,
            text=f"Latest raw: {last.raw_line[:92]}",
            anchor="sw",
            fill="#7f90a0",
            font=("Consolas", 9),
        )

    def draw_relation_panel(self, x, y, w, h):
        self.draw_panel(x, y, w, h, "L-R relation map", "Right = close vehicle, Up = darker surroundings")
        gx, gy, gw, gh = x + 60, y + 48, w - 90, h - 88
        self.canvas.create_rectangle(gx, gy, gx + gw, gy + gh, fill="#101820", outline="#2e3a44")
        self.draw_risk_background(gx, gy, gw, gh)
        self.draw_grid(gx, gy, gw, gh, x_ticks=4, y_ticks=4)
        self.draw_risk_contour(gx, gy, gw, gh, 30, "#7aa37f")
        self.draw_risk_contour(gx, gy, gw, gh, 70, "#d5865e")

        self.canvas.create_text(
            gx,
            gy + gh + 26,
            text="R 0: far",
            anchor="nw",
            fill="#8b9aaa",
            font=("Segoe UI", 9),
        )
        self.canvas.create_text(
            gx + gw,
            gy + gh + 26,
            text="R 4095: close",
            anchor="ne",
            fill="#8b9aaa",
            font=("Segoe UI", 9),
        )
        self.canvas.create_text(
            gx - 42,
            gy + gh,
            text="Bright",
            anchor="nw",
            fill="#8b9aaa",
            font=("Segoe UI", 9),
        )
        self.canvas.create_text(
            gx - 42,
            gy,
            text="Dark",
            anchor="sw",
            fill="#8b9aaa",
            font=("Segoe UI", 9),
        )

        samples = list(self.history)
        if not samples:
            self.draw_empty_text(gx, gy, gw, gh, "No L/R samples")
            return

        tail = samples[-min(len(samples), 220) :]
        for index, sample in enumerate(tail):
            alpha = (index + 1) / len(tail)
            r = 2 if alpha < 0.85 else 3
            color = self.fade_color(self.risk_color(sample.risk), "#101820", alpha)
            px = gx + (sample.resistance / ADC_MAX) * gw
            py = gy + gh - (sample.light / ADC_MAX) * gh
            self.canvas.create_oval(px - r, py - r, px + r, py + r, fill=color, outline="")

        last = samples[-1]
        px = gx + (last.resistance / ADC_MAX) * gw
        py = gy + gh - (last.light / ADC_MAX) * gh
        self.canvas.create_oval(px - 7, py - 7, px + 7, py + 7, fill="#f5f7fa", outline="#101820", width=2)
        self.canvas.create_text(
            px + 10,
            py - 10,
            text=f"L={last.light} R={last.resistance}",
            anchor="sw",
            fill="#edf2f7",
            font=("Consolas", 9, "bold"),
        )

    def draw_distance_curve(self, x, y, w, h):
        self.draw_panel(x, y, w, h, "Vehicle distance -> R", "Closer distance makes R higher")
        gx, gy, gw, gh = x + 44, y + 50, w - 72, h - 86
        self.draw_grid(gx, gy, gw, gh, x_ticks=4, y_ticks=4)

        points = []
        for i in range(80):
            t = i / 79
            distance = t
            resistance = 1.0 - distance
            px = gx + t * gw
            py = gy + gh - resistance * gh
            points.extend([px, py])
        self.canvas.create_line(*points, fill="#ffb347", width=3, smooth=True)

        self.canvas.create_text(gx, gy + gh + 22, text="Close", anchor="nw", fill="#8b9aaa", font=("Segoe UI", 9))
        self.canvas.create_text(
            gx + gw,
            gy + gh + 22,
            text="Far",
            anchor="ne",
            fill="#8b9aaa",
            font=("Segoe UI", 9),
        )
        self.canvas.create_text(gx - 8, gy, text="R high", anchor="se", fill="#8b9aaa", font=("Segoe UI", 9))
        self.canvas.create_text(
            gx - 8,
            gy + gh,
            text="R low",
            anchor="ne",
            fill="#8b9aaa",
            font=("Segoe UI", 9),
        )

        if self.sample is not None:
            close_ratio = self.sample.resistance / ADC_MAX
            px = gx + (1.0 - close_ratio) * gw
            py = gy + gh - close_ratio * gh
            self.canvas.create_oval(px - 6, py - 6, px + 6, py + 6, fill="#f5f7fa", outline="")

    def draw_light_curve(self, x, y, w, h):
        self.draw_panel(x, y, w, h, "Ambient brightness -> L", "Brighter surroundings make L lower")
        gx, gy, gw, gh = x + 44, y + 50, w - 72, h - 86
        self.draw_grid(gx, gy, gw, gh, x_ticks=4, y_ticks=4)

        points = []
        for i in range(80):
            t = i / 79
            brightness = t
            light = 1.0 - brightness
            px = gx + t * gw
            py = gy + gh - light * gh
            points.extend([px, py])
        self.canvas.create_line(*points, fill="#58b7ff", width=3, smooth=True)

        self.canvas.create_text(gx, gy + gh + 22, text="Dark", anchor="nw", fill="#8b9aaa", font=("Segoe UI", 9))
        self.canvas.create_text(
            gx + gw,
            gy + gh + 22,
            text="Bright",
            anchor="ne",
            fill="#8b9aaa",
            font=("Segoe UI", 9),
        )
        self.canvas.create_text(gx - 8, gy, text="L high", anchor="se", fill="#8b9aaa", font=("Segoe UI", 9))
        self.canvas.create_text(
            gx - 8,
            gy + gh,
            text="L low",
            anchor="ne",
            fill="#8b9aaa",
            font=("Segoe UI", 9),
        )

        if self.sample is not None:
            dark_ratio = self.sample.light / ADC_MAX
            px = gx + (1.0 - dark_ratio) * gw
            py = gy + gh - dark_ratio * gh
            self.canvas.create_oval(px - 6, py - 6, px + 6, py + 6, fill="#f5f7fa", outline="")

    def draw_panel(self, x, y, w, h, title, subtitle):
        self.round_rect(x, y, x + w, y + h, radius=8, fill="#151c22", outline="#27323c")
        self.canvas.create_text(
            x + 16,
            y + 14,
            text=title,
            anchor="nw",
            fill="#eef3f8",
            font=("Segoe UI", 13, "bold"),
        )
        self.canvas.create_text(
            x + 16,
            y + 36,
            text=subtitle,
            anchor="nw",
            fill="#8fa0b0",
            font=("Segoe UI", 9),
        )

    def draw_grid(self, x, y, w, h, x_ticks, y_ticks):
        self.canvas.create_rectangle(x, y, x + w, y + h, outline="#2e3a44")
        for i in range(1, x_ticks):
            px = x + (w * i / x_ticks)
            self.canvas.create_line(px, y, px, y + h, fill="#22303a")
        for i in range(1, y_ticks):
            py = y + (h * i / y_ticks)
            self.canvas.create_line(x, py, x + w, py, fill="#22303a")

    def plot_line(self, x, y, w, h, values, color, width=2):
        if len(values) < 2:
            return

        points = []
        denom = max(1, len(values) - 1)
        for index, value in enumerate(values):
            px = x + (index / denom) * w
            py = y + h - (clamp(value, 0, ADC_MAX) / ADC_MAX) * h
            points.extend([px, py])
        self.canvas.create_line(*points, fill=color, width=width, smooth=True)

    def draw_legend(self, x, y):
        self.canvas.create_line(x, y + 8, x + 24, y + 8, fill="#58b7ff", width=3)
        self.canvas.create_text(x + 32, y, text="L darkness", anchor="nw", fill="#b7c4d1", font=("Segoe UI", 9))
        self.canvas.create_line(x, y + 28, x + 24, y + 28, fill="#ffb347", width=3)
        self.canvas.create_text(
            x + 32,
            y + 20,
            text="R closeness",
            anchor="nw",
            fill="#b7c4d1",
            font=("Segoe UI", 9),
        )

    def draw_risk_background(self, x, y, w, h):
        steps = 32
        for xi in range(steps):
            for yi in range(steps):
                r_norm = (xi + 0.5) / steps
                l_norm = 1.0 - ((yi + 0.5) / steps)
                risk = (l_norm * LIGHT_WEIGHT) + (r_norm * RESISTANCE_WEIGHT)
                base = self.risk_color(risk)
                color = self.fade_color(base, "#101820", 0.22)
                x0 = x + xi * w / steps
                x1 = x + (xi + 1) * w / steps + 1
                y0 = y + yi * h / steps
                y1 = y + (yi + 1) * h / steps + 1
                self.canvas.create_rectangle(x0, y0, x1, y1, fill=color, outline="")

    def draw_risk_contour(self, x, y, w, h, threshold, color):
        points = []
        for i in range(101):
            r_norm = i / 100
            l_norm = (threshold - (RESISTANCE_WEIGHT * r_norm)) / LIGHT_WEIGHT
            if 0.0 <= l_norm <= 1.0:
                px = x + r_norm * w
                py = y + h - l_norm * h
                points.extend([px, py])

        if len(points) >= 4:
            self.canvas.create_line(*points, fill=color, width=2, dash=(5, 5))
            self.canvas.create_text(
                points[-2] - 4,
                points[-1] - 4,
                text=f"{threshold}%",
                anchor="se",
                fill=color,
                font=("Segoe UI", 9, "bold"),
            )

    def draw_empty_text(self, x, y, w, h, text):
        self.canvas.create_text(
            x + w / 2,
            y + h / 2,
            text=text,
            fill="#6f7f8e",
            font=("Segoe UI", 12),
        )

    def round_rect(self, x1, y1, x2, y2, radius=8, **kwargs):
        points = [
            x1 + radius,
            y1,
            x2 - radius,
            y1,
            x2,
            y1,
            x2,
            y1 + radius,
            x2,
            y2 - radius,
            x2,
            y2,
            x2 - radius,
            y2,
            x1 + radius,
            y2,
            x1,
            y2,
            x1,
            y2 - radius,
            x1,
            y1 + radius,
            x1,
            y1,
        ]
        return self.canvas.create_polygon(points, smooth=True, **kwargs)

    def risk_color(self, risk):
        if risk <= 30:
            return STATUS_COLORS["SAFE"]
        if risk < 70:
            return STATUS_COLORS["WARNING"]
        return STATUS_COLORS["DANGER"]

    def fade_color(self, foreground, background, amount):
        amount = clamp(amount, 0.0, 1.0)
        fg = self.hex_to_rgb(foreground)
        bg = self.hex_to_rgb(background)
        mixed = tuple(int(bg[i] + (fg[i] - bg[i]) * amount) for i in range(3))
        return "#%02x%02x%02x" % mixed

    @staticmethod
    def hex_to_rgb(color):
        color = color.lstrip("#")
        return tuple(int(color[i : i + 2], 16) for i in (0, 2, 4))

    def on_close(self):
        self.disconnect_reader()
        self.destroy()


def build_parser():
    parser = argparse.ArgumentParser(
        description="Visualize STM32 L/R data as a light and vehicle relation graph."
    )
    parser.add_argument("--port", help="Serial port, for example COM3")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD, help="Serial baud rate")
    parser.add_argument("--auto", action="store_true", help="Use the first detected serial port")
    parser.add_argument("--demo", action="store_true", help="Run generated demo data")
    parser.add_argument(
        "--history",
        type=int,
        default=DEFAULT_HISTORY,
        help="Maximum number of samples kept on the graph",
    )
    return parser


def main():
    args = build_parser().parse_args()
    args.history = max(30, args.history)
    app = RelationGraphApp(args)
    app.mainloop()


if __name__ == "__main__":
    main()
