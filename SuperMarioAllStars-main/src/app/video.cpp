#include "video.h"
#include <d3dcompiler.h>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace {

template <typename T>
void safeRelease(T*& p) {
  if (p) {
    p->Release();
    p = nullptr;
  }
}

const char* kShader = R"HLSL(
cbuffer P : register(b0) {
  float4 srcRect;   // uv x0,y0,x1,y1 of the active area
  float4 texSize;   // w,h,1/w,1/h
  float4 outSize;   // viewport w,h,1/w,1/h
  float4 f0;        // filter, scanlines, curve, pass
  float4 f1;        // brightness, contrast, saturation, gamma
  float4 f2;        // profile, colorblind, src w, src h
  float4 f3;        // fade, pillarbox, temperature, 0
};
Texture2D tex : register(t0);
SamplerState sPoint : register(s0);
SamplerState sLinear : register(s1);

struct VSOut { float4 pos : SV_Position; float2 uv : TEXCOORD0; };

VSOut vs(uint id : SV_VertexID) {
  VSOut o;
  float2 p = float2((id << 1) & 2, id & 2);
  o.pos = float4(p * float2(2, -2) + float2(-1, 1), 0, 1);
  o.uv = p;
  return o;
}

float2 clampUv(float2 uv) {
  float2 lo = srcRect.xy + 0.5 * texSize.zw;
  float2 hi = srcRect.zw - 0.5 * texSize.zw;
  return clamp(uv, lo, hi);
}

float3 texNearest(float2 uv) { return tex.SampleLevel(sPoint, clampUv(uv), 0).rgb; }
float3 texLinear(float2 uv) { return tex.SampleLevel(sLinear, clampUv(uv), 0).rgb; }

float3 texSharp(float2 uv) {
  float2 texel = uv * texSize.xy;
  float2 scale = max(floor(outSize.xy / f2.zw), 1.0);
  float2 tf = floor(texel);
  float2 s = frac(texel);
  float2 region = 0.5 - 0.5 / scale;
  float2 cd = s - 0.5;
  float2 f = (cd - clamp(cd, -region, region)) * scale + 0.5;
  return texLinear((tf + f) * texSize.zw);
}

// Horizontal-only sharp sample at a fixed row, used by the CRT beam model.
float3 texRow(float x, float row) {
  float2 scale = max(floor(outSize.xy / f2.zw), 1.0);
  float tf = floor(x);
  float s = frac(x);
  float region = 0.5 - 0.5 / max(scale.x, 1.0);
  float cd = s - 0.5;
  float f = (cd - clamp(cd, -region, region)) * max(scale.x * 0.6, 1.0) + 0.5;
  return texLinear(float2((tf + f) * texSize.z, (row + 0.5) * texSize.w));
}

float3 crt(float2 uv, float2 pos, bool aperture) {
  float2 texel = uv * texSize.xy;
  float y = texel.y - 0.5;
  float yf = floor(y);
  float t = y - yf;
  float3 c0 = texRow(texel.x, yf);
  float3 c1 = texRow(texel.x, yf + 1.0);
  c0 = c0 * c0;  // to linear-ish
  c1 = c1 * c1;
  float strength = f0.y;
  float3 w0 = exp(-t * t * lerp(3.0, 12.0, strength) / (0.6 + 0.8 * sqrt(c0)));
  float3 w1 = exp(-(1.0 - t) * (1.0 - t) * lerp(3.0, 12.0, strength) / (0.6 + 0.8 * sqrt(c1)));
  float3 col = c0 * w0 + c1 * w1;
  col *= lerp(1.0, 1.55, strength);
  if (aperture) {
    int m = (int)fmod(pos.x, 3.0);
    float3 mask = m == 0 ? float3(1.0, 0.7, 0.7) : (m == 1 ? float3(0.7, 1.0, 0.7) : float3(0.7, 0.7, 1.0));
    col *= lerp(float3(1, 1, 1), mask * 1.25, strength);
  }
  return sqrt(max(col, 0.0));
}

