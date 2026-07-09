#!/usr/bin/env python
"""Live serial plotter for the STM32 potentiometer UART output."""

from __future__ import annotations

import argparse
import re
import sys
import threading
import time
from collections import deque
from dataclasses import dataclass

import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import serial
from serial.tools import list_ports


ADC_RE = re.compile(r"ADC\s*=\s*(\d+)")
MV_RE = re.compile(r"V\s*=\s*(\d+)\s*mV", re.IGNORECASE)


@dataclass
class Sample:
    t: float
    adc: int
    mv: int | None


def choose_port(requested: str | None) -> str:
    if requested:
        return requested

    ports = list(list_ports.comports())
    if not ports:
        raise SystemExit("No serial ports found. Connect the ST-Link VCP and retry.")

    st_ports = [
        port
        for port in ports
        if "stlink" in port.description.replace(" ", "").lower()
        or "st-link" in port.description.lower()
        or "stmicroelectronics" in port.description.lower()
    ]
    return (st_ports[0] if st_ports else ports[0]).device


def parse_sample(line: str, start_time: float) -> Sample | None:
    adc_match = ADC_RE.search(line)
    if not adc_match:
        return None

    mv_match = MV_RE.search(line)
    return Sample(
        t=time.monotonic() - start_time,
        adc=int(adc_match.group(1)),
        mv=int(mv_match.group(1)) if mv_match else None,
    )


def serial_reader(
    ser: serial.Serial,
    samples: deque[Sample],
    lock: threading.Lock,
    stop_event: threading.Event,
) -> None:
    start_time = time.monotonic()

    while not stop_event.is_set():
        try:
            raw = ser.readline()
        except serial.SerialException as exc:
            print(f"\nSerial read error: {exc}", file=sys.stderr, flush=True)
            stop_event.set()
            return

        if not raw:
            continue

        line = raw.decode("utf-8", errors="replace").strip()
        if line:
            print(line, flush=True)

        sample = parse_sample(line, start_time)
        if sample is None:
            continue

        with lock:
            samples.append(sample)


def main() -> int:
    parser = argparse.ArgumentParser(description="Plot STM32 potentiometer ADC values over UART.")
    parser.add_argument("--port", help="Serial port, for example COM5. Defaults to ST-Link VCP.")
    parser.add_argument("--baud", type=int, default=9600, help="UART baud rate.")
    parser.add_argument("--window", type=float, default=20.0, help="Visible time window in seconds.")
    parser.add_argument("--max-samples", type=int, default=2000, help="Samples kept in memory.")
    args = parser.parse_args()

    port = choose_port(args.port)
    samples: deque[Sample] = deque(maxlen=args.max_samples)
    lock = threading.Lock()
    stop_event = threading.Event()

    try:
        ser = serial.Serial(port, args.baud, timeout=0.2)
    except serial.SerialException as exc:
        print(f"Could not open {port} at {args.baud} baud: {exc}", file=sys.stderr)
        return 1

    print(f"Opened {port} at {args.baud} baud. Close the graph window to stop.", flush=True)

    reader = threading.Thread(
        target=serial_reader,
        args=(ser, samples, lock, stop_event),
        daemon=True,
    )
    reader.start()

    fig, ax = plt.subplots(figsize=(10, 5.5))
    (adc_line,) = ax.plot([], [], color="#0f766e", linewidth=2.0, label="ADC")
    current_text = ax.text(
        0.02,
        0.95,
        "Waiting for ADC data...",
        transform=ax.transAxes,
        va="top",
        ha="left",
        fontsize=11,
        bbox={"boxstyle": "round,pad=0.35", "facecolor": "white", "edgecolor": "#cbd5e1"},
    )

    ax.set_title(f"Potentiometer ADC Live Plot ({port}, {args.baud} baud)")
    ax.set_xlabel("Time (s)")
    ax.set_ylabel("ADC raw count")
    ax.set_ylim(-50, 4145)
    ax.grid(True, color="#d1d5db", alpha=0.75)
    ax.legend(loc="upper right")

    def update(_frame: int):
        with lock:
            snapshot = list(samples)

        if not snapshot:
            return adc_line, current_text

        times = [sample.t for sample in snapshot]
        adc_values = [sample.adc for sample in snapshot]
        adc_line.set_data(times, adc_values)

        latest = snapshot[-1]
        xmin = max(0.0, latest.t - args.window)
        xmax = max(args.window, latest.t + 0.5)
        ax.set_xlim(xmin, xmax)

        mv_text = f", {latest.mv} mV" if latest.mv is not None else ""
        current_text.set_text(f"ADC {latest.adc}{mv_text}")
        return adc_line, current_text

    animation = FuncAnimation(fig, update, interval=100, blit=False, cache_frame_data=False)

    try:
        plt.show()
    finally:
        stop_event.set()
        reader.join(timeout=1.0)
        ser.close()
        # Keep a reference alive for matplotlib backends that require it.
        _ = animation

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
