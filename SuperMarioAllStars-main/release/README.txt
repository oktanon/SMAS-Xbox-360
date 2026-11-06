SUPER MARIO ALL-STARS - PC & ANDROID PORT
Version 1.0

A native Windows and Android version of Super Mario All-Stars (SNES), with true
widescreen, simultaneous two-player co-op, a launcher, save states, rewind,
achievements, cheats, level select, a sound test and a music player.

The four games: Super Mario Bros., Super Mario Bros.: The Lost Levels,
Super Mario Bros. 2 and Super Mario Bros. 3.


=====================================================================
 IMPORTANT: YOU NEED YOUR OWN ROM
=====================================================================

This download contains NO game data. It runs from a ROM of your own copy of
the cartridge:

    Super Mario All-Stars (USA).sfc      (CRC32 925637C7)

Without it no game will start. Other versions/dumps of All-Stars also run (the
launcher shows their file name in amber), but widescreen, cheats and the level
select are tuned for the USA version.

This is an unofficial fan project. It is not affiliated with, endorsed by or
connected to Nintendo. Super Mario and all related names are trademarks of
Nintendo. Please do not ask for, or share, ROM files.


=====================================================================
 WINDOWS
=====================================================================

Requirements: Windows 10 or 11 (64-bit), a graphics card with Direct3D 11.
Nothing else needs installing.

1. Unzip the folder anywhere you like (not inside Program Files).
2. Put "Super Mario All-Stars (USA).sfc" in the same folder as
   SuperMarioAllStars.exe (or in a "roms" folder next to it).
3. Run SuperMarioAllStars.exe.

The ROM's location is remembered. If it isn't found, the launcher says
"ROM not found" - copy the ROM in while the launcher is open and it is picked
up within a second.

Optional shortcuts: start a game directly with
    SuperMarioAllStars.exe --game smb1      (or smbll, smb2, smb3, allstars)

Files the game creates next to the exe:
    smas_port.ini        your settings (delete it to reset all settings)
    achievements.ini     your achievements
    saves\               battery saves (smas.srm) and save states
    screenshots\         screenshots (F12)


=====================================================================
 ANDROID
=====================================================================

Requirements: Android 6.0 or later, 64-bit phone or tablet.

1. Copy the APK to your device and open it. Android will ask you to allow
   installing apps from that source - allow it, then install.
2. On first launch the card says "ROM not found": tap it and pick your
   "Super Mario All-Stars (USA).sfc" in the file picker. It is copied into the
   app, so you only do this once.
   (Or copy the ROM with a computer into
    Android/data/com.darhkfox.allstars/files, or a "roms" folder there.)

Saves, states and settings are kept in the app's own storage. Screenshots go to
Android/data/com.darhkfox.allstars/files/screenshots. Leaving the app saves your
game progress.


=====================================================================
 CONTROLS
=====================================================================

                      Keyboard (player 1)    Controller (Xbox layout)
  D-pad               Arrow keys             D-pad / left stick
  B / A / Y / X       Z / X / A / S          bottom / right / left / top
  L / R               Q / W                  LB / RB
  Start               Enter                  Menu / Start
  Open the menu       Esc or Right Shift     View / Select
  Game's Select       (Menu > Press Select)  Left stick press
  Save / load state   F5 / F8
  Fast forward (hold) Tab                    RT
  Rewind (hold)       Backspace              LT
  Fullscreen          F11 or Alt+Enter
  Screenshot          F12

  Player 2 keyboard:  I J K L (D-pad), N = B (jump), H = Y (run), M = A,
                      U = X, P = Start

PlayStation, Switch Pro and USB/SNES-style controllers work too. Everything can
be rebound under Menu > Options > Controls (keyboard and controller, both
players). Each controller keeps its player number when others are plugged in or
out.

The games only need their own Select button for the All-Stars save menu (pause,
then Select): use the left stick press, or "Press Select In Game" in the menu,
or set Controls > Select Button to "Tap: Game Select - Hold: Menu".

Android touch controls: an SNES pad over the game (D-pad bottom left, A B X Y
bottom right, L and R at the sides, Select and Start bottom centre, Menu top
left). They hide while a controller is in use. Android's Back button opens and
closes the menu.


=====================================================================
 TWO-PLAYER CO-OP (Super Mario Bros., The Lost Levels, Super Mario Bros. 3)
=====================================================================

Mario and Luigi play at the same time. Two ways to start:
  - On the game's title screen choose CO-OP GAME (under 1 PLAYER GAME /
    2 PLAYER GAME, or under BATTLE GAME in SMB3), or
  - Menu > Options > Multiplayer > Two-Player Co-op.
Choosing 1 PLAYER GAME on a title turns co-op off again.

Player 1 (controller 1) is Mario - or Luigi in a Lost Levels LUIGI GAME.
Player 2 (controller 2, or the player 2 keys) is the other brother.

How it plays:
  - Lives are shared (a co-op game starts with at least 10). Both brothers can
    stomp, collect power-ups, hit blocks, ride lifts, take pipes, vines, doors
    and the flagpole.
  - If one brother dies while the other plays on, he comes back beside the
    other one, small and blinking, for one life.
  - SMB1 / Lost Levels: whoever is further right moves the screen; the one
    behind is held at its left edge.
  - SMB3: the screen follows the point between the two. The status bar and
    the World box show both brothers, and the level start card reads
    "MARIO START! / LUIGI START!".
  - Left idle: a brother whose controller hasn't been touched for 3 seconds
    while the other plays on can't be hurt by enemies, and is brought along
    to the other if the screen leaves him behind (so one person can play with
    co-op left on). He becomes normal again as soon as his controller is used.
  - To keep co-op smooth, the emulated console runs at double speed while
    co-op is on, so busy scenes don't slow down. One-player games keep the
    original console timing exactly.
  - Save states and rewind keep both players.


