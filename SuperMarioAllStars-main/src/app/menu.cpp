#include "menu.h"
#include <algorithm>
#include <cmath>

MenuItem MenuItem::action(std::string l, std::function<void()> f, std::string h) {
  MenuItem m;
  m.kind = Action;
  m.label = std::move(l);
  m.onActivate = std::move(f);
  m.help = std::move(h);
  return m;
}
MenuItem MenuItem::submenu(std::string l, std::function<void()> f, std::string h) {
  MenuItem m = action(std::move(l), std::move(f), std::move(h));
  m.kind = Submenu;
  return m;
}
MenuItem MenuItem::toggle(std::string l, std::function<int()> g, std::function<void(int)> s, std::string h) {
  MenuItem m;
  m.kind = Toggle;
  m.label = std::move(l);
  m.get = std::move(g);
  m.set = std::move(s);
  m.help = std::move(h);
  return m;
}
MenuItem MenuItem::choice(std::string l, std::vector<std::string> opts, std::function<int()> g,
                          std::function<void(int)> s, std::string h) {
  MenuItem m;
  m.kind = Choice;
  m.label = std::move(l);
  auto o = std::make_shared<std::vector<std::string>>(std::move(opts));
  m.options = [o]() { return *o; };
  m.get = std::move(g);
  m.set = std::move(s);
  m.help = std::move(h);
  return m;
}
MenuItem MenuItem::slider(std::string l, int mn, int mx, int st, std::function<int()> g, std::function<void(int)> s,
                          std::string suffix, std::string h) {
  MenuItem m;
  m.kind = Slider;
  m.label = std::move(l);
  m.minV = mn;
  m.maxV = mx;
  m.step = st;
  m.get = std::move(g);
  m.set = std::move(s);
  m.suffix = std::move(suffix);
  m.help = std::move(h);
  m.wrap = false;
  return m;
}
MenuItem MenuItem::info(std::string l, std::string h) {
  MenuItem m;
  m.kind = Label;
  m.label = std::move(l);
  m.help = std::move(h);
  return m;
}
MenuItem MenuItem::separator(std::string l) {
  MenuItem m;
  m.kind = Separator;
  m.label = std::move(l);
  return m;
}

std::wstring Menu::widen(const std::string& s) { return widenUtf8(s); }

namespace {
// The palette (as in the Mickey & Donald Collection's menus).
ColorF rgb(uint32_t c, float a = 1.0f) {
  return colorF(((c >> 16) & 0xff) / 255.0f, ((c >> 8) & 0xff) / 255.0f, (c & 0xff) / 255.0f, a);
}
ColorF withA(ColorF c, float a) { return colorF(c.r, c.g, c.b, c.a * a); }
ColorF mix(ColorF x, ColorF y, float t) {
  return colorF(x.r + (y.r - x.r) * t, x.g + (y.g - x.g) * t, x.b + (y.b - x.b) * t, x.a + (y.a - x.a) * t);
}
const uint32_t kPanel = 0x12151C, kCrumb = 0x8A93A6, kSub = 0xB7BFCE, kText = 0xD4DAE6, kDim = 0x5A6070,
               kValue = 0xA9B3C6, kOff = 0x3A404E, kKnob = 0xF4F6FA, kHint = 0xC9D0DC, kKeys = 0x7C8598,
               kGold = 0xF2C14E;
constexpr float kLine = 1.33f;  // Segoe UI line height per pixel of font size
}  // namespace

void Menu::text(const std::wstring& s, RectF r, float size, bool bold, ColorF c,
                TextAlign align, ParaAlign palign) {
  text(s, r, size, bold ? kWeightSemiBold : kWeightNormal, c, align, palign);
}

void Menu::text(const std::wstring& s, RectF r, float size, int weight, ColorF c,
                TextAlign align, ParaAlign palign) {
  if (cv_) cv_->text(s, r, size, weight, c, align, palign);
}

float Menu::measure(const std::wstring& s, float size, bool bold) {
  return measure(s, size, bold ? kWeightSemiBold : kWeightNormal);
}