float3 lcd(float2 uv) {
  float2 texel = uv * texSize.xy;
  float2 f = frac(texel);
  float3 c = texNearest(uv);
  float2 scale = outSize.xy / f2.zw;
  float2 edge = smoothstep(0.0, 1.5 / scale, f) * smoothstep(0.0, 1.5 / scale, 1.0 - f);
  float g = lerp(1.0, edge.x * edge.y, 0.55 * f0.y + 0.2);
  return c * g;
}

float3 applyProfile(float3 c) {
  int prof = (int)f2.x;
  float l = dot(c, float3(0.299, 0.587, 0.114));
  if (prof == 1) {         // vivid
    c = lerp(l.xxx, c, 1.3);
    c = (c - 0.5) * 1.08 + 0.5;
  } else if (prof == 2) {  // CRT: warmer, deeper blacks, gentle bloom in brights
    c = pow(max(c, 0.0), 1.15);
    c = mul(float3x3(1.02, 0.02, -0.04, 0.0, 1.0, 0.0, -0.02, 0.03, 0.95), c);
  } else if (prof == 3) {  // muted
    c = lerp(l.xxx, c, 0.72);
    c = (c - 0.5) * 0.9 + 0.52;
  } else if (prof == 4) {  // grayscale
    c = l.xxx;
  } else if (prof == 5) {  // sepia
    c = float3(dot(c, float3(0.393, 0.769, 0.189)), dot(c, float3(0.349, 0.686, 0.168)),
               dot(c, float3(0.272, 0.534, 0.131)));
  } else if (prof == 6) {  // pocket handheld: 4 greens
    float q = floor(saturate(l) * 3.999);
    float3 pal[4] = { float3(0.06, 0.22, 0.06), float3(0.19, 0.38, 0.19), float3(0.55, 0.67, 0.06),
                      float3(0.61, 0.74, 0.06) };
    c = pal[(int)q];
  } else if (prof == 7) {  // red monochrome
    c = float3(pow(saturate(l), 0.8), 0.0, 0.0);
  } else if (prof == 8) {  // night / warm
    c = c * float3(1.0, 0.86, 0.62);
  } else if (prof == 9) {  // console accurate: SNES DAC response
    c = pow(max(c, 0.0), 1.18) * 1.04;
    c = lerp(l.xxx * 1.02, c, 0.92);
  }
  return c;
}

float3 daltonize(float3 c, int mode) {
  // Simulate deficiency in LMS space, then shift the lost information into
  // channels the viewer can distinguish.
  float3x3 rgb2lms = float3x3(17.8824, 43.5161, 4.11935, 3.45565, 27.1554, 3.86714, 0.0299566, 0.184309, 1.46709);
  float3x3 lms2rgb = float3x3(0.0809444479, -0.130504409, 0.116721066, -0.0102485335, 0.0540193266,
                              -0.113614708, -0.000365296938, -0.00412161469, 0.693511405);
  float3 lms = mul(rgb2lms, c);
  float3 s;
  if (mode == 1) s = float3(2.02344 * lms.y - 2.52581 * lms.z, lms.y, lms.z);
  else if (mode == 2) s = float3(lms.x, 0.494207 * lms.x + 1.24827 * lms.z, lms.z);
  else s = float3(lms.x, lms.y, -0.395913 * lms.x + 0.801109 * lms.y);
  float3 sim = mul(lms2rgb, s);
  float3 err = c - sim;
  float3 corr = float3(0.0, err.r * 0.7 + err.g, err.r * 0.7 + err.b);
  return saturate(c + corr);
}

