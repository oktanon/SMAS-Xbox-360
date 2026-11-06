// Direct3D 11 presentation with shader filters + Direct2D/DirectWrite UI layer.
#pragma once
#include <windows.h>
#include <d3d11_1.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <dxgi1_5.h>
#include <memory>
#include <cstdint>
#include <string>
#include <vector>
#include "app/canvas_d2d.h"
#include "slang.h"

struct VideoParams {
  int srcW = 256, srcH = 224;  // active source size (pixels)
  IRect dest{};                // destination rect in back buffer pixels
  int filter = 1;
  float scanlines = 0.45f;
  float curve = 0.0f;
  int colorProfile = 0;
  float brightness = 1, contrast = 1, saturation = 1, gamma = 1;
  int colorblind = 0;
  float temperature = 0;       // -1 cool .. +1 warm
  int pillarbox = 1;           // 0 black, 1 blurred, 2 dark frame
  float fade = 1.0f;           // global fade (menu dim handled by UI)
  unsigned frame = 0;          // the game's frame count (slang shaders' FrameCount)
};

class Video {
public:
  bool init(HWND hwnd);
  void shutdown();
  void resize();
  bool setExclusive(bool on, HMONITOR monitor);
  bool exclusive() const { return exclusive_; }
  // Whether the swap chain really is in exclusive fullscreen right now (Windows
  // leaves it on Alt+Tab); updates exclusive() to match.
  bool fullscreenNow() {
    BOOL fs = FALSE;
    if (swap_) swap_->GetFullscreenState(&fs, nullptr);
    if (!fs) exclusive_ = false;
    return fs != FALSE;
  }
  bool tearingSupported() const { return tearing_; }
  // The GPU was removed or reset (driver update or crash, TDR): every D3D/D2D
  // object is dead and the whole video layer must be rebuilt (shutdown + init).
  bool deviceLost() const { return deviceLost_; }
  bool ready() const { return swap_ != nullptr && !deviceLost_; }
  void simulateDeviceLost() {  // (--test-device-lost)
    deviceLost_ = true;
    lastError_ = (long)DXGI_ERROR_DEVICE_REMOVED;
  }
  long lastError() const { return lastError_; }
  const std::string& diag() const { return diag_; }
  int width() const { return width_; }
  int height() const { return height_; }

  void upload(const uint32_t* px, int w, int h, int stride);
  void draw(const VideoParams& p);
  void clear();  // black, no game picture (the launcher)
  // Capture the last uploaded frame (for screenshots): returns w,h, BGRA
  ID2D1DeviceContext* ui() { return d2dCtx_; }
  IDWriteFactory* dwrite() { return dwrite_; }
  Canvas* canvas() { return canvas_.get(); }  // (the UI layer, between beginUi and endUi)
  ID2D1Factory1* d2dFactory() { return d2dFactory_; }
  void beginUi();
  void endUi();
  // syncInterval 1 = wait for vblank, 0 = immediately (tearing / variable refresh)
  void present(int syncInterval);
  // Wait for the vertical blank of the display showing the window. Drivers can
  // force vsync off (or use "fast sync"), so Present alone may not throttle.
  bool waitForVBlank();
  void refreshOutput();  // re-query the display after the window moved
  bool frameStats(DXGI_FRAME_STATISTICS* fs) { return swap_ && SUCCEEDED(swap_->GetFrameStatistics(fs)); }
  // Read back the current back buffer (0x00RRGGBB). Call after drawing, before present.
  bool capture(std::vector<uint32_t>& out, int* w, int* h);
  // A slang shader preset in place of the built-in filters (empty: off);
  // false (and why) if it couldn't be loaded.
  bool setSlang(const std::wstring& path, std::string* error) { return dev_ && slang_.load(dev_, path, error); }
  bool slangActive() const { return slang_.active(); }
  bool slangAvailable() { return slang_.available(); }

private:
  HWND hwnd_ = nullptr;
  int width_ = 0, height_ = 0;
  bool exclusive_ = false;
  bool deviceLost_ = false;
  void checkDevice(HRESULT hr);
  long lastError_ = 0;
  std::string diag_;
  bool tearing_ = false;
  UINT swapFlags_ = 0;
  IDXGIOutput* output_ = nullptr;
  ID3D11Device* dev_ = nullptr;
  ID3D11DeviceContext* ctx_ = nullptr;
  IDXGISwapChain1* swap_ = nullptr;
  ID3D11RenderTargetView* rtv_ = nullptr;
  ID3D11Texture2D* tex_ = nullptr;
  ID3D11ShaderResourceView* srv_ = nullptr;
  ID3D11VertexShader* vs_ = nullptr;
  ID3D11PixelShader* ps_ = nullptr;
  ID3D11Buffer* cb_ = nullptr;
  ID3D11SamplerState* sPoint_ = nullptr;
  ID3D11SamplerState* sLinear_ = nullptr;
  ID3D11RasterizerState* rs_ = nullptr;
  ID3D11BlendState* bs_ = nullptr;
  ID2D1Factory1* d2dFactory_ = nullptr;
  ID2D1Device* d2dDev_ = nullptr;
  ID2D1DeviceContext* d2dCtx_ = nullptr;
  ID2D1Bitmap1* d2dTarget_ = nullptr;
  IDWriteFactory* dwrite_ = nullptr;
  std::unique_ptr<D2DCanvas> canvas_;
  int texW_ = 0, texH_ = 0;
  SlangChain slang_;

  bool createTargets();
  void releaseTargets();
  bool compileShaders();
  void drawPass(const VideoParams& p, int pass, const D3D11_VIEWPORT& vp);
};
