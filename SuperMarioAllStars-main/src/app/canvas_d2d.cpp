// Canvas on Direct2D / DirectWrite (Windows): the video layer's 2D context.
#include "app/canvas_d2d.h"
#include <dwrite_3.h>
#include <algorithm>
#include <cmath>

namespace {
unsigned g_epoch = 0;
D2D1_COLOR_F dc(ColorF c) { return D2D1::ColorF(c.r, c.g, c.b, c.a); }
D2D1_RECT_F dr(RectF r) { return D2D1::RectF(r.left, r.top, r.right, r.bottom); }
}  // namespace

D2DCanvas::D2DCanvas(ID2D1DeviceContext* ctx, IDWriteFactory* dw) : ctx_(ctx), dw_(dw), epoch_(++g_epoch) {
  if (ctx_) ctx_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 1), &brush_);
  if (ctx_) ctx_->GetFactory(&factory_);
}

D2DCanvas::~D2DCanvas() {
  for (auto& f : fcache_)
    if (f.f) f.f->Release();
  if (titleFonts_) titleFonts_->Release();
  for (ID2D1PathGeometry* g : shapes_)
    if (g) g->Release();
  for (Image& i : images_)
    if (i.bmp) i.bmp->Release();
  if (brush_) brush_->Release();
  if (factory_) factory_->Release();
}

ID2D1SolidColorBrush* D2DCanvas::brush(ColorF c) {
  brush_->SetColor(dc(c));
  return brush_;
}

void D2DCanvas::fillRect(RectF r, ColorF c) { ctx_->FillRectangle(dr(r), brush(c)); }
void D2DCanvas::strokeRect(RectF r, float width, ColorF c) { ctx_->DrawRectangle(dr(r), brush(c), width); }
void D2DCanvas::fillRound(RectF r, float radius, ColorF c) {
  ctx_->FillRoundedRectangle(D2D1::RoundedRect(dr(r), radius, radius), brush(c));
}
void D2DCanvas::strokeRound(RectF r, float radius, float width, ColorF c) {
  ctx_->DrawRoundedRectangle(D2D1::RoundedRect(dr(r), radius, radius), brush(c), width);
}
void D2DCanvas::strokeEllipse(float cx, float cy, float rx, float ry, float width, ColorF c) {
  ctx_->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), rx, ry), brush(c), width);
}

// (in 2 px strips: a gradient brush came out in broad bands)
void D2DCanvas::fillGradient(RectF r, ColorF top, ColorF bottom) {
  float h = r.bottom - r.top;
  if (h <= 0) return;
  for (float y = r.top; y < r.bottom; y += 2) {
    float t = (y + 1 - r.top) / h;
    ColorF c = {top.r + (bottom.r - top.r) * t, top.g + (bottom.g - top.g) * t, top.b + (bottom.b - top.b) * t,
                top.a + (bottom.a - top.a) * t};
    ctx_->FillRectangle(D2D1::RectF(r.left, y, r.right, std::min(r.bottom, y + 2)), brush(c));
  }
}

ID2D1PathGeometry* D2DCanvas::geometry(const PointF* pts, const int* counts, int figures) {
  if (!factory_) return nullptr;
  ID2D1PathGeometry* g = nullptr;
  ID2D1GeometrySink* sink = nullptr;
  if (FAILED(factory_->CreatePathGeometry(&g)) || FAILED(g->Open(&sink))) {
    if (g) g->Release();
    return nullptr;
  }
  for (int f = 0; f < figures; f++) {
    int n = counts[f];
    if (n >= 3) {
      sink->BeginFigure(D2D1::Point2F(pts[0].x, pts[0].y), D2D1_FIGURE_BEGIN_FILLED);
      for (int i = 1; i < n; i++) sink->AddLine(D2D1::Point2F(pts[i].x, pts[i].y));
      sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    }
    pts += n;
  }
  sink->Close();
  sink->Release();
  return g;
}

void D2DCanvas::fillPolygon(const PointF* pts, int n, ColorF c) {
  if (ID2D1PathGeometry* g = geometry(pts, &n, 1)) {
    ctx_->FillGeometry(g, brush(c));
    g->Release();
  }
}

void D2DCanvas::strokePolygon(const PointF* pts, int n, float width, ColorF c) {
  if (ID2D1PathGeometry* g = geometry(pts, &n, 1)) {
    ctx_->DrawGeometry(g, brush(c), width);
    g->Release();
  }
}

