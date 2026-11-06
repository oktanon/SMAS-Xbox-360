// Canvas on Direct2D / DirectWrite (Windows): see canvas.h.
#pragma once
#include <d2d1_1.h>
#include <dwrite.h>
#include <vector>
#include "app/canvas.h"

class D2DCanvas : public Canvas {
public:
  D2DCanvas(ID2D1DeviceContext* ctx, IDWriteFactory* dw);
  ~D2DCanvas() override;
  unsigned epoch() const override { return epoch_; }
  void fillRect(RectF r, ColorF c) override;
  void strokeRect(RectF r, float width, ColorF c) override;
  void fillRound(RectF r, float radius, ColorF c) override;
  void strokeRound(RectF r, float radius, float width, ColorF c) override;
  void strokeEllipse(float cx, float cy, float rx, float ry, float width, ColorF c) override;
  void fillGradient(RectF r, ColorF top, ColorF bottom) override;
  void fillPolygon(const PointF* pts, int n, ColorF c) override;
  void strokePolygon(const PointF* pts, int n, float width, ColorF c) override;
  int makeShape(const std::vector<PointF>& pts, const std::vector<int>& counts) override;
  void fillShape(int shape, ColorF c, float scale, float tx, float ty) override;
  void freeShape(int shape) override;
  void text(const std::wstring& s, RectF r, float size, int weight, ColorF c, TextAlign align,
            ParaAlign para) override;
  float measure(const std::wstring& s, float size, int weight) override;
  void titleText(const std::wstring& s, RectF r, float size, ColorF c, TextAlign align, ParaAlign para) override;
  float measureTitle(const std::wstring& s, float size) override;
  void pushClip(RectF r) override;
  void popClip() override;
  int makeImage(int w, int h, bool alpha) override;
  void updateImage(int image, const uint32_t* px, int stridePixels) override;
  void drawImage(int image, RectF dst, float opacity, bool smooth) override;
  void freeImage(int image) override;

private:
  ID2D1DeviceContext* ctx_ = nullptr;
  IDWriteFactory* dw_ = nullptr;
  ID2D1Factory* factory_ = nullptr;
  ID2D1SolidColorBrush* brush_ = nullptr;
  unsigned epoch_ = 0;
  struct FormatKey {
    int size, weight, a, p;
    bool title;
    IDWriteTextFormat* f;
  };
  std::vector<FormatKey> fcache_;
  std::vector<ID2D1PathGeometry*> shapes_;
  struct Image {
    ID2D1Bitmap* bmp = nullptr;
    bool alpha = false;
    int w = 0, h = 0;
  };
  std::vector<Image> images_;
  ID2D1SolidColorBrush* brush(ColorF c);
  IDWriteTextFormat* format(float size, int weight, TextAlign a, ParaAlign p, bool title = false);
  IDWriteFontCollection* titleFonts_ = nullptr;  // (the title font's file, beside the exe)
  bool titleTried_ = false;
  void loadTitleFont();
  ID2D1PathGeometry* geometry(const PointF* pts, const int* counts, int figures);
};
