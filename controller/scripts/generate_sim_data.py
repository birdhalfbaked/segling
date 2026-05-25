"""Generate seeded I2C simulator register files under simulated_data/."""

from __future__ import annotations

import argparse
import random
import struct
from pathlib import Path

# Keep in sync with integrations/ahrs.h and comms_sim.c address switch.
IMU_ADDRESS = 0x68
IMU_START_REG = 0x3B
IMU_DATA_SIZE = 14

MAG_ADDRESS = 0x0C
MAG_START_REG = 0x02
MAG_DATA_SIZE = 6

MAG_MAX = 4095


def _format_reg_line(reg: int, payload: bytes) -> str:
    parts = [f"{reg:02x}"] + [f"{byte:02x}" for byte in payload]
    return " ".join(parts)


def _int16_samples(rng: random.Random, count: int, lo: int, hi: int) -> bytes:
    values = [rng.randint(lo, hi) for _ in range(count)]
    out = bytearray()
    for value in values:
        out.extend(struct.pack("<h", value))
    return bytes(out)


def generate_imu_dat(rng: random.Random) -> str:
    # MPU6050-style burst: accel xyz, temp, gyro xyz (14 bytes @ 0x3B).
    payload = _int16_samples(rng, 3, -8000, 8000)
    payload += struct.pack("<h", rng.randint(2000, 3000))
    payload += _int16_samples(rng, 3, -500, 500)
    if len(payload) != IMU_DATA_SIZE:
        raise RuntimeError(f"imu payload size {len(payload)} != {IMU_DATA_SIZE}")
    lines = [
        f"# seed-derived IMU burst @ 0x{IMU_START_REG:02X} (addr 0x{IMU_ADDRESS:02X})",
        _format_reg_line(IMU_START_REG, payload),
    ]
    return "\n".join(lines) + "\n"


def generate_mag_dat(rng: random.Random) -> str:
    payload = _int16_samples(rng, 3, -MAG_MAX, MAG_MAX)
    if len(payload) != MAG_DATA_SIZE:
        raise RuntimeError(f"mag payload size {len(payload)} != {MAG_DATA_SIZE}")
    lines = [
        f"# seed-derived magnetometer @ 0x{MAG_START_REG:02X} (addr 0x{MAG_ADDRESS:02X})",
        _format_reg_line(MAG_START_REG, payload),
    ]
    return "\n".join(lines) + "\n"


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate seeded simulator files for comms SIM backend."
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=42,
        help="RNG seed (default: 42)",
    )
    parser.add_argument(
        "--out-dir",
        type=Path,
        default=Path("simulated_data"),
        help="Output directory (default: simulated_data)",
    )
    args = parser.parse_args()

    rng = random.Random(args.seed)
    out_dir = args.out_dir
    out_dir.mkdir(parents=True, exist_ok=True)

    files = {
        "imu.dat": generate_imu_dat(rng),
        "mag.dat": generate_mag_dat(rng),
    }
    for name, content in files.items():
        path = out_dir / name
        path.write_text(content, encoding="ascii")
        print(f"wrote {path}")


if __name__ == "__main__":
    main()
