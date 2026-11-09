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

void FileSelect::draw(Menu& menu, int w, int h, const std::function<bool(RectF)>& drawCover, ColorF accent,
                      const std::string& hint) {
  Canvas* cv = menu.canvas();
  if (!open_ || !cv) return;
  if (cv != canvas_ || cv->epoch() != epoch_) {
    for (State& s : states_) s.image = 0;  // (the old canvas's: gone with it)
    canvas_ = cv;
    epoch_ = cv->epoch();
  }
  const float W = (float)w, H = (float)h, a = anim_;
  const float base = std::min(H, W * 0.75f);
  const float fSmall = std::round(base / 40), fBody = std::round(base / 28), fTitle = std::round(base / 18);
  const auto text = [&](RectF r, const std::string& t, float size, bool bold, ColorF c, TextAlign al = kAlignLeading,
                        ParaAlign pa = kParaCenter) {
    cv->text(Menu::widen(t), r, size, bold ? kWeightBold : kWeightNormal, alpha(c, a), al, pa);
  };
  const ColorF ink = hex(0xFFF2F4FA), dim = hex(0xFF9AA4BA), panelC = hex(0xF0141B33);

  // the launcher dimmed, a panel over it
  cv->fillRect(rectF(0, 0, W, H), colorF(0.01f, 0.015f, 0.04f, 0.72f * a));
  const float pw = std::min(W * 0.86f, H * 1.6f), ph = H * 0.80f;
  const float rise = (1.0f - a) * H * 0.04f;
  const RectF panel = rectF((W - pw) * 0.5f, (H - ph) * 0.5f + rise, (W + pw) * 0.5f, (H + ph) * 0.5f + rise);
  const float rad = H * 0.025f, pad = H * 0.04f;
  menu.fillRound(panel, rad, alpha(panelC, a));
  menu.outlineRound(panel, rad, std::max(2.0f, H / 300.0f), alpha(accent, a));

  // left: the cover, the game's name under it
  const float leftW = pw * 0.30f;
  const RectF coverR = rectF(panel.left + pad, panel.top + pad, panel.left + pad + leftW, panel.bottom - pad - fTitle * 2.6f);
  if (!drawCover(coverR)) menu.fillRound(coverR, rad * 0.5f, alpha(hex(0xFF0B1230), a));
  text(rectF(coverR.left - pad * 0.5f, coverR.bottom + pad * 0.3f, coverR.right + pad * 0.5f, panel.bottom - pad), title_,
       fTitle * 0.62f, true, ink, kAlignCenter, kParaNear);

  // right: the tabs, then the files or the states
  const float rx0 = coverR.right + pad * 1.2f, rx1 = panel.right - pad;
  float y = panel.top + pad;
  {
    const char* names[2] = {"Files", "Save States"};
    float x = rx0;
    for (int t = 0; t < 2; t++) {
      const float tw = cv->measure(Menu::widen(names[t]), fBody, kWeightBold);
      const RectF r = rectF(x, y, x + tw, y + fBody * kLine);
      text(r, names[t], fBody, true, t == tab_ ? ink : dim);
      if (t == tab_) cv->fillRect(rectF(r.left, r.bottom + 2, r.right, r.bottom + 6), alpha(accent, a));
      x += tw + pad;
    }
    text(rectF(x, y, rx1, y + fBody * kLine), "LB / RB", fSmall, false, dim, kAlignTrailing);
  }
  y += fBody * kLine + pad * 0.8f;
  const float bottom = panel.bottom - pad - fSmall * kLine * 1.6f;

  if (tab_ == 0) {
    const float gap = H * 0.015f, rowH = (bottom - y - gap * (kFiles - 1)) / kFiles;
    for (int i = 0; i < kFiles; i++) {
      const RectF r = rectF(rx0, y + i * (rowH + gap), rx1, y + i * (rowH + gap) + rowH);
      const bool sel = i == file_;
      menu.fillRound(r, rad * 0.6f, alpha(sel ? alpha(accent, 0.30f) : hex(0x40FFFFFF), a * (sel ? 1.0f : 0.25f)));
      if (sel) menu.outlineRound(r, rad * 0.6f, std::max(2.0f, H / 360.0f), alpha(accent, a));
      // the file's letter in a disc
      const float cy = (r.top + r.bottom) * 0.5f, d = rowH * 0.62f, cx = r.left + rowH * 0.55f;
      menu.fillRound(rectF(cx - d * 0.5f, cy - d * 0.5f, cx + d * 0.5f, cy + d * 0.5f), d * 0.5f,
                     alpha(sel ? accent : hex(0xFF2A3354), a));
      text(rectF(cx - d, cy - d * 0.5f, cx + d, cy + d * 0.5f), std::string(1, (char)('A' + i)), fBody, true,
           sel ? hex(0xFF0B1230) : ink, kAlignCenter);
      const float tx = cx + d * 0.5f + pad * 0.6f;
      const SaveFileInfo& f = files_[i];
      if (f.used) {
        text(rectF(tx, r.top, rx1 - pad, cy + fBody * 0.15f), f.where, fBody, true, ink, kAlignLeading, kParaFar);
        std::string sub = f.twoPlayers ? "2 Players" : "1 Player";
        if (game_ != GameId::SMB1) sub = "Saved progress";
        if (f.secondQuest) sub += "   \xE2\x80\xA2   Second Quest";
        text(rectF(tx, cy + fBody * 0.2f, rx1 - pad, r.bottom), sub, fSmall, false, dim, kAlignLeading, kParaNear);
        if (f.secondQuest) {
          const float sr = rowH * 0.16f;
          menu.star(rx1 - pad - sr, cy, sr, alpha(hex(0xFFFFD400), a), alpha(hex(0xFF3A1D0E), a), std::max(1.0f, H / 700.0f));
        }
      } else {
        text(rectF(tx, r.top, rx1 - pad, r.bottom), "New Game", fBody, false, dim);
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
      menu.fillRound(t, rad * 0.5f, alpha(hex(0xFF0B1230), a));
      if (st.used && st.w > 0 && !st.image) {
        st.image = cv->makeImage(st.w, st.h);
        if (st.image) cv->updateImage(st.image, st.px.data(), st.w);
      }
      const float lineH = fSmall * kLine * 1.2f;
      if (st.image) {
        // (the picture whole, in the tile above its line)
        const float aw = tw, ah = th - lineH, k = std::min(aw / st.w, ah / st.h);
        const float dw = st.w * k, dh = st.h * k;
        cv->pushClip(t);
        cv->drawImage(st.image, rectF(t.left + (aw - dw) * 0.5f, t.top + (ah - dh) * 0.5f, t.left + (aw + dw) * 0.5f,
                                      t.top + (ah + dh) * 0.5f),
                      a, true);
        cv->popClip();
      }
      const std::string label = std::to_string(s) + "   " + (st.used ? st.when : std::string("Empty"));
      text(rectF(t.left + pad * 0.3f, t.bottom - lineH, t.right - pad * 0.3f, t.bottom), label, fSmall, sel, st.used ? ink : dim,
           st.image ? kAlignCenter : kAlignCenter);
      menu.outlineRound(t, rad * 0.5f, sel ? std::max(3.0f, H / 240.0f) : 1.0f, alpha(sel ? accent : hex(0xFF2A3354), a));
    }
  }
  text(rectF(rx0, panel.bottom - pad - fSmall * kLine, rx1, panel.bottom - pad), hint, fSmall, false, dim, kAlignTrailing);
  (void)time_;
}