float Menu::measure(const std::wstring& s, float size, int weight) {
  return cv_ && !s.empty() ? cv_->measure(s, size, weight) : 0.0f;
}

void Menu::fillRound(RectF r, float radius, ColorF c) { roundRect(r, radius, c); }

void Menu::outlineRound(RectF r, float radius, float width, ColorF c) {
  if (cv_) cv_->strokeRound(r, radius, width, c);
}

void Menu::roundRect(RectF r, float radius, ColorF c) {
  if (cv_) cv_->fillRound(r, radius, c);
}

void Menu::push(std::shared_ptr<MenuPage> p) {
  if (p->rebuild) p->rebuild(*p);
  if (p->sel >= (int)p->items.size()) p->sel = 0;
  stack_.push_back(p);
  toasts_.erase(std::remove_if(toasts_.begin(), toasts_.end(), [](const Toast& t) { return t.hint; }),
                toasts_.end());
  if (!p->items.empty() && !selectable(p->items[p->sel])) moveSel(1);
}

void Menu::pop() {
  if (stack_.empty()) return;
  if (stack_.back()->onLeave) stack_.back()->onLeave();
  stack_.pop_back();
  refresh();
}

void Menu::closeAll() {
  while (!stack_.empty()) pop();
}

void Menu::refresh() {
  MenuPage* p = top();
  if (p && p->rebuild) {
    int sel = p->sel;
    p->rebuild(*p);
    p->sel = std::min(sel, (int)p->items.size() - 1);
    if (p->sel < 0) p->sel = 0;
    if (!p->items.empty() && !selectable(p->items[p->sel])) moveSel(1);
  }
}

void Menu::confirm(const std::string& question, std::function<void()> yes, const std::string& detail) {
  auto p = std::make_shared<MenuPage>();
  p->title = question;
  p->subtitle = detail;
  p->items.push_back(MenuItem::action("Yes", [this, yes]() {
    auto y = yes;  // (popping destroys this item)
    pop();
    y();
  }));
  p->items.push_back(MenuItem::action("No", [this]() { pop(); }));
  p->sel = 1;
  push(p);
}

bool Menu::selectable(const MenuItem& it) const {
  if (it.kind == MenuItem::Label || it.kind == MenuItem::Separator) return false;
  if (it.enabled && !it.enabled()) return false;
  return true;
}

void Menu::moveSel(int dir) {
  MenuPage* p = top();
  if (!p || p->items.empty()) return;
  int n = (int)p->items.size();
  int s = p->sel;
  for (int i = 0; i < n; i++) {
    s = (s + dir + n) % n;
    if (selectable(p->items[s])) {
      p->sel = s;
      return;
    }
  }
}

// Callbacks are copied before they run: an action may close the menu or
// rebuild the page, which destroys the item (and the original std::function).
void Menu::activate(MenuItem& it) {
  if (it.onActivate) {  // an explicit action wins (e.g. play the chosen number)
    auto f = it.onActivate;
    f();
    return;
  }
  switch (it.kind) {
    case MenuItem::Toggle:
      if (it.set) {
        auto set = it.set;
        set(it.get() ? 0 : 1);
      }
      break;
    case MenuItem::Choice:
      adjust(it, 1);
      break;
    default: break;
  }
}

void Menu::adjust(MenuItem& it, int dir) {
  if (!it.get || !it.set) return;
  auto set = it.set;  // see activate()
  int v = it.get();
  if (it.kind == MenuItem::Toggle) {
    set(v ? 0 : 1);
  } else if (it.kind == MenuItem::Choice) {
    int n = (int)it.options().size();
    if (n <= 0) return;
    v += dir;
    if (it.wrap) v = (v + n) % n;
    else v = std::clamp(v, 0, n - 1);
    set(v);
  } else if (it.kind == MenuItem::Slider) {
    set(std::clamp(v + dir * it.step, it.minV, it.maxV));
  }
}

