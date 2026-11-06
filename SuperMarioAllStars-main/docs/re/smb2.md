# Super Mario Bros. 2 (All-Stars) — reverse-engineering findings

The code runs while `$7FFF00 == 6` (NMI `JSL $118003` → `$1191C2`). The game logic lives in banks
`$11-$15` (main flow `$11`, objects/player `$12`, graphics/HUD `$13`, scrolling/rendering `$14`,
tables and BG3 `$15`). The SNES version keeps the NES SMB2 RAM layout for most game variables.
All RAM addresses are `$7E` WRAM; the code accesses them through the `$00-$3F` mirrors.

**How findings were checked:** `smastool` (`re_A/smastool.exe`) with save states in `re_A/states/`,
using `poke` plus a screenshot or HUD check, `watch`/`rwatch` to find the code that writes or reads
an address, `trace`, `vramlog` with `camaddr`, and `disasm`. Python scripts in `re_A/` render
areas straight from RAM: `render_area.py`, `render_bg23.py` and `check_ring.py`. **V** = verified,
**C** = read from code but not tested in game, **U** = unverified.

---

## 1. State detection

### 1a. Which screen is running

Key bytes:

| addr | meaning |
|---|---|
| `$0100` | NMI mode. `$C0` = gameplay NMI (level scroll/stream path, `$1191E9`). `$40` = simple NMI for title, char select, card, bonus and ending (`$1191A8`). Bit 7 is tested with `BIT $0100` at `$1191E0`. |
| `$0217` | `$80` = an area is loaded and running (set at `$1181F4`, cleared at `$118192/$11819D/$118B81/$118DBB`). `$01` = char select / level card (`INC` at `$118995`). `$00` otherwise. When non-zero the NMI also runs the animated-CHR DMA (`$138B37`). |
| `$0775` | BGMODE shadow (written to `$2105` by the NMI at `$1192D1`). `2` = title/story, char select, Bonus Chance (all use offset-per-tile). `1` = normal areas, level card, game over, ending. `9` = mode 1 + BG3 priority, used in areas where `$0772 != 0` (caves and indoors). |
| `$04ED` | Game-mode request, handled by `$118B31` and then cleared. `0` = in game, `1` = died with lives left (goes to char select), `2` = game over, `3` = level clear (Bonus Chance if `$062B > 0`, then next level; ending if `$0533 == $13`), `4` = warp. **V**: each value was poked in gameplay and the result seen. |
| `$0050` | Player state. `7` = dying (**V**, poke → death). `8` = going through a door (poke shows the door animation). `4` = entering a jar (**C**, `$1287B0`). |
| `$022B` | Pause-window step (1..5 while opening, `4` while open, `1,0,$FF,0` while closing). It is `0` in normal play. The warp screen also leaves it at `$FF`, so only trust it when `$0217 == $80`. |
| `$0789` | Set to `1` once the Bonus Chance slot game has started (`$118E68`). It is `0` on the title screen. |

Values measured from save states. The columns are `$0100 $0217 $0775 $0050 $022B $0789`:

| screen | values | |
|---|---|---|
| title / story | `40 00 02 00 00 00` | `$02E6` = `$FF` once Start is pressed: the character select is wiped in by scrolling the title's parts (offset-per-tile). The outermost 8 px column is blank black. |
| character select | `40 01 02 ..` | The frame is a lattice of 16 px diamonds, orange and blue alternating both ways. It sits inside a blank 8 px column at each screen edge (BG1 tile `$0FE`). Tile column 1 holds a diamond's left half and column 30 its right half, free of the frame's inner outline. The wipe out to the card scrolls parts of the map (offset-per-tile), and the card is already in those columns as it starts. |
| level card ("WORLD x-y") | `40 01 01 ..` | same as char select except BGMODE. BG1 (hofs 256): background tile `$0C8`, with 2x2-tile motifs (`$C0`, `$C2`, `$C4`, `$C6`; tiles +1, +16, +17) scattered about one per 36 tiles. Blank columns 0 and 31 (`$0FE`) cut some motifs in half. The level picture and the logo are BG1 too. Same in every world. |
| gameplay (any area) | `C0 80 01/09 00` | |
| pause menu | `C0 80 .. .. 04` | the pause loop is `$11849C-$118588` |
| death animation | `C0 80 .. 07` | |
| game over screen | `C0 00 01 07` | `$11 = $13` |
| Bonus Chance (slots) | `40 00 02 .. .. 01` | |
| warp transition (sampled 400 frames after `$04ED=4`) | `C0 00 01 .. FF` | |
| ending (Subcon room, contributors) | `40 01/00 01` | |

**Recommended gameplay check for widescreen:** `$7FFF00==6 && $0100==$C0 && $0217==$80`. It is true
in every level area, including pause and the death animation. It is false on every 4:3 screen
listed above and during area loads, where `$0217` is 0 and the screen is force-blanked.
Pause = the same check plus `$022B != 0`.

For exact screen IDs, hook the program counter at these routine entries (all in bank `$11`):

