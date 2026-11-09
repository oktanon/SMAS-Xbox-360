// See file_select.h.
#include "app/file_select.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kLine = 1.33f;  // (as launcher.cpp's)

ColorF hex(uint32_t c) {
  return colorF(((c >> 16) & 255) / 255.0f, ((c >> 8) & 255) / 255.0f, (c & 255) / 255.0f, ((c >> 24) & 255) / 255.0f);
}
ColorF alpha(ColorF c, float a) { return colorF(c.r, c.g, c.b, c.a * a); }

// The game's entry in a file: its place in the battery save and its size (with the checksum word).
bool entryOf(GameId g, int file, int* at, int* size) {
  static const int kOff[4] = {0x00, 0x09, 0x11, 0x1A}, kSize[4] = {9, 8, 9, 0x106};
  const int i = g == GameId::SMB1 ? 0 : g == GameId::SMBLL ? 1 : g == GameId::SMB2 ? 2 : g == GameId::SMB3 ? 3 : -1;
  if (i < 0 || file < 0 || file >= FileSelect::kFiles) return false;
  *at = 0x10 + file * 0x120 + kOff[i];
  *size = kSize[i];
  return true;
}
}  // namespace

bool readSaveFile(GameId g, int file, const uint8_t* sram, SaveFileInfo* out) {
  *out = SaveFileInfo{};
  int at = 0, size = 0;
  if (!sram || !entryOf(g, file, &at, &size)) return false;
  const uint8_t* e = sram + at;
  unsigned sum = 0;
  for (int i = 0; i < size - 2; i++) sum += e[i];
  sum += (unsigned)(e[size - 2] | e[size - 1] << 8);
  if ((sum & 0xffff) != 0 || (e[0] & 0x80)) return true;  // (a new file; or not one the cartridge would read)
  out->used = true;
  const int world = e[0];
  if (g == GameId::SMBLL) {
    // (worlds 1-9, then A-D)
    const std::string w = world < 9 ? std::to_string(world + 1) : std::string(1, (char)('A' + std::min(world - 9, 3)));
    out->where = "World " + w + "-" + std::to_string((e[1] & 3) + 1);
  } else {
    out->where = "World " + std::to_string(world + 1);
  }
  if (g == GameId::SMB1) {
    out->secondQuest = e[5] != 0;
    out->twoPlayers = e[6] != 0;
  }
  return true;
}

void eraseSaveFile(GameId g, int file, uint8_t* sram, const std::vector<uint8_t>& rom) {
  int at = 0, size = 0;
  if (!sram || !entryOf(g, file, &at, &size)) return;
  // (the defaults' table at $00:8DE3, LoROM: at 0x0DE3 of the ROM, after a copier's 512-byte header if there is one;
  // each game's at the same offset as its entry in a file)
  const size_t head = rom.size() % 1024 == 512 ? 512 : 0;
  const size_t from = head + 0x0DE3 + (size_t)(at - 0x10 - file * 0x120);
  if (from + size - 2 > rom.size()) return;
  unsigned sum = 0;
  for (int i = 0; i < size - 2; i++) sum += sram[at + i] = rom[from + i];
  const unsigned check = (0x10000u - (sum & 0xffff)) & 0xffff;
  sram[at + size - 2] = (uint8_t)check;
  sram[at + size - 1] = (uint8_t)(check >> 8);
}

void FileSelect::open(GameId g, int card, const std::string& title, const uint8_t* sram, std::vector<State> states) {
  freeImages();
  open_ = true;
  game_ = g;
  card_ = card;
  title_ = title;
  states_ = std::move(states);
  states_.resize(kSlots + 1);
  refreshFiles(sram);
  tab_ = 0;
  anim_ = 0;
  repeatDir_ = 0;
  // (on the last file played, or the first)
  file_ = 0;
  for (int i = kFiles - 1; i >= 0; i--)
    if (files_[i].used) file_ = i;
}

void FileSelect::refreshFiles(const uint8_t* sram) {
  for (int i = 0; i < kFiles; i++) readSaveFile(game_, i, sram, &files_[i]);
}

void FileSelect::close() {
  freeImages();
  states_.clear();
  open_ = false;
}

void FileSelect::freeImages() {
  for (State& s : states_) {
    if (s.image && canvas_ && canvas_->epoch() == epoch_) canvas_->freeImage(s.image);
    s.image = 0;
  }
}

void FileSelect::releaseGpu() {
  freeImages();
  canvas_ = nullptr;
}

