#!/usr/bin/env python3
"""Run the bounded NNUE mutation corpus with an external process deadline."""
import argparse
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument("--seconds", type=int, default=1800)
parser.add_argument("--output", type=Path, default=Path("/private/tmp/fischer-stockfish-audit"))
args = parser.parse_args()
if not 1 <= args.seconds <= 3600:
    parser.error("seconds must be between 1 and 3600")
script = Path(__file__).with_name("embedded_safety.py")
subprocess.run(["rtk", "proxy", "python3", str(script), "nnue-mutate", "--output", str(args.output), "--jobs", "1"], check=True)
subprocess.run(["rtk", "proxy", str(args.output / "stockfish-nnue-mutate"), str(args.seconds), str(args.output / "nnue-last-case.bin")], check=True, timeout=args.seconds + 30)