=====================================================================
 WIDESCREEN
=====================================================================

All four games can fill a 16:9, 16:10 or 21:9 screen with real game content,
not stretched (Menu > Options > Display > Aspect Ratio; "Auto" fills your
display). The status bars move to the screen edges.

  Widescreen Enemies: "Active In Full View" (enemies come in from the edges of
  the wide picture and are active across it) or "Original Timing" (enemies
  behave exactly as in the 4:3 original).

Some full-screen scenes (world maps, boss rooms, some rooms) stay 4:3 or are
shown centred, as they were designed as a single screen.


=====================================================================
 MENU & OPTIONS (highlights)
=====================================================================

  Save states    9 slots (choose the slot in the menu); F5 saves to it, F8
                 loads it. A state can be of any of the four games.
  Rewind         hold Backspace / LT to go back about the last minute of play.
  Level Select   any world and level of all four games.
  Cheats         per game (infinite lives, star power, invincibility, ...).
  Sound Select   the classic sound test.
  Music Player   named track lists for every game.
  Display        window / borderless / fullscreen, window size, aspect ratio,
                 scaling, VSync (turn it off for G-SYNC / FreeSync), black
                 frame insertion, high frame rate (100 Hz+ displays).
  Graphics       filters (sharp bilinear, CRT scanlines, aperture grille, LCD
                 grid, ...), screen curvature, sprite-limit removal.
                 Windows: Slang Shader runs RetroArch slang presets from the
                 "shaders" folder next to the exe (needs librashader.dll).
                 Seven CRT presets come included: CRT Lottes, CRT Geom,
                 CRT Royale, CRT Easymode, CRT Aperture, zfast CRT and CRT Pi.
                 You can add more (e.g. the libretro slang-shaders collection)
                 to that folder. Some presets were made for a 4:3 picture and
                 can stretch or frame oddly in widescreen.
  Colour         colour profiles, brightness, contrast, gamma, colour-vision
                 assist, and Mario's Colours / Luigi's Colours (alternate
                 outfits, Super Mario Bros. and The Lost Levels only).
  Launcher       "Launcher Shows": one Super Mario All-Stars card (default)
                 or The Four Games as separate cards.


=====================================================================
 ACHIEVEMENTS
=====================================================================

82 achievements across the four games plus two for the whole collection
(Menu > Achievements).

Fair play: while achievements are on, cheats, level select, fast forward and
rewind are switched off. Turn achievements off under Options to use them.
A run counts when the game is started from the launcher or with Reset Game.


=====================================================================
 TROUBLESHOOTING
=====================================================================

"ROM not found"
    Put the .sfc file next to SuperMarioAllStars.exe (Windows) or pick it from
    the card (Android). The name doesn't matter; the contents do.

Card text in amber
    You have a different version/dump of the ROM. It runs, but widescreen,
    cheats and level select are made for the USA version.

Controller not working
    Menu > Options > Controls: check which controller each player uses and
    rebind if needed. Plug the controller in before starting, or reconnect it -
    it keeps its player number.

Mario looks green
    Menu > Options > Colour > Mario's Colours is set to an alternate outfit
    (e.g. Forest). Set it back to Original.

Uneven scrolling / tearing
    Turn VSync on (or off on a G-SYNC / FreeSync display). Fullscreen usually
    gives the smoothest result.

The picture goes black for a moment
    If the graphics driver resets, the picture comes back by itself and the
    game keeps running.

Reset all settings
    Close the game and delete smas_port.ini (achievements and saves are kept).


=====================================================================
 KNOWN ISSUES
=====================================================================

  - In co-op, the Super Mario Bros. 3 title's CO-OP GAME line sits close under
    BATTLE GAME.
  - Android: the MENU button overlaps the first letter of MARIO in the
    SMB1 / Lost Levels status bar, and the SELECT button covers "TYPE A" on the
    file-select screen.
  - Super Mario Bros. 5-4 (widescreen): a firebar can hit a player who jumps
    just above the top of the screen.


=====================================================================
 CREDITS & LICENCES
=====================================================================

Super Mario All-Stars PC Port
  YouTube   youtube.com/adhgaming
  Reddit    u/Darhkwing
  GitHub    github.com/Darhkfox
  X         x.com/ADH_Gaming
  Patreon   patreon.com/c/TeamRedTail

Third-party components:
  SDL2 (Android build) - Sam Lantinga and contributors, zlib licence.
  Luckiest Guy font - Astigmatic, Apache License 2.0.
  librashader (Windows slang shaders) - Mozilla Public License 2.0
    (librashader-LICENSE.md; source: github.com/SnowflakePowered/librashader).
  Included slang shader presets, unmodified from the libretro slang-shaders
    collection (github.com/libretro/slang-shaders):
      CRT Lottes - Timothy Lottes, public domain.
      CRT Geom - cgwg, Themaister and DOLLS, GPL v2 or later.
      CRT Royale - TroggleMonkey, GPL v2 or later (helpers MIT).
      CRT Easymode, CRT Aperture - EasyMode, GPL.
      zfast CRT - Greg Hogan (SoltanGris42), GPL v2 or later.
      CRT Pi - davej, GPL v2 or later.
    See shaders\README.txt and shaders\LICENSE-GPL-2.0.txt. Each shader file
    keeps its own copyright and licence notice.

Super Mario All-Stars, Super Mario Bros. and all related characters and names
are property of Nintendo. This project contains no Nintendo code or data.
