"""
Advanced ADAS/HUD demo for STM32 UART data.

Install:
    python -m pip install PySide6 pyserial

Run demo mode:
    python advanced_adas_hud.py --demo

Run with STM32 UART:
    python advanced_adas_hud.py --port COM3 --baud 9600

Expected UART line:
    DIST=2300,LIGHT=1800

Keyboard:
    F11  toggle fullscreen
    R    toggle demo auto-motion
    Esc  exit fullscreen, then quit
"""

from __future__ import annotations

import argparse
import math
import queue
import random
import re
import sys
import threading
import time
from dataclasses import dataclass
from typing import Dict, Optional, Tuple

try:
    from PySide6.QtCore import QPointF, QRectF, QSize, Qt, QTimer
    from PySide6.QtGui import (
        QColor,
        QConicalGradient,
        QFont,
        QFontDatabase,
        QGuiApplication,
        QLinearGradient,
        QPainter,
        QPainterPath,
        QPen,
        QRadialGradient,
    )
    from PySide6.QtWidgets import QApplication, QMainWindow, QWidget

    QT_API = "PySide6"
except ImportError:
    try:
        from PyQt5.QtCore import QPointF, QRectF, QSize, Qt, QTimer
        from PyQt5.QtGui import (
            QColor,
            QConicalGradient,
            QFont,
            QFontDatabase,
            QGuiApplication,
            QLinearGradient,
            QPainter,
            QPainterPath,
            QPen,
            QRadialGradient,
        )
        from PyQt5.QtWidgets import QApplication, QMainWindow, QWidget

        QT_API = "PyQt5"
    except ImportError as exc:
        raise SystemExit(
            "PySide6 또는 PyQt5가 필요합니다.\n"
            "설치: python -m pip install PySide6 pyserial"
        ) from exc

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    serial = None
    list_ports = None


ADC_MAX = 4095.0
DEFAULT_BAUD = 9600
DIST_DANGER_MM = 650.0
DIST_SAFE_MM = 5200.0
LIGHT_DARK_ADC = 320.0
LIGHT_SAFE_ADC = 2900.0
STALE_AFTER_SECONDS = 2.0

FIELD_RE = re.compile(
    r"\b(?P<key>DIST|DISTANCE|D|R|LIGHT|L)\s*[:=]\s*(?P<value>-?\d+(?:\.\d+)?)",
    re.IGNORECASE,
)


def clamp(value: float, low: float, high: float) -> float:
    return max(low, min(high, value))


def lerp(a: float, b: float, t: float) -> float:
    return a + (b - a) * t


def smooth(current: float, target: float, speed: float, dt: float) -> float:
    amount = 1.0 - math.exp(-speed * dt)
    return lerp(current, target, amount)


def ease_out_cubic(t: float) -> float:
    t = clamp(t, 0.0, 1.0)
    return 1.0 - pow(1.0 - t, 3.0)


def inverse_lerp(a: float, b: float, value: float) -> float:
    if abs(b - a) < 1e-6:
        return 0.0
    return clamp((value - a) / (b - a), 0.0, 1.0)


def color(hex_value: str, alpha: int = 255) -> QColor:
    c = QColor(hex_value)
    c.setAlpha(int(clamp(alpha, 0, 255)))
    return c


def mix_color(a: QColor, b: QColor, t: float, alpha: Optional[int] = None) -> QColor:
    t = clamp(t, 0.0, 1.0)
    mixed = QColor(
        int(lerp(a.red(), b.red(), t)),
        int(lerp(a.green(), b.green(), t)),
        int(lerp(a.blue(), b.blue(), t)),
        int(lerp(a.alpha(), b.alpha(), t)) if alpha is None else alpha,
    )
    return mixed


def risk_from_distance(distance_mm: float) -> float:
    t = inverse_lerp(DIST_DANGER_MM, DIST_SAFE_MM, distance_mm)
    return clamp((1.0 - t) * 100.0, 0.0, 100.0)


def risk_from_light(light_adc: float) -> float:
    t = inverse_lerp(LIGHT_DARK_ADC, LIGHT_SAFE_ADC, light_adc)
    return clamp((1.0 - t) * 100.0, 0.0, 100.0)


def combined_risk(distance_mm: float, light_adc: float) -> Tuple[float, float, float]:
    r = risk_from_distance(distance_mm)
    l = risk_from_light(light_adc)
    c = clamp((r * 0.7) + (l * 0.3), 0.0, 100.0)
    return r, l, c


@dataclass
class SensorFrame:
    distance_mm: float
    light_adc: float
    raw_line: str = "DEMO"
    received_at: float = 0.0

    @property
    def distance_m(self) -> float:
        return self.distance_mm / 1000.0

    @property
    def risks(self) -> Tuple[float, float, float]:
        return combined_risk(self.distance_mm, self.light_adc)


def parse_sensor_line(line: str) -> Optional[SensorFrame]:
    fields: Dict[str, float] = {}
    for match in FIELD_RE.finditer(line):
        key = match.group("key").upper()
        value = float(match.group("value"))
        fields[key] = value

    distance = None
    for key in ("DIST", "DISTANCE", "D", "R"):
        if key in fields:
            distance = fields[key]
            break

    light = None
    for key in ("LIGHT", "L"):
        if key in fields:
            light = fields[key]
            break

    if distance is None or light is None:
        return None

    # STM32에서 받은 R 값을 5000-R 로 변환
    distance = 5000.0 - distance

    # 범위를 벗어나지 않도록 제한
    distance = clamp(distance, 0.0, 5000.0)

    return SensorFrame(
    distance_mm=distance,
    light_adc=clamp(light, 0.0, ADC_MAX),
    raw_line=line,
    received_at=time.time(),
)

class SerialReader(threading.Thread):
    def __init__(self, port: str, baud: int, events: "queue.Queue[Tuple[str, object]]", stop_event: threading.Event):
        super().__init__(daemon=True)
        self.port = port
        self.baud = baud
        self.events = events
        self.stop_event = stop_event
        self.device = None

    def run(self) -> None:
        if serial is None:
            self.events.put(("error", "pyserial is not installed"))
            return

        try:
            self.device = serial.Serial(
                self.port,
                self.baud,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.12,
            )
            self.device.reset_input_buffer()
            self.events.put(("connected", self.port))

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
            self.events.put(("disconnected", self.port))