void Menu::tickToasts(float dt) {
  time_ += dt;
  for (auto& t : toasts_) t.time -= dt;
  toasts_.erase(std::remove_if(toasts_.begin(), toasts_.end(), [](const Toast& t) { return t.time <= 0; }),
                toasts_.end());
  if (!banners_.empty()) {
    banners_.front().t += dt;
    if (banners_.front().t > 0.35f + 4.0f + 0.5f) banners_.erase(banners_.begin());
  }
}

void Menu::update(const MenuInput& in, float dt) {
  if (stack_.empty() || capturing) return;
  // Keep the page (and the closures its items hold) alive until we return,
  // even if an action closes the menu.
  std::shared_ptr<MenuPage> keep = stack_.back();
  MenuPage* p = keep.get();

  // Auto-repeat for held directions
  int dir = in.up ? -1 : in.down ? 1 : in.left ? -2 : in.right ? 2 : 0;
  bool fire = false;
  if (dir != repeatDir_) {
    repeatDir_ = dir;
    repeatTimer_ = 0.38f;
    fire = dir != 0;
  } else if (dir != 0) {
    repeatTimer_ -= dt;
    if (repeatTimer_ <= 0) {
      repeatTimer_ = 0.07f;
      fire = true;
    }
  }
  MenuInput e;
  if (fire) {
    e.up = dir == -1;
    e.down = dir == 1;
    e.left = dir == -2;
    e.right = dir == 2;
  }
  e.confirm = in.confirm;
  e.back = in.back;
  e.tabLeft = in.tabLeft;
  e.tabRight = in.tabRight;

  if (e.up || e.down) p->freeScroll = false;
  if (p->inputCustom && p->inputCustom(*this, e)) return;
  if (e.back) {
    pop();
    return;
  }
  if (p->items.empty()) return;
  if (e.up) moveSel(-1);
  if (e.down) moveSel(1);
  MenuItem& it = p->items[p->sel];
  if (!selectable(it)) return;
  if (e.left || e.right) {
    adjust(it, e.left ? -1 : 1);
    return;  // the setter may have rebuilt the page; `it` is no longer valid
  }
  if (e.confirm) activate(it);
}

void Menu::notify(const std::string& text, float seconds, bool hint) {
  if (!toasts_.empty() && toasts_.back().text == text) {
    toasts_.back().time = toasts_.back().total = seconds;
    return;
  }
  toasts_.push_back({text, seconds, seconds, hint});
  if (toasts_.size() > 4) toasts_.erase(toasts_.begin());
}

void Menu::achievement(const std::string& name, const std::string& desc) { banners_.push_back({name, desc, 0.0f}); }

// In play: top right, below SMB1's HUD rows (scanlines 16-31, so from about
// 15.5% of the height) and the FPS counter, stacking down. With the menu open
// the top holds titles, so toasts sit bottom centre over the key hints.
void Menu::drawToasts(int w, int h, float s) {
  bool bottom = open();
  float size = bottom ? 19 * s : 24 * s;
  float lh = size * kLine, th = lh * 1.5f;
  float y = bottom ? h - th - h * 0.012f : std::max(64 * s, h * 0.155f);
  for (size_t i = 0; i < toasts_.size(); i++) {
    const Toast& t = toasts_[bottom ? toasts_.size() - 1 - i : i];  // newest nearest the edge
    float a = std::min(1.0f, t.time * 3.0f) * std::min(1.0f, (t.total - t.time) * 6.0f + 0.2f);
    std::wstring ws = widen(t.text);
    float tw = std::min(measure(ws, size, false), w * 0.8f);
    float bw = tw + h * 0.05f;
    RectF r = bottom ? rectF((w - bw) * 0.5f, y, (w + bw) * 0.5f, y + th)
                           : rectF(w - bw - 16 * s, y, w - 16 * s, y + th);
    roundRect(r, th * 0.3f, rgb(0x181C26, 0.94f * a));
    roundRect(rectF(r.left, r.top + th * 0.2f, r.left + std::max(3.0f, h / 300.0f), r.top + th * 0.8f), 2.0f,
              withA(accent_, a));
    text(ws, rectF(r.left + h * 0.025f, r.top, r.right - h * 0.02f, r.bottom), size, false, rgb(0xFFFFFF, a));
    y += bottom ? -th * 1.15f : th * 1.15f;
  }
}

