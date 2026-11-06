#include "slang.h"
#define LIBRA_RUNTIME_D3D11
#include "../../third_party/librashader/librashader_ld.h"
#include <algorithm>

namespace {
libra_instance_t g_libra;

template <class T>
void safeRelease(T*& p) {
  if (p) p->Release();
  p = nullptr;
}

std::string utf8(const std::wstring& w) {
  if (w.empty()) return {};
  int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
  std::string s(n, 0);
  WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), s.data(), n, nullptr, nullptr);
  return s;
}

// librashader's message for an error (and the error freed).
std::string message(libra_error_t e) {
  std::string s = "unknown error";
  char* text = nullptr;
  if (g_libra.error_write && g_libra.error_write(e, &text) == 0 && text) {
    s = text;
    if (g_libra.error_free_string) g_libra.error_free_string(&text);
  }
  if (g_libra.error_free) g_libra.error_free(&e);
  return s;
}
}  // namespace

bool SlangChain::available() {
  if (!tried_) {
    tried_ = true;
    g_libra = librashader_load_instance();
    ok_ = g_libra.instance_loaded && g_libra.preset_create && g_libra.d3d11_filter_chain_create &&
          g_libra.d3d11_filter_chain_frame && g_libra.d3d11_filter_chain_free;
  }
  return ok_;
}

bool SlangChain::load(ID3D11Device* dev, const std::wstring& path, std::string* error) {
  if (path.empty()) {
    release();
    return true;
  }
  if (!available()) {
    if (error) *error = "librashader.dll not found";
    return false;
  }
  libra_shader_preset_t preset = nullptr;
  if (libra_error_t e = g_libra.preset_create(utf8(path).c_str(), &preset)) {
    if (error) *error = message(e);
    return false;
  }
  filter_chain_d3d11_opt_t opt{};
  opt.version = LIBRASHADER_CURRENT_VERSION;
  libra_d3d11_filter_chain_t chain = nullptr;
  libra_error_t e = g_libra.d3d11_filter_chain_create(&preset, dev, &opt, &chain);
  if (preset && g_libra.preset_free) g_libra.preset_free(&preset);
  if (e) {
    if (error) *error = message(e);
    return false;
  }
  release();
  chain_ = chain;
  dev_ = dev;
  return true;
}

void SlangChain::draw(ID3D11DeviceContext* ctx, ID3D11Texture2D* src, int w, int h, ID3D11RenderTargetView* out,
                      float x, float y, float vw, float vh, unsigned frame) {
  if (!chain_ || w <= 0 || h <= 0) return;
  if (w != srcW_ || h != srcH_ || !srcTex_) {
    safeRelease(srcSrv_);
    safeRelease(srcTex_);
    D3D11_TEXTURE2D_DESC td{};
    td.Width = w;
    td.Height = h;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    if (FAILED(dev_->CreateTexture2D(&td, nullptr, &srcTex_))) return;
    dev_->CreateShaderResourceView(srcTex_, nullptr, &srcSrv_);
    srcW_ = w;
    srcH_ = h;
  }
  D3D11_BOX box{0, 0, 0, (UINT)w, (UINT)h, 1};
  ctx->CopySubresourceRegion(srcTex_, 0, 0, 0, 0, src, 0, &box);
  libra_viewport_t vp{x, y, (uint32_t)std::max(1.0f, vw), (uint32_t)std::max(1.0f, vh)};
  frame_d3d11_opt_t fo{};
  fo.version = LIBRASHADER_CURRENT_VERSION;
  fo.total_subframes = 1;
  fo.current_subframe = 1;
  fo.frames_per_second = 60.0988f;
  auto chain = (libra_d3d11_filter_chain_t)chain_;
  if (libra_error_t e = g_libra.d3d11_filter_chain_frame(&chain, ctx, frame, srcSrv_, out, &vp, nullptr, &fo))
    message(e);
}

void SlangChain::release() {
  if (chain_) {
    auto chain = (libra_d3d11_filter_chain_t)chain_;
    g_libra.d3d11_filter_chain_free(&chain);
  }
  chain_ = nullptr;
  safeRelease(srcSrv_);
  safeRelease(srcTex_);
  srcW_ = srcH_ = 0;
}
