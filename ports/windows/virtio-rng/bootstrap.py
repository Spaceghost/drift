#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Private Windows prerequisites for real, paired Virtio RNG device trials."""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import shutil
import struct
import subprocess
import sys
import time
import urllib.request
import zipfile

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
LOCK = HERE / "prerequisites.json"


def digest(path, algorithm="sha256"):
    value = hashlib.new(algorithm)
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def validate_download(path, record):
    if path.stat().st_size != record["bytes"]:
        raise RuntimeError(f"wrong download size: {path}")
    for algorithm in ("sha256", "sha512"):
        if algorithm in record and digest(path, algorithm) != record[algorithm]:
            raise RuntimeError(f"{algorithm} mismatch: {path}")


def download(record, cache):
    path = cache / record["url"].rsplit("/", 1)[1]
    if path.exists():
        validate_download(path, record)
        return path
    partial = path.with_name(path.name + ".partial")
    for attempt in range(3):
        try:
            request = urllib.request.Request(record["url"], headers={"User-Agent": "drift-prerequisites/1"})
            with urllib.request.urlopen(request, timeout=60) as response, partial.open("wb") as target:
                shutil.copyfileobj(response, target, 1024 * 1024)
            validate_download(partial, record)
            partial.replace(path)
            return path
        except Exception:
            partial.unlink(missing_ok=True)
            if attempt == 2:
                raise
            time.sleep(attempt + 1)
    raise AssertionError("unreachable")


def pe_machine(path):
    with path.open("rb") as source:
        dos = source.read(64)
        if len(dos) != 64 or dos[:2] != b"MZ":
            raise RuntimeError(f"not a native Windows PE executable: {path}")
        source.seek(struct.unpack_from("<I", dos, 0x3c)[0])
        header = source.read(6)
    if len(header) != 6 or header[:4] != b"PE\0\0":
        raise RuntimeError(f"invalid PE header: {path}")
    return struct.unpack_from("<H", header, 4)[0]


def call(arguments, directory, name, env=None):
    result = subprocess.run(list(map(str, arguments)), capture_output=True, text=True,
                            errors="replace", timeout=120, env=env)
    (directory / (name + ".log")).write_text(result.stdout + result.stderr, encoding="utf-8")
    if result.returncode:
        raise RuntimeError(f"{name} failed ({result.returncode}); see {directory / (name + '.log')}")
    return result.stdout


def compiler_candidates(explicit):
    if explicit:
        yield Path(shutil.which(explicit) or explicit).resolve()
        return
    candidates = []
    for name in ("i686-w64-mingw32-gcc.exe", "i686-w64-mingw32-gcc"):
        if found := shutil.which(name):
            candidates.append(Path(found))
    roots = [Path("C:/msys64"), Path("C:/tools/msys64")]
    for key in ("MSYS2_LOCATION", "MSYS2_ROOT"):
        if os.environ.get(key):
            roots.append(Path(os.environ[key]))
    if os.environ.get("RUNNER_TEMP"):
        roots.append(Path(os.environ["RUNNER_TEMP"]) / "msys64")
    for entry in os.environ.get("PATH", "").split(os.pathsep):
        directory = Path(entry)
        if directory.name.lower() == "bin":
            roots.extend([directory.parent, directory.parent.parent])
    candidates += [root / "mingw32/bin/gcc.exe" for root in roots]
    seen = set()
    for path in candidates:
        path = path.resolve()
        if path not in seen and path.is_file():
            seen.add(path)
            yield path


SMOKE = r'''#include <winsock2.h>
#include <windows.h>
#include <stddef.h>
#include <stdint.h>
_Static_assert(sizeof(void*)==4 && sizeof(size_t)==4 && sizeof(unsigned long)==4,"ILP32 required");
int main(void) {
    WSADATA data; LARGE_INTEGER clock;
    if(WSAStartup(MAKEWORD(2,2),&data)) return 1;
    if(!QueryPerformanceCounter(&clock)) return 2;
    return WSACleanup()!=0;
}
'''


def validate_compiler(compiler, work, index):
    env = dict(os.environ)
    env["PATH"] = str(compiler.parent) + os.pathsep + env.get("PATH", "")
    label = "compiler-" + str(index)
    target = call([compiler, "-dumpmachine"], work, label + "-target", env).strip()
    if target not in ("i686-w64-mingw32", "i686-w64-mingw32.static"):
        raise RuntimeError(f"compiler target must be i686 PE/ILP32, got {target}")
    source, binary = work / "compiler-smoke.c", work / (label + "-smoke.exe")
    source.write_text(SMOKE, encoding="ascii")
    call([compiler, "-std=c11", "-Wall", "-Wextra", "-Werror", source, "-o", binary, "-lws2_32"],
         work, label + "-build", env)
    if pe_machine(binary) != 0x14c:
        raise RuntimeError("compiler did not produce an i386 PE32 executable")
    call([binary], work, label + "-execute", env)
    version = call([compiler, "--version"], work, label + "-version", env).splitlines()[0]
    return {"path": str(compiler), "bin": str(compiler.parent), "target": target,
            "version": version, "sha256": digest(compiler), "smoke_sha256": digest(binary)}, env


