import argparse
import math
import queue
import random
import re
import threading
import time
import tkinter as tk
from dataclasses import dataclass

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    serial = None
    list_ports = None


ADC_MAX = 4095
DEFAULT_BAUD = 9600
RETRY_DELAY_SECONDS = 2.0
FONT_KR = "Malgun Gothic"
FONT_MONO = "Consolas"

MIN_DISTANCE_M = 1.2
MAX_DISTANCE_M = 8.0

FIELD_RE = re.compile(r"\b(?P<key>STATUS|L|R|C)\s*[:=]\s*(?P<value>[A-Za-z0-9]+)")


@dataclass
class SensorSample:
    light_adc: int
    resistance_adc: int
    risk_percent: int
    status: str
    received_at: float
    raw_line: str


def clamp(value, low, high):
    return max(low, min(high, value))


def lerp(a, b, t):
    return a + (b - a) * t


def hex_to_rgb(color):
    color = color.lstrip("#")
    return tuple(int(color[i : i + 2], 16) for i in (0, 2, 4))


def rgb_to_hex(rgb):
    return "#%02x%02x%02x" % tuple(clamp(int(v), 0, 255) for v in rgb)


def blend_hex(night_color, day_color, brightness):
    brightness = clamp(brightness, 0.0, 1.0)
    n = hex_to_rgb(night_color)
    d = hex_to_rgb(day_color)
    return rgb_to_hex(lerp(n[i], d[i], brightness) for i in range(3))


def calculate_risk_percent(light_adc, resistance_adc):
    weighted = (light_adc * 35) + (resistance_adc * 65)
    return int(clamp(round(weighted * 100 / (ADC_MAX * 100)), 0, 100))


def status_from_risk(risk):
    if risk >= 70:
        return "DANGER"
    if risk >= 31:
        return "WARNING"
    return "SAFE"


def parse_sensor_line(line):
    fields = {}
    for match in FIELD_RE.finditer(line):
        fields[match.group("key").upper()] = match.group("value")

    if "L" not in fields or "R" not in fields:
        return None

    try:
        light = clamp(int(fields["L"]), 0, ADC_MAX)
        resistance = clamp(int(fields["R"]), 0, ADC_MAX)
        risk = clamp(int(fields.get("C", calculate_risk_percent(light, resistance))), 0, 100)
    except ValueError:
        return None

    status = fields.get("STATUS", status_from_risk(risk)).upper()
    if status not in {"SAFE", "WARNING", "DANGER"}:
        status = status_from_risk(risk)

    return SensorSample(
        light_adc=light,
        resistance_adc=resistance,
        risk_percent=risk,
        status=status,
        received_at=time.time(),
        raw_line=line,
    )


class SerialReader(threading.Thread):
    def __init__(self, port, baud, out_queue, stop_event):
        super().__init__(daemon=True)
        self.port = port
        self.baud = baud
        self.out_queue = out_queue
        self.stop_event = stop_event
        self.serial_port = None

    def run(self):
        if serial is None:
            self.out_queue.put(("error", "pyserial이 설치되어 있지 않습니다. pip install pyserial 후 다시 실행하세요."))
            return

        try:
            self.serial_port = serial.Serial(
                self.port,
                self.baud,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.2,
            )
            self.serial_port.reset_input_buffer()
            self.out_queue.put(("connected", self.port))

            while not self.stop_event.is_set():
                raw = self.serial_port.readline()
                if not raw:
                    continue
                line = raw.decode("utf-8", errors="ignore").strip()
                if line:
                    self.out_queue.put(("line", line))
        except Exception as exc:
            self.out_queue.put(("error", str(exc)))
        finally:
            if self.serial_port is not None:
                try:
                    self.serial_port.close()
                except Exception:
                    pass
            self.out_queue.put(("disconnected", self.port))


class DemoReader(threading.Thread):
    def __init__(self, out_queue, stop_event):
        super().__init__(daemon=True)
        self.out_queue = out_queue
        self.stop_event = stop_event
        self.phase = 0.0

    def run(self):
        self.out_queue.put(("connected", "DEMO"))
        while not self.stop_event.is_set():
            self.phase += 0.055
            light = int(clamp(2050 + math.sin(self.phase * 0.42) * 1850, 0, ADC_MAX))
            resistance = int(clamp(1980 + math.sin(self.phase) * 1780 + random.uniform(-70, 70), 0, ADC_MAX))
            risk = calculate_risk_percent(light, resistance)
            status = status_from_risk(risk)
            self.out_queue.put(("line", f"L={light} R={resistance} C={risk} STATUS={status} OLED=DEMO"))
            time.sleep(0.07)
        self.out_queue.put(("disconnected", "DEMO"))


