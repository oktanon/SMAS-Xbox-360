// See select_covers.h. The select's layout, measured on its pictures (smastool, run 030's task): a blue background
// (0x4273b5), the centred box's top-left corner at (88, 63) of the 256 x 224 picture (Super Mario Bros. 3's at x 89),
// its shadow 0x424242, 8 pixels wide, 4 pixels right of and below it. Its size is each box's own: 80 x 120, The Lost
// Levels' 88 x 96, Super Mario Bros. 3's 83 x 120. So the box is found rather than assumed: its top down the middle
// column, its right edge where the shadow starts across it, its bottom where the shadow starts down the middle, its
// left edge 4 pixels left of where the shadow below it ends.
#include "app/select_covers.h"

#include <chrono>

#include "core/snes.h"
#include "game/smas.h"

namespace {
constexpr uint32_t kBlue = 0x4273b5, kShadow = 0x424242;
constexpr int kTitleFrame = 600;     // the title fully faded in
constexpr int kGiveUp = 6000;        // frames: something isn't as expected (another ROM, a hack)
constexpr int kStill = 10;           // frames the boxes must stay put
}  // namespace

SelectCovers::SelectCovers(const std::vector<uint8_t>& rom) {
  snes_ = std::make_unique<Snes>();
  if (!snes_->loadRom(rom)) {
    failed_ = true;
    return;
  }
  game_ = std::make_unique<SmasGame>(*snes_);
  game_->install();
  snes_->reset(true);
  game_->setWidescreen(0);
  game_->hudAnchor = false;
  game_->achievements().newRun(false);
  snes_->apu.dsp.musicGain = 0;
  snes_->apu.dsp.sfxGain = 0;
}

SelectCovers::~SelectCovers() = default;

void SelectCovers::runFrame(bool draw) {
  snes_->input[0] = hold_ > 0 ? buttons_ : 0;
  snes_->input[1] = 0;
  if (hold_ > 0) hold_--;
  snes_->ppu.skipRender = !draw;
  game_->beforeFrame();
  snes_->runFrame();
  game_->afterFrame();
  std::vector<int16_t> discard;
  snes_->takeAudio(discard);
  frame_++;
}

// The boxes' band of the picture (below the arrow, which bobs; above the names).
uint32_t SelectCovers::boxesHash() const {
  uint32_t h = 2166136261u;
  for (int y = 56; y < 200; y++) {
    const uint32_t* row = snes_->ppu.frame + (size_t)y * kLineW + kMaxExt;
    for (int x = 0; x < 256; x++) h = (h ^ row[x]) * 16777619u;
  }
  return h;
}

bool SelectCovers::cutBox(Pic& out) const {
  auto px = [&](int x, int y) { return snes_->ppu.frame[(size_t)y * kLineW + kMaxExt + x] & 0xffffff; };
  auto shadowRun = [&](int x, int y, int dx, int dy) {
    for (int i = 0; i < 6; i++)
      if (px(x + dx * i, y + dy * i) != kShadow) return false;
    return true;
  };
  if (px(128, 34) != kBlue) return false;  // (not the select: between its title and the arrow)
  int top = -1, right = -1, bottom = -1, left = -1;
  for (int y = 56; y < 120 && top < 0; y++)
    if (px(128, y) != kBlue) top = y;
  if (top < 0) return false;
  for (int x = 129; x < 240 && right < 0; x++)
    if (shadowRun(x, top + 20, 1, 0)) right = x - 1;
  for (int y = top + 20; y < 210 && bottom < 0; y++)
    if (shadowRun(128, y, 0, 1)) bottom = y - 1;
  if (right < 0 || bottom < 0) return false;
  for (int x = 128; x > 4 && left < 0; x--)
    if (px(x, bottom + 4) != kShadow) left = x + 1 - 4;
  if (left < 0 || right - left < 40 || bottom - top < 40) return false;
  out.w = right - left + 1;
  out.h = bottom - top + 1;
  out.px.resize((size_t)out.w * out.h);
  for (int y = 0; y < out.h; y++)
    for (int x = 0; x < out.w; x++) out.px[(size_t)y * out.w + x] = px(left + x, top + y);
  return true;
}

bool SelectCovers::step(int budgetUs) {
  if (failed_ || phase_ == 6) return true;
  using clock = std::chrono::steady_clock;
  const auto start = clock::now();
  while (clock::now() - start < std::chrono::microseconds(budgetUs)) {
    if (frame_ > kGiveUp) {
      failed_ = true;
      return true;
    }
    if (phase_ == 0) {
      // to the title, nothing drawn but its last frames
      runFrame(frame_ >= kTitleFrame - 2);
      if (frame_ < kTitleFrame) continue;
      Pic& t = pics[4];
      t.w = 256;
      t.h = snes_->ppu.frameHeight();
      t.px.resize((size_t)t.w * t.h);
      for (int y = 0; y < t.h; y++)
        for (int x = 0; x < 256; x++)
          t.px[(size_t)y * 256 + x] = snes_->ppu.frame[(size_t)y * kLineW + kMaxExt + x] & 0xffffff;
      new_ |= 1u << 4;
      buttons_ = kPadStart;
      hold_ = 2;
      phase_ = 2;
      continue;
    }
    // the select: each box once it has settled, then Right to the next
    runFrame(true);
    const uint32_t h = boxesHash();
    still_ = h == lastHash_ ? still_ + 1 : 0;
    lastHash_ = h;
    if (still_ < kStill || h == takenHash_ || hold_ > 0) continue;
    const int g = phase_ - 2;
    if (!cutBox(pics[g])) continue;
    takenHash_ = h;
    new_ |= 1u << g;
    if (g == 3) {
      game_.reset();  // (done: the console's memory back)
      snes_.reset();
      phase_ = 6;
      return true;
    }
    buttons_ = kPadRight;
    hold_ = 2;
    still_ = 0;
    phase_++;
  }
  return phase_ == 6;
}