class HudState:
    def __init__(self) -> None:
        self.distance_mm = 2300.0
        self.light_adc = 1800.0
        self.r_risk = 0.0
        self.l_risk = 0.0
        self.c_risk = 0.0
        self.speed = 92.0
        self.target_distance_mm = 2300.0
        self.target_light_adc = 1800.0
        self.connection = "DEMO"
        self.raw_line = "DIST=2300,LIGHT=1800"
        self.last_rx = 0.0
        self.message = "DEMO MODE"

    def set_sample(self, frame: SensorFrame, connection: str) -> None:
        self.target_distance_mm = frame.distance_mm
        self.target_light_adc = frame.light_adc
        self.raw_line = frame.raw_line
        self.last_rx = frame.received_at or time.time()
        self.connection = connection

    def tick(self, dt: float, phase: float) -> None:
        self.distance_mm = smooth(self.distance_mm, self.target_distance_mm, 8.5, dt)
        self.light_adc = smooth(self.light_adc, self.target_light_adc, 6.5, dt)
        target_r, target_l, target_c = combined_risk(self.distance_mm, self.light_adc)
        self.r_risk = smooth(self.r_risk, target_r, 9.0, dt)
        self.l_risk = smooth(self.l_risk, target_l, 9.0, dt)
        self.c_risk = smooth(self.c_risk, target_c, 7.5, dt)

        cruise = 107.0 - self.c_risk * 0.46
        cruise += math.sin(phase * 0.8) * 1.8
        if self.c_risk > 72:
            cruise -= 8.0 * math.sin(phase * 12.0) ** 2
        self.speed = smooth(self.speed, clamp(cruise, 38.0, 112.0), 3.0, dt)