FileSelect::Action FileSelect::update(const MenuInput& in, bool erase, float dt) {
  if (!open_) return kNone;
  anim_ = std::min(1.0f, anim_ + dt * 6.0f);
  time_ += dt;
  if (in.back) return kBack;
  if (in.tabLeft || in.tabRight) tab_ ^= 1;
  // (the directions held: once, then repeated)
  const int dir = in.up ? 1 : in.down ? 2 : in.left ? 3 : in.right ? 4 : 0;
  bool step = false;
  if (dir != repeatDir_) {
    repeatDir_ = dir;
    repeatTimer_ = 0.4f;
    step = dir != 0;
  } else if (dir) {
    repeatTimer_ -= dt;
    if (repeatTimer_ <= 0) {
      repeatTimer_ = 0.12f;
      step = true;
    }
  }
  if (step) {
    if (tab_ == 0) {
      if (dir == 1) file_ = (file_ + kFiles - 1) % kFiles;
      if (dir == 2) file_ = (file_ + 1) % kFiles;
    } else {
      // (a 3 x 3 grid)
      int r = (slot_ - 1) / 3, c = (slot_ - 1) % 3;
      if (dir == 1) r = (r + 2) % 3;
      if (dir == 2) r = (r + 1) % 3;
      if (dir == 3) c = (c + 2) % 3;
      if (dir == 4) c = (c + 1) % 3;
      slot_ = r * 3 + c + 1;
    }
  }
  if (in.confirm) {
    if (tab_ == 0) return kPlayFile;
    if (states_[slot_].used) return kLoadState;
  }
  if (erase && tab_ == 0 && files_[file_].used) return kEraseFile;
  return kNone;
}