class DrivingDashboard(tk.Tk):
    def __init__(self, args):
        super().__init__()
        self.title("실시간 주행 정보 디스플레이")
        self.minsize(760, 620)
        self.configure(bg="#24282c")

        self.args = args
        self.events = queue.Queue()
        self.stop_event = None
        self.reader = None
        self.sample = None
        self.connected = False
        self.retry_at = 0.0
        self.connection_text = "수신 대기"
        self.last_error = ""

        self.port = args.port or ""
        self.baud = args.baud

        self.canvas = tk.Canvas(self, bg="#24282c", highlightthickness=0, bd=0)
        self.canvas.pack(fill=tk.BOTH, expand=True)
        self.bind("<Escape>", lambda _event: self.on_close())
        self.bind("<F5>", lambda _event: self.reconnect())

        self.refresh_ports()
        if args.simulate:
            self.after(250, self.start_demo)
        elif self.port:
            self.after(250, self.connect_serial)
        elif serial is None:
            self.last_error = "pyserial이 없습니다. pip install pyserial 후 실행하거나 --simulate로 화면을 확인하세요."
        else:
            self.last_error = "COM 포트를 찾지 못했습니다. 보드 USB 연결 후 --port COM번호로 실행하세요."

        self.after(25, self.process_events)
        self.after(33, self.render)
        self.protocol("WM_DELETE_WINDOW", self.on_close)

    def refresh_ports(self):
        if self.port or list_ports is None:
            return
        ports = [port.device for port in list_ports.comports()]
        if ports:
            self.port = ports[0]

    def reconnect(self):
        self.disconnect_reader()
        if self.args.simulate:
            self.start_demo()
        else:
            self.refresh_ports()
            self.connect_serial()

    def connect_serial(self):
        if not self.port:
            self.last_error = "COM 포트를 찾지 못했습니다. 보드 USB 연결과 장치 관리자 COM 번호를 확인하세요."
            self.connection_text = "포트 없음"
            return

        self.disconnect_reader()
        self.last_error = ""
        self.stop_event = threading.Event()
        self.reader = SerialReader(self.port, self.baud, self.events, self.stop_event)
        self.reader.start()
        self.connection_text = f"{self.port} 연결 중"
        self.title(f"실시간 주행 정보 디스플레이 - {self.port}")

    def start_demo(self):
        self.disconnect_reader()
        self.last_error = ""
        self.stop_event = threading.Event()
        self.reader = DemoReader(self.events, self.stop_event)
        self.reader.start()
        self.connection_text = "DEMO 실행 중"
        self.title("실시간 주행 정보 디스플레이 - DEMO")

    def disconnect_reader(self):
        if self.stop_event is not None:
            self.stop_event.set()
        self.reader = None
        self.stop_event = None
        self.connected = False

    def process_events(self):
        while True:
            try:
                event, payload = self.events.get_nowait()
            except queue.Empty:
                break

            if event == "connected":
                self.connected = True
                self.retry_at = 0.0
                self.last_error = ""
                self.connection_text = f"{payload} 연결됨"
                self.title(f"실시간 주행 정보 디스플레이 - {payload}")
            elif event == "disconnected":
                self.connected = False
                if self.reader is None:
                    self.connection_text = "연결 해제"
            elif event == "error":
                self.connected = False
                self.last_error = self.humanize_serial_error(payload)
                self.connection_text = "수신 오류"
                self.reader = None
                self.stop_event = None
                if not self.args.simulate and self.port:
                    self.retry_at = time.time() + RETRY_DELAY_SECONDS
            elif event == "line":
                sample = parse_sensor_line(payload)
                if sample is not None:
                    self.sample = sample

        if (
            self.retry_at > 0.0
            and self.reader is None
            and not self.connected
            and time.time() >= self.retry_at
        ):
            self.retry_at = 0.0
            self.connect_serial()

        self.after(25, self.process_events)

    @staticmethod
    def humanize_serial_error(message):
        lower = message.lower()
        if "access is denied" in lower or "permission" in lower:
            return "COM 포트가 다른 프로그램에서 사용 중입니다. 시리얼 모니터를 닫고 다시 시도하세요."
        if "file not found" in lower or "cannot find" in lower:
            return "지정한 COM 포트를 찾을 수 없습니다. USB 연결과 포트 번호를 확인하세요."
        return message

    def convert_sample(self, sample):
        r_ratio = sample.resistance_adc / ADC_MAX
        if self.args.invert_r:
            r_ratio = 1.0 - r_ratio

        light_ratio = sample.light_adc / ADC_MAX
        if self.args.invert_light:
            brightness = light_ratio
        else:
            brightness = 1.0 - light_ratio

        return clamp(r_ratio, 0.0, 1.0), clamp(brightness, 0.0, 1.0)

    def distance_from_level(self, r_level):
        return lerp(self.args.max_distance, self.args.min_distance, r_level)

    def status_color(self, status, brightness):
        if status == "DANGER":
            return blend_hex("#902338", "#e62b4d", brightness)
        if status == "WARNING":
            return blend_hex("#91621c", "#f0a122", brightness)
        if status == "SAFE":
            return blend_hex("#1f7246", "#26b56f", brightness)
        return blend_hex("#55606a", "#7b8794", brightness)

    def render(self):
        width = max(self.canvas.winfo_width(), 1)
        height = max(self.canvas.winfo_height(), 1)
        self.canvas.delete("all")

        if self.sample is None:
            r_level = 0.42
            brightness = 0.72
            risk = 0
            status = "WAIT"
            light = 0
            resistance = 0
            stale = True
        else:
            r_level, brightness = self.convert_sample(self.sample)
            risk = self.sample.risk_percent
            status = self.sample.status
            light = self.sample.light_adc
            resistance = self.sample.resistance_adc
            stale = (time.time() - self.sample.received_at) > 1.5

        distance_m = self.distance_from_level(r_level)
        color = self.status_color(status, brightness)
        self.draw_dashboard(width, height, r_level, brightness, distance_m, status, risk, light, resistance, color, stale)

        if self.last_error:
            self.draw_error(width, height, brightness, self.last_error)

        self.after(33, self.render)

    def tone(self, night_color, day_color, brightness):
        return blend_hex(night_color, day_color, brightness)

    def draw_dashboard(self, w, h, r_level, brightness, distance_m, status, risk, light, resistance, color, stale):
        title_h = 58
        outer = self.tone("#1c2024", "#d8dde0", brightness)
        title_bg = self.tone("#20272d", "#edf1f3", brightness)
        title_fg = self.tone("#eef4f7", "#202428", brightness)
        panel_bg = self.tone("#202225", "#55585b", brightness)
        border = self.tone("#68717b", "#c7ced3", brightness)

        self.canvas.create_rectangle(0, 0, w, h, fill=outer, outline="")
        self.canvas.create_rectangle(0, 0, w, title_h, fill=title_bg, outline="")
        self.canvas.create_text(
            w / 2,
            title_h / 2,
            text="실시간 주행 정보 디스플레이",
            fill=title_fg,
            font=(FONT_KR, 25, "bold"),
        )

        panel_x = 10
        panel_top = title_h + 3
        panel_bottom = h - 10
        self.canvas.create_rectangle(panel_x, panel_top, w - panel_x, panel_bottom, fill=panel_bg, outline=border, width=2)

        right_w = 148
        road_w = min(520, w - right_w - 58)
        road_w = max(430, road_w)
        road_x = max(16, (w - road_w - right_w) / 2)
        if road_x + road_w + right_w + 12 > w:
            road_x = 16
            road_w = max(380, w - road_x - right_w - 18)

        road_top = panel_top
        road_bottom = panel_bottom
        road_h = road_bottom - road_top
        road_center_x = road_x + road_w / 2

        own_h = min(158, road_h * 0.29)
        own_w = own_h * 0.62
        front_h = own_h * lerp(0.63, 0.72, r_level)
        front_w = front_h * 0.63
        own_y = road_bottom - own_h * 0.55 - 8
        far_y = road_top + front_h * 0.62 + 42
        close_y = own_y - own_h * 0.62 - front_h * 0.56
        front_y = lerp(far_y, close_y, r_level)

        self.draw_road(road_x, road_top, road_w, road_h, brightness)
        self.draw_distance_line(road_center_x, front_y, front_h, own_y, own_h, brightness)
        self.draw_car(road_center_x, front_y, front_w, front_h, "#d8a319", "#f1d04a", brightness, "차", front=True)
        self.draw_car(road_center_x, own_y, own_w, own_h, "#1f5fae", "#4d92e7", brightness, "차", front=False)

        ruler_x = road_x + road_w + 30
        self.draw_ruler(ruler_x, far_y, close_y, brightness, distance_m)
        self.draw_knob(road_x + road_w - 54, road_top + 122, 26, r_level, brightness)
        self.draw_distance_readout(ruler_x + 18, road_top, road_bottom, brightness, distance_m)
        self.draw_status_readout(ruler_x + 18, road_bottom - 138, brightness, color, status, risk, light, resistance, stale)

    def draw_road(self, x, y, w, h, brightness):
        shoulder = self.tone("#111315", "#34383c", brightness)
        asphalt = self.tone("#171a1d", "#4b4e50", brightness)
        lane = self.tone("#9ea1a0", "#f2f1e9", brightness)
        lane_shadow = self.tone("#262b2e", "#73777b", brightness)

        self.canvas.create_rectangle(x - 5, y, x + w + 5, y + h, fill=shoulder, outline="")
        self.canvas.create_rectangle(x + 8, y, x + w - 8, y + h, fill=asphalt, outline="")

        texture_light = self.tone("#24272a", "#6d7072", brightness)
        texture_dark = self.tone("#101214", "#393c3f", brightness)
        for i in range(170):
            px = x + 12 + ((i * 37) % max(1, int(w - 24)))
            py = y + 8 + ((i * 83) % max(1, int(h - 16)))
            fill = texture_light if i % 3 == 0 else texture_dark
            self.canvas.create_line(px, py, px + 1, py, fill=fill, width=1)

        for lane_x in (x + w * 0.33, x + w * 0.67):
            self.canvas.create_line(lane_x + 1, y + 18, lane_x + 1, y + h - 18, fill=lane_shadow, width=3)
            dash_h = 28
            gap = 32
            dash_y = y + 24
            while dash_y < y + h - 24:
                self.canvas.create_line(lane_x, dash_y, lane_x, min(dash_y + dash_h, y + h - 24), fill=lane, width=6)
                dash_y += dash_h + gap

        edge = self.tone("#262a2d", "#9aa2a7", brightness)
        self.canvas.create_line(x + 8, y, x + 8, y + h, fill=edge, width=2)
        self.canvas.create_line(x + w - 8, y, x + w - 8, y + h, fill=edge, width=2)

    def draw_distance_line(self, cx, front_y, front_h, own_y, own_h, brightness):
        line_color = self.tone("#7f878a", "#d2d7d9", brightness)
        y0 = front_y + front_h * 0.50
        y1 = own_y - own_h * 0.49
        if y1 > y0:
            self.canvas.create_line(cx, y0, cx, y1, fill=line_color, width=2)
            self.canvas.create_oval(cx - 3, y0 - 3, cx + 3, y0 + 3, fill=line_color, outline="")
            self.canvas.create_oval(cx - 3, y1 - 3, cx + 3, y1 + 3, fill=line_color, outline="")

    def draw_car(self, cx, cy, w, h, night_color, day_color, brightness, label, front):
        body = self.tone(night_color, day_color, brightness)
        outline = self.tone("#06080a", "#34363a", brightness)
        glass = self.tone("#0c171f", "#426073", brightness)
        light = self.tone("#776831", "#fff3a5", brightness)
        tail = self.tone("#691d2c", "#ff5168", brightness)
        text = self.tone("#f3f8fb", "#ffffff", brightness) if not front else self.tone("#1f2428", "#202428", brightness)

        x0 = cx - w / 2
        x1 = cx + w / 2
        y0 = cy - h / 2
        y1 = cy + h / 2

        self.canvas.create_oval(cx - w * 0.47, y1 - h * 0.13, cx + w * 0.47, y1 + h * 0.05, fill="#141414", outline="")
        self.canvas.create_polygon(
            cx,
            y0,
            x1 - w * 0.10,
            y0 + h * 0.12,
            x1,
            y0 + h * 0.42,
            x1 - w * 0.10,
            y1 - h * 0.12,
            cx + w * 0.25,
            y1,
            cx - w * 0.25,
            y1,
            x0 + w * 0.10,
            y1 - h * 0.12,
            x0,
            y0 + h * 0.42,
            x0 + w * 0.10,
            y0 + h * 0.12,
            fill=body,
            outline=outline,
            width=2,
        )
        self.canvas.create_polygon(
            cx - w * 0.24,
            y0 + h * 0.20,
            cx + w * 0.24,
            y0 + h * 0.20,
            cx + w * 0.34,
            y0 + h * 0.40,
            cx - w * 0.34,
            y0 + h * 0.40,
            fill=glass,
            outline=outline,
            width=1,
        )
        self.canvas.create_polygon(
            cx - w * 0.27,
            y1 - h * 0.24,
            cx + w * 0.27,
            y1 - h * 0.24,
            cx + w * 0.20,
            y1 - h * 0.09,
            cx - w * 0.20,
            y1 - h * 0.09,
            fill=glass,
            outline=outline,
            width=1,
        )
        self.canvas.create_text(cx, cy + h * 0.05, fill=text, font=(FONT_KR, max(13, int(h * 0.15)), "bold"), text=label)

        self.canvas.create_rectangle(x0 + w * 0.14, y0 + h * 0.06, x0 + w * 0.34, y0 + h * 0.10, fill=light, outline="")
        self.canvas.create_rectangle(x1 - w * 0.34, y0 + h * 0.06, x1 - w * 0.14, y0 + h * 0.10, fill=light, outline="")
        self.canvas.create_rectangle(x0 + w * 0.14, y1 - h * 0.08, x0 + w * 0.34, y1 - h * 0.04, fill=tail, outline="")
        self.canvas.create_rectangle(x1 - w * 0.34, y1 - h * 0.08, x1 - w * 0.14, y1 - h * 0.04, fill=tail, outline="")

    def draw_ruler(self, x, far_y, close_y, brightness, distance_m):
        line = self.tone("#aeb5ba", "#f1f5f7", brightness)
        text = self.tone("#c8d0d5", "#ffffff", brightness)
        muted = self.tone("#6f7a82", "#c2c8cc", brightness)

        self.canvas.create_line(x, far_y - 24, x, close_y + 24, fill=line, width=2)
        minor = 0.5
        mark = math.ceil(self.args.min_distance / minor) * minor
        while mark <= self.args.max_distance + 0.001:
            t = (mark - self.args.min_distance) / (self.args.max_distance - self.args.min_distance)
            y = lerp(close_y, far_y, t)
            is_major = abs((mark / 2) - round(mark / 2)) < 0.01
            tick = 18 if is_major else 8
            self.canvas.create_line(x, y, x + tick, y, fill=line if is_major else muted, width=2 if is_major else 1)
            if is_major and mark >= 2.0:
                self.canvas.create_text(x + tick + 8, y, anchor="w", fill=text, font=(FONT_MONO, 11), text=f"{mark:.0f}m")
            mark += minor

        current_t = (distance_m - self.args.min_distance) / (self.args.max_distance - self.args.min_distance)
        current_y = lerp(close_y, far_y, current_t)
        self.canvas.create_line(x - 10, current_y, x + 28, current_y, fill=self.tone("#ffffff", "#ffffff", brightness), width=3)

    def draw_knob(self, cx, cy, radius, r_level, brightness):
        dial = self.tone("#20242a", "#5d6268", brightness)
        rim = self.tone("#aeb6bd", "#f0f2f3", brightness)
        mark = self.tone("#f6d365", "#ffe18b", brightness)
        text = self.tone("#d8dde1", "#ffffff", brightness)

        self.canvas.create_oval(cx - radius, cy - radius, cx + radius, cy + radius, fill=dial, outline=rim, width=2)
        for i in range(12):
            angle = math.radians(225 - i * 22)
            inner = radius * 0.72
            outer = radius * 0.88
            self.canvas.create_line(
                cx + math.cos(angle) * inner,
                cy - math.sin(angle) * inner,
                cx + math.cos(angle) * outer,
                cy - math.sin(angle) * outer,
                fill=rim,
                width=1,
            )

        angle = math.radians(225 - r_level * 240)
        self.canvas.create_line(cx, cy, cx + math.cos(angle) * radius * 0.62, cy - math.sin(angle) * radius * 0.62, fill=mark, width=3)
        self.canvas.create_oval(cx - 4, cy - 4, cx + 4, cy + 4, fill=mark, outline="")
        self.canvas.create_text(cx + radius + 8, cy + 9, anchor="w", fill=text, font=(FONT_KR, 10, "bold"), text="거리 조절")

    def draw_distance_readout(self, x, road_top, road_bottom, brightness, distance_m):
        text = self.tone("#e8edf1", "#ffffff", brightness)
        muted = self.tone("#a6b0b8", "#eef2f4", brightness)
        y = road_top + (road_bottom - road_top) * 0.45
        self.canvas.create_text(x, y, anchor="w", fill=muted, font=(FONT_KR, 13, "bold"), text="거리")
        self.canvas.create_text(x, y + 34, anchor="w", fill=text, font=(FONT_KR, 23, "bold"), text=f"{distance_m:.1f}m")

    def draw_status_readout(self, x, y, brightness, color, status, risk, light, resistance, stale):
        text = self.tone("#e8edf1", "#ffffff", brightness)
        muted = self.tone("#a8b2ba", "#eef2f4", brightness)
        status_text = {"SAFE": "안전", "WARNING": "주의", "DANGER": "위험", "WAIT": "대기"}.get(status, status)
        if stale:
            status_text = "대기"

        self.canvas.create_text(x, y, anchor="w", fill=muted, font=(FONT_KR, 11, "bold"), text="상태")
        self.canvas.create_text(x, y + 25, anchor="w", fill=color, font=(FONT_KR, 18, "bold"), text=status_text)
        self.canvas.create_text(x, y + 56, anchor="w", fill=text, font=(FONT_MONO, 11), text=f"L {light:4d}")
        self.canvas.create_text(x, y + 78, anchor="w", fill=text, font=(FONT_MONO, 11), text=f"R {resistance:4d}")
        self.canvas.create_text(x, y + 104, anchor="w", fill=muted, font=(FONT_KR, 10), text=self.connection_text)
        if risk:
            self.canvas.create_text(x, y + 124, anchor="w", fill=muted, font=(FONT_MONO, 10), text=f"C {risk:3d}%")

    def draw_error(self, width, height, brightness, message):
        box_w = min(width - 64, 680)
        box_h = 50
        x0 = (width - box_w) / 2
        y0 = height - 72
        bg = self.tone("#33151d", "#fff0f3", brightness)
        fg = self.tone("#ffdce4", "#8a1d31", brightness)
        border = self.tone("#cc4760", "#cc4760", brightness)
        self.canvas.create_rectangle(x0, y0, x0 + box_w, y0 + box_h, fill=bg, outline=border, width=2)
        self.canvas.create_text(
            x0 + 16,
            y0 + box_h / 2,
            anchor="w",
            fill=fg,
            font=(FONT_KR, 11, "bold"),
            text=self.truncate_text(message, max(28, int(box_w / 9))),
        )

    @staticmethod
    def truncate_text(text, limit):
        if len(text) <= limit:
            return text
        return text[: max(0, limit - 3)] + "..."

    def on_close(self):
        self.disconnect_reader()
        self.destroy()


def build_parser():
    parser = argparse.ArgumentParser(description="STM32F103 실시간 주행 정보 디스플레이")
    parser.add_argument("--port", help="예: COM3")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD, help="기본값: 9600")
    parser.add_argument("--simulate", action="store_true", help="보드 없이 GUI 동작 확인")
    parser.add_argument("--invert-r", action="store_true", help="R 값이 커질수록 멀어지는 배선이면 사용")
    parser.add_argument("--invert-light", action="store_true", help="CDS L 값이 커질수록 밝아지는 배선이면 사용")
    parser.add_argument("--min-distance", type=float, default=MIN_DISTANCE_M, help="R 최대값일 때 표시 거리")
    parser.add_argument("--max-distance", type=float, default=MAX_DISTANCE_M, help="R 최소값일 때 표시 거리")
    return parser


def main():
    args = build_parser().parse_args()
    if args.max_distance <= args.min_distance:
        raise SystemExit("--max-distance는 --min-distance보다 커야 합니다.")
    app = DrivingDashboard(args)
    app.mainloop()


if __name__ == "__main__":
    main()