class HudWidget(QWidget):
    def __init__(self, args: argparse.Namespace):
        super().__init__()
        self.args = args
        self.setMinimumSize(QSize(960, 560))
        self.setMouseTracking(True)
        self.setFocusPolicy(Qt.StrongFocus)

        self.state = HudState()
        self.events: "queue.Queue[Tuple[str, object]]" = queue.Queue()
        self.stop_event: Optional[threading.Event] = None
        self.reader: Optional[SerialReader] = None
        self.serial_connected = False
        self.demo_mode = True
        self.demo_auto = True
        self.active_slider: Optional[str] = None
        self.slider_hitboxes: Dict[str, QRectF] = {}
        self.button_hitboxes: Dict[str, QRectF] = {}
        self.demo_distance_mm = 2300.0
        self.demo_light_adc = 1800.0
        self.phase = 0.0
        self.last_tick = time.perf_counter()
        self.flash = 0.0
        self.scan_offset = 0.0
        self.frame_count = 0
        self.font_family = self.pick_font_family()

        self.timer = QTimer(self)
        self.timer.timeout.connect(self.on_tick)
        self.timer.start(16)

        self.startup_connection()

    def startup_connection(self) -> None:
        if self.args.demo:
            self.start_demo("DEMO MODE")
            return

        port = self.args.port or self.first_serial_port()
        if port:
            self.start_serial(port, self.args.baud)
        else:
            self.start_demo("NO UART - DEMO MODE")

    def first_serial_port(self) -> Optional[str]:
        if list_ports is None:
            return None
        ports = [port.device for port in list_ports.comports()]
        return ports[0] if ports else None

    def start_serial(self, port: str, baud: int) -> None:
        self.stop_reader()
        self.demo_mode = False
        self.serial_connected = False
        self.state.connection = f"UART {port}"
        self.state.message = f"CONNECTING {port}"
        self.stop_event = threading.Event()
        self.reader = SerialReader(port, baud, self.events, self.stop_event)
        self.reader.start()

    def start_demo(self, message: str = "DEMO MODE") -> None:
        self.stop_reader()
        self.demo_mode = True
        self.serial_connected = False
        self.state.connection = "DEMO"
        self.state.message = message
        self.demo_distance_mm = self.state.target_distance_mm
        self.demo_light_adc = self.state.target_light_adc

    def stop_reader(self) -> None:
        if self.stop_event is not None:
            self.stop_event.set()
        self.reader = None
        self.stop_event = None

    def closeEvent(self, event) -> None:  # noqa: N802 - Qt naming convention
        self.stop_reader()
        event.accept()

    def on_tick(self) -> None:
        now = time.perf_counter()
        dt = clamp(now - self.last_tick, 0.001, 0.05)
        self.last_tick = now
        self.phase += dt
        self.scan_offset = (self.scan_offset + dt * 0.55) % 1.0
        self.frame_count += 1

        self.process_events()
        if self.demo_mode:
            self.update_demo_targets(dt)
        elif self.serial_connected and time.time() - self.state.last_rx > STALE_AFTER_SECONDS:
            self.state.message = "WAITING FOR UART DATA"

        self.state.tick(dt, self.phase)
        self.flash = smooth(self.flash, 1.0 if self.state.c_risk >= 72.0 else 0.0, 5.5, dt)
        self.update()

    def process_events(self) -> None:
        while True:
            try:
                kind, payload = self.events.get_nowait()
            except queue.Empty:
                break

            if kind == "connected":
                self.serial_connected = True
                self.state.connection = f"UART {payload}"
                self.state.message = "UART LIVE"
            elif kind == "disconnected":
                if not self.demo_mode:
                    self.serial_connected = False
            elif kind == "error":
                self.start_demo(f"UART ERROR - {payload}")
            elif kind == "line":
                frame = parse_sensor_line(str(payload))
                if frame is not None:
                    self.state.set_sample(frame, self.state.connection)
                    self.state.message = "UART LIVE" if not self.demo_mode else "DEMO MODE"

    def update_demo_targets(self, dt: float) -> None:
        if self.demo_auto:
            risk_wave = (math.sin(self.phase * 0.55) + 1.0) * 0.5
            pulse = max(0.0, math.sin(self.phase * 0.23 - 0.7)) ** 8
            near_event = max(0.0, math.sin(self.phase * 0.83 + 1.8)) ** 10

            target_distance = 5100.0 - risk_wave * 2100.0 - pulse * 2450.0 - near_event * 1400.0
            target_distance += math.sin(self.phase * 2.1) * 80.0
            target_light = 3150.0 - (math.sin(self.phase * 0.38 + 1.2) + 1.0) * 980.0
            target_light -= max(0.0, math.sin(self.phase * 0.52 - 1.1)) ** 4 * 1450.0
            target_light += random.uniform(-12.0, 12.0)

            self.demo_distance_mm = clamp(target_distance, 520.0, 6400.0)
            self.demo_light_adc = clamp(target_light, 40.0, ADC_MAX)

        line = f"DIST={int(self.demo_distance_mm)},LIGHT={int(self.demo_light_adc)}"
        frame = SensorFrame(self.demo_distance_mm, self.demo_light_adc, line, time.time())
        self.state.set_sample(frame, "DEMO")
        self.state.message = "DEMO AUTO" if self.demo_auto else "DEMO MANUAL"

    def paintEvent(self, _event) -> None:  # noqa: N802 - Qt naming convention
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing, True)
        painter.setRenderHint(QPainter.TextAntialiasing, True)
        w = self.width()
        h = self.height()

        self.draw_background(painter, w, h)
        self.draw_horizon(painter, w, h)
        self.draw_road(painter, w, h)
        self.draw_traffic(painter, w, h)
        self.draw_vehicle(painter, w, h)
        self.draw_warning_overlay(painter, w, h)
        self.draw_top_status(painter, w, h)
        self.draw_speed_cluster(painter, w, h)
        self.draw_side_metrics(painter, w, h)
        self.draw_demo_controls(painter, w, h)
        self.draw_scanlines(painter, w, h)

        painter.end()

    def draw_background(self, p: QPainter, w: int, h: int) -> None:
        bg = QLinearGradient(0, 0, 0, h)
        bg.setColorAt(0.0, color("#061115"))
        bg.setColorAt(0.38, color("#0b1a1f"))
        bg.setColorAt(1.0, color("#03070a"))
        p.fillRect(0, 0, w, h, bg)

        center = QPointF(w * 0.5, h * 0.58)
        radial = QRadialGradient(center, max(w, h) * 0.66)
        radial.setColorAt(0.0, color("#1cc7d8", 40))
        radial.setColorAt(0.55, color("#06252c", 24))
        radial.setColorAt(1.0, color("#000000", 0))
        p.fillRect(0, 0, w, h, radial)

        vignette = QRadialGradient(QPointF(w * 0.5, h * 0.55), max(w, h) * 0.82)
        vignette.setColorAt(0.0, color("#000000", 0))
        vignette.setColorAt(0.72, color("#000000", 15))
        vignette.setColorAt(1.0, color("#000000", 180))
        p.fillRect(0, 0, w, h, vignette)

        p.save()
        p.setPen(QPen(color("#53f3ff", 16), 1.0))
        grid_y = h * 0.36
        for i in range(13):
            y = grid_y + (i / 12.0) ** 1.9 * h * 0.52
            p.drawLine(QPointF(w * 0.03, y), QPointF(w * 0.97, y))
        for i in range(-8, 9):
            x0 = w * 0.5 + i * w * 0.06
            p.drawLine(QPointF(x0, h * 0.37), QPointF(w * 0.5 + i * w * 0.17, h))
        p.restore()

    def draw_horizon(self, p: QPainter, w: int, h: int) -> None:
        y = h * 0.34
        p.save()
        p.setPen(QPen(color("#bdfaff", 85), 1.0))
        p.drawLine(QPointF(w * 0.08, y), QPointF(w * 0.92, y))

        for i in range(9):
            x = w * (0.13 + i * 0.095)
            p.setPen(QPen(color("#8df7ff", 22 + i % 3 * 12), 1.0))
            p.drawLine(QPointF(x, y - 14), QPointF(x + w * 0.015, y))

        arc_rect = QRectF(w * 0.34, h * 0.14, w * 0.32, h * 0.28)
        p.setPen(QPen(color("#5ff5ff", 34), 1.4))
        p.drawArc(arc_rect, 20 * 16, 140 * 16)
        p.restore()

    def road_point(self, w: int, h: int, side: float, t: float, curve: float = 0.0) -> QPointF:
        t = clamp(t, 0.0, 1.0)
        horizon = QPointF(w * 0.5, h * 0.35)
        road_width_top = w * 0.09
        road_width_bottom = w * 0.70
        y = lerp(horizon.y(), h * 1.02, t)
        road_half = lerp(road_width_top, road_width_bottom, ease_out_cubic(t)) * 0.5
        center_shift = math.sin(t * math.pi) * curve * w * 0.07
        return QPointF(horizon.x() + center_shift + side * road_half, y)

    def draw_road(self, p: QPainter, w: int, h: int) -> None:
        risk_t = self.state.c_risk / 100.0
        curve = math.sin(self.phase * 0.34) * 0.42
        left_top = self.road_point(w, h, -1.0, 0.0, curve)
        right_top = self.road_point(w, h, 1.0, 0.0, curve)
        right_bottom = self.road_point(w, h, 1.0, 1.0, curve)
        left_bottom = self.road_point(w, h, -1.0, 1.0, curve)

        road = QPainterPath()
        road.moveTo(left_top)
        road.lineTo(right_top)
        road.lineTo(right_bottom)
        road.lineTo(left_bottom)
        road.closeSubpath()

        road_fill = QLinearGradient(0, h * 0.33, 0, h)
        road_fill.setColorAt(0.0, color("#0d242b", 145))
        road_fill.setColorAt(0.55, color("#0b151a", 215))
        road_fill.setColorAt(1.0, color("#030609", 245))
        p.fillPath(road, road_fill)

        p.save()
        p.setClipPath(road)
        for i in range(18):
            t = ((i / 17.0) + self.scan_offset * 0.12) % 1.0
            y_alpha = int(16 + ease_out_cubic(t) * 44)
            a = self.road_point(w, h, -0.92, t, curve)
            b = self.road_point(w, h, 0.92, t, curve)
            p.setPen(QPen(color("#78eaff", y_alpha), 1.0 + t * 1.3))
            p.drawLine(a, b)
        p.restore()

        lane_color = mix_color(color("#e9ffff"), color("#ff3146"), max(0.0, (risk_t - 0.66) / 0.34))
        assist_color = mix_color(color("#50f7ff"), color("#ff293e"), max(0.0, (risk_t - 0.58) / 0.42))

        self.draw_glow_polyline(p, [left_top, left_bottom], lane_color, 2.2, 75)
        self.draw_glow_polyline(p, [right_top, right_bottom], lane_color, 2.2, 75)

        for side in (-0.36, 0.36):
            points = [self.road_point(w, h, side, t / 12.0, curve) for t in range(13)]
            self.draw_glow_polyline(p, points, assist_color, 3.2, 95)

        p.save()
        p.setPen(QPen(color("#bffcff", 105), 2.0, Qt.DashLine, Qt.RoundCap))
        dash_points = [self.road_point(w, h, 0.0, t / 10.0, curve) for t in range(11)]
        for a, b in zip(dash_points[1::2], dash_points[2::2]):
            p.drawLine(a, b)
        p.restore()

        if self.state.c_risk > 62:
            p.save()
            alert_path = QPainterPath()
            t_near = inverse_lerp(100.0, 70.0, clamp(self.state.c_risk, 70.0, 100.0))
            top_t = lerp(0.54, 0.33, t_near)
            left = self.road_point(w, h, -0.5, top_t, curve)
            right = self.road_point(w, h, 0.5, top_t, curve)
            bottom_right = self.road_point(w, h, 0.72, 0.98, curve)
            bottom_left = self.road_point(w, h, -0.72, 0.98, curve)
            alert_path.moveTo(left)
            alert_path.lineTo(right)
            alert_path.lineTo(bottom_right)
            alert_path.lineTo(bottom_left)
            alert_path.closeSubpath()
            pulse = (math.sin(self.phase * 9.0) + 1.0) * 0.5
            grad = QLinearGradient(0, h * 0.35, 0, h)
            grad.setColorAt(0.0, color("#ff233c", int(14 + pulse * 22)))
            grad.setColorAt(1.0, color("#ff233c", int(85 + pulse * 80)))
            p.fillPath(alert_path, grad)
            p.restore()

    def draw_traffic(self, p: QPainter, w: int, h: int) -> None:
        risk_t = self.state.c_risk / 100.0
        distance_t = inverse_lerp(DIST_DANGER_MM, DIST_SAFE_MM, self.state.distance_mm)
        lead_t = lerp(0.69, 0.19, distance_t)
        lead_y = lerp(h * 0.36, h * 0.80, lead_t)
        lead_scale = lerp(0.52, 1.22, lead_t)
        lead_x = w * 0.5 + math.sin(self.phase * 0.54) * w * 0.012

        for side, alpha, y_t in [(-0.82, 40, 0.28), (0.78, 34, 0.24), (-0.58, 30, 0.16)]:
            car_pos = self.road_point(w, h, side, y_t + math.sin(self.phase * 0.2 + side) * 0.02, 0.0)
            self.draw_car_icon(p, car_pos.x(), car_pos.y(), w * 0.038, h * 0.052, 0.58, color("#83f4ff", alpha), ghost=True)

        beam = QPainterPath()
        beam.moveTo(w * 0.5, h * 0.82)
        beam.lineTo(lead_x - w * 0.06 * lead_scale, lead_y + h * 0.04 * lead_scale)
        beam.lineTo(lead_x + w * 0.06 * lead_scale, lead_y + h * 0.04 * lead_scale)
        beam.closeSubpath()
        grad = QLinearGradient(0, h * 0.42, 0, h * 0.84)
        grad.setColorAt(0.0, color("#54f7ff", 0))
        grad.setColorAt(1.0, color("#54f7ff", int(18 + risk_t * 55)))
        p.fillPath(beam, grad)

        car_color = mix_color(color("#eaffff"), color("#ff3348"), max(0.0, (risk_t - 0.55) / 0.45))
        self.draw_car_icon(
            p,
            lead_x,
            lead_y,
            w * 0.062 * lead_scale,
            h * 0.078 * lead_scale,
            1.0,
            car_color,
            ghost=False,
        )

        if self.state.c_risk > 56:
            pulse = (math.sin(self.phase * 10.0) + 1.0) * 0.5
            p.save()
            p.setPen(QPen(color("#ff3146", int(90 + 90 * pulse)), 2.0 + pulse * 2.0))
            for i in range(3):
                radius = (w * 0.055 + i * w * 0.018) * lead_scale * (1.0 + pulse * 0.08)
                p.drawEllipse(QPointF(lead_x, lead_y + h * 0.025 * lead_scale), radius, radius * 0.42)
            p.restore()

        p.save()
        p.setPen(QPen(color("#d9ffff", 130), 1.2))
        label = f"{self.state.distance_mm/ 1000:.1f} m"
        label_rect = QRectF(lead_x - w * 0.07, lead_y + h * 0.068 * lead_scale, w * 0.14, 26)
        self.draw_glow_text(p, label_rect, label, 16, color("#d9ffff"), color("#36f4ff", 80), Qt.AlignCenter)
        p.restore()

    def draw_vehicle(self, p: QPainter, w: int, h: int) -> None:
        x = w * 0.5
        y = h * 0.82
        body_w = w * 0.132
        body_h = h * 0.155

        p.save()
        shadow = QRadialGradient(QPointF(x, y + body_h * 0.34), body_w * 0.85)
        shadow.setColorAt(0.0, color("#3af7ff", 48))
        shadow.setColorAt(0.75, color("#1ad7e7", 12))
        shadow.setColorAt(1.0, color("#000000", 0))
        p.setBrush(shadow)
        p.setPen(Qt.NoPen)
        p.drawEllipse(QPointF(x, y + body_h * 0.33), body_w * 0.78, body_h * 0.34)
        p.restore()

        self.draw_car_icon(p, x, y, body_w, body_h, 1.25, color("#f5ffff"), ghost=False, own=True)

        p.save()
        p.setPen(QPen(color("#56f6ff", 170), 1.6))
        for side in (-1, 1):
            path = QPainterPath()
            path.moveTo(x + side * body_w * 0.38, y + body_h * 0.16)
            path.cubicTo(
                x + side * body_w * 0.9,
                y + body_h * 0.34,
                x + side * body_w * 1.18,
                y + body_h * 0.64,
                x + side * body_w * 1.5,
                y + body_h * 0.86,
            )
            p.drawPath(path)
        p.restore()

    def draw_car_icon(
        self,
        p: QPainter,
        cx: float,
        cy: float,
        width: float,
        height: float,
        scale: float,
        main_color: QColor,
        ghost: bool = False,
        own: bool = False,
    ) -> None:
        w = width * scale
        h = height * scale
        body = QPainterPath()
        body.moveTo(cx - w * 0.34, cy + h * 0.42)
        body.cubicTo(cx - w * 0.47, cy + h * 0.24, cx - w * 0.43, cy - h * 0.18, cx - w * 0.22, cy - h * 0.36)
        body.lineTo(cx + w * 0.22, cy - h * 0.36)
        body.cubicTo(cx + w * 0.43, cy - h * 0.18, cx + w * 0.47, cy + h * 0.24, cx + w * 0.34, cy + h * 0.42)
        body.closeSubpath()

        p.save()
        if ghost:
            p.setPen(QPen(main_color, 1.4))
            p.setBrush(color("#4ff7ff", main_color.alpha() // 5))
            p.drawPath(body)
            p.restore()
            return

        glow = QColor(main_color)
        glow.setAlpha(70 if not own else 105)
        for i in range(4, 0, -1):
            p.setPen(QPen(glow, i * 2.2))
            p.setBrush(Qt.NoBrush)
            p.drawPath(body)

        body_grad = QLinearGradient(cx, cy - h * 0.42, cx, cy + h * 0.45)
        body_grad.setColorAt(0.0, color("#ffffff", 240))
        body_grad.setColorAt(0.52, QColor(main_color))
        body_grad.setColorAt(1.0, color("#2c5962", 245))
        p.setPen(QPen(color("#dcffff", 230), 1.4))
        p.setBrush(body_grad)
        p.drawPath(body)

        cabin = QPainterPath()
        cabin.moveTo(cx - w * 0.21, cy - h * 0.22)
        cabin.cubicTo(cx - w * 0.16, cy - h * 0.38, cx + w * 0.16, cy - h * 0.38, cx + w * 0.21, cy - h * 0.22)
        cabin.lineTo(cx + w * 0.16, cy + h * 0.02)
        cabin.lineTo(cx - w * 0.16, cy + h * 0.02)
        cabin.closeSubpath()
        glass = QLinearGradient(cx, cy - h * 0.32, cx, cy + h * 0.04)
        glass.setColorAt(0.0, color("#efffff", 210))
        glass.setColorAt(1.0, color("#25d8ff", 170))
        p.setPen(QPen(color("#eaffff", 180), 1.0))
        p.setBrush(glass)
        p.drawPath(cabin)

        p.setPen(QPen(color("#0a1014", 120), 2.2))
        p.drawLine(QPointF(cx - w * 0.29, cy + h * 0.28), QPointF(cx - w * 0.14, cy + h * 0.27))
        p.drawLine(QPointF(cx + w * 0.29, cy + h * 0.28), QPointF(cx + w * 0.14, cy + h * 0.27))
        p.setPen(QPen(color("#ff4054", 220), 2.5))
        p.drawLine(QPointF(cx - w * 0.31, cy + h * 0.35), QPointF(cx - w * 0.18, cy + h * 0.37))
        p.drawLine(QPointF(cx + w * 0.31, cy + h * 0.35), QPointF(cx + w * 0.18, cy + h * 0.37))
        p.restore()

    def draw_warning_overlay(self, p: QPainter, w: int, h: int) -> None:
        if self.flash < 0.01:
            return

        pulse = (math.sin(self.phase * 11.0) + 1.0) * 0.5
        alpha = int(self.flash * (38 + pulse * 48))
        p.save()
        border = QLinearGradient(0, 0, w, h)
        border.setColorAt(0.0, color("#ff1f39", alpha))
        border.setColorAt(0.5, color("#ff6675", int(alpha * 0.35)))
        border.setColorAt(1.0, color("#ff1f39", alpha))
        p.setPen(QPen(color("#ff3348", int(120 * self.flash)), 3.0 + pulse * 2.0))
        p.setBrush(Qt.NoBrush)
        inset = 8 + pulse * 6
        p.drawRoundedRect(QRectF(inset, inset, w - inset * 2, h - inset * 2), 18, 18)
        p.fillRect(0, 0, w, h, color("#ff1832", int(15 * self.flash)))

        alert_w = min(w * 0.42, 460.0)
        rect = QRectF((w - alert_w) * 0.5, h * 0.075, alert_w, h * 0.086)
        path = QPainterPath()
        path.addRoundedRect(rect, 10, 10)
        grad = QLinearGradient(rect.left(), rect.top(), rect.right(), rect.bottom())
        grad.setColorAt(0.0, color("#4d050b", int(165 * self.flash)))
        grad.setColorAt(0.5, color("#ff263e", int(190 * self.flash)))
        grad.setColorAt(1.0, color("#4d050b", int(165 * self.flash)))
        p.fillPath(path, grad)
        p.setPen(QPen(color("#ffd6db", int(230 * self.flash)), 1.2))
        p.drawPath(path)

        icon_x = rect.left() + rect.height() * 0.58
        icon_y = rect.center().y()
        self.draw_warning_triangle(p, icon_x, icon_y, rect.height() * 0.34, self.flash)
        text_rect = QRectF(rect.left() + rect.height(), rect.top(), rect.width() - rect.height() * 1.2, rect.height())
        self.draw_glow_text(
            p,
            text_rect,
            "COLLISION RISK  |  BUZZER ON",
            max(15, int(h * 0.024)),
            color("#fff7f8", int(245 * self.flash)),
            color("#ff243d", int(200 * self.flash)),
            Qt.AlignCenter,
            bold=True,
        )
        p.restore()

    def draw_warning_triangle(self, p: QPainter, cx: float, cy: float, radius: float, opacity: float) -> None:
        p.save()
        tri = QPainterPath()
        tri.moveTo(cx, cy - radius)
        tri.lineTo(cx + radius * 0.9, cy + radius * 0.72)
        tri.lineTo(cx - radius * 0.9, cy + radius * 0.72)
        tri.closeSubpath()
        p.setBrush(color("#ffcc6b", int(215 * opacity)))
        p.setPen(QPen(color("#fff3c8", int(245 * opacity)), 1.2))
        p.drawPath(tri)
        p.setPen(QPen(color("#501015", int(230 * opacity)), 2.2))
        p.drawLine(QPointF(cx, cy - radius * 0.42), QPointF(cx, cy + radius * 0.2))
        p.drawPoint(QPointF(cx, cy + radius * 0.45))
        p.restore()

    def draw_top_status(self, p: QPainter, w: int, h: int) -> None:
        top_y = h * 0.035
        risk_t = self.state.c_risk / 100.0
        accent = mix_color(color("#56f7ff"), color("#ff3348"), max(0.0, (risk_t - 0.58) / 0.42))

        p.save()
        self.draw_chip(p, QRectF(w * 0.07, top_y, w * 0.12, 38), "HDA", f"{int(100 - self.state.c_risk * 0.28)}%", accent)
        self.draw_chip(p, QRectF(w * 0.205, top_y, w * 0.14, 38), "AUTO", "ACTIVE", color("#9aff7a"))
        self.draw_chip(p, QRectF(w * 0.72, top_y, w * 0.21, 38), self.state.connection, self.state.message, accent)

        cx = w * 0.5
        gy = top_y + 18
        p.setPen(QPen(color("#dffeff", 170), 1.4))
        p.setBrush(Qt.NoBrush)
        p.drawEllipse(QPointF(cx, gy), 17, 17)
        p.drawLine(QPointF(cx - 10, gy), QPointF(cx + 10, gy))
        p.drawLine(QPointF(cx, gy - 10), QPointF(cx, gy + 10))
        p.setPen(QPen(accent, 3.0, Qt.SolidLine, Qt.RoundCap))
        p.drawArc(QRectF(cx - 28, gy - 28, 56, 56), 35 * 16, int((110 - risk_t * 45) * 16))
        self.draw_glow_text(
            p,
            QRectF(cx - 100, top_y + 28, 200, 28),
            "LANE KEEP",
            13,
            color("#cfffff", 180),
            color("#36f4ff", 80),
            Qt.AlignCenter,
        )
        p.restore()

    def draw_chip(self, p: QPainter, rect: QRectF, title: str, value: str, accent: QColor) -> None:
        p.save()
        path = QPainterPath()
        path.addRoundedRect(rect, 8, 8)
        grad = QLinearGradient(rect.left(), rect.top(), rect.right(), rect.bottom())
        grad.setColorAt(0.0, color("#071115", 188))
        grad.setColorAt(1.0, color("#0b2830", 142))
        p.fillPath(path, grad)
        p.setPen(QPen(QColor(accent.red(), accent.green(), accent.blue(), 120), 1.2))
        p.drawPath(path)

        self.draw_glow_text(
            p,
            QRectF(rect.left() + 12, rect.top() + 4, rect.width() * 0.44, rect.height() - 8),
            title,
            15,
            QColor(accent),
            QColor(accent.red(), accent.green(), accent.blue(), 75),
            Qt.AlignVCenter | Qt.AlignLeft,
            bold=True,
        )
        self.draw_text(
            p,
            QRectF(rect.left() + rect.width() * 0.45, rect.top() + 5, rect.width() * 0.5, rect.height() - 10),
            value,
            11,
            color("#d9ffff", 185),
            Qt.AlignVCenter | Qt.AlignRight,
        )
        p.restore()

    def draw_speed_cluster(self, p: QPainter, w: int, h: int) -> None:
        cx = w * 0.5
        cy = h * 0.91
        radius = min(w, h) * 0.19
        risk_t = self.state.c_risk / 100.0
        accent = mix_color(color("#5cf8ff"), color("#ff3348"), max(0.0, (risk_t - 0.58) / 0.42))

        p.save()
        arc_bg = QRectF(cx - radius, cy - radius, radius * 2, radius * 2)
        p.setPen(QPen(color("#cfffff", 34), 8.0, Qt.SolidLine, Qt.RoundCap))
        p.drawArc(arc_bg, 205 * 16, 130 * 16)
        p.setPen(QPen(accent, 6.0, Qt.SolidLine, Qt.RoundCap))
        p.drawArc(arc_bg, 205 * 16, int((130.0 * (1.0 - risk_t * 0.55)) * 16))

        speed_rect = QRectF(cx - radius * 0.9, cy - radius * 0.72, radius * 1.8, radius * 0.72)
        self.draw_glow_text(
            p,
            speed_rect,
            f"{int(round(self.state.speed)):02d}",
            int(radius * 0.48),
            color("#f2ffff", 245),
            color("#41f5ff", 130),
            Qt.AlignCenter,
            bold=True,
        )
        self.draw_text(
            p,
            QRectF(cx + radius * 0.25, cy - radius * 0.19, radius * 0.42, radius * 0.22),
            "km/h",
            int(radius * 0.12),
            color("#d7ffff", 210),
            Qt.AlignLeft | Qt.AlignVCenter,
        )
        self.draw_glow_text(
            p,
            QRectF(cx - radius * 0.34, cy - radius * 0.16, radius * 0.68, radius * 0.22),
            "AUTO",
            int(radius * 0.14),
            color("#b5ff8a", 245),
            color("#70ff78", 115),
            Qt.AlignCenter,
            bold=True,
        )

        for i in range(19):
            angle = math.radians(205 + i * (130 / 18.0))
            r1 = radius * (0.87 if i % 3 else 0.81)
            r2 = radius * 0.95
            a = QPointF(cx + math.cos(angle) * r1, cy + math.sin(angle) * r1)
            b = QPointF(cx + math.cos(angle) * r2, cy + math.sin(angle) * r2)
            p.setPen(QPen(color("#d9ffff", 80 if i % 3 else 150), 1.2 if i % 3 else 1.8))
            p.drawLine(a, b)
        p.restore()

    def draw_side_metrics(self, p: QPainter, w: int, h: int) -> None:
        left = QRectF(w * 0.055, h * 0.58, min(w * 0.27, 310), h * 0.24)
        right = QRectF(w - left.width() - w * 0.055, h * 0.58, left.width(), left.height())
        bottom = QRectF(w * 0.67, h * 0.83, min(w * 0.26, 290), h * 0.09)

        self.draw_metric_panel(
            p,
            left,
            "DIST",
            f"{self.state.distance_mm/ 1000:.2f} m",
            self.state.r_risk,
            "R",
            color("#56f7ff"),
        )
        self.draw_metric_panel(
            p,
            right,
            "LIGHT",
            f"{int(self.state.light_adc)} adc",
            self.state.l_risk,
            "L",
            color("#f1ffff"),
        )

        accent = mix_color(color("#56f7ff"), color("#ff3348"), max(0.0, (self.state.c_risk - 58.0) / 42.0))
        self.draw_compact_risk(p, bottom, self.state.c_risk, accent)

    def draw_metric_panel(self, p: QPainter, rect: QRectF, label: str, value: str, risk: float, symbol: str, accent: QColor) -> None:
        p.save()
        path = self.hud_panel_path(rect)
        grad = QLinearGradient(rect.left(), rect.top(), rect.right(), rect.bottom())
        grad.setColorAt(0.0, color("#071015", 154))
        grad.setColorAt(0.58, color("#0a2730", 98))
        grad.setColorAt(1.0, color("#061015", 132))
        p.fillPath(path, grad)
        p.setPen(QPen(QColor(accent.red(), accent.green(), accent.blue(), 95), 1.1))
        p.drawPath(path)

        self.draw_text(
            p,
            QRectF(rect.left() + 16, rect.top() + 10, rect.width() * 0.45, 28),
            label,
            14,
            color("#dfffff", 175),
            Qt.AlignLeft | Qt.AlignVCenter,
            bold=True,
        )
        self.draw_glow_text(
            p,
            QRectF(rect.left() + 16, rect.top() + 38, rect.width() - 32, 42),
            value,
            26,
            color("#efffff", 235),
            color("#42f2ff", 92),
            Qt.AlignLeft | Qt.AlignVCenter,
            bold=True,
        )

        bar_rect = QRectF(rect.left() + 18, rect.bottom() - 34, rect.width() - 36, 8)
        p.setPen(Qt.NoPen)
        p.setBrush(color("#cfffff", 26))
        p.drawRoundedRect(bar_rect, 4, 4)

        risk_color = mix_color(accent, color("#ff3348"), max(0.0, (risk - 58.0) / 42.0))
        fill = QRectF(bar_rect.left(), bar_rect.top(), bar_rect.width() * risk / 100.0, bar_rect.height())
        p.setBrush(risk_color)
        p.drawRoundedRect(fill, 4, 4)

        gauge_center = QPointF(rect.right() - 54, rect.top() + 45)
        self.draw_mini_gauge(p, gauge_center, 32, risk, symbol, risk_color)
        p.restore()

    def draw_compact_risk(self, p: QPainter, rect: QRectF, risk: float, accent: QColor) -> None:
        p.save()
        path = self.hud_panel_path(rect)
        p.fillPath(path, color("#071116", 172))
        p.setPen(QPen(QColor(accent.red(), accent.green(), accent.blue(), 135), 1.3))
        p.drawPath(path)
        self.draw_text(
            p,
            QRectF(rect.left() + 14, rect.top(), rect.width() * 0.25, rect.height()),
            "C",
            18,
            accent,
            Qt.AlignCenter,
            bold=True,
        )
        self.draw_glow_text(
            p,
            QRectF(rect.left() + rect.width() * 0.25, rect.top(), rect.width() * 0.35, rect.height()),
            f"{int(round(risk))}",
            34,
            color("#f5ffff"),
            QColor(accent.red(), accent.green(), accent.blue(), 110),
            Qt.AlignCenter,
            bold=True,
        )
        state_text = "BUZZER ON" if risk >= 72 else ("CAUTION" if risk >= 45 else "CLEAR")
        self.draw_text(
            p,
            QRectF(rect.left() + rect.width() * 0.62, rect.top(), rect.width() * 0.33, rect.height()),
            state_text,
            13,
            color("#ffe4e8" if risk >= 72 else "#dfffff", 230),
            Qt.AlignCenter,
            bold=True,
        )
        p.restore()

    def hud_panel_path(self, rect: QRectF) -> QPainterPath:
        cut = min(18.0, rect.height() * 0.27)
        path = QPainterPath()
        path.moveTo(rect.left() + cut, rect.top())
        path.lineTo(rect.right(), rect.top())
        path.lineTo(rect.right() - cut, rect.bottom())
        path.lineTo(rect.left(), rect.bottom())
        path.lineTo(rect.left(), rect.top() + cut)
        path.closeSubpath()
        return path

    def draw_mini_gauge(self, p: QPainter, center: QPointF, radius: float, risk: float, symbol: str, accent: QColor) -> None:
        p.save()
        rect = QRectF(center.x() - radius, center.y() - radius, radius * 2, radius * 2)
        p.setPen(QPen(color("#dfffff", 40), 4.0, Qt.SolidLine, Qt.RoundCap))
        p.drawArc(rect, 210 * 16, 120 * 16)
        p.setPen(QPen(accent, 4.2, Qt.SolidLine, Qt.RoundCap))
        p.drawArc(rect, 210 * 16, int(120 * risk / 100.0 * 16))
        self.draw_text(
            p,
            QRectF(center.x() - radius, center.y() - radius * 0.5, radius * 2, radius),
            f"{symbol}{int(round(risk))}",
            13,
            color("#f3ffff", 225),
            Qt.AlignCenter,
            bold=True,
        )
        p.restore()

    def draw_demo_controls(self, p: QPainter, w: int, h: int) -> None:
        self.slider_hitboxes.clear()
        self.button_hitboxes.clear()
        if not self.demo_mode:
            return

        panel_w = min(360.0, w * 0.36)
        panel_h = 126.0
        rect = QRectF(w * 0.055, h - panel_h - 28.0, panel_w, panel_h)

        p.save()
        path = self.hud_panel_path(rect)
        p.fillPath(path, color("#071116", 142))
        p.setPen(QPen(color("#55f4ff", 96), 1.2))
        p.drawPath(path)
        self.draw_text(
            p,
            QRectF(rect.left() + 18, rect.top() + 10, rect.width() - 36, 22),
            "DEMO INPUT",
            12,
            color("#dfffff", 180),
            Qt.AlignLeft | Qt.AlignVCenter,
            bold=True,
        )

        self.draw_demo_slider(
            p,
            QRectF(rect.left() + 18, rect.top() + 42, rect.width() - 36, 24),
            "DIST",
            self.demo_distance_mm,
            500.0,
            6500.0,
            "mm",
        )
        self.draw_demo_slider(
            p,
            QRectF(rect.left() + 18, rect.top() + 74, rect.width() - 36, 24),
            "LIGHT",
            self.demo_light_adc,
            0.0,
            ADC_MAX,
            "adc",
        )

        button_rect = QRectF(rect.right() - 96, rect.top() + 8, 76, 24)
        self.button_hitboxes["auto"] = button_rect
        self.draw_small_button(p, button_rect, "AUTO", self.demo_auto)
        p.restore()

    def draw_demo_slider(
        self,
        p: QPainter,
        rect: QRectF,
        name: str,
        value: float,
        low: float,
        high: float,
        unit: str,
    ) -> None:
        p.save()
        t = inverse_lerp(low, high, value)
        track = QRectF(rect.left() + 62, rect.center().y() - 3, rect.width() - 142, 6)
        self.slider_hitboxes[name.lower()] = QRectF(track.left() - 8, track.top() - 11, track.width() + 16, 28)
        p.setPen(Qt.NoPen)
        p.setBrush(color("#dfffff", 30))
        p.drawRoundedRect(track, 3, 3)

        fill = QRectF(track.left(), track.top(), track.width() * t, track.height())
        p.setBrush(color("#55f4ff", 155))
        p.drawRoundedRect(fill, 3, 3)

        knob_x = track.left() + track.width() * t
        p.setBrush(color("#f4ffff", 235))
        p.setPen(QPen(color("#55f4ff", 160), 1.0))
        p.drawEllipse(QPointF(knob_x, track.center().y()), 7.0, 7.0)

        self.draw_text(p, QRectF(rect.left(), rect.top(), 56, rect.height()), name, 11, color("#dfffff", 190), Qt.AlignLeft | Qt.AlignVCenter, bold=True)
        self.draw_text(
            p,
            QRectF(track.right() + 10, rect.top(), 68, rect.height()),
            f"{int(value)} {unit}",
            10,
            color("#dfffff", 190),
            Qt.AlignRight | Qt.AlignVCenter,
        )
        p.restore()

    def draw_small_button(self, p: QPainter, rect: QRectF, text: str, active: bool) -> None:
        p.save()
        path = QPainterPath()
        path.addRoundedRect(rect, 6, 6)
        p.fillPath(path, color("#1bfb90" if active else "#10252c", 172))
        p.setPen(QPen(color("#baffdc" if active else "#55f4ff", 160), 1.0))
        p.drawPath(path)
        self.draw_text(
            p,
            rect,
            text,
            10,
            color("#04120b" if active else "#dfffff", 230),
            Qt.AlignCenter,
            bold=True,
        )
        p.restore()

    def draw_scanlines(self, p: QPainter, w: int, h: int) -> None:
        p.save()
        p.setPen(QPen(color("#ffffff", 10), 1.0))
        step = max(4, int(h / 120))
        for y in range(0, h, step * 2):
            p.drawLine(0, y, w, y)

        sweep_y = (self.scan_offset * h * 1.15) - h * 0.08
        sweep = QLinearGradient(0, sweep_y - 24, 0, sweep_y + 24)
        sweep.setColorAt(0.0, color("#55f4ff", 0))
        sweep.setColorAt(0.5, color("#55f4ff", 24))
        sweep.setColorAt(1.0, color("#55f4ff", 0))
        p.fillRect(QRectF(0, sweep_y - 24, w, 48), sweep)
        p.restore()

    def draw_glow_polyline(self, p: QPainter, points, c: QColor, width: float, glow_alpha: int) -> None:
        if len(points) < 2:
            return
        p.save()
        for g in range(4, 0, -1):
            gc = QColor(c)
            gc.setAlpha(int(glow_alpha / (g + 1)))
            p.setPen(QPen(gc, width + g * 4.0, Qt.SolidLine, Qt.RoundCap, Qt.RoundJoin))
            for a, b in zip(points[:-1], points[1:]):
                p.drawLine(a, b)
        line = QColor(c)
        line.setAlpha(235)
        p.setPen(QPen(line, width, Qt.SolidLine, Qt.RoundCap, Qt.RoundJoin))
        for a, b in zip(points[:-1], points[1:]):
            p.drawLine(a, b)
        p.restore()

    def draw_glow_text(
        self,
        p: QPainter,
        rect: QRectF,
        text: str,
        size: int,
        fg: QColor,
        glow: QColor,
        flags,
        bold: bool = False,
    ) -> None:
        p.save()
        font = self.hud_font(size, bold)
        p.setFont(font)
        for offset, alpha_scale in [(3, 0.30), (2, 0.45), (1, 0.65)]:
            g = QColor(glow)
            g.setAlpha(int(g.alpha() * alpha_scale))
            p.setPen(g)
            for dx, dy in ((-offset, 0), (offset, 0), (0, -offset), (0, offset)):
                p.drawText(rect.translated(dx, dy), flags, text)
        p.setPen(fg)
        p.drawText(rect, flags, text)
        p.restore()

    def draw_text(self, p: QPainter, rect: QRectF, text: str, size: int, c: QColor, flags, bold: bool = False) -> None:
        p.save()
        p.setFont(self.hud_font(size, bold))
        p.setPen(c)
        p.drawText(rect, flags, text)
        p.restore()

    def hud_font(self, size: int, bold: bool = False) -> QFont:
        font = QFont(self.font_family, max(7, size))
        font.setBold(bold)
        font.setLetterSpacing(QFont.AbsoluteSpacing, 0)
        return font

    def pick_font_family(self) -> str:
        families = QFontDatabase.families()
        for candidate in ("Segoe UI Variable Display", "Segoe UI", "Malgun Gothic", "Arial"):
            if candidate in families:
                return candidate
        return ""

    def mousePressEvent(self, event) -> None:  # noqa: N802 - Qt naming convention
        pos = event.pos()
        for name, rect in self.slider_hitboxes.items():
            if rect.contains(pos):
                self.active_slider = name
                self.demo_auto = False
                self.update_slider_from_pos(name, pos.x())
                return
        for name, rect in self.button_hitboxes.items():
            if rect.contains(pos) and name == "auto":
                self.demo_auto = not self.demo_auto
                self.update()
                return

    def mouseMoveEvent(self, event) -> None:  # noqa: N802 - Qt naming convention
        if self.active_slider:
            self.update_slider_from_pos(self.active_slider, event.pos().x())

    def mouseReleaseEvent(self, _event) -> None:  # noqa: N802 - Qt naming convention
        self.active_slider = None

    def update_slider_from_pos(self, name: str, x: float) -> None:
        rect = self.slider_hitboxes.get(name)
        if rect is None:
            return
        t = inverse_lerp(rect.left() + 8, rect.right() - 8, x)
        if name == "dist":
            self.demo_distance_mm = lerp(500.0, 6500.0, t)
        elif name == "light":
            self.demo_light_adc = lerp(0.0, ADC_MAX, t)
        self.update()

    def keyPressEvent(self, event) -> None:  # noqa: N802 - Qt naming convention
        key = event.key()
        window = self.window()
        if key == Qt.Key_Escape:
            if window.isFullScreen():
                window.showNormal()
            else:
                window.close()
        elif key == Qt.Key_F11:
            if window.isFullScreen():
                window.showNormal()
            else:
                window.showFullScreen()
        elif key == Qt.Key_R:
            self.demo_auto = not self.demo_auto
            self.demo_mode = True
            self.state.connection = "DEMO"
        else:
            super().keyPressEvent(event)


class HudWindow(QMainWindow):
    def __init__(self, args: argparse.Namespace):
        super().__init__()
        self.setWindowTitle("Advanced ADAS HUD Demo")
        self.setCentralWidget(HudWidget(args))
        self.resize(1280, 720)
        self.setStyleSheet("background:#020608;")

    def closeEvent(self, event) -> None:  # noqa: N802 - Qt naming convention
        widget = self.centralWidget()
        if isinstance(widget, HudWidget):
            widget.stop_reader()
        event.accept()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="PyQt/PySide QPainter ADAS HUD demo for STM32 UART")
    parser.add_argument("--port", help="UART port, for example COM3")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD, help=f"UART baud rate, default {DEFAULT_BAUD}")
    parser.add_argument("--demo", action="store_true", help="Force demo mode without UART")
    parser.add_argument("--fullscreen", action="store_true", help="Start fullscreen")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if hasattr(QGuiApplication, "setHighDpiScaleFactorRoundingPolicy") and hasattr(Qt, "HighDpiScaleFactorRoundingPolicy"):
        QGuiApplication.setHighDpiScaleFactorRoundingPolicy(Qt.HighDpiScaleFactorRoundingPolicy.PassThrough)
    app = QApplication(sys.argv)
    app.setApplicationName("Advanced ADAS HUD Demo")
    app.setStyle("Fusion")

    window = HudWindow(args)
    if args.fullscreen:
        window.showFullScreen()
    else:
        window.show()

    if hasattr(app, "exec"):
        return int(app.exec())
    return int(app.exec_())


if __name__ == "__main__":
    raise SystemExit(main())
