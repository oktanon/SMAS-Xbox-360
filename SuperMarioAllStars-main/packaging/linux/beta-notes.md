**Beta: not tested on hardware yet.** This is a first Linux / Steam Deck build of the Super Mario All-Stars PC port. Please report anything that doesn't work (see below).

**No game data is included.** You need your own **Super Mario All-Stars (USA)** ROM (CRC32 `925637C7`), dumped from your own cartridge. This is a free fan project, not affiliated with Nintendo.

## Install
1. Download `SuperMarioAllStars-Linux-x86_64.AppImage` below. On a Steam Deck, do this in Desktop Mode.
2. Make it executable: right-click it, then Properties → Permissions → "Is executable". In a terminal you can run `chmod +x` on it instead.
3. Run it. Click the "ROM not found" card and choose your `Super Mario All-Stars (USA).sfc`. It's copied in, so you only do this once. You can also drag the ROM onto the window, or put it in a `roms` folder beside the AppImage.
4. **Steam Deck Gaming Mode:** in Desktop Mode, open Steam and choose Add a Non-Steam Game, then pick the AppImage.

## Notes
- **Fullscreen:** F11 or Alt+Enter switches between windowed and fullscreen.
- **Where your files go:** saves, states and settings are kept in `~/.local/share/SuperMarioAllStars`.
- **Crackling sound?** Raise Options → Audio → Latency, for example to 100 ms. Sound now goes through PulseAudio by default; setting `SDL_AUDIODRIVER` overrides that.
- **Not in this version:** the slang shaders and High Frame Rate are Windows-only. The built-in CRT filters work.

## Found a problem?
Please [open an issue](https://github.com/Darhkfox/SuperMarioAllStars/issues) and include:
- Your device: Steam Deck LCD or OLED, or your Linux distro and graphics card.
- Whether you were in Desktop Mode or Gaming Mode.
- What happened.
- The log file at `~/.local/share/SuperMarioAllStars/smas_port.log`.