int D2DCanvas::makeShape(const std::vector<PointF>& pts, const std::vector<int>& counts) {
  ID2D1PathGeometry* g = geometry(pts.data(), counts.data(), (int)counts.size());
  for (size_t i = 0; i < shapes_.size(); i++)
    if (!shapes_[i]) {
      shapes_[i] = g;
      return (int)i + 1;
    }
  shapes_.push_back(g);
  return (int)shapes_.size();
}

void D2DCanvas::fillShape(int shape, ColorF c, float scale, float tx, float ty) {
  if (shape < 1 || shape > (int)shapes_.size() || !shapes_[shape - 1]) return;
  bool moved = scale != 1.0f || tx != 0 || ty != 0;
  if (moved) ctx_->SetTransform(D2D1::Matrix3x2F::Scale(scale, scale) * D2D1::Matrix3x2F::Translation(tx, ty));
  ctx_->FillGeometry(shapes_[shape - 1], brush(c));
  if (moved) ctx_->SetTransform(D2D1::Matrix3x2F::Identity());
}

void D2DCanvas::freeShape(int shape) {
  if (shape < 1 || shape > (int)shapes_.size() || !shapes_[shape - 1]) return;
  shapes_[shape - 1]->Release();
  shapes_[shape - 1] = nullptr;
}

// The title font, from its file beside the exe (a font set of its own: it isn't installed).
void D2DCanvas::loadTitleFont() {
  titleTried_ = true;
  wchar_t exe[MAX_PATH];
  GetModuleFileNameW(nullptr, exe, MAX_PATH);
  std::wstring path = exe;
  path = path.substr(0, path.find_last_of(L"\\/") + 1) + L"LuckiestGuy-Regular.ttf";
  IDWriteFactory5* f5 = nullptr;
  if (FAILED(dw_->QueryInterface(__uuidof(IDWriteFactory5), (void**)&f5)) || !f5) return;
  IDWriteFontSetBuilder1* builder = nullptr;
  IDWriteFontFile* file = nullptr;
  IDWriteFontSet* set = nullptr;
  IDWriteFontCollection1* coll = nullptr;
  if (SUCCEEDED(f5->CreateFontSetBuilder(&builder)) && SUCCEEDED(f5->CreateFontFileReference(path.c_str(), nullptr, &file)) &&
      SUCCEEDED(builder->AddFontFile(file)) && SUCCEEDED(builder->CreateFontSet(&set)) &&
      SUCCEEDED(f5->CreateFontCollectionFromFontSet(set, &coll)))
    titleFonts_ = coll;
  if (set) set->Release();
  if (file) file->Release();
  if (builder) builder->Release();
  f5->Release();
}

