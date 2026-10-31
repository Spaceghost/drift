# Native Windows Virtio RNG device trials

`bootstrap.py` supplies a native i686 GCC and a native x64 QEMU device server
for the shared [paired controller runner](../../linux/virtio-rng/run.py).
It writes only to its private `--work` directory. The QEMU installer is treated
as an NSIS archive and extracted with an existing full `7z.exe`; it is never
executed. No administrator permission, service, registry change or global
installation is needed. GitHub Windows runners include 7-Zip. A local machine
can pass `--sevenzip` explicitly.

The script first tries an existing MSYS2 MINGW32/native i686 GCC. MINGW32 is
deprecated but its official repository still contains GCC 16.2.0-6, published
2026-10-13. This is separate from the discontinued 32-bit MSYS runtime. An
existing compiler is accepted only after an ILP32/Winsock program compiles to
i386 PE32 and executes on the Windows host. Rejected candidates are recorded.

If none passes, the script downloads the pinned standalone WinLibs compiler.
Use `--force-pinned-compiler` for the same compiler archive on every run.
All downloads must match the lock's byte count and SHA256; QEMU must also match
the publisher's SHA512. Existing cached downloads are checked again before use.

| Download | Pin | SHA256 |
| --- | --- | --- |
| WinLibs i686 GCC, POSIX/DWARF, MSVCRT | GCC 16.2.0 / MinGW-w64 14.0.0 release 3 | `805d3e022d005fe05a4bd53d80fef72fbb1f2ed8db6e14d51b6fa227980ffa8a` |
| QEMU Windows x64 | 11.1.0 / 20260811 | `f98a8aeb5f7faea9765b6dee28316c266cd179d80354a2fed8e50176f9a2e59f` |

`prerequisites.json` includes exact release URLs and publisher checksum URLs.
The WinLibs SHA256 matches both its release checksum file and GitHub asset
digest. The QEMU SHA256 was computed from the complete archive after matching
its publisher SHA512. QEMU is checked for AMD64 PE identity, version and the
`virtio-rng-pci`/`rng-egd` device models before the script emits `tools.json`.
That record includes the compiler target/version/executable hash, PE smoke-test
hash, extractor hash, QEMU executable/archive hashes and the lock hash. It is
prerequisite evidence; the shared runner provides actual device evidence.

Run from PowerShell on native 64-bit Windows with Python 3.10 or later:

```powershell
python ports/windows/virtio-rng/bootstrap.py --work build/windows-virtio-tools
$tools = Get-Content build/windows-virtio-tools/tools.json | ConvertFrom-Json
$env:PATH = $tools.cc_bin + ";" + $env:PATH
python ports/linux/virtio-rng/run.py --work build/windows-virtio-proof --cc $tools.cc --qemu $tools.qemu --variants native,linux,windows --requests 64 --rounds 4
```

The bootstrap also writes `cc`, `cc_bin`, `qemu` and `tools_json` step outputs
when `GITHUB_OUTPUT` is set, and adds the selected compiler directory through
`GITHUB_PATH`. It does not alter the machine's persisted PATH. All three paired
controllers use the same i686 compiler and workload. The source Linux profile
requires pointer and `unsigned long` widths to agree, so x64 Windows LLP64 is
rejected explicitly. `check_llp64.py --cc PATH_TO_UCRT64_GCC` verifies that
specific negative compile of the actual `imports/linux/virtio-rng/driver.c`
translation unit after a separate LLP64 control compile succeeds.

These controllers run as ordinary Windows processes and own a real QEMU virtual
PCI device through the generic register/DMA broker. This is not Windows kernel
driver loading, `.sys` binary import or physical device/interrupt qualification.

Primary prerequisite sources:

- [MSYS2 MINGW32 package repository](https://repo.msys2.org/mingw/mingw32/)
- [MSYS2 environment deprecations](https://www.msys2.org/docs/environments/)
- [MSYS2 Windows support policy](https://www.msys2.org/docs/windows_support/)
- [WinLibs standalone compiler](https://winlibs.com/)
- [WinLibs pinned release](https://github.com/brechtsanders/winlibs_mingw/releases/tag/16.2.0posix-23.1.3-14.0.0-msvcrt-r3)
- [QEMU official download guidance](https://www.qemu.org/download/)
- [QEMU Windows archive and checksums](https://qemu.weilnetz.de/w64/2026/)
