#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Require the unchanged Linux source personality to reject Windows LLP64."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

REPO = Path(__file__).resolve().parents[3]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", required=True, help="native x86_64-w64-mingw32 GCC")
    parser.add_argument("--work", type=Path, default=REPO / "build/windows-llp64-rejection")
    args = parser.parse_args()
    if os.name != "nt":
        parser.error("run this qualification on native Windows")
    cc = Path(shutil.which(args.cc) or args.cc).resolve()
    env = dict(os.environ)
    env["PATH"] = str(cc.parent) + os.pathsep + env.get("PATH", "")
    target = subprocess.check_output([cc, "-dumpmachine"], text=True, env=env).strip()
    if target != "x86_64-w64-mingw32":
        parser.error(f"expected native x86_64-w64-mingw32 GCC, got {target}")
    work = args.work.resolve()
    work.mkdir(parents=True, exist_ok=True)
    control = work / "control.c"
    control.write_text('_Static_assert(sizeof(void*)==8 && sizeof(unsigned long)==4,"LLP64 control");\n', encoding="ascii")
    subprocess.run([cc, "-std=c11", "-Wall", "-Wextra", "-Werror", "-c", control, "-o", work / "control.o"],
                   check=True, env=env, timeout=120)
    profile = REPO / "imports/linux/virtio-rng"
    source = profile / "driver.c"
    artifact = work / "unsupported.o"
    artifact.unlink(missing_ok=True)
    result = subprocess.run([cc, "-std=c11", "-Wall", "-Wextra", "-Werror", "-I" + str(profile),
                             "-I" + str(profile / "compat"), "-I" + str(REPO / "include"),
                             "-c", source, "-o", artifact],
                            capture_output=True, text=True, errors="replace", env=env, timeout=120)
    diagnostic = result.stdout + result.stderr
    (work / "unsupported.log").write_text(diagnostic, encoding="utf-8")
    if result.returncode == 0 or artifact.exists() or "LLP64 is unsupported" not in diagnostic:
        raise RuntimeError("Linux source LLP64 rejection was not the expected explicit compile-time error")
    header = profile / "compat/virtio_rng_compat.h"
    report = {"status": "passed", "target": target, "control": "LLP64 layout compiled",
              "source": str(source.relative_to(REPO)),
              "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
              "rejection": "Linux source requires LP64 or ILP32; LLP64 is unsupported",
              "compiler": subprocess.check_output([cc, "--version"], text=True, env=env).splitlines()[0],
              "header_sha256": hashlib.sha256(header.read_bytes()).hexdigest(), "negative_exit": result.returncode}
    (work / "result.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