// "Achievement unlocked": a small card (a star, the name and what it was for)
// slides down at the top, stays a few seconds and fades.
void Menu::drawBanner(int w, int h, float s) {
  if (banners_.empty()) return;
  const Banner& b = banners_.front();
  const float kIn = 0.35f, kStay = 4.0f;
  float a = b.t < kIn ? b.t / kIn : b.t > kIn + kStay ? std::max(0.0f, 1.0f - (b.t - kIn - kStay) / 0.5f) : 1.0f;
  float slide = b.t < kIn ? 1.0f - (1.0f - a) * (1.0f - a) : 1.0f;
  const std::wstring head = L"Achievement unlocked", name = widen(b.name), desc = widen(b.desc);
  float ss = 19 * s, sb = 26 * s;
  float ls = ss * kLine, lb = sb * kLine, pad = ls * 0.55f;
  float textH = ls + lb + (desc.empty() ? 0.0f : ls);
  float icon = textH * 0.78f;
  float maxTw = w * 0.86f - icon - pad * 3;
  float nameS = std::min(1.0f, maxTw / std::max(1.0f, measure(name, sb, true)));
  float descS = std::min(1.0f, maxTw / std::max(1.0f, measure(desc, ss, false)));
  float tw = std::max({measure(head, ss, false), measure(name, sb, true) * nameS, measure(desc, ss, false) * descS});
  float bw = icon + tw + pad * 3, bh = textH + pad * 1.6f;
  RectF r = rectF((w - bw) * 0.5f, h * 0.025f - (1 - slide) * (bh + h * 0.025f), 0, 0);
  r.right = r.left + bw;
  r.bottom = r.top + bh;
  roundRect(r, bh * 0.2f, rgb(0x141824, 0.94f * a));
  outlineRound(r, bh * 0.2f, std::max(1.5f, h / 480.0f), rgb(kGold, a));
  RectF ic = rectF(r.left + pad * 1.1f, r.top + (bh - icon) * 0.5f, 0, 0);
  ic.right = ic.left + icon;
  ic.bottom = ic.top + icon;
  roundRect(ic, icon * 0.5f, rgb(0x2B2617, a));
  cv_->strokeEllipse((ic.left + ic.right) * 0.5f, (ic.top + ic.bottom) * 0.5f, icon * 0.5f, icon * 0.5f,
                     std::max(1.5f, h / 540.0f), rgb(kGold, a));
  drawStar((ic.left + ic.right) * 0.5f, (ic.top + ic.bottom) * 0.5f, icon * 0.34f, rgb(0xFFD83D, a),
           rgb(0xB8860B, a), std::max(1.0f, h / 900.0f));
  float tx = ic.right + pad, y = r.top + pad * 0.8f;
  text(head, rectF(tx, y, r.right, y + ls), ss, false, rgb(kGold, a), kAlignLeading,
       kParaNear);
  y += ls;
  text(name, rectF(tx, y, r.right, y + lb), sb * nameS, true, rgb(0xFFFFFF, a), kAlignLeading,
       kParaNear);
  y += lb;
  if (!desc.empty())
    text(desc, rectF(tx, y, r.right, y + ls), ss * descS, false, rgb(kSub, a), kAlignLeading,
         kParaNear);
}

// A five-pointed star (point up) of outer radius r, drawn as a shape so it sits
// exactly in the middle: its box, not its circumcircle, is centred on (cx, cy)
// (the lower points reach only cos 36° = 0.809 r below the centre).
void Menu::drawStar(float cx, float cy, float r, ColorF fill, ColorF edge, float edgeW) {
  const float kPi = 3.14159265f, inner = r * 0.42f, oy = cy + r * (1.0f - 0.809f) * 0.5f;
  PointF pts[10];
  for (int i = 0; i < 10; i++) {
    float ang = -kPi / 2 + i * kPi / 5, rr = i % 2 ? inner : r;
    pts[i] = {cx + rr * std::cos(ang), oy + rr * std::sin(ang)};
  }
  cv_->fillPolygon(pts, 10, fill);
  cv_->strokePolygon(pts, 10, edgeW, edge);
}