IDWriteTextFormat* D2DCanvas::format(float size, int weight, TextAlign a, ParaAlign p, bool title) {
  int isz = (int)std::lround(size * 4);
  for (auto& f : fcache_)
    if (f.size == isz && f.weight == weight && f.a == (int)a && f.p == (int)p && f.title == title) return f.f;
  if (title && !titleTried_) loadTitleFont();
  IDWriteTextFormat* tf = nullptr;
  if (title && titleFonts_)
    dw_->CreateTextFormat(L"Luckiest Guy", titleFonts_, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                          DWRITE_FONT_STRETCH_NORMAL, std::max(size, 1.0f), L"en-us", &tf);
  else
    dw_->CreateTextFormat(L"Segoe UI", nullptr, (DWRITE_FONT_WEIGHT)(title ? kWeightBold : weight),
                          DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, std::max(size, 1.0f), L"en-us", &tf);
  if (tf) {
    tf->SetTextAlignment(a == kAlignTrailing ? DWRITE_TEXT_ALIGNMENT_TRAILING
                         : a == kAlignCenter ? DWRITE_TEXT_ALIGNMENT_CENTER
                                             : DWRITE_TEXT_ALIGNMENT_LEADING);
    tf->SetParagraphAlignment(p == kParaFar      ? DWRITE_PARAGRAPH_ALIGNMENT_FAR
                              : p == kParaCenter ? DWRITE_PARAGRAPH_ALIGNMENT_CENTER
                                                 : DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    tf->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
  }
  fcache_.push_back({isz, weight, (int)a, (int)p, title, tf});
  if (fcache_.size() > 96) {
    if (fcache_.front().f) fcache_.front().f->Release();
    fcache_.erase(fcache_.begin());
  }
  return tf;
}

void D2DCanvas::text(const std::wstring& s, RectF r, float size, int weight, ColorF c, TextAlign align,
                     ParaAlign para) {
  IDWriteTextFormat* f = format(size, weight, align, para);
  if (!f) return;
  ctx_->DrawTextW(s.c_str(), (UINT32)s.size(), f, dr(r), brush(c), D2D1_DRAW_TEXT_OPTIONS_CLIP);
}

float D2DCanvas::measure(const std::wstring& s, float size, int weight) {
  IDWriteTextFormat* f = format(size, weight, kAlignLeading, kParaNear);
  if (!f || s.empty()) return 0;
  IDWriteTextLayout* l = nullptr;
  if (FAILED(dw_->CreateTextLayout(s.c_str(), (UINT32)s.size(), f, 100000.0f, 1000.0f, &l)) || !l) return 0;
  DWRITE_TEXT_METRICS m{};
  l->GetMetrics(&m);
  l->Release();
  return m.widthIncludingTrailingWhitespace;
}

void D2DCanvas::titleText(const std::wstring& s, RectF r, float size, ColorF c, TextAlign align, ParaAlign para) {
  IDWriteTextFormat* f = format(size, kWeightNormal, align, para, true);
  if (!f) return;
  ctx_->DrawTextW(s.c_str(), (UINT32)s.size(), f, dr(r), brush(c), D2D1_DRAW_TEXT_OPTIONS_CLIP);
}

float D2DCanvas::measureTitle(const std::wstring& s, float size) {
  IDWriteTextFormat* f = format(size, kWeightNormal, kAlignLeading, kParaNear, true);
  if (!f || s.empty()) return 0;
  IDWriteTextLayout* l = nullptr;
  if (FAILED(dw_->CreateTextLayout(s.c_str(), (UINT32)s.size(), f, 100000.0f, 1000.0f, &l)) || !l) return 0;
  DWRITE_TEXT_METRICS m{};
  l->GetMetrics(&m);
  l->Release();
  return m.widthIncludingTrailingWhitespace;
}

void D2DCanvas::pushClip(RectF r) { ctx_->PushAxisAlignedClip(dr(r), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE); }
void D2DCanvas::popClip() { ctx_->PopAxisAlignedClip(); }

int D2DCanvas::makeImage(int w, int h, bool alpha) {
  Image im;
  im.alpha = alpha;
  im.w = w;
  im.h = h;
  D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
      D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, alpha ? D2D1_ALPHA_MODE_PREMULTIPLIED : D2D1_ALPHA_MODE_IGNORE));
  ctx_->CreateBitmap(D2D1::SizeU(w, h), nullptr, 0, props, &im.bmp);
  for (size_t i = 0; i < images_.size(); i++)
    if (!images_[i].bmp) {
      images_[i] = im;
      return (int)i + 1;
    }
  images_.push_back(im);
  return (int)images_.size();
}

void D2DCanvas::updateImage(int image, const uint32_t* px, int stridePixels) {
  if (image < 1 || image > (int)images_.size() || !images_[image - 1].bmp) return;
  const Image& im = images_[image - 1];
  if (im.alpha) {  // (Direct2D's are premultiplied)
    std::vector<uint32_t> pm((size_t)im.w * im.h);
    for (int y = 0; y < im.h; y++)
      for (int x = 0; x < im.w; x++) {
        const uint32_t v = px[(size_t)y * stridePixels + x], a = v >> 24;
        auto ch = [&](int s) { return ((v >> s & 0xff) * a + 127) / 255 << s; };
        pm[(size_t)y * im.w + x] = a << 24 | ch(16) | ch(8) | ch(0);
      }
    im.bmp->CopyFromMemory(nullptr, pm.data(), im.w * 4);
    return;
  }
  im.bmp->CopyFromMemory(nullptr, px, stridePixels * 4);
}

void D2DCanvas::drawImage(int image, RectF dst, float opacity, bool smooth) {
  if (image < 1 || image > (int)images_.size() || !images_[image - 1].bmp) return;
  ctx_->DrawBitmap(images_[image - 1].bmp, dr(dst), opacity,
                   smooth ? D2D1_INTERPOLATION_MODE_LINEAR : D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
}

void D2DCanvas::freeImage(int image) {
  if (image < 1 || image > (int)images_.size() || !images_[image - 1].bmp) return;
  images_[image - 1].bmp->Release();
  images_[image - 1].bmp = nullptr;
}