def extract_zip(archive, target):
    with zipfile.ZipFile(archive) as source:
        for member in source.namelist():
            relative = PurePosixPath(member.replace("\\", "/"))
            if relative.is_absolute() or ".." in relative.parts or ":" in member:
                raise RuntimeError("unsafe ZIP path")
        source.extractall(target)


def sevenzip(explicit):
    candidates = [explicit] if explicit else [shutil.which("7z.exe"), shutil.which("7z")]
    for key in ("ProgramFiles", "ProgramFiles(x86)"):
        if os.environ.get(key):
            candidates.append(str(Path(os.environ[key]) / "7-Zip/7z.exe"))
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return Path(candidate).resolve()
    raise RuntimeError("7z.exe is required to extract the pinned NSIS archive; Windows GitHub runners provide 7-Zip. Use --sevenzip PATH.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work", type=Path, default=REPO / "build/windows-virtio-prerequisites")
    parser.add_argument("--cc", help="validate this existing native i686 GCC instead of autodetection")
    parser.add_argument("--sevenzip", help="path to an existing full 7z.exe (NSIS extraction support)")
    parser.add_argument("--force-pinned-compiler", action="store_true", help="always use the locked WinLibs archive")
    args = parser.parse_args()
    if os.name != "nt":
        raise RuntimeError("run on native Windows; a cross-compile does not qualify this test")
    if args.cc and args.force_pinned_compiler:
        parser.error("--cc and --force-pinned-compiler are mutually exclusive")
    work = args.work.resolve()
    work.mkdir(parents=True, exist_ok=True)
    cache = work / "downloads"
    cache.mkdir(exist_ok=True)
    records = json.loads(LOCK.read_text(encoding="utf-8"))
    extractor = sevenzip(args.sevenzip)  # Fail before downloading if extraction cannot run.
    attempts = []
    compiler = env = None
    if not args.force_pinned_compiler:
        for index, candidate in enumerate(compiler_candidates(args.cc)):
            try:
                compiler, env = validate_compiler(candidate, work, index)
                compiler["source"] = "existing-native-i686-gcc"
                break
            except (OSError, RuntimeError, subprocess.SubprocessError) as error:
                attempts.append({"path": str(candidate), "error": str(error)})
                if args.cc:
                    raise
    if compiler is None:
        archive = download(records["winlibs"], cache)
        directory = work / "winlibs"
        if directory.exists():
            shutil.rmtree(directory)
        extract_zip(archive, directory)
        compiler, env = validate_compiler(directory / "mingw32/bin/gcc.exe", work, "pinned")
        compiler.update(source="pinned-winlibs", archive_sha256=records["winlibs"]["sha256"])
    archive = download(records["qemu"], cache)
    directory = work / "qemu"
    if directory.exists():
        shutil.rmtree(directory)
    directory.mkdir()
    call([extractor, "x", "-y", "-o" + str(directory), archive], work, "qemu-extract")
    binaries = list(directory.rglob("qemu-system-x86_64.exe"))
    if len(binaries) != 1:
        raise RuntimeError("expected exactly one extracted qemu-system-x86_64.exe")
    qemu = binaries[0]
    if pe_machine(qemu) != 0x8664:
        raise RuntimeError("QEMU must be a native AMD64 PE executable")
    version = call([qemu, "--version"], work, "qemu-version", env).splitlines()[0]
    if "11.1.0" not in version:
        raise RuntimeError("unexpected QEMU version from pinned archive")
    devices = call([qemu, "-device", "help"], work, "qemu-devices", env)
    objects = call([qemu, "-object", "help"], work, "qemu-objects", env)
    if "virtio-rng-pci" not in devices or "rng-egd" not in objects:
        raise RuntimeError("QEMU lacks the required real Virtio RNG/EGD device models")
    result = {"format": 1, "platform": sys.platform, "lock_sha256": digest(LOCK),
              "cc": compiler["path"], "cc_bin": compiler["bin"], "compiler": compiler,
              "compiler_candidates_rejected": attempts, "qemu": str(qemu),
              "qemu_version": version, "qemu_sha256": digest(qemu),
              "qemu_archive_sha256": records["qemu"]["sha256"],
              "extractor": str(extractor), "extractor_sha256": digest(extractor)}
    output = work / "tools.json"
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    if os.environ.get("GITHUB_OUTPUT"):
        with open(os.environ["GITHUB_OUTPUT"], "a", encoding="utf-8") as target:
            for key, value in {"cc": compiler["path"], "cc_bin": compiler["bin"], "qemu": str(qemu), "tools_json": str(output)}.items():
                target.write(f"{key}={value}\n")
    if os.environ.get("GITHUB_PATH"):
        with open(os.environ["GITHUB_PATH"], "a", encoding="utf-8") as target:
            target.write(compiler["bin"] + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