float3 applyColor(float3 c) {
  c = pow(max(c, 0.0), 1.0 / f1.w);
  c = (c - 0.5) * f1.y + 0.5;
  c *= f1.x;
  float l = dot(c, float3(0.299, 0.587, 0.114));
  c = lerp(l.xxx, c, f1.z);
  c = applyProfile(saturate(c));
  float tmp = f3.z;
  if (tmp > 0) c *= lerp(float3(1, 1, 1), float3(1.0, 0.9, 0.74), tmp);
  else if (tmp < 0) c *= lerp(float3(1, 1, 1), float3(0.82, 0.92, 1.0), -tmp);
  int cb = (int)f2.y;
  if (cb > 0) c = daltonize(c, cb);
  return saturate(c);
}

float4 ps(VSOut i) : SV_Target {
  int passId = (int)f0.w;
  float2 uv = i.uv;
  if (passId == 1) {
    // Blurred, darkened backdrop for pillarbox areas
    float2 suv = lerp(srcRect.xy, srcRect.zw, uv);
    float3 acc = 0;
    float wsum = 0;
    [unroll] for (int y = -3; y <= 3; y++) {
      [unroll] for (int x = -3; x <= 3; x++) {
        float w = exp(-(x * x + y * y) / 8.0);
        acc += texLinear(suv + float2(x * 6.0, y * 4.0) * texSize.zw) * w;
        wsum += w;
      }
    }
    float3 c = applyColor(acc / wsum) * 0.32;
    float vign = 1.0 - 0.5 * pow(abs(uv.x * 2 - 1), 3);
    return float4(c * vign * f3.x, 1);
  }
  float curve = f0.z;
  if (curve > 0.0) {
    float2 cc = uv * 2 - 1;
    cc *= 1.0 + (cc.yx * cc.yx) * curve * 0.12;
    uv = cc * 0.5 + 0.5;
    if (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1) return float4(0, 0, 0, 1);
  }
  float2 suv = lerp(srcRect.xy, srcRect.zw, uv);
  int filter = (int)f0.x;
  float3 c;
  if (filter == 0) c = texNearest(suv);
  else if (filter == 1) c = texSharp(suv);
  else if (filter == 2) c = texLinear(suv);
  else if (filter == 3) c = crt(suv, i.pos.xy, false);
  else if (filter == 4) c = crt(suv, i.pos.xy, true);
  else c = lcd(suv);
  c = applyColor(c);
  if (curve > 0.0) {
    float2 cc = uv * 2 - 1;
    float v = saturate((1.0 - cc.x * cc.x) * (1.0 - cc.y * cc.y) * 12.0);
    c *= v;
  }
  return float4(c * f3.x, 1);
}
)HLSL";

struct Constants {
  float srcRect[4];
  float texSize[4];
  float outSize[4];
  float f0[4];
  float f1[4];
  float f2[4];
  float f3[4];
  float pad[4];
};

}  // namespace

