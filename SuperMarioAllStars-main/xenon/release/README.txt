SUPER MARIO ALL-STARS FOR XBOX 360 (RGH/JTAG)
Version 1.0

An unofficial port of the Super Mario All-Stars PC port (by Darhkfox, v1.1.1) to
the Xbox 360, as a homebrew .xex title. The four games (Super Mario Bros.,
The Lost Levels, Super Mario Bros. 2 and Super Mario Bros. 3) with the PC
port's launcher, true widescreen, two-player co-op, save states, rewind, cheats,
level select, the music player and the game's 82 achievements.

This package contains no game data. You need your own ROM. Super Mario and
related names are trademarks of Nintendo; this project is not affiliated with
or endorsed by Nintendo.


INSTALLING
----------
You need an Xbox 360 with RGH or JTAG, a homebrew dashboard (Aurora, FSD...)
and your own "Super Mario All-Stars (USA)" ROM (.sfc, 2 MiB, CRC32 925637C7).

1. Make a folder on the console's hard drive or on a USB drive.
2. Copy SuperMarioAllStars.xex into it.
3. Copy the ROM into the same folder (or into a "roms" folder inside it).
   Any file name works (.sfc or .smc); the game checks the contents.
4. Launch SuperMarioAllStars.xex from your dashboard.

The game creates its files beside the .xex: "saves" (battery saves, save
states, covers), smas_port.ini (settings), achievements.ini and two logs.

Optional: a "covers" folder with smb1, lostlevels, smb2, smb3, allstars and
background (.png or .jpg) for the launcher, and an "msu" folder for MSU-1
music. Options can create both folders.


CONTROLS
--------
Up to four controllers.

  Pause menu              Back
  Save / load state       Back + RB / Back + LB
  Rewind / fast forward   LT / RT
  4:3 <-> widescreen      Click the right stick
  Pause                   Guide button
  Quit to the dashboard   Hold Back + LB + RB for 3 seconds, or Quit in the
                          launcher

Hotkeys can be changed in Options > Hotkeys.


PERFORMANCE AND KNOWN LIMITATIONS
---------------------------------
- 4:3: 60 fps in the four games.
- Widescreen: 60 fps in SMB1, The Lost Levels, SMB2 and SMB3. SMB1 co-op in
  widescreen is at the limit and may drop a frame now and then. SMB3 co-op and
  the SMB3 battle game in widescreen are the least smooth: play them in 4:3 for
  a steady 60.
- Save states are not compatible with the PC version (battery saves and
  settings are).
- The "Super Mario All-Stars + Super Mario World" ROM is not supported.
- No keyboard, no file picker, no network features.
- Native Xbox 360 achievements are not included (the game's own achievements
  work, with their in-game banner).

If something goes wrong, the logs beside the .xex (smas_port.log and
SuperMarioAllStars.log) say what happened.


LICENSES
--------
The port is by Ttano, under the MIT licence. The notices of everything inside
the .xex are in THIRD_PARTY_NOTICES.txt in this package.