void Menu::drawOsd(int w, int h, const std::string& fps, const std::string& status) {
  if (!cv_) return;
  float s = std::max(h / 1080.0f, 0.55f);
  auto card = [&](RectF r, const std::string& t, ColorF c) {
    roundRect(r, (r.bottom - r.top) * 0.3f, rgb(0x181C26, 0.85f));
    text(widen(t), r, 20 * s, true, c, kAlignCenter);
  };
  if (!status.empty()) card(rectF(16 * s, 16 * s, 236 * s, 52 * s), status, rgb(0xFFD866));
  if (!fps.empty()) card(rectF(w - 196 * s, 16 * s, w - 16 * s, 52 * s), fps, rgb(0x9BE59B));
  drawToasts(w, h, s);
  drawBanner(w, h, s);
}

void Menu::draw(int w, int h, float anim) {
  MenuPage* p = top();
  if (!p || !cv_) return;
  scale_ = std::max(h / 1080.0f, 0.55f);
  const float s = scale_, W = (float)w, H = (float)h;
  const float a = std::clamp(anim, 0.0f, 1.0f);
  const float sizeTitle = 48 * s, sizeBody = 26 * s, sizeSmall = 19 * s;
  const float lhTitle = sizeTitle * kLine, lhBody = sizeBody * kLine, lhSmall = sizeSmall * kLine;

  // Dim the game
  cv_->fillRect(rectF(0, 0, W, H), colorF(0.02f, 0.027f, 0.047f, 0.6f * a));

  // The panel: dark, rounded, outlined in the accent colour, with a soft shadow.
  const bool wide = p->wide || p->listFraction < 0.999f;
  float pw = std::min({W * (wide ? 0.8f : 0.62f), H * (wide ? 1.5f : 1.15f), W - 32 * s});
  float ph = H * 0.84f;
  RectF panel = rectF((W - pw) * 0.5f, (H - ph) * 0.5f + (1 - a) * 24 * s, 0, 0);
  panel.right = panel.left + pw;
  panel.bottom = panel.top + ph;
  const float rad = H * 0.018f, pad = H * 0.035f;
  roundRect(rectF(panel.left + rad * 0.3f, panel.top + rad * 0.5f, panel.right + rad * 0.3f,
                        panel.bottom + rad * 0.5f),
            rad, rgb(0, 0.5f * a));
  roundRect(panel, rad, rgb(kPanel, 0.918f * a));
  outlineRound(panel, rad, std::max(1.5f, H / 540.0f), withA(mix(accent_, rgb(kPanel), 0.35f), a));

  // Long titles and hints shrink to fit the panel instead of running out of it.
  auto fit = [&](const std::wstring& t, float size, bool bold, float room) {
    return size * std::min(1.0f, room / std::max(1.0f, measure(t, size, bold)));
  };
  const float tx = panel.left + pad, inner = pw - pad * 2;
  float y = panel.top + pad * 0.8f;
  if (stack_.size() > 1) {  // breadcrumb: the pages this one was opened from
    std::wstring crumb;
    for (size_t i = 0; i + 1 < stack_.size(); i++) crumb += widen(stack_[i]->title) + L"  »  ";
    text(crumb, rectF(tx, y, panel.right - pad, y + lhSmall), fit(crumb, sizeSmall, false, inner), false,
         rgb(kCrumb, a));
    y += lhSmall;
  }
  std::wstring title = widen(p->title);
  float titleRoom = inner;
  closeHit_ = RectF{};
  if (touchUi) {
    // (top right: back, or out of the menu)
    const float cs = lhTitle * 0.9f;
    closeHit_ = rectF(panel.right - pad - cs, y + (lhTitle - cs) * 0.5f, panel.right - pad, y + (lhTitle + cs) * 0.5f);
    roundRect(closeHit_, cs * 0.5f, rgb(0x262B38, a));
    text(stack_.size() > 1 ? L"\x2190" : L"\x00D7", closeHit_, sizeBody, true, rgb(kText, a), kAlignCenter);
    titleRoom -= cs + pad * 0.5f;
  }
  text(title, rectF(tx, y, panel.right - pad, y + lhTitle), fit(title, sizeTitle, true, titleRoom), true,
       withA(accent_, a));
  y += lhTitle;
  if (!p->subtitle.empty()) {
    std::wstring sub = widen(p->subtitle);
    text(sub, rectF(tx, y - lhSmall * 0.15f, panel.right - pad, y + lhSmall * 0.85f),
         fit(sub, sizeSmall, false, inner), false, rgb(kSub, a));
    y += lhSmall;
  }
  y += pad * 0.35f;
  cv_->fillRect(rectF(tx, y, panel.right - pad, y + std::max(1.0f, H / 720.0f)), withA(accent_, 0.35f * a));
  y += pad * 0.45f;

  // The list (beside a custom panel on pages that have one)
  const float row = lhBody * 1.32f, footerH = lhSmall * 3.2f;
  const float listTop = y, listBottom = panel.bottom - footerH - pad * 0.5f;
  const float listW = inner * p->listFraction;
  const float vx = tx + listW;  // values end here
  const int visible = std::max(1, (int)((listBottom - listTop) / row));
  const int n = (int)p->items.size();
  // Keep the selection in view (smooth scroll); on the last selectable row,
  // also bring the read-only rows after it (e.g. Options' Hotkeys) into view.
  float target = p->scroll;
  if (n > 0 && !p->freeScroll) {
    if (p->sel < target + 1) target = (float)std::max(0, p->sel - 1);
    if (p->sel > target + visible - 2) target = (float)(p->sel - visible + 2);
    int last = -1;
    for (int i = n - 1; i >= 0 && last < 0; i--)
      if (selectable(p->items[i])) last = i;
    if (p->sel == last) target = std::max(target, std::min((float)p->sel, (float)(n - visible)));
  }
  target = std::clamp(target, 0.0f, (float)std::max(0, n - visible));
  p->scroll += (target - p->scroll) * 0.35f;
  if (std::fabs(target - p->scroll) < 0.01f) p->scroll = target;

  cv_->pushClip(rectF(panel.left, listTop - row * 0.1f, panel.right, listBottom + row * 0.1f));
  rowHits_.clear();
  rowH_ = row;
  int first = (int)std::floor(p->scroll);
  for (int i = first; i < n && i < first + visible + 1; i++) {
    float ry = listTop + (i - p->scroll) * row;
    if (ry < listTop - row * 0.5f || ry + row > listBottom + row * 0.5f) continue;
    MenuItem& it = p->items[i];
    if (ry + row * 0.5f >= listTop && ry + row * 0.5f <= listBottom)
      rowHits_.push_back({rectF(panel.left + pad * 0.5f, ry, vx + pad * 0.5f, ry + row), i});
    const bool en = !it.enabled || it.enabled();
    const bool sel = i == p->sel && selectable(it);
    if (it.kind == MenuItem::Separator) {
      if (!it.label.empty())
        text(widen(it.label), rectF(tx, ry + row - lhSmall * 1.15f, vx, ry + row - lhSmall * 0.05f), sizeSmall,
             true, withA(accent_, 0.9f * a));
      continue;
    }
    if (sel) {
      float pulse = 0.22f + 0.06f * std::sin(time_ * 4.0f);
      RectF hr = rectF(panel.left + pad * 0.5f, ry + row * 0.06f, vx + pad * 0.5f, ry + row * 0.94f);
      roundRect(hr, row * 0.2f, withA(accent_, pulse * a));
      float bh = (hr.bottom - hr.top);
      roundRect(rectF(hr.left, hr.top + bh * 0.18f, hr.left + std::max(3.0f, H / 300.0f), hr.top + bh * 0.82f),
                2.0f, withA(accent_, a));
    }
    ColorF col = rgb(!en ? kDim : sel ? 0xFFFFFF : kText, a);
    ColorF vcol = rgb(!en ? kDim : sel ? 0xFFFFFF : kValue, a);
    const float lx = tx + (sel ? pad * 0.35f : 0);
    const RectF rr = rectF(lx, ry, vx, ry + row);
    std::wstring label = widen(it.label);
    // the value on the right, and what the label may use
    std::wstring val;
    float valRoom = 0;  // space the value's control takes
    switch (it.kind) {
      case MenuItem::Toggle: {
        bool on = it.get && it.get();
        float sh = lhBody * 0.62f, sw = sh * 1.9f;
        RectF sr = rectF(vx - sw, ry + (row - sh) * 0.5f, vx, ry + (row + sh) * 0.5f);
        roundRect(sr, sh * 0.5f, on ? withA(accent_, (en ? 1.0f : 0.4f) * a) : rgb(kOff, a));
        float kx = on ? sr.right - sh : sr.left;
        roundRect(rectF(kx + sh * 0.12f, sr.top + sh * 0.12f, kx + sh * 0.88f, sr.top + sh * 0.88f), sh * 0.38f,
                  rgb(kKnob, a));
        valRoom = sw + sh * 0.7f;
        if (it.valueText) {  // a note beside the switch (what the setting needs, ...)
          std::wstring note = widen(it.valueText());
          float right = sr.left - sh * 0.7f, left = lx + measure(label, sizeBody, sel) + sh;
          float room = std::max(0.0f, right - left);
          float w0 = measure(note, sizeBody, false);
          if (!note.empty() && room > w0 * 0.35f)
            text(note, rectF(left, ry, right, ry + row), sizeBody * std::min(1.0f, room / std::max(1.0f, w0)), false,
                 vcol, kAlignTrailing);
          valRoom = std::max(valRoom, vx - left);
        }
        break;
      }
      case MenuItem::Choice: {
        if (it.valueText) val = widen(it.valueText());
        else if (it.get && it.options) {
          auto o = it.options();
          int v = it.get();
          val = widen((v >= 0 && v < (int)o.size()) ? o[v] : "?");
        }
        if (sel) val = L"◀  " + val + L"  ▶";
        break;
      }
      case MenuItem::Slider: {
        int c = it.get ? it.get() : 0;
        std::wstring txt = it.valueText ? widen(it.valueText()) : std::to_wstring(c) + widen(it.suffix);
        float bw = pw * 0.22f, bh = std::max(3.0f, H / 200.0f);
        float numW = measure(L"00000", sizeBody, false);
        RectF br = rectF(vx - bw - numW, ry + row * 0.5f - bh * 0.5f, vx - numW, ry + row * 0.5f + bh * 0.5f);
        roundRect(br, bh * 0.5f, rgb(kOff, a));
        float f = it.maxV > it.minV ? (float)(c - it.minV) / (it.maxV - it.minV) : 0;
        f = std::clamp(f, 0.0f, 1.0f);
        roundRect(rectF(br.left, br.top, br.left + bw * f, br.bottom), bh * 0.5f,
                  en ? withA(accent_, a) : rgb(kDim, a));
        float kx = br.left + bw * f;
        roundRect(rectF(kx - bh * 1.4f, br.top - bh * 0.9f, kx + bh * 1.4f, br.bottom + bh * 0.9f), bh * 1.4f,
                  rgb(kKnob, a));
        text(txt, rr, sizeBody, false, vcol, kAlignTrailing);
        valRoom = bw + numW + bh * 2;
        break;
      }
      case MenuItem::Bind:
        if (capturing && sel) {
          val = L"Press…";
          vcol = withA(accent_, a);
        } else if (it.valueText) {
          val = widen(it.valueText());
        }
        break;
      default:
        if (it.valueText) val = widen(it.valueText());
        break;
    }
    float labelRoom = vx - lx - valRoom - (val.empty() ? 0.0f : measure(val, sizeBody, false) + pad * 0.5f);
    float labelSize = fit(label, sizeBody, sel, std::max(labelRoom, (vx - lx) * 0.3f));
    text(label, rectF(lx, ry, vx, ry + row), labelSize, sel, col);
    if (!val.empty()) {
      float room = std::max(vx - lx - measure(label, labelSize, sel) - pad * 0.5f, (vx - lx) * 0.25f);
      text(val, rr, fit(val, sizeBody, false, room), false, vcol, kAlignTrailing);
    }
  }
  cv_->popClip();

  // scroll indicators
  if (p->scroll > 0.05f)
    text(L"↑", rectF(tx, listTop - lhSmall * 1.05f, tx + listW, listTop), sizeSmall, false, rgb(kCrumb, a),
         kAlignCenter);
  if (p->scroll + visible < n - 0.05f)
    text(L"↓", rectF(tx, listBottom - lhSmall * 0.3f, tx + listW, listBottom + lhSmall * 0.7f), sizeSmall,
         false, rgb(kCrumb, a), kAlignCenter);

  if (p->drawCustom) {
    RectF cr = p->listFraction < 0.999f ? rectF(vx + pad, listTop, panel.right - pad, listBottom)
                                              : rectF(tx, listTop, panel.right - pad, listBottom);
    p->drawCustom(*this, cv_, cr);
  }

  // Footer: what the selected row does, and the keys
  float fy = panel.bottom - footerH;
  cv_->fillRect(rectF(tx, fy, panel.right - pad, fy + std::max(1.0f, H / 720.0f)), colorF(1, 1, 1, 0.19f * a));
  std::wstring help;
  if (capturing) help = widen(captureText);
  else if (n > 0 && p->sel < n && selectable(p->items[p->sel])) help = widen(p->items[p->sel].help);
  if (!help.empty())
    text(help, rectF(tx, fy + lhSmall * 0.35f, panel.right - pad, fy + lhSmall * 1.35f),
         fit(help, sizeSmall, false, inner), false, capturing ? withA(accent_, a) : rgb(kHint, a));
  std::wstring keys = keysHint.empty() ? L"↑↓ Select      ←→ Change      Enter / Ⓐ Confirm      Esc / Ⓑ Back" : keysHint;
  text(keys, rectF(tx, fy + lhSmall * 1.75f, panel.right - pad, fy + lhSmall * 2.75f),
       fit(keys, sizeSmall, false, inner), false, rgb(kKeys, a));
}