bool Video::init(HWND hwnd) {
  hwnd_ = hwnd;
  deviceLost_ = false;
  exclusive_ = false;
  UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
  D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0};
  D3D_FEATURE_LEVEL got;
  HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, levels, 3, D3D11_SDK_VERSION,
                                 &dev_, &got, &ctx_);
  if (FAILED(hr)) {
    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags, levels, 3, D3D11_SDK_VERSION, &dev_, &got,
                           &ctx_);
    if (FAILED(hr)) return false;
  }
  IDXGIDevice* dxgiDev = nullptr;
  dev_->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDev);
  IDXGIAdapter* adapter = nullptr;
  dxgiDev->GetAdapter(&adapter);
  IDXGIFactory2* factory = nullptr;
  adapter->GetParent(__uuidof(IDXGIFactory2), (void**)&factory);
  IDXGIFactory5* f5 = nullptr;
  if (SUCCEEDED(factory->QueryInterface(__uuidof(IDXGIFactory5), (void**)&f5))) {
    BOOL allow = FALSE;
    if (SUCCEEDED(f5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allow, sizeof(allow))))
      tearing_ = allow;
    f5->Release();
  }
  RECT rc;
  GetClientRect(hwnd, &rc);
  width_ = std::max<int>(1, rc.right - rc.left);
  height_ = std::max<int>(1, rc.bottom - rc.top);
  DXGI_SWAP_CHAIN_DESC1 sd{};
  sd.Width = width_;
  sd.Height = height_;
  sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  sd.SampleDesc.Count = 1;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.BufferCount = 2;
  sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  // (No FRAME_LATENCY_WAITABLE_OBJECT: it makes SetFullscreenState fail. The
  // main loop waits for the display's vblank instead.)
  swapFlags_ = (tearing_ ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0) | DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
  sd.Flags = swapFlags_;
  hr = factory->CreateSwapChainForHwnd(dev_, hwnd, &sd, nullptr, nullptr, &swap_);
  factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
  factory->Release();
  adapter->Release();
  if (FAILED(hr)) {
    dxgiDev->Release();
    return false;
  }
  // Keep at most one frame queued for low input lag.
  IDXGIDevice1* dxgiDev1 = nullptr;
  if (SUCCEEDED(dxgiDev->QueryInterface(__uuidof(IDXGIDevice1), (void**)&dxgiDev1))) {
    dxgiDev1->SetMaximumFrameLatency(1);
    dxgiDev1->Release();
  }

  // Direct2D on the same device
  D2D1_FACTORY_OPTIONS fo{};
  hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &fo, (void**)&d2dFactory_);
  if (SUCCEEDED(hr)) hr = d2dFactory_->CreateDevice(dxgiDev, &d2dDev_);
  if (SUCCEEDED(hr)) hr = d2dDev_->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2dCtx_);
  dxgiDev->Release();
  if (FAILED(hr)) return false;
  DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&dwrite_);
  canvas_ = std::make_unique<D2DCanvas>(d2dCtx_, dwrite_);

  // Game texture
  texW_ = 640;
  texH_ = 240;
  D3D11_TEXTURE2D_DESC td{};
  td.Width = texW_;
  td.Height = texH_;
  td.MipLevels = 1;
  td.ArraySize = 1;
  td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  td.SampleDesc.Count = 1;
  td.Usage = D3D11_USAGE_DYNAMIC;
  td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  td.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  if (FAILED(dev_->CreateTexture2D(&td, nullptr, &tex_))) return false;
  dev_->CreateShaderResourceView(tex_, nullptr, &srv_);

  D3D11_SAMPLER_DESC sdesc{};
  sdesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
  sdesc.AddressU = sdesc.AddressV = sdesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
  sdesc.MaxLOD = D3D11_FLOAT32_MAX;
  dev_->CreateSamplerState(&sdesc, &sPoint_);
  sdesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  dev_->CreateSamplerState(&sdesc, &sLinear_);

  D3D11_RASTERIZER_DESC rd{};
  rd.FillMode = D3D11_FILL_SOLID;
  rd.CullMode = D3D11_CULL_NONE;
  rd.ScissorEnable = FALSE;
  rd.DepthClipEnable = TRUE;
  dev_->CreateRasterizerState(&rd, &rs_);

  D3D11_BUFFER_DESC bd{};
  bd.ByteWidth = sizeof(Constants);
  bd.Usage = D3D11_USAGE_DYNAMIC;
  bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  dev_->CreateBuffer(&bd, nullptr, &cb_);

  if (!compileShaders()) return false;
  return createTargets();
}

bool Video::compileShaders() {
  ID3DBlob* vsb = nullptr;
  ID3DBlob* psb = nullptr;
  ID3DBlob* err = nullptr;
  HRESULT hr = D3DCompile(kShader, strlen(kShader), "shader", nullptr, nullptr, "vs", "vs_4_0",
                          D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &vsb, &err);
  if (FAILED(hr)) {
    if (err) {
      OutputDebugStringA((const char*)err->GetBufferPointer());
      MessageBoxA(hwnd_, (const char*)err->GetBufferPointer(), "Shader error", MB_OK);
      err->Release();
    }
    return false;
  }
  hr = D3DCompile(kShader, strlen(kShader), "shader", nullptr, nullptr, "ps", "ps_4_0",
                  D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &psb, &err);
  if (FAILED(hr)) {
    if (err) {
      OutputDebugStringA((const char*)err->GetBufferPointer());
      MessageBoxA(hwnd_, (const char*)err->GetBufferPointer(), "Shader error", MB_OK);
      err->Release();
    }
    vsb->Release();
    return false;
  }
  dev_->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), nullptr, &vs_);
  dev_->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), nullptr, &ps_);
  vsb->Release();
  psb->Release();
  return vs_ && ps_;
}

