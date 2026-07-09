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
from tkinter import ttk

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

FIELD_RE = re.compile(
    r"\b(?P<key>L|R|C|STATUS|OLED)\s*[:=]\s*(?P<value>[A-Za-z0-9_.-]+)",
    re.IGNORECASE,
)

STATUS_COLORS = {
    "SAFE": "#24a164",
    "WARNING": "#e1a12a",
    "DANGER": "#d33b4a",
    "WAIT": "#6d7785",
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
    weighted = (light * 35) + (resistance * 65)
    return int(clamp(round(weighted * 100 / (ADC_MAX * 100)), 0, 100))


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
            self.phase += 0.055
            light = int(clamp(2100 + math.sin(self.phase * 0.55) * 1850, 0, ADC_MAX))
            resistance = int(
                clamp(1950 + math.sin(self.phase) * 1750 + random.uniform(-60, 60), 0, ADC_MAX)
            )
            risk = calculate_risk(light, resistance)
            status = status_from_risk(risk)
            self.events.put(("line", f"L={light} R={resistance} C={risk} STATUS={status} OLED=DEMO"))
            time.sleep(0.06)
        self.events.put(("closed", "DEMO"))


class MonitorApp(tk.Tk):
    def __init__(self, args):
        super().__init__()
        self.args = args
        self.title("STM32 UART Realtime Monitor")
        self.geometry("980x680")
        self.minsize(820, 560)
        self.configure(bg="#101418")

        self.events = queue.Queue()
        self.stop_event = None
        self.reader = None
        self.sample = None
        self.last_error = ""
        self.connection = "Idle"
        self.history = deque(maxlen=240)
        self.raw_lines = deque(maxlen=7)

        self.port_var = tk.StringVar(value=args.port or "")
        self.baud_var = tk.StringVar(value=str(args.baud))
        self.connection_var = tk.StringVar(value="Idle")

        self.build_ui()
        self.refresh_ports(select_first=not args.port)

        if args.demo:
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
        style.configure("Toolbar.TFrame", background="#141a20")
        style.configure("Toolbar.TLabel", background="#141a20", foreground="#d6dde5")
        style.configure("Status.TLabel", background="#141a20", foreground="#9fb0c2")

        toolbar = ttk.Frame(self, style="Toolbar.TFrame", padding=(10, 8))
        toolbar.pack(fill=tk.X)

        ttk.Label(toolbar, text="Port", style="Toolbar.TLabel").pack(side=tk.LEFT)
        self.port_combo = ttk.Combobox(toolbar, textvariable=self.port_var, width=12)
        self.port_combo.pack(side=tk.LEFT, padx=(6, 12))

        ttk.Label(toolbar, text="Baud", style="Toolbar.TLabel").pack(side=tk.LEFT)
        ttk.Entry(toolbar, textvariable=self.baud_var, width=8).pack(side=tk.LEFT, padx=(6, 12))

        ttk.Button(toolbar, text="Refresh", command=self.refresh_ports).pack(side=tk.LEFT, padx=(0, 6))
        ttk.Button(toolbar, text="Connect", command=self.connect_serial).pack(side=tk.LEFT, padx=(0, 6))
        ttk.Button(toolbar, text="Disconnect", command=self.disconnect_reader).pack(side=tk.LEFT, padx=(0, 6))
        ttk.Button(toolbar, text="Demo", command=self.start_demo).pack(side=tk.LEFT, padx=(0, 16))

        ttk.Label(toolbar, textvariable=self.connection_var, style="Status.TLabel").pack(
            side=tk.LEFT, fill=tk.X, expand=True
        )

        self.canvas = tk.Canvas(self, bg="#101418", highlightthickness=0)
        self.canvas.pack(fill=tk.BOTH, expand=True)

        self.bind("<F5>", lambda _event: self.refresh_ports())
        self.bind("<Return>", lambda _event: self.connect_serial())
        self.bind("<Escape>", lambda _event: self.on_close())

    def refresh_ports(self, select_first=False):
        if list_ports is None:
            self.port_combo["values"] = ()
            if serial is None:
                self.set_error("pyserial is not installed. Run: python -m pip install pyserial")
            return

        ports = [port.device for port in list_ports.comports()]
        self.port_combo["values"] = ports
        if select_first and ports and not self.port_var.get():
            self.port_var.set(ports[0])
        if not ports:
            self.set_error("No COM ports found. Check USB serial wiring and driver.")

    def connect_serial(self):
        port = self.port_var.get().strip()
        if not port:
            self.set_error("Select or type a COM port first, for example COM3.")
            return

        try:
            baud = int(self.baud_var.get())
        except ValueError:
            self.set_error("Baud must be a number. main.c uses 9600.")
            return

        self.disconnect_reader()
        self.last_error = ""
        self.connection = f"Opening {port}..."
        self.connection_var.set(self.connection)
        self.stop_event = threading.Event()
        self.reader = SerialReader(port, baud, self.events, self.stop_event)
        self.reader.start()

    def start_demo(self):
        self.disconnect_reader()
        self.last_error = ""
        self.connection = "Demo running"
        self.connection_var.set(self.connection)
        self.stop_event = threading.Event()
        self.reader = DemoReader(self.events, self.stop_event)
        self.reader.start()

    def disconnect_reader(self):
        if self.stop_event is not None:
            self.stop_event.set()
        self.stop_event = None
        self.reader = None
        self.connection = "Idle"
        self.connection_var.set(self.connection)

    def process_events(self):
        while True:
            try:
                event, payload = self.events.get_nowait()
            except queue.Empty:
                break

            if event == "opened":
                self.connection = f"Connected: {payload}"
                self.connection_var.set(self.connection)
                self.last_error = ""
            elif event == "closed":
                if self.reader is None:
                    self.connection = "Idle"
                else:
                    self.connection = f"Closed: {payload}"
                self.connection_var.set(self.connection)
            elif event == "error":
                self.set_error(self.humanize_error(payload))
                self.reader = None
                self.stop_event = None
            elif event == "line":
                self.raw_lines.appendleft(payload)
                sample = parse_sensor_line(payload)
                if sample is not None:
                    self.sample = sample
                    self.history.append(sample)

        self.after(20, self.process_events)

    def set_error(self, message):
        self.last_error = message
        self.connection = "Error"
        self.connection_var.set(message)

    @staticmethod
    def humanize_error(message):
        lower = message.lower()
        if "access is denied" in lower or "permission" in lower:
            return "The COM port is already open in another program."
        if "cannot find" in lower or "file not found" in lower:
            return "The selected COM port was not found."
        return message

    def draw(self):
        w = max(self.canvas.winfo_width(), 1)
        h = max(self.canvas.winfo_height(), 1)
        self.canvas.delete("all")

        sample = self.sample
        stale = sample is None or (time.time() - sample.received_at) > STALE_AFTER_SECONDS
        if sample is None:
            light = resistance = risk = 0
            status = "WAIT"
            oled = "-"
        else:
            light = sample.light
            resistance = sample.resistance
            risk = sample.risk
            status = "WAIT" if stale else sample.status
            oled = sample.oled

        l_level = light / ADC_MAX
        r_level = resistance / ADC_MAX
        distance = self.distance_from_r(r_level)

        self.draw_background(w, h, l_level)
        self.draw_header(w, status, risk, stale)

        left = 32
        top = 96
        panel_w = min(430, w * 0.46)
        self.draw_meter_panel(left, top, panel_w, light, resistance, risk, status, oled, distance)

        scene_x = left + panel_w + 28
        scene_w = max(300, w - scene_x - 32)
        scene_h = max(310, h - 145)
        self.draw_road_scene(scene_x, top, scene_w, scene_h, r_level, l_level, distance)

        history_top = h - 126
        if h > 610:
            self.draw_history(left, history_top, panel_w, 86)
            self.draw_raw_log(scene_x, history_top, scene_w, 86)

        if self.last_error:
            self.draw_error(w, h, self.last_error)

        self.after(REFRESH_MS, self.draw)

    def distance_from_r(self, r_level):
        r_level = clamp(r_level, 0.0, 1.0)
        return self.args.max_distance - (self.args.max_distance - self.args.min_distance) * r_level

    def draw_background(self, w, h, l_level):
        brightness = 1.0 - clamp(l_level, 0.0, 1.0)
        top = self.mix("#080b10", "#b9d7f2", brightness)
        bottom = self.mix("#11161b", "#eef3f7", brightness)
        steps = 80
        for i in range(steps):
            t = i / max(1, steps - 1)
            color = self.mix(top, bottom, t)
            y0 = int(h * i / steps)
            y1 = int(h * (i + 1) / steps) + 1
            self.canvas.create_rectangle(0, y0, w, y1, fill=color, outline="")

    def draw_header(self, w, status, risk, stale):
        color = STATUS_COLORS.get(status, STATUS_COLORS["WAIT"])
        self.canvas.create_rectangle(0, 0, w, 72, fill="#111820", outline="")
        self.canvas.create_text(
            28,
            26,
            anchor="w",
            text="STM32 UART Realtime Monitor",
            fill="#f2f6fb",
            font=("Segoe UI", 20, "bold"),
        )
        self.canvas.create_text(
            30,
            54,
            anchor="w",
            text="L: dark increases | R: near increases | USART2 9600 8N1",
            fill="#9fb0c2",
            font=("Segoe UI", 10),
        )

        badge_w = 190
        badge_x = w - badge_w - 28
        self.canvas.create_rectangle(badge_x, 17, badge_x + badge_w, 56, fill="#17212a", outline=color, width=2)
        self.canvas.create_text(
            badge_x + 18,
            36,
            anchor="w",
            text="NO DATA" if stale else status,
            fill=color,
            font=("Segoe UI", 17, "bold"),
        )
        self.canvas.create_text(
            badge_x + badge_w - 16,
            37,
            anchor="e",
            text=f"{risk:3d}%",
            fill="#f2f6fb",
            font=("Consolas", 17, "bold"),
        )

    def draw_meter_panel(self, x, y, w, light, resistance, risk, status, oled, distance):
        self.draw_panel(x, y, w, 356, "Live ADC")
        self.draw_bar(
            x + 22,
            y + 54,
            w - 44,
            34,
            "L  CDS dark level",
            light,
            ADC_MAX,
            "#4aa3ff",
            "Bright",
            "Dark",
        )
        self.draw_bar(
            x + 22,
            y + 124,
            w - 44,
            34,
            "R  Distance near level",
            resistance,
            ADC_MAX,
            "#ffbf47",
            "Far",
            "Near",
        )
        self.draw_bar(
            x + 22,
            y + 194,
            w - 44,
            34,
            "C  Risk",
            risk,
            100,
            STATUS_COLORS.get(status, STATUS_COLORS["WAIT"]),
            "Safe",
            "Danger",
        )

        value_y = y + 272
        self.draw_value_box(x + 22, value_y, 106, "L ADC", f"{light}")
        self.draw_value_box(x + 142, value_y, 106, "R ADC", f"{resistance}")
        self.draw_value_box(x + 262, value_y, 106, "Gap", f"{distance:.1f} m")
        self.canvas.create_text(
            x + 22,
            y + 333,
            anchor="w",
            fill="#9fb0c2",
            text=f"OLED {oled}   {self.connection}",
            font=("Segoe UI", 10),
        )

    def draw_panel(self, x, y, w, h, title):
        self.canvas.create_rectangle(x, y, x + w, y + h, fill="#121a22", outline="#283646", width=1)
        self.canvas.create_text(x + 18, y + 22, anchor="w", fill="#f2f6fb", text=title, font=("Segoe UI", 14, "bold"))

    def draw_bar(self, x, y, w, h, title, value, max_value, color, left_label, right_label):
        ratio = clamp(value / max_value, 0.0, 1.0)
        self.canvas.create_text(x, y - 12, anchor="w", fill="#dbe4ee", text=title, font=("Segoe UI", 11, "bold"))
        self.canvas.create_text(
            x + w,
            y - 12,
            anchor="e",
            fill="#f2f6fb",
            text=f"{value}/{max_value}",
            font=("Consolas", 11, "bold"),
        )
        self.canvas.create_rectangle(x, y, x + w, y + h, fill="#202b36", outline="#334455")
        self.canvas.create_rectangle(x, y, x + w * ratio, y + h, fill=color, outline="")
        self.canvas.create_line(x + w * ratio, y - 3, x + w * ratio, y + h + 3, fill="#ffffff", width=2)
        self.canvas.create_text(x, y + h + 16, anchor="w", fill="#8fa1b3", text=left_label, font=("Segoe UI", 9))
        self.canvas.create_text(x + w, y + h + 16, anchor="e", fill="#8fa1b3", text=right_label, font=("Segoe UI", 9))

    def draw_value_box(self, x, y, w, title, value):
        self.canvas.create_rectangle(x, y, x + w, y + 48, fill="#18232d", outline="#2f4252")
        self.canvas.create_text(x + 10, y + 14, anchor="w", fill="#8fa1b3", text=title, font=("Segoe UI", 9, "bold"))
        self.canvas.create_text(x + 10, y + 33, anchor="w", fill="#f2f6fb", text=value, font=("Consolas", 13, "bold"))

    def draw_road_scene(self, x, y, w, h, r_level, l_level, distance):
        self.draw_panel(x, y, w, h, "Distance View")
        road_x = x + 28
        road_y = y + 48
        road_w = min(w - 56, 520)
        road_h = h - 82
        cx = road_x + road_w * 0.50

        brightness = 1.0 - clamp(l_level, 0.0, 1.0)
        asphalt = self.mix("#15191e", "#4b5054", brightness)
        shoulder = self.mix("#0d1014", "#252a2e", brightness)
        lane = self.mix("#8a9096", "#f4f1dc", brightness)

        self.canvas.create_rectangle(road_x, road_y, road_x + road_w, road_y + road_h, fill=shoulder, outline="")
        self.canvas.create_polygon(
            road_x + road_w * 0.17,
            road_y,
            road_x + road_w * 0.83,
            road_y,
            road_x + road_w * 0.97,
            road_y + road_h,
            road_x + road_w * 0.03,
            road_y + road_h,
            fill=asphalt,
            outline="#303a44",
        )

        for lane_x in (road_x + road_w * 0.39, road_x + road_w * 0.61):
            dash_y = road_y + 18
            while dash_y < road_y + road_h - 20:
                self.canvas.create_line(lane_x, dash_y, lane_x, dash_y + 26, fill=lane, width=5)
                dash_y += 52

        self_y = road_y + road_h - 74
        front_min_y = road_y + 78
        front_max_y = self_y - 104
        front_y = front_min_y + (1.0 - r_level) * max(20, front_max_y - front_min_y)

        self.draw_car(cx, front_y, 78, "#d8a12a", "FRONT")
        self.draw_car(cx, self_y, 104, "#2f7de1", "ME")
        self.canvas.create_line(cx, front_y + 42, cx, self_y - 56, fill="#e7edf3", dash=(5, 5), width=2)

        readout_x = road_x + road_w - 116
        self.canvas.create_rectangle(readout_x, road_y + 16, readout_x + 96, road_y + 78, fill="#121a22", outline="#34495d")
        self.canvas.create_text(readout_x + 12, road_y + 36, anchor="w", fill="#8fa1b3", text="Estimate", font=("Segoe UI", 9, "bold"))
        self.canvas.create_text(readout_x + 12, road_y + 61, anchor="w", fill="#f2f6fb", text=f"{distance:.1f} m", font=("Consolas", 16, "bold"))

    def draw_car(self, cx, cy, size, color, label):
        w = size * 0.62
        h = size
        x0 = cx - w / 2
        x1 = cx + w / 2
        y0 = cy - h / 2
        y1 = cy + h / 2
        self.canvas.create_oval(cx - w * 0.55, y1 - 8, cx + w * 0.55, y1 + 9, fill="#0a0d10", outline="")
        self.canvas.create_polygon(
            cx,
            y0,
            x1,
            y0 + h * 0.35,
            x1 - w * 0.12,
            y1,
            x0 + w * 0.12,
            y1,
            x0,
            y0 + h * 0.35,
            fill=color,
            outline="#081018",
            width=2,
        )
        self.canvas.create_polygon(
            cx - w * 0.25,
            y0 + h * 0.22,
            cx + w * 0.25,
            y0 + h * 0.22,
            cx + w * 0.34,
            y0 + h * 0.43,
            cx - w * 0.34,
            y0 + h * 0.43,
            fill="#263d4c",
            outline="#081018",
        )
        self.canvas.create_text(cx, cy + h * 0.13, fill="#f8fbff", text=label, font=("Segoe UI", 10, "bold"))

    def draw_history(self, x, y, w, h):
        self.draw_panel(x, y, w, h, "Recent L/R")
        plot_x = x + 18
        plot_y = y + 32
        plot_w = w - 36
        plot_h = h - 46
        self.canvas.create_rectangle(plot_x, plot_y, plot_x + plot_w, plot_y + plot_h, fill="#101820", outline="#2f4252")
        if len(self.history) < 2:
            return

        points_l = []
        points_r = []
        samples = list(self.history)
        for index, sample in enumerate(samples):
            px = plot_x + plot_w * index / max(1, len(samples) - 1)
            points_l.extend((px, plot_y + plot_h * (1 - sample.light / ADC_MAX)))
            points_r.extend((px, plot_y + plot_h * (1 - sample.resistance / ADC_MAX)))

        self.canvas.create_line(*points_l, fill="#4aa3ff", width=2)
        self.canvas.create_line(*points_r, fill="#ffbf47", width=2)
        self.canvas.create_text(plot_x + 8, plot_y + 10, anchor="w", fill="#4aa3ff", text="L", font=("Consolas", 10, "bold"))
        self.canvas.create_text(plot_x + 28, plot_y + 10, anchor="w", fill="#ffbf47", text="R", font=("Consolas", 10, "bold"))

    def draw_raw_log(self, x, y, w, h):
        self.draw_panel(x, y, w, h, "UART Lines")
        text_y = y + 34
        for line in list(self.raw_lines)[:3]:
            self.canvas.create_text(
                x + 18,
                text_y,
                anchor="w",
                fill="#c6d3df",
                text=self.truncate(line, max(24, int((w - 36) / 8))),
                font=("Consolas", 10),
            )
            text_y += 18

    def draw_error(self, w, h, message):
        box_w = min(w - 48, 720)
        x = (w - box_w) / 2
        y = h - 54
        self.canvas.create_rectangle(x, y, x + box_w, y + 38, fill="#321822", outline="#d33b4a", width=2)
        self.canvas.create_text(
            x + 14,
            y + 19,
            anchor="w",
            fill="#ffd9df",
            text=self.truncate(message, max(32, int((box_w - 28) / 8))),
            font=("Segoe UI", 10, "bold"),
        )

    @staticmethod
    def truncate(text, limit):
        if len(text) <= limit:
            return text
        return text[: max(0, limit - 3)] + "..."

    @staticmethod
    def mix(color_a, color_b, t):
        t = clamp(t, 0.0, 1.0)
        a = tuple(int(color_a[i : i + 2], 16) for i in (1, 3, 5))
        b = tuple(int(color_b[i : i + 2], 16) for i in (1, 3, 5))
        c = tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))
        return f"#{c[0]:02x}{c[1]:02x}{c[2]:02x}"

    def on_close(self):
        self.disconnect_reader()
        self.destroy()


def build_parser():
    parser = argparse.ArgumentParser(description="Realtime GUI monitor for STM32 UART L/R/C output.")
    parser.add_argument("--port", help="Serial port, for example COM3.")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD, help="Baud rate. main.c uses 9600.")
    parser.add_argument("--demo", action="store_true", help="Run without hardware using simulated sensor data.")
    parser.add_argument("--no-auto", dest="auto", action="store_false", help="Do not auto-connect to the first port.")
    parser.set_defaults(auto=True)
    parser.add_argument("--min-distance", type=float, default=1.0, help="Shown distance when R is maximum.")
    parser.add_argument("--max-distance", type=float, default=8.0, help="Shown distance when R is minimum.")
    return parser


def main():
    args = build_parser().parse_args()
    if args.max_distance <= args.min_distance:
        raise SystemExit("--max-distance must be greater than --min-distance")
    app = MonitorApp(args)
    app.mainloop()


if __name__ == "__main__":
    main()
