# Super Mario All-Stars for Xbox 360 (RGH)

A port of the [PC port of *Super Mario All-Stars*](SuperMarioAllStars-main/README.md) (by Darhkfox, v1.1.1) to an
**Xbox 360 with RGH/JTAG**, as a `.xex` title launched from the dashboard (Aurora, FSD...).

The project **contains no cartridge data**: the game runs from your own ROM. This is an unofficial fan project, not
affiliated with or endorsed by Nintendo; *Super Mario* and related names are trademarks of Nintendo. Please do not ask
for, or share, ROM files.

> The rest of the documentation (`AGENTS.md`, `docs/`, `runs/`) is in Spanish.

## Status: version 1.0

Everything the PC port has, on a physical console, except what a console can't have:

- the launcher, the four games and the All-Stars menu, with battery saves;
- the whole pause menu: save states, Level Select, Cheats, Sound Select, Music Player, Options, Credits;
- true widescreen, colour filters and palettes, two-player co-op (SMB1, The Lost Levels, SMB3);
- rewind and fast forward, and the game's 82 achievements with their in-game banner.

**Performance on the console:** 60 fps in 4:3 in the four games. In widescreen, SMB1, The Lost Levels, SMB2 and SMB3
reach 60 (about 14.5-15 ms per frame). SMB1 co-op in widescreen sits at the limit (about 16 ms) and may drop a frame
now and then. SMB3 co-op and the SMB3 battle game in widescreen are the least smooth.

**Not in this version:** native Xbox 360 achievements (plan in [`docs/ACHIEVEMENTS_X360.md`](docs/ACHIEVEMENTS_X360.md)).

**Known limitations**

- Save states are not compatible with the PC version (different byte order). Battery saves, settings and PNGs are.
- The *Super Mario All-Stars + Super Mario World* ROM is not supported.
- No keyboard, file picker, slang shaders, window modes or network features.
- Some dashboards show the file name instead of the title, although the `.xex` carries its XDBF.

Full roadmap: [`docs/ROADMAP.md`](docs/ROADMAP.md).

## Installing

You need an Xbox 360 with RGH/JTAG, a homebrew dashboard and your own *Super Mario All-Stars (USA)* ROM (`.sfc`,
2 MiB, CRC32 `925637C7`).

1. Make a folder on the console's drive or on a USB drive and copy `SuperMarioAllStars.xex` into it.
2. Copy the ROM into the same folder or into a `roms\` folder inside it. Any file name works; the game checks the
   contents.
3. Launch the `.xex`. The game creates `saves\`, `smas_port.ini`, `achievements.ini` and its logs beside it.

Optional: a `covers\` folder (`smb1`, `lostlevels`, `smb2`, `smb3`, `allstars`, `background` as `.png` or `.jpg`) and
an `msu\` folder for MSU-1 music. The release package's [`README.txt`](SuperMarioAllStars-main/xenon/release/README.txt)
says the same for players.

## Controls

Up to four controllers.

| Action | How |
|---|---|
| Pause menu | Back |
| Save / load state | Back + RB / Back + LB (Options > Hotkeys) |
| Rewind / fast forward | LT / RT |
| 4:3 ↔ widescreen | Click the right stick |
| Pause | Guide button |
| Quit to the dashboard | Hold Back + LB + RB for 3 s, or Quit in the launcher |

## How the port differs from the PC version

The games, the emulation core, the achievements, the widescreen and co-op hooks, cheats, level select and music player
are **the same code**. The port changes shared code only inside `#if` blocks, with the original in the `#else`, so PC
and Android stay as upstream. What it adds:

- a native title (ID `534D4153`, "Super Mario All-Stars", with an XDBF like a retail game's);
- a platform layer in `SuperMarioAllStars-main/src/platform/xenon/`: video and the app's `Canvas` on vulkan4xenon,
  audio and controllers on SDL3, a main loop at 59.94 Hz;
- controller-only hotkeys, a save-slot screen with per-game states, Save & Quit to the launcher, covers cut from the
  ROM, and the game filters as 26 precompiled shader variants;
- performance work for the in-order 3.2 GHz PowerPC: a fast PPU identical frame by frame to the original
  (`SMAS_PPU_FAST`), drawing on two hardware threads, page-based CPU reads (`SMAS_CPU_FAST`), rewind on another
  thread ([`docs/PERFORMANCE.md`](docs/PERFORMANCE.md));
- fixes for bugs found in upstream (`SMAS_PORT_FIXES`, [`docs/UPSTREAM.md`](docs/UPSTREAM.md)).

## Building

Everything builds in **WSL (Ubuntu)** from a PowerShell at the repository root, with relative paths (not Git Bash,
which rewrites them). You need:

- GCC 15, binutils and newlib for `powerpc-xsdk-elf` in `xsdk-gcc` in the WSL home (`xsdk-main/docs/GETTING_STARTED.md`);
- `cmake`, `ninja`, `make`, `python3`, `g++`, and for the byte-order checks `qemu-ppc` and `powerpc-linux-gnu-g++`;
- the external repositories `xsdk-main/`, `vulkan4xenon-main/` and `SDL-xenon-xenon/` beside this README (not
  versioned here).

```powershell
wsl.exe -d Ubuntu --exec bash -c 'cd xsdk-main && make'
```

```powershell
wsl.exe -d Ubuntu --exec bash -c 'cd vulkan4xenon-main && make XSDK=../xsdk-main'
```

```powershell
wsl.exe -d Ubuntu --exec bash -c 'cd SDL-xenon-xenon && cmake -S. -Bbuild-xsdk -GNinja -DCMAKE_TOOLCHAIN_FILE=$PWD/../xsdk-main/cmake/xsdk.cmake -DCMAKE_BUILD_TYPE=Release && cmake --build build-xsdk'
```

```powershell
wsl.exe -d Ubuntu --exec bash -c 'cmake -S SuperMarioAllStars-main/xenon -B build-xenon -GNinja -DCMAKE_TOOLCHAIN_FILE=$PWD/xsdk-main/cmake/xsdk.cmake -DCMAKE_BUILD_TYPE=Release && cmake --build build-xenon --target SuperMarioAllStars'
```

The release package (the `.xex`, `README.txt` and `THIRD_PARTY_NOTICES.txt`, plus a `.zip`) goes to
`build-xenon/release/`:

```powershell
wsl.exe -d Ubuntu --exec bash -c 'bash SuperMarioAllStars-main/xenon/release/make_release.sh 1.0'
```

Shaders, test titles and compiler flags: [`AGENTS.md`](AGENTS.md) §3.

## Testing

Only the physical console closes a milestone.

1. **Byte order** (no console): `SuperMarioAllStars-main/xenon/qemu/compare.sh` runs the same four-game script with
   `smastool` on x86 and on big-endian PowerPC under `qemu-ppc` and compares memory and framebuffer hashes;
   `compare_ppu.sh` compares the fast PPU frame by frame. See [`docs/ENDIANNESS.md`](docs/ENDIANNESS.md).
2. **Xenia** (`XSDK Xenia/`): a smoke test only, not valid for performance.
3. **The console**: each test is a folder in [`runs/`](runs/README.md) with its `run.md` and the logs it returned.

## Working in this repository

Read [`AGENTS.md`](AGENTS.md) first. In short: one task is one commit and one note in [`docs/tasks/`](docs/tasks/)
(indexed in [`docs/CHANGELOG.md`](docs/CHANGELOG.md)); each test is a folder in `runs/`; no absolute paths anywhere;
never commit the ROM, saves or builds; SDL-xenon is not modified. The work was done with AI agents following
`AGENTS.md`.

## Licenses and credits

- The PC port is by **Darhkfox** (MIT, [`SuperMarioAllStars-main/LICENSE`](SuperMarioAllStars-main/LICENSE)).
- **xsdk** and **vulkan4xenon** (MIT, with libxenon and Xenia code under the 3-clause BSD licence inside xsdk),
  **SDL 3** (zlib), the **DejaVu Sans** (Bitstream Vera licence) and **Luckiest Guy** (Apache 2.0) fonts, and
  **stb_truetype** / **stb_image** (public domain) go into the `.xex`. The release package carries all their notices
  in `THIRD_PARTY_NOTICES.txt`, built by `make_release.sh` from each project's own licence files.
- *Super Mario All-Stars* and its characters are the property of Nintendo. This repository contains no ROMs or data
  extracted from them.