bool Video::createTargets() {
  ID3D11Texture2D* bb = nullptr;
  if (FAILED(swap_->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb))) return false;
  dev_->CreateRenderTargetView(bb, nullptr, &rtv_);
  bb->Release();
  IDXGISurface* surf = nullptr;
  if (SUCCEEDED(swap_->GetBuffer(0, __uuidof(IDXGISurface), (void**)&surf))) {
    D2D1_BITMAP_PROPERTIES1 bp = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 96, 96);
    d2dCtx_->CreateBitmapFromDxgiSurface(surf, &bp, &d2dTarget_);
    surf->Release();
  }
  return rtv_ != nullptr;
}

void Video::releaseTargets() {
  if (d2dCtx_) d2dCtx_->SetTarget(nullptr);
  safeRelease(d2dTarget_);
  safeRelease(rtv_);
}

void Video::resize() {
  if (!swap_) return;
  RECT rc;
  GetClientRect(hwnd_, &rc);
  int w = std::max<int>(1, rc.right - rc.left), h = std::max<int>(1, rc.bottom - rc.top);
  if (w == width_ && h == height_ && rtv_) return;
  width_ = w;
  height_ = h;
  releaseTargets();
  ctx_->ClearState();
  ctx_->Flush();
  HRESULT hr = swap_->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, swapFlags_);
  checkDevice(hr);
  if (!deviceLost_) createTargets();
}

void Video::checkDevice(HRESULT hr) {
  if (SUCCEEDED(hr) && hr != DXGI_STATUS_OCCLUDED) return;
  bool lost = hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET || hr == DXGI_ERROR_DEVICE_HUNG ||
              hr == DXGI_ERROR_DRIVER_INTERNAL_ERROR;
  if (!lost && dev_ && FAILED(dev_->GetDeviceRemovedReason())) lost = true;
  if (lost) {
    deviceLost_ = true;
    lastError_ = (long)(hr == DXGI_ERROR_DEVICE_REMOVED && dev_ ? dev_->GetDeviceRemovedReason() : hr);
  }
}

bool Video::setExclusive(bool on, HMONITOR monitor) {
  if (!swap_) return false;
  if (on == exclusive_) return true;
  safeRelease(output_);
  if (on) {
    IDXGIOutput* target = nullptr;
    IDXGIDevice* dxgiDev = nullptr;
    dev_->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDev);
    IDXGIAdapter* adapter = nullptr;
    dxgiDev->GetAdapter(&adapter);
    DXGI_ADAPTER_DESC ad{};
    adapter->GetDesc(&ad);
    int outputs = 0;
    for (UINT i = 0;; i++) {
      IDXGIOutput* out = nullptr;
      if (adapter->EnumOutputs(i, &out) == DXGI_ERROR_NOT_FOUND) break;
      outputs++;
      DXGI_OUTPUT_DESC od;
      out->GetDesc(&od);
      if (od.Monitor == monitor) {
        target = out;
        break;
      }
      out->Release();
    }
    char dbuf[256];
    snprintf(dbuf, sizeof dbuf, "adapter '%ls', outputs %d, target %s", ad.Description, outputs,
             target ? "found" : "not found");
    diag_ = dbuf;
    adapter->Release();
    dxgiDev->Release();
    HRESULT hr = swap_->SetFullscreenState(TRUE, target);
    if (FAILED(hr)) {
      char buf[96];
      snprintf(buf, sizeof buf, "SetFullscreenState failed: 0x%08lx\n", (unsigned long)hr);
      OutputDebugStringA(buf);
      lastError_ = (long)hr;
      if (target) target->Release();
      return false;
    }
    exclusive_ = true;
    if (target) target->Release();
  } else {
    swap_->SetFullscreenState(FALSE, nullptr);
    exclusive_ = false;
  }
  width_ = height_ = 0;
  resize();
  return true;
}

