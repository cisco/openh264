#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (c) 2026, Robbie Foust
# See the repository LICENSE for the redistribution conditions and disclaimer.
"""Optional synthetic B-slice regression using the native h264dec console.

Requires Python 3 and a developer FFmpeg build with libx264. No sample footage,
external downloads, application process, or encoder/decoder production changes.
Both console API paths are compared strictly over every Y, U, and V byte.
"""
import argparse
import hashlib
import json
import pathlib
import subprocess
import sys


CASES = {
    "spatial-cabac": "direct=spatial:cabac=1",
    "spatial-cavlc4": "direct=spatial:cabac=0:8x8dct=0",
    "temporal-cabac4": "direct=temporal:cabac=1:8x8dct=0",
    "temporal-cavlc4": "direct=temporal:cabac=0:8x8dct=0",
    "unweighted-cabac": "direct=spatial:cabac=1:weightb=0",
    "unfiltered-cabac": "direct=spatial:cabac=1:no-deblock=1",
    "p-frame-control": "bframes=0",
}
BASE_PARAMS = ("aud=1:repeat-headers=1:keyint=120:min-keyint=120:"
               "scenecut=0:bframes=2:b-adapt=0:threads=1")
FRAME_SIZE = 320 * 240 * 3 // 2
FRAME_COUNT = 40


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def run(command, output, name):
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            timeout=120, check=False)
    (output / (name + ".stdout")).write_bytes(result.stdout)
    (output / (name + ".stderr")).write_bytes(result.stderr)
    if result.returncode:
        raise RuntimeError("{} failed: see {}".format(name, output / (name + ".stderr")))
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--decoder", required=True, type=pathlib.Path)
    parser.add_argument("--ffmpeg", default="ffmpeg")
    parser.add_argument("--output", required=True, type=pathlib.Path,
                        help="New directory; existing directories are refused")
    parser.add_argument("--cases", nargs="+", choices=list(CASES), default=list(CASES))
    args = parser.parse_args()
    decoder = args.decoder.resolve()
    if not decoder.is_file():
        raise RuntimeError("native h264dec executable is missing")
    output = args.output.resolve()
    output.mkdir(parents=False, exist_ok=False)
    version = run([args.ffmpeg, "-version"], output, "ffmpeg-version").decode(errors="replace")
    report = {"decoder_sha256": sha256(decoder.read_bytes()),
              "ffmpeg_version": version, "cases": {}, "pass": True}
    for name in args.cases:
        directory = output / name
        directory.mkdir()
        movie = directory / "fixture.mp4"
        stream = directory / "fixture.264"
        reference = directory / "reference.yuv"
        params = BASE_PARAMS + ":" + CASES[name]
        common = [args.ffmpeg, "-hide_banner", "-loglevel", "error", "-threads", "1"]
        run(common + ["-f", "lavfi", "-i", "testsrc2=size=320x240:rate=10",
                      "-frames:v", str(FRAME_COUNT), "-an", "-c:v", "libx264",
                      "-preset", "medium", "-pix_fmt", "yuv420p",
                      "-x264-params", params, str(movie)], directory, "encode")
        run(common + ["-i", str(movie), "-map", "0:v:0", "-c:v", "copy",
                      "-bsf:v", "h264_mp4toannexb", "-f", "h264", str(stream)],
            directory, "extract")
        run(common + ["-i", str(movie), "-map", "0:v:0", "-an", "-pix_fmt",
                      "yuv420p", "-f", "rawvideo", str(reference)], directory, "reference")
        expected = reference.read_bytes()
        if len(expected) != FRAME_COUNT * FRAME_SIZE:
            raise RuntimeError("reference frame count is not 40")
        case = {"x264_params": params, "fixture_sha256": sha256(stream.read_bytes()),
                "reference_sha256": sha256(expected), "modes": {}}
        for legacy in (False, True):
            mode = "frame2" if legacy else "no-delay"
            raw_file = directory / (mode + ".yuv")
            run([str(decoder), str(stream), str(raw_file), "-ec", "0", "-trace", "2"]
                + (["-legacy"] if legacy else []), directory, mode)
            raw = raw_file.read_bytes()
            difference = max((abs(a - b) for a, b in zip(raw, expected)), default=-1)
            passed = raw == expected
            case["modes"][mode] = {"pass": passed, "bytes": len(raw),
                                   "frames": len(raw) / FRAME_SIZE,
                                   "max_abs_yuv": difference,
                                   "yuv_sha256": sha256(raw)}
            report["pass"] = report["pass"] and passed
            print("{}/{}: {} frames, max YUV difference {}, pass={}".format(
                name, mode, len(raw) / FRAME_SIZE, difference, passed), flush=True)
        report["cases"][name] = case
        (output / "report.json").write_text(json.dumps(report, indent=2))
    return 0 if report["pass"] else 1


if __name__ == "__main__":
    sys.exit(main())