void FileSelect::draw(Menu& menu, int w, int h, const std::function<bool(RectF)>& drawCover, const std::string& hint) {
  Canvas* cv = menu.canvas();
  if (!open_ || !cv) return;
  if (cv != canvas_ || cv->epoch() != epoch_) {
    for (State& s : states_) s.image = 0;  // (the old canvas's: gone with it)
    canvas_ = cv;
    epoch_ = cv->epoch();
  }
  const float W = (float)w, H = (float)h, a = anim_;
  const Menu::Skin k = menu.skin();
  const ColorF accent = menu.accent();
  const float base = std::min(H, W * 0.75f);
  const float fSmall = std::round(base / 40), fBody = std::round(base / 28);
  // (the menus' faces: Retro's pixels, Storybook's heavier text)
  const auto text = [&](RectF r, const std::string& t, float size, bool bold, ColorF c, TextAlign al = kAlignLeading,
                        ParaAlign pa = kParaCenter) { menu.text(Menu::widen(t), r, size, bold, alpha(c, a), al, pa); };
  const auto measure = [&](const std::string& t, float size, bool bold) { return menu.measure(Menu::widen(t), size, bold); };

  // the panel in the menus' style, the game's name its title
  const float pw = std::min(W * 0.86f, H * 1.6f), ph = H * 0.84f;
  const float rise = (1.0f - a) * H * 0.04f;
  const RectF panel = rectF((W - pw) * 0.5f, (H - ph) * 0.5f + rise, (W + pw) * 0.5f, (H + ph) * 0.5f + rise);
  const RectF in = menu.drawFrame(w, h, panel, Menu::widen(title_), a);
  const float rad = k.retro ? 0.0f : k.story ? H * 0.02f : H * 0.012f, pad = H * 0.035f;

  // the keys under a rule
  const float keysH = fSmall * kLine, footTop = in.bottom - keysH - pad * 0.4f;
  menu.drawRule(in.left, in.right, footTop, a);
  text(rectF(in.left, in.bottom - keysH, in.right, in.bottom), hint, fSmall, false, k.keys, kAlignTrailing);
  const float bottom = footTop - pad * 0.5f;

  // left: the cover
  const RectF coverR = rectF(in.left, in.top, in.left + (in.right - in.left) * 0.30f, bottom);
  if (!drawCover(coverR)) menu.drawBox(coverR, false, a, rad);

  // right: the tabs, then the files or the states
  const float rx0 = coverR.right + pad * 1.2f, rx1 = in.right;
  float y = in.top;
  {
    const char* names[2] = {"Files", "Save States"};
    float x = rx0;
    for (int t = 0; t < 2; t++) {
      const float tw = measure(names[t], fBody, true);
      const RectF r = rectF(x, y, x + tw, y + fBody * kLine);
      text(r, names[t], fBody, true, t == tab_ ? k.tab : alpha(k.sub, 0.6f));
      if (t == tab_) cv->fillRect(rectF(r.left, r.bottom + 2, r.right, r.bottom + 2 + std::max(3.0f, H / 200.0f)), alpha(k.tab, a));
      x += tw + pad;
    }
    text(rectF(x, y, rx1, y + fBody * kLine), "LB / RB", fSmall, false, k.keys, kAlignTrailing);
  }
  y += fBody * kLine + pad * 0.7f;

  if (tab_ == 0) {
    const float gap = H * 0.015f, rowH = (bottom - y - gap * (kFiles - 1)) / kFiles;
    for (int i = 0; i < kFiles; i++) {
      const RectF r = rectF(rx0, y + i * (rowH + gap), rx1, y + i * (rowH + gap) + rowH);
      const bool sel = i == file_;
      menu.drawBox(r, sel, a, rad);
      // the file's letter in a disc (Retro: a block)
      const float cy = (r.top + r.bottom) * 0.5f, d = rowH * 0.62f, cx = r.left + rowH * 0.55f;
      const RectF disc = rectF(cx - d * 0.5f, cy - d * 0.5f, cx + d * 0.5f, cy + d * 0.5f);
      const float dr = k.retro ? 0.0f : d * 0.5f;
      const ColorF discC = sel ? (k.story ? hex(0xFFFFFFFF) : k.retro ? hex(0xFFFCBC3C) : accent)
                               : (k.story ? accent : k.retro ? hex(0xFF3C3C3C) : hex(0xFF2A3354));
      menu.fillRound(disc, dr, alpha(discC, a));
      if (k.story) menu.outlineRound(disc, dr, std::max(2.0f, H / 400.0f), alpha(k.edge, a));
      const ColorF letterC = sel ? (k.story ? k.edge : hex(0xFF0B1230)) : (k.story ? hex(0xFFFFFFFF) : k.text);
      text(rectF(cx - d, cy - d * 0.5f, cx + d, cy + d * 0.5f), std::string(1, (char)('A' + i)), fBody, true, letterC,
           kAlignCenter);
      const float tx = cx + d * 0.5f + pad * 0.6f;
      const ColorF mainC = sel ? k.rowSel : k.text, subC = sel ? alpha(k.rowSel, 0.75f) : k.sub;
      const SaveFileInfo& f = files_[i];
      if (f.used) {
        text(rectF(tx, r.top, rx1 - pad, cy + fBody * 0.15f), f.where, fBody, true, mainC, kAlignLeading, kParaFar);
        std::string sub = f.twoPlayers ? "2 Players" : "1 Player";
        if (game_ != GameId::SMB1) sub = "Saved progress";
        if (f.secondQuest) sub += "   \xE2\x80\xA2   Second Quest";
        text(rectF(tx, cy + fBody * 0.2f, rx1 - pad, r.bottom), sub, fSmall, false, subC, kAlignLeading, kParaNear);
        if (f.secondQuest) {
          const float sr = rowH * 0.16f;
          menu.star(rx1 - pad - sr, cy, sr, alpha(hex(0xFFFFD400), a), alpha(hex(0xFF3A1D0E), a), std::max(1.0f, H / 700.0f));
        }
      } else {
        text(rectF(tx, r.top, rx1 - pad, r.bottom), "New Game", fBody, false, sel ? k.rowSel : k.sub);
      }
    }
  } else {
    const float gap = H * 0.018f;
    const float tw = (rx1 - rx0 - gap * 2) / 3, th = (bottom - y - gap * 2) / 3;
    for (int s = 1; s <= kSlots; s++) {
      const int r = (s - 1) / 3, c = (s - 1) % 3;
      const RectF t = rectF(rx0 + c * (tw + gap), y + r * (th + gap), rx0 + c * (tw + gap) + tw, y + r * (th + gap) + th);
      const bool sel = s == slot_;
      State& st = states_[s];
      menu.drawBox(t, sel, a, rad * 0.8f);
      if (st.used && st.w > 0 && !st.image) {
        st.image = cv->makeImage(st.w, st.h);
        if (st.image) cv->updateImage(st.image, st.px.data(), st.w);
      }
      const float lineH = fSmall * kLine * 1.2f, inset = std::max(3.0f, H / 200.0f);
      if (st.image) {
        // (the picture whole, in the tile above its line)
        const float aw = tw - inset * 2, ah = th - lineH - inset, k2 = std::min(aw / st.w, ah / st.h);
        const float dw = st.w * k2, dh = st.h * k2, ix = t.left + inset + (aw - dw) * 0.5f, iy = t.top + inset + (ah - dh) * 0.5f;
        cv->drawImage(st.image, rectF(ix, iy, ix + dw, iy + dh), a, true);
      }
      const std::string label = std::to_string(s) + "   " + (st.used ? st.when : std::string("Empty"));
      text(rectF(t.left + pad * 0.3f, t.bottom - lineH, t.right - pad * 0.3f, t.bottom), label, fSmall, sel,
           sel ? k.rowSel : st.used ? k.text : k.sub, kAlignCenter);
    }
  }
  (void)time_;
}