void Video::shutdown() {
  if (swap_ && exclusive_) swap_->SetFullscreenState(FALSE, nullptr);
  exclusive_ = false;
  safeRelease(output_);
  releaseTargets();
  slang_.release();
  canvas_.reset();
  safeRelease(dwrite_);
  safeRelease(d2dCtx_);
  safeRelease(d2dDev_);
  safeRelease(d2dFactory_);
  safeRelease(bs_);
  safeRelease(rs_);
  safeRelease(cb_);
  safeRelease(sPoint_);
  safeRelease(sLinear_);
  safeRelease(vs_);
  safeRelease(ps_);
  safeRelease(srv_);
  safeRelease(tex_);
  safeRelease(swap_);
  safeRelease(ctx_);
  safeRelease(dev_);
}

void Video::upload(const uint32_t* px, int w, int h, int stride) {
  if (!tex_) return;
  D3D11_MAPPED_SUBRESOURCE m;
  if (FAILED(ctx_->Map(tex_, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) return;
  w = std::min(w, texW_);
  h = std::min(h, texH_);
  for (int y = 0; y < h; y++) {
    uint8_t* dst = (uint8_t*)m.pData + y * m.RowPitch;
    memcpy(dst, px + y * stride, w * 4);
  }
  ctx_->Unmap(tex_, 0);
}

void Video::drawPass(const VideoParams& p, int pass, const D3D11_VIEWPORT& vp) {
  D3D11_MAPPED_SUBRESOURCE m;
  if (SUCCEEDED(ctx_->Map(cb_, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) {
    Constants c{};
    c.srcRect[0] = 0;
    c.srcRect[1] = 0;
    c.srcRect[2] = (float)p.srcW / texW_;
    c.srcRect[3] = (float)p.srcH / texH_;
    c.texSize[0] = (float)texW_;
    c.texSize[1] = (float)texH_;
    c.texSize[2] = 1.0f / texW_;
    c.texSize[3] = 1.0f / texH_;
    c.outSize[0] = vp.Width;
    c.outSize[1] = vp.Height;
    c.outSize[2] = 1.0f / std::max(vp.Width, 1.0f);
    c.outSize[3] = 1.0f / std::max(vp.Height, 1.0f);
    c.f0[0] = (float)p.filter;
    c.f0[1] = p.scanlines;
    c.f0[2] = p.curve;
    c.f0[3] = (float)pass;
    c.f1[0] = p.brightness;
    c.f1[1] = p.contrast;
    c.f1[2] = p.saturation;
    c.f1[3] = p.gamma;
    c.f2[0] = (float)p.colorProfile;
    c.f2[1] = (float)p.colorblind;
    c.f2[2] = (float)p.srcW;
    c.f2[3] = (float)p.srcH;
    c.f3[0] = p.fade;
    c.f3[1] = (float)p.pillarbox;
    c.f3[2] = p.temperature;
    memcpy(m.pData, &c, sizeof(c));
    ctx_->Unmap(cb_, 0);
  }
  ctx_->RSSetViewports(1, &vp);
  ctx_->Draw(3, 0);
}

void Video::draw(const VideoParams& p) {
  if (!rtv_) return;
  float black[4] = {0, 0, 0, 1};
  ctx_->OMSetRenderTargets(1, &rtv_, nullptr);
  ctx_->ClearRenderTargetView(rtv_, black);
  ctx_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  ctx_->IASetInputLayout(nullptr);
  ctx_->VSSetShader(vs_, nullptr, 0);
  ctx_->PSSetShader(ps_, nullptr, 0);
  ctx_->PSSetShaderResources(0, 1, &srv_);
  ID3D11SamplerState* ss[2] = {sPoint_, sLinear_};
  ctx_->PSSetSamplers(0, 2, ss);
  ctx_->PSSetConstantBuffers(0, 1, &cb_);
  ctx_->RSSetState(rs_);

  bool fullCover = p.dest.left <= 0 && p.dest.top <= 0 && p.dest.right >= width_ && p.dest.bottom >= height_;
  if (p.pillarbox == 1 && !fullCover) {
    D3D11_VIEWPORT vp{0, 0, (float)width_, (float)height_, 0, 1};
    drawPass(p, 1, vp);
  }
  D3D11_VIEWPORT vp{(float)p.dest.left, (float)p.dest.top, (float)(p.dest.right - p.dest.left),
                    (float)(p.dest.bottom - p.dest.top), 0, 1};
  if (slang_.active()) {
    // (a slang preset: its own passes from the game picture into the same
    // rectangle; the colour settings are the preset's)
    slang_.draw(ctx_, tex_, std::min(p.srcW, texW_), std::min(p.srcH, texH_), rtv_, vp.TopLeftX, vp.TopLeftY,
                vp.Width, vp.Height, p.frame);
    return;
  }
  drawPass(p, 0, vp);
}

void Video::clear() {
  if (!rtv_) return;
  float black[4] = {0, 0, 0, 1};
  ctx_->OMSetRenderTargets(1, &rtv_, nullptr);
  ctx_->ClearRenderTargetView(rtv_, black);
}

void Video::beginUi() {
  if (!d2dCtx_ || !d2dTarget_) return;
  d2dCtx_->SetTarget(d2dTarget_);
  d2dCtx_->BeginDraw();
  d2dCtx_->SetTransform(D2D1::Matrix3x2F::Identity());
}

void Video::endUi() {
  if (!d2dCtx_ || !d2dTarget_) return;
  d2dCtx_->EndDraw();
}

bool Video::capture(std::vector<uint32_t>& out, int* w, int* h) {
  ID3D11Texture2D* bb = nullptr;
  if (FAILED(swap_->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb))) return false;
  D3D11_TEXTURE2D_DESC d;
  bb->GetDesc(&d);
  d.Usage = D3D11_USAGE_STAGING;
  d.BindFlags = 0;
  d.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  d.MiscFlags = 0;
  ID3D11Texture2D* st = nullptr;
  if (FAILED(dev_->CreateTexture2D(&d, nullptr, &st))) {
    bb->Release();
    return false;
  }
  ctx_->CopyResource(st, bb);
  D3D11_MAPPED_SUBRESOURCE m;
  bool ok = false;
  if (SUCCEEDED(ctx_->Map(st, 0, D3D11_MAP_READ, 0, &m))) {
    out.resize((size_t)d.Width * d.Height);
    for (UINT y = 0; y < d.Height; y++) memcpy(&out[y * d.Width], (uint8_t*)m.pData + y * m.RowPitch, d.Width * 4);
    for (auto& p : out) p &= 0xffffff;
    ctx_->Unmap(st, 0);
    *w = d.Width;
    *h = d.Height;
    ok = true;
  }
  st->Release();
  bb->Release();
  return ok;
}

void Video::present(int syncInterval) {
  if (!swap_) return;
  UINT flags = 0;
  if (syncInterval == 0 && tearing_ && !exclusive_) flags |= DXGI_PRESENT_ALLOW_TEARING;
  checkDevice(swap_->Present(syncInterval, flags));
}

bool Video::waitForVBlank() {
  if (!swap_) return false;
  if (!output_ && FAILED(swap_->GetContainingOutput(&output_))) output_ = nullptr;
  return output_ && SUCCEEDED(output_->WaitForVBlank());
}

void Video::refreshOutput() { safeRelease(output_); }