| routine | address |
|---|---|
| SMB2 boot (reads the save buffer `$7FFB00`) | `$118122` |
| title init | `$128F48` |
| char select / card init | `$118927` |
| area init (common) | `$1181A8` |
| vertical-area loop | `$1183E7` |
| horizontal-area loop | `$1182CE` |
| subspace/jar entry | `$1185A0` |
| pause | `$11849C` |
| game-mode dispatcher | `$118B31` |
| mode 1 (restart) | `$118B46` |
| mode 2 (game over) | `$118BB3` |
| mode 3 (level clear) | `$118D9D` |
| Bonus Chance | `$118E5D` |
| mode 4 (warp) | `$118CBE` |
| ending | `$11905A` |

### 1b. World, level, area

| addr | meaning | |
|---|---|---|
| `$0635` | world, 0-6 | **V** (poke plus level select) |
| `$0533` | level, 0-19 (1-1=0 … 7-2=19). This is the level of the current area. | **V** |
| `$0629` | level within the world, 0-2. Computed by the card code as `$0533 - WorldFirstLevel[$0635]`. | **V** |
| `$11C9B4` | ROM table WorldFirstLevel = `00 03 06 09 0C 0F 12 14` | **V** |
| `$0534` | area, 0-9 | **V** (1-1: start area 0 is vertical, the door leads to area 2) |
| `$0535` | entry page | **V** |
| `$0536` | transition type (0 = level start, 1 = door) | **V** |
| `$0537` | player's current page (index into the door-pointer table) | **C** |
| `$051F + 2*page` | Per-page door/area pointers, 2 bytes each: level, then `area<<4 \| entry page`. Loaded by `$14E30F`. | **V** (1-1 page 3 = `00 20` → level 0, area 2, page 0) |
| `$0541` | last page index of the current area (area length − 1) | **V** (1-1 area 0 = 3, area 2 = 9) |
| `$04E8-$04EB` | restart checkpoint: copy of `$0533-$0536`, saved by `$14E02E`, restored after death by `$14E061` | **V** (used by level-select recipe B) |
| `$04E2-$04E7` | checkpoint player X, Y, screen X/Y, Y speed, state | **C** |
| subspace screen | Entering (`$14F0F7`) saves the camera: `$050B` = `$0200` (scroll low), `$050D` = `$0201`, `$0516` = `$04BF` (page). `$12F394` copies the 16 metatile columns from `(camera + 8) & ~15` of the current area into **page 10 of area 0** (`$7E2960`), column j to 15 - j, through `$12F427` (hidden mushrooms `$41`/`$42` become the mushroom). In subspace the renderer reads that page for any area (`$14F20C`: Y = 10, area offset 0) and swaps each tile's halves with H flip; the camera is `$0A00`. Only BG1 and OBJ are on. | **V** (1-1: screen tile c = level tile 73 - c, all 960 tiles matched; the port's wide margins continue the mirror) |
| `$0628` | 0 = normal, 1 = inside a jar, 2 = subspace. The renderer mirrors tiles when it is 2 (`$14F33C`), enemy spawning is off when it is 2, and the horizontal loop jumps to `$1185A0` when it is non-zero. | **C** only. Forcing a jar in 1-1 crashed because 1-1 has no jar area. |
| jar tops | Pressing down on a tile enters it (`$12878C`): `$6A` or `$6E` normally, `$6F` in subspace (the warp jars). `$6E` jars lead to a jar room (`$0628 = 1`, the room on page 10, past the area's pages, BG2/BG3 one fixed screen at scroll 0, the dark inside a W1 colour-math window). The `$6A` jars tried (5-2, 6-1) led to another area. Jar rooms entered: 1-2 (x 928), 2-1, 2-2, 2-3, 6-1 (two) (2026-10-21). | **V** (entered at 21:9) |
| player position | X `$14`:`$28`, Y `$1E`:`$32` (NES layout) | **V** (poked onto a jar) |

### 1c. Character

`$008F`: 0 = Mario, 1 = Peach, 2 = Toad, 3 = Luigi. The select screen shows them in the order
Mario, Luigi, Toad, Peach. **V**: each character was picked and the screenshot shows the right sprite.
Per-character stats are copied to `$0548-$055E` when the level starts (see §2).

### 1d. Horizontal or vertical area

`$00EB`: 1 = horizontal, 0 = vertical. **V**: the area loader branches on it at `$118250`
(`LDA $EB / BNE horizontal / BRL $8329 vertical`), and 1-1 area 0 has `$EB=0` while area 2 has `$EB=1`.

---

## 2. Cheat addresses (all **V** unless marked)

| what | addr | encoding / notes | test |
|---|---|---|---|
| lives | `$04EE` | Remaining tries, binary. The select screen shows "EXTRA LIFE… n" with n = `$04EE` after the death decrement. A death that brings it to 0 gives game over. Values over 99 show garbage (`$FF` shows "P5"). | poke 8, die → shows 7. poke 1, die → GAME OVER. |
| health | `$04C3` | `$0F` = 1 heart, `$1F` = 2, `$2F` = 3, `$3F` = 4. Filled hearts = (value>>4)+1. Below `$10` the character is small; a hit at `$0F` kills. | poke `$0F` → 1 heart and small Mario. poke `$3F` → 4 full hearts. |
| max health | `$04C4` | 0 = 2 hearts, 1 = 3, 2 = 4 (the HUD draws `$04C4+2` hearts) | poke 2 → 4 hearts shown |
| star / invincibility timer | `$04E1` | Counts down by 1 every 8 frames (`$14DFB5`, when `$10 & 7 == 0`). At 8 the music changes. Non-zero = invincible and palette-flashing. | poke `$FF`: a Shyguy dies on contact and health does not drop. Without it, touching the Shyguy costs one heart. |
| cherries | `$062A` | Count collected. Cleared by the game-mode handler (`$118B3D`: death, level end). The NES rule that 5 cherries spawn a Starman is **U** here. | Collecting the 1-1 cherry: 0 → 1, and its tile `$4E` in level RAM became `$40` |
| Bonus Chance coins | `$062B` | Number of slot plays; each play uses one. 0 skips Bonus Chance (`$118E55`). | poke 3 + `$04ED=3` → Bonus Chance, coins shown "×2" after one play |
| levels cleared per character | `$062D + char` (`$062D` Mario, `$062E` Peach, `$062F` Toad, `$0630` Luigi) | Capped at 99. Shown on the ending "CONTRIBUTOR" screen. | forced level clear → Mario shows 1 |
| coins picked up in subspace | `$062B` | Presumably the same counter as the Bonus Chance coins (**U**: subspace not played through) | |
| **character stats** | `$0548-$055E` | NES CharacterStats layout. `$0548-$054D` pull-up speeds. `$054E/$054F` jump speed standing (no item / carrying). `$0550/$0551` charged. `$0552/$0553` running. `$0554` quicksand. `$0555` float time. `$0556/$0557/$0558` gravity (A released / A held / quicksand). `$0559-$055B` run speed right. `$055C-$055E` run speed left. Reloaded from ROM by `$12FA35/$12FA45` at every level start and after death, but **not** on area change. | see the next two rows |
| "super jump always" | `$054E,$054F,$0552,$0553 = $90` (or `$98` = the charged value) | | Mario's jump peak rose from 52 px to 101 px |
| float for any character | `$0555 = $3C` (Peach's value) | `$04CA` is the running float counter | Mario with `$3C` hovers ~60 frames |
| force level clear / ending / warp | `$04ED = 3`, `$0533=$13` + `$04ED=3`, `$04ED = 4` | see §1a | **V** |
| debug | SRAM `$700007 != 0` | Enables a hidden freeze: L toggles it and R steps one frame (`$1182D2-$1182EC`, `$1183EB`) | **C** |

---

## 3. Level select

Level index = `WorldFirstLevel[world] + n`:

| world | levels |
|---|---|
| 1 | 1-1=0, 1-2=1, 1-3=2 |
| 2 | 2-1=3 … |
| 3 | 3-1=6 |
| 4 | 4-1=9 |
| 5 | 5-1=12 |
| 6 | 6-1=15 |
| 7 | 7-1=18, 7-2=19 |

**Recipe A (from the title screen, recommended):**

1. Wait until the SMB2 title/story screen is up. Condition: `$7FFF00==6`, `$0775==2`, `$0217==0`,
   `$0100==$40`, `$0789==0`. The boot code at `$118122` has already loaded `$0635/$0533` from the
   save buffer `$7FFB00/01` by then, so do not write before the title starts.
2. Write `$7E0533 = level (0-19)` and `$7E0635 = world (0-6)`. They must agree: the world must
   contain the level.
3. Press Start, hold 2 frames. The character select appears; A is accepted as early as 60 frames later.
   Its init (`$118927`) builds the card text from `$0635/$0533`.
4. Press A (or move the cursor first). The card shows "WORLD w-l" with the right pips ~160 frames
   later. Gameplay is running ~300-400 frames after A (`$0217==$80`).

Tested and screenshotted:

| target | `$0533` / `$0635` | result |
|---|---|---|
| 2-1 | `03/01` | card "WORLD 2-1", desert at night with pyramids |
| 4-2 | `0A/03` | ice area |
| 5-3 | `0E/04` | cave |
| 7-2 | `13/06` | cloud start of the castle level |

Side effects:
- The small picture on the card keeps the scenery of the world whose graphics were loaded at boot.
  It is cosmetic: the text, pips, palette and the level itself are correct.
- Lives and counters are unchanged.
- "SAVE & CONTINUE" from the pause menu will save the new world.
- `$0534-$0536` are zeroed by the level start at `$11883A`, so every level starts at area 0.

**Recipe B (mid-game warp through the death path, also V):** during gameplay write `$0050=7`
(start dying), `$04E8=level`, `$04E9=$04EA=$04EB=0`, `$0533=level`, `$0635=world`. Let the game
reach the char select (~300 frames; mode 2) and press A. Tested: 1-1 → 6-2 (`$10/$05`). The card
here also shows the correct world picture, because `$12F9BF` reloads the world graphics. It costs
one life, so add 1 to `$04EE` to compensate. Setting `$04E9/$04EA` should also allow restarting in
another area (**U**).

---

## 4. Rendering (widescreen)

### 4.1 PPU setup during gameplay

All BG layers use 64×64-tile maps and 8×8 tiles. VRAM addresses are word addresses.

| layer | role | map base | chr base | bpp |
|---|---|---|---|---|
| BG1 | playfield | `$0000` (BG1SC `$03`) | `$2000` | 4 |
| BG2 | background (SMAS addition) | `$1000` (BG2SC `$13`) | `$2000` | 4 |
| BG3 | far background / overlay | `$4800` (BG3SC `$4B`) | `$5000` | 2 |
| OBJ | sprites; the HUD is sprites | OBJSEL `$03`: base `$6000`, 8×8/16×16 | | |

There are two area profiles (**V** from PPU dumps of 1-1, 7-2, 4-2 and 5-3):

- **Outdoor** (`$0772 == 0`): mode 1. TM = BG1+OBJ (`$11`), TS = BG2+BG3 (`$06`), CGWSEL `$02`
  (math adds the sub screen), CGADSUB `$20` (backdrop only). BG2 and BG3 therefore appear only where
  BG1 and OBJ are transparent. HDMA channels 4-6 are on (`$023A=$70`), see 4.5.
- **Cave / indoor** (`$0772 != 0`): BGMODE `$09` (mode 1 + BG3 on top). TM `$15` (BG1+BG3+OBJ),
  TS `$13`, CGADSUB `$64` (half-add on BG3 and backdrop). BG3 is a translucent overlay, BG3
  scrolls 1:1 with BG1, and there is no HDMA.

NMI shadows, copied in `$11925F-$11930E`:

| shadow | register |
|---|---|
| `$0200/01` | BG1HOFS |
| `$0206/07` | BG1VOFS; the NMI adds `$04C6` (shake) to the low byte |
| `$0202/03` | BG2HOFS |
| `$0208/09` | BG2VOFS |
| `$0204/05` | BG3HOFS |
| `$020A/0B` | BG3VOFS |
| `$0235/36` | W12SEL/W34SEL |
| `$0237` | WOBJSEL |
| `$0238/39` | CGWSEL/CGADSUB |
| `$023B/3C` | TMW/TSW |
| `$023D-$023F` | COLDATA |
| `$0243/44` | TM/TS |
| `$024C` | BG1SC |
| `$0775` | BGMODE |
| `$FE` | INIDISP |
| `$023A` | HDMAEN |

OAM: buffer `$0800-$09FF`. Per-sprite byte `$0A20+i` holds bit0 = X bit 8 and bit1 = size. The NMI
packs these into `$0A00-$0A1F` (`$1193A0`) and DMAs `$0220` bytes from `$000800` to `$2104`.
No IRQ is used (`$4200 = $81`).

### 4.2 Camera variables (**V**, logged per frame while scrolling)

**Horizontal areas**
- camera X = `$04BF` (page) : `$04C1` (low byte).
- right edge = `$04C0:$04C2` = camX + 255.
- The camera has no Y movement: `$CA:$CB` = 0 and BG1VOFS = 0 apart from shake.
- The camera is kept in `[0, $0541*256]`. It follows the player so that the player sits at screen
  x = `$78`: `$BA = PlayerX − $78 − camX`, then the scroll loop at `$14EE7A/$14EE99` moves 1 px
  per step.
- BG1HOFS = `camX XOR $100` (9 bits).
- BG2HOFS = `((camX>>1) XOR $100) & $1FF`, or the camera itself (`$0200/$0201`) when `$02C3` != 0 (`$14EEC6`; the boss rooms, e.g. 1-3's Mouser room).
- BG3HOFS = `((camX>>2) XOR $100) & $1FF`. In `$0772` areas BG3HOFS = BG1HOFS.

**Vertical areas**
- camera Y = `$CA` (high) : `$CB` (low), linear pixels.
- camera X = 0 and BG1HOFS = 0.
- BG1VOFS = camY + an offset that is a multiple of 16: `$100` near the area's top, other values further down (camY `$1B0` → `$3E0`, `$2D0` → `$230`): the scroll is kept separately (poking `$CA/$CB` doesn't change it) and trails the camera by ~4 px while moving.
- BG2VOFS = BG3VOFS = camY/2 + `$100` (outdoor).
- camY max = `$0541*240`.

**Player position:** X = `$14:$28`, Y = `$1E:$32`, screen X = `$0428`, screen Y = `$042A:$042B`,
X speed = `$3C`, Y speed = `$46` (signed, negative = up; **V**).

### 4.3 Tilemap arrangement and streaming (**V** with `vramlog`, `camaddr 7E04BF 7E04C1` or `7E00CA 7E00CB`)

**Horizontal areas.** BG1 uses the top two 32×32 screens as a 512-px ring:
map x = `(worldX XOR $100) & $1FF`. Even pages go to VRAM `$0400`, odd pages to `$0000`. Rows 0-29
hold all 15 metatile rows. The last metatile row (world y 224-239) is never on screen, because
BG1VOFS = 0 and 224 lines are shown.
- Column pointers `$CE,X` hold `col<<4 | page`. `$CF` is the left stream pointer, `$D0` the right one
  (inits at `$14ED60`).
- When camX crosses a 16-px boundary (tracked in `$0538`), `$051E` is set. `$14F1ED/$14F203`
  advance the pointers and `$14F24C` renders one 16-px column: two 8-px tile columns of 30 words.
- The column is queued at `$0300/$0302`. Entry format: VRAM address high, VRAM address low,
  flags (`$80` = +32 increment), byte count − 1 (`$3B`), then data. The queue ends with `$FF`.
  The NMI uploads it (`$119321 → $1194C7`) in the same vblank as the new scroll.
- Moving right draws column `(camX & ~15) + $110`. Moving left draws `(camX & ~15) − $30`.
  Measured at camX `$1B1`: VRAM `$0418` = world x `$2C0`. At camX `$1AF` going left: VRAM `$000E`
  = world x `$170`.
- **Always-valid window: `[(camX&~15) − 48, (camX&~15) + 288)`.** That is **48-63 px beyond the left
  edge and 17-32 px beyond the right edge**. Columns outside that window hold whatever was there
  before; they are right only if the player came from that side (up to ~224 px after running right).
  `check_ring.py` compares VRAM with RAM level data and confirms this.
- At area load, about 30-32 columns are drawn starting at `entry page*256 − 80` (`$CE` starts at `$CF−$20`). When the entry page is 0,
  the left margin holds the **tail of page 9** (junk). The port must blank or clamp world x < 0 and
  x ≥ (`$0541`+1)*256.

**BG2 and BG3** stream the same way in their own scroll space, so the margins are the same in screen
pixels. BG2: pointers `$0257/$0258`, flag `$0265`, code `$13F21D/$13F238/$13F277`, queue
`$7F0000/$7F0002`. BG3: pointers `$0725/$0726`, flag `$073A`, code `$15821B/$158236/$15826D`,
queue `$7F0800/$7F0802`. Measured: BG2 column at map x 32 when BG2HOFS = `$110` (right edge
+17); BG3 likewise.

**Vertical areas.** Only the left 32 map columns are used (VRAM `$0000` and `$0800`, map x = world x
0-255). The ring is vertical: map y = `(worldY + $100) & $1FF`. Rows are written one 8-px tile row
(32 words) per 8 px of scroll by `$14EC4C` (pointer update at `$14EB5F/$14EBB1/$14EBCF`, pointers
`$CF` top and `$D0` bottom as `row<<4 | page`).
- Going down, the new row is ~64-80 px below the visible bottom (row at camY+288 when camY = `$130`).
- Going up, the new row is ~112-124 px above the top (VRAM `$0940` = world y 592 written at camY = `$2C4`).
- **Map columns 32-63 hold junk from the title screen**, confirmed with bgshot.
- **Vertical areas wrap horizontally**: walking left from x=`$19` gives x=`$FB` with the same page,
  and the game draws the player twice (OAM #32/33 at x=509 and #34/35 at x=253). A widescreen view
  should show wrapped tiles (map x = screenX & 255) or blank margins.
  Enemies are not duplicated.

### 4.4 Area data kept in RAM (**V**: whole areas were rendered from RAM and matched the game, see `re_A/shots/render_a2full.png`, `render_a0.png`, `render_l42.png`)

- **All areas of the current level are in RAM**:
  - area `a` at **`$7E2000 + a*$0B00`**, 10 pages of `$F0` bytes each (page `p` at `+p*$F0`)
  - page = 15 rows × 16 columns, **1 byte per 16×16 metatile**, index = `row*16 + col`
  - horizontal: page p covers x `p*256..+255`, y 0-239
  - vertical: page p covers y `p*240..+239` (240 px per page, while the camera and object Y are
    linear), x 0-255
  - Levels are decoded at level start. Entering an area runs a pass that fixes up edge tiles.
  - Tile changes (grass pulled, cherries, bombed blocks) are written back into this RAM immediately
    (**V**: cherry `$4E` → `$40` at `$7E3854`).
- **Alternate-graphics flags** at the same offset `+$7000`: `$7E9000 + a*$0B00`. A non-zero byte
  selects the alternate table (world-4 ice, etc.).
- **BG2 layer data**: `$7F2000 + area*$1000`, same page layout. The metatile table pointer is the
  24-bit value at `$7E02DF`. ID `$FF` = the empty entry at `[$13A613]`.
- **BG3 layer data**: `$7FC000` + page*`$F0`. Table pointer at `$7E0744-46`. `$FF` = `[$15934F]`.
  BG3 is 2bpp, chr `$5000`.

**Metatile to tilemap conversion for BG1** (`$14F24C` horizontal, `$14EC4C` vertical):
- Table = pointer (`$11AD64[id>>6]` low, `$11AD68[id>>6]` high) in bank `$11`, which gives
  `$11AD6C`, `$11AF6C`, `$11B16C`, `$11B36C`.
- Entry = `table + (id & $3F)*8`: four SNES tilemap words **TL, TR, BL, BR**.
- If the flag byte at `+$7000` ≠ 0: table = `$15A4CE` (pointer stored at `$15A4C8`), index = the
  full `id*8`. Set `$076C`, and in world 4 (`$0635==3`) also `$0769`. Both are cleared for every
  tile (the loop returns to `$14F285`), so they only affect the flagged tile itself.

Post-processing, in this order:

| condition | change |
|---|---|
| `$0628==2` (subspace) | swap L/R and XOR `$4000`; with `$0769` also `(w&$FF)^$4A00` |
| `$0628==1` and the tile is `$AB` (jar) | OR `$2000` (priority) |
| `$0769` set | `(w&$FF)^$0A00` |
| `$0773` set and `$076C` set | `(w&$1FF)\|$1800` |

The vertical renderer has the flag, jar and `$0773` cases but no mirroring or world-4 case.
`re_A/render_area.py` implements all of this. `render_bg23.py` renders BG2 and BG3 from RAM.
CHR and palettes are whatever is currently in VRAM/CGRAM. Animated tiles are updated in chr
`$2000-$22FF` every frame.

### 4.5 HDMA, IRQ, windows, color math
- **Gameplay:**
  - Outdoor areas: HDMA channels 4/5/6 in indirect mode to COLDATA `$2132`, tables at
    `$1D20/$1D60/$1DA0` (built by `$14FAE7`). This is a vertical sky gradient seen through the
    backdrop color math. It is per-scanline only, so widescreen margins get it for free.
  - `$023A` is `$70` or `0` per area, from the table at `$11CF52`.
  - No scroll splits, no IRQ, no windows (TMW/TSW = 0).
- **Jar entry** (`$04EF != 0`): `$14D874` sets up channel 7 → WH0/WH1 (`$2126`), a window effect (**C**).
- **Pause:** HDMA channels 3 and 7 drive window 1/2 edges (`$138E3B`). The logic is WBGLOG `$15`,
  WOBJLOG `$06`, TMW `$17`, CGWSEL `$10`, CGADSUB `$B7` (dim). The menu is OBJ text plus BG3 at
  x ≈ 56-200. It is laid out for 4:3.
- **Warp screen:** channel 7 in mode `$42` → BG1HOFS (`$14D345`), a wavy effect.
- **Title/story, character select, Bonus Chance:** BG mode 2 with offset-per-tile (`$0775=2`, stored at
  `$128F65/$14D7A5`). The OPT table in BG3 map `$4800` is all `$2100` on the title and Bonus Chance
  screens, so every column except the leftmost 8 px shows BG1's (256,256) quadrant. On the char
  select the table is `$0000`, i.e. no offset.

---

## 5. Objects and the screen edge

### 5.1 Object table (NES layout)
- Arrays are indexed with X. The player uses the base address; objects 0-8 use base+1+X.

| field | player | objects |
|---|---|---|
| X high (page) | `$14` | `$15+X` |
| Y high | `$1E` | `$1F+X` |
| X low | `$28` | `$29+X` |
| Y low | `$32` | `$33+X` |
| X speed | `$3C` | `$3D+X` |
| Y speed | `$46` | `$47+X` |
| state | `$50` | `$51+X` (0 = empty, 1 = alive, 2 = dying/falling) |
| type | | `$90+X` (NES enemy IDs: `$01` red Shyguy, `$02` pink Shyguy …) |

- Other fields: `$0441+X` = index into the level enemy list (used to clear the "spawned" bit),
  `$A8+X` = being carried, `$86+X` timer, `$65+X`/`$046E+X` flags.
- Slots 0-4 take level enemies (spawn searches X=4..0). Slots 5-8 are for items and projectiles.
- **V** for position, state and type: logged each frame; spawn and despawn values match the code.
  Other fields **C**.
- The enemy list pointer is `($CC)`. Its format is a per-page count, then pairs of
  (type | `$80` once spawned, `col<<4 | row`).

### 5.2 Spawning (`$129D6A`, called from the per-frame object update at `$129B05`)
- **Horizontal:**
  `col = (camX_lo + T_lo[Y]) & $F0`, `page = camPage + T_hi[Y] + C`, with `Y = $6E`
  (1 = player moving right or still, 2 = moving left).
  - `T_lo` table at `$11978F`: `FF 18 E0`. `T_hi` at `$119791`: `E0 01 FF` (the two tables overlap).
  - Y=1 → offset **+$118**: spawn column at screen x 265-280, i.e. 9-24 px past the right edge.
    Measured: new enemies at rel x 271-280.
  - Y=2 → offset **−$20**: spawn column at rel −47..−32. Measured: −46/−47.
  - Only the side the player is moving toward spawns.
- **Area load** (`$12:9C3B`, horizontal): spawns what is already on screen by checking 24 columns
  starting at `(camX − $30) & $F0` (operands: `SBC #$30` at `$129C40`, count `LDA #$17` at
  `$129C4D`), calling the column check at `$129D92`. It ignores the spawn tables, so a widened
  range needs this sweep widened too.
- **Near-player check** (`$129E9D`): before spawning, `JSR $C1DA` sets `$0F` = player X lo −
  enemy X lo, and `LDA $0F / ADC #$18 / CMP #$30 / BCC` refuses the spawn when that is within ±24.
  Only the **low bytes** are compared, so an enemy ~256 px from the player also counts as "on top
  of the player". The 4:3 spawn column is never that far away; a widened one usually is.
- **Vertical** (`$129E02`): the same tables applied to `$CA:$CB`. Y alternates 1/2 by frame parity,
  or comes from `$D8`. That gives rows at camY+265..280 (41-56 px below the 224-line screen) and
  camY−47..−32.
- **To widen by N px per side:**
  - right offset = `$0118+N`: patch `$119790` (low byte) and `$119792` (high byte).
  - left offset = `$FFE0−N`: patch `$119791` (low byte) and `$119793` (high byte).
  - `$119791` is also Y=0's high byte. It must stay ≥ `$0A`, so that path keeps doing nothing.

- **Free slot** (`$129DDD`): `LDX #$04` searches slots 4..0 for `$51,X == 0` and returns at `$129DE6` when all are
  taken, leaving the entry unspawned. Nothing checks that column again until the spawn edge crosses it anew.
  **V**: 4-1 area 1 at 21:9, the Hawkmouth ($43, page 9 col D) came into range with the crystal, a Flurry,
  an Autobomb and two fireballs in slots 0-4, and never appeared (4:3: slots free). The port frees one that
  the 4:3 despawn range would have removed already (out of sight first).
- **V** 5-1 area 1 (waterfall) at 21:9: the Trouters ($13) at x $380/$3C0 came into the wide spawn range while four logs
  and a Trouter held slots 0-4 (the logs kept alive by the wide despawn range) and were refused at `$129DE6`; the
  port retries a refused entry while it stays in range, and frees a margin-only slot for one inside the game's
  own spawn range.
- Level-list types are the NES IDs + 5 in this version (Flurry `$29`, Autobomb `$26`, its fireball `$27`,
  Hawkmouth `$42`/`$43`, crystal `$44`). **C** from the 4-1 list and slots.

### 5.3 Despawning (`$12A7AF-$12A84B`)
- Each object is checked on alternate frames, by slot parity vs `$10`.
- An object is removed (state `$51+X` ← 0, spawned bit cleared in the list) unless
  `$11CC8C[type] & 8` (never despawn) or it is carried.
- **Keep ranges:**
  - Y: `[camY − $30, camY + $FF + $30]`
  - X: `[camX − $30, camX + $FF + $30]`
- The immediate operands to change:

| operand | instruction |
|---|---|
| `$12A7BF` | `SBC #$30`, top |
| `$12A7D0` | `ADC #$30`, bottom |
| `$12A7FA` | `SBC #$30`, left |
| `$12A80A` | `ADC #$30`, right |

- **V**: despawned at rel x −52/−54 (running right, checked every 2 frames) and at +304..307
  (running left).

### 5.4 Sprite X (**V**)
- `$12A6F9` computes per-object "offscreen" bits in `$ED`, one per 8-px column. It uses the X
  offsets at `$1197E4` (`00 08 10 18`) and the bit masks at `$1197E0` (`08 04 02 01`). A bit is set
  when `(objX + off) − camX` falls outside 0..255.
- The draw routines write the low byte of `objX − camX` as the OAM X, and **copy the offscreen bit
  into X bit 8** (`$0A20+i`). Examples: `$12C0ED` for 16×16 sprites, `$12A69B` for pairs.
- So OAM X is effectively a correct signed 9-bit value for anything within ±256 px. A Shyguy at
  rel +270 was in OAM at x=271, and objects off the left edge get `$1xx` (negative).
- **Nothing is clipped by the generic path**, so widescreen margins show objects. However, some
  object routines skip drawing or logic when `$ED`/`$EE` (horizontal/vertical offscreen) is
  non-zero. Sites that read `$ED`, to review:
  - `$12A2C2`, `$12AC8D`, `$12ACE6`, `$12AEDD` (skips drawing), `$12AFB5`
  - `$12BCE9`, `$12BEBC`, `$12C164`, `$12C171`, `$12C1A4`, `$12C717`
  - `$12C998`, `$12CBBC`, `$12CED2`, `$12D72B`, `$12D780`, `$12D78A`
  - `$12DC56`, `$12DCD9`, `$12DD31`, `$12E0A3`
- To make sprites in the margins behave, widen the offscreen test in `$12A6F9` (the camera compare
  at `$12A72E-$12A738`) as well as the spawn and despawn constants.

---

## 6. HUD (**V**)
- The only in-level HUD is the **life meter**: 16×16 sprites in **OAM #0 … #(`$04C4`+1)**.
- Position: **x = 8, y = 40, 56, 72, 88**. Tile from table `$11AB63` by health (`$082` = filled),
  attribute `$32` or `$34` (palette 1 or 2, priority 3). `$0A20+i` = 2 (large, X bit 8 = 0).
- It is drawn every frame by `$13FB12 → $13FB47`, from the area routine at `$14DF81`. The pause
  code draws it too (`$1184AA`).
- Constants:

| operand | instruction |
|---|---|
| `$13FB48` | `LDA #$28`, first Y |
| `$13FB68` | `LDA #$08`, X |
| `$13FB81` | `ADC #$10`, Y step |
| `$13FB8A` | `LDA #$02`, high bits |

- To anchor the meter to the widened left edge, use X = 8 − margin with X bit 8 set.
- There is no score, coin or timer display. The pause menu (OBJ + BG3, 4:3) shows world and lives.

---

## 7. Things a widescreen port must handle
1. **Emulator bug (all mode-2 screens render black):** the title/story, character select and Bonus
   Chance use BG mode 2, and the current PPU renders them solid black. This includes the given
   `states/smb2_title.state`. The cause is in `src/core/ppu.cpp` (~line 436), `Ppu::renderBg`:
   `if (opt) key = 0x7ffffffe;` makes the tile-cache key constant. Only the first tile of each line
   is fetched and repeated across the line, and that tile is opaque black on these screens.
   **V**: poking `$0775=1` makes the screens appear. A Python simulation of real offset-per-tile
   produces the proper Bonus Chance screen.
2. Vertical areas are exactly 256 px wide and wrap horizontally. BG map columns 32-63 hold junk.
   Either wrap the margins (tiles `x&255`; the player is already drawn twice, enemies are not) or
   blank them.
3. Horizontal areas: blank or clamp outside `[0, ($0541+1)*256)`. At area start the left ring side
   holds page 9's tail. Single-screen boss rooms (`$0541=0`) have both margins outside the area.
4. The guaranteed look-ahead is only +17 px right and +48 px left, for BG1, BG2 and BG3 alike.
   Wider margins need one of: the port rendering columns from RAM (4.4), patching the pointer
   inits in `$14ED60` (`$CF = (P−1)|$D0`, `$D0 = (P+1)|$10`) plus the matching BG2/BG3 inits, or
   using the stale-but-valid ring contents opportunistically.
5. Enemies spawn 9-24 px past the right edge and only in the direction of travel, and despawn
   48 px outside. Both are visible in widescreen unless patched (§5).
6. Screens that should stay 4:3: title/story, char select, level card, Bonus Chance, game over,
   warp (wavy HDMA on BG1HOFS), ending/contributor/credits, and the pause menu box (windowed dim).
7. Cave/indoor areas (`$0772`) put a translucent BG3 overlay above everything. It streams like BG1.
8. The bottom metatile row of every area (y 224-239) is never visible, because the SNES shows 224
   lines and the scroll is not offset.
9. Door and area changes run with `$0217=0` and force blank. Death returns to the mode-2 char select.
10. Screen shake (`$04C6`) moves BG1 vertically only.

---

## Appendix: states in `re_A/states/`

| state | contents |
|---|---|
| `boot2` | SMB2 title from a clean boot |
| `charsel2` | character select (mode 2) |
| `card` | level card |
| `w11_land` | 1-1 area 0, vertical |
| `w11_walk` | 1-1 area 0 at the bottom door |
| `w11_area1` | 1-1 area 2, horizontal, camera 0 |
| `w11_run` | area 2 at the cherry |
| `mid` | area 2, camera `$211` |
| `pause` | pause menu |
| `dying` | death animation |
| `gameover` | game over screen |
| `bonus` | Bonus Chance |
| `ending`, `ending2` | ending |
| `lvl_42`, `lvl_53`, `lvl_72` | levels reached through level select |
| `char1`-`char3` | Luigi, Toad, Peach |
| `obj_spawn` | a Shyguy just spawned off the right edge |

## Progress signals (used by the achievements, src/game/achievements.cpp)

* **Level clear**: `$062D + $008F` (the clearing character's count) goes up by one in the frame of the clear,
  with `$0533` still the cleared level. Bosses end levels 2, 5, 8, 11, 14, 17; Wart `$13`. (**V**: forced clears.)
* **Warp**: `$04ED = 4` changes `$0533`/`$0635` in the same frame, while `$0217` is still `$80`. The jars warp two or
  three worlds ahead. (**V**)
* **Bonus Chance win**: `$04EE` goes up while `$0100==$40 && $0217==0 && $0775==2 && $0789==1`. (**V**: won a life with 9 coins.)
* **Health between levels**: `$04C3`/`$04C4` are reset to `$1F`/0 while `$0217` is 0, so a check on in-area health
  drops doesn't see them. (**V**)

## BG2/BG3 layouts and the wide margins left of x 0

* **BG3 outdoors** (`$0772 == 0`, horizontal) is built at area entry by `$14F538`: 4 pages, each one
  of 4 cloud patterns (pointers `$158928`), in an order taken from the table `$158900` at
  `($10 & 7) * 4`, so the frame counter picks one of 8 orders each time an area is entered
  (vertical areas and jars: fixed `$20`, 6 pages). A cloud drawn on the last page runs on into
  page 0, so the layer is a 1024-px loop. The QA states made by door-teleporting each
  happened to get one order; climbing into 1-1's sky gave another, whose page 0 starts inside
  that cloud. **V**: all 8 orders rendered (`K:/smas_qa/ach/edge/variants.py`, poke `$10`
  before a door teleport).
* **BG2** (per area at `$7F2000 + area*$1000`, decoded at level start) repeats every 1, 2, 3 or 4
  pages in most areas; its last drawn page can stop short. Cave walls repeat every 2.
* The port draws left of x 0 from the loop (BG3; BG2 with a period), else repeats page 0
  where its first and last columns agree (drawn vs `$FF`), else mirrors it about x 0.
