#!/usr/bin/env python3
"""Build standalone embedded safety harnesses without touching the host build."""
import argparse
import concurrent.futures
from pathlib import Path
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument("kind", choices=["native", "syzygy", "mutate", "nnue", "nnue-mutate"])
parser.add_argument("--output", type=Path, default=Path("/private/tmp/fischer-stockfish-audit"))
parser.add_argument("--jobs", type=int, default=2)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
args.output.mkdir(parents=True, exist_ok=True)
flags = ["-std=gnu++20", "-DSTOCKFISH_EMBEDDED", "-DNNUE_EMBEDDING_OFF", "-DNO_PEXT", "-O1", "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all"]
if args.kind == "native":
    flags += ["-DFISCHER_TESTING"]
    sources = [p for p in (root / "src").rglob("*.cpp") if p.name != "main.cpp" and "universal" not in p.parts]
    sources += [root / "tests/embedded_native_regression.cpp"]
else:
    sources = [] if args.kind.startswith("nnue") else [root / "src/syzygy/tbprobe.cpp"]
    flags += ["-DFISCHER_FUZZING", "-ffunction-sections", "-fdata-sections"]
    if args.kind == "mutate":
        flags += ["-DFISCHER_MUTATION_RUNNER"]
        sources += [root / "tests/embedded_syzygy_fuzzer.cpp"]
    elif args.kind == "nnue-mutate":
        sources += [root / "tests/embedded_nnue_mutations.cpp"]
    else:
        sources += [root / f"tests/embedded_{args.kind}_regression.cpp"]

def compile_one(source):
    target = args.output / (args.kind + "-" + str(source.relative_to(root)).replace("/", "-") + ".o")
    subprocess.run(["rtk", "proxy", "clang++", *flags, "-c", str(source), "-o", str(target)], check=True)
    return target

with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
    objects = list(pool.map(compile_one, sources))
target = args.output / f"stockfish-{args.kind}"
strip = "-Wl,-dead_strip" if sys.platform == "darwin" else "-Wl,--gc-sections"
subprocess.run(["rtk", "proxy", "clang++", "-fsanitize=address,undefined", *(str(p) for p in objects), strip, "-o", str(target)], check=True)
print(target)