// A tap: the close button backs out; a row is chosen and used (a slider
// moves the way of the half tapped).
void Menu::tap(float x, float y) {
  if (stack_.empty() || capturing) return;
  std::shared_ptr<MenuPage> keep = stack_.back();
  MenuPage* p = keep.get();
  auto in = [&](const RectF& r) { return x >= r.left && x < r.right && y >= r.top && y < r.bottom; };
  if (closeHit_.right > closeHit_.left && in(closeHit_)) {
    pop();
    return;
  }
  for (const RowHit& h : rowHits_) {
    if (!in(h.r) || h.index >= (int)p->items.size()) continue;
    MenuItem& it = p->items[h.index];
    if (!selectable(it)) return;
    p->sel = h.index;
    if (it.kind == MenuItem::Slider) adjust(it, x < (h.r.left + h.r.right) * 0.5f ? -1 : 1);
    else if (it.kind == MenuItem::Choice && !it.onActivate && x < h.r.left + (h.r.right - h.r.left) * 0.5f)
      adjust(it, -1);
    else activate(it);
    return;
  }
}

void Menu::drag(float dy) {
  MenuPage* p = top();
  if (!p || dy == 0) return;
  p->freeScroll = true;
  p->scroll = std::max(0.0f, std::min(p->scroll - dy / std::max(rowH_, 1.0f), (float)std::max(0, (int)p->items.size() - 1)));
}
