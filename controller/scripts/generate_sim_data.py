"""Generate seeded binary I2C simulator files under simulated_data/."""

from __future__ import annotations

import argparse
import random
import struct
from pathlib import Path

# Keep in sync with integrations/ahrs.h and comms_sim.c address switch.
IMU_ADDRESS = 0x68
IMU_START_REG = 0x3B
IMU_FRAME_LEN = 14

MAG_ADDRESS = 0x0C
MAG_START_REG = 0x02
MAG_FRAME_LEN = 6

MAG_MAX = 4095
DEFAULT_FRAMES = 10


def _int16_samples(rng: random.Random, count: int, lo: int, hi: int) -> bytes:
    values = [rng.randint(lo, hi) for _ in range(count)]
    out = bytearray()
    for value in values:
        out.extend(struct.pack("<h", value))
    return bytes(out)


def _imu_frame(rng: random.Random) -> bytes:
    payload = _int16_samples(rng, 3, -8000, 8000)
    payload += struct.pack("<h", rng.randint(2000, 3000))
    payload += _int16_samples(rng, 3, -500, 500)
    if len(payload) != IMU_FRAME_LEN:
        raise RuntimeError(f"imu frame size {len(payload)} != {IMU_FRAME_LEN}")
    return payload


def _mag_frame(rng: random.Random) -> bytes:
    payload = _int16_samples(rng, 3, -MAG_MAX, MAG_MAX)
    if len(payload) != MAG_FRAME_LEN:
        raise RuntimeError(f"mag frame size {len(payload)} != {MAG_FRAME_LEN}")
    return payload


def _write_binary_frames(path: Path, frames: list[bytes]) -> None:
    path.write_bytes(b"".join(frames))


def generate_imu_dat(rng: random.Random, frame_count: int) -> list[bytes]:
    return [_imu_frame(rng) for _ in range(frame_count)]


def generate_mag_dat(rng: random.Random, frame_count: int) -> list[bytes]:
    return [_mag_frame(rng) for _ in range(frame_count)]


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate seeded binary simulator files for comms SIM backend."
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=42,
        help="RNG seed (default: 42)",
    )
    parser.add_argument(
        "--frames",
        type=int,
        default=DEFAULT_FRAMES,
        help=f"Frames per device file (default: {DEFAULT_FRAMES})",
    )
    parser.add_argument(
        "--out-dir",
        type=Path,
        default=Path("simulated_data"),
        help="Output directory (default: simulated_data)",
    )
    args = parser.parse_args()

    if args.frames < 1:
        raise SystemExit("--frames must be >= 1")

    rng = random.Random(args.seed)
    out_dir = args.out_dir
    out_dir.mkdir(parents=True, exist_ok=True)

    files = {
        "imu.dat": generate_imu_dat(rng, args.frames),
        "mag.dat": generate_mag_dat(rng, args.frames),
    }
    for name, frames in files.items():
        path = out_dir / name
        _write_binary_frames(path, frames)
        print(f"wrote {path} ({len(frames)} frames, {frames[0].__len__()} bytes each)")


if __name__ == "__main__":
    main()
