#define DIRECTINPUT_VERSION 0x0800
#include "input.h"
#include <algorithm>
#include <dinput.h>
#include <xinput.h>
#include <wbemidl.h>
#include <cmath>
#include <cstring>

namespace {

enum {
  kA = 0, kB, kX, kY, kLB, kRB, kBack, kStart, kLS, kRS, kUp, kDown, kLeft, kRight, kLT, kRT
};

// Collect VID/PID pairs of XInput devices (their raw-input path contains "IG_")
// so the DirectInput enumeration can skip them.
std::vector<DWORD> xinputVidPids() {
  std::vector<DWORD> out;
  UINT n = 0;
  if (GetRawInputDeviceList(nullptr, &n, sizeof(RAWINPUTDEVICELIST)) != 0 || n == 0) return out;
  std::vector<RAWINPUTDEVICELIST> list(n);
  if (GetRawInputDeviceList(list.data(), &n, sizeof(RAWINPUTDEVICELIST)) == (UINT)-1) return out;
  for (UINT i = 0; i < n; i++) {
    if (list[i].dwType != RIM_TYPEHID) continue;
    wchar_t name[512];
    UINT sz = 512;
    if (GetRawInputDeviceInfoW(list[i].hDevice, RIDI_DEVICENAME, name, &sz) == (UINT)-1) continue;
    if (!wcsstr(name, L"IG_") && !wcsstr(name, L"ig_")) continue;
    RID_DEVICE_INFO info{};
    info.cbSize = sizeof(info);
    sz = sizeof(info);
    if (GetRawInputDeviceInfoW(list[i].hDevice, RIDI_DEVICEINFO, &info, &sz) == (UINT)-1) continue;
    out.push_back(MAKELONG(info.hid.dwVendorId, info.hid.dwProductId));
  }
  return out;
}

struct EnumCtx {
  IDirectInput8W* di;
  std::vector<DWORD> xinputIds;
  std::vector<std::pair<GUID, std::pair<std::wstring, DWORD>>> found;
};

BOOL CALLBACK diEnum(LPCDIDEVICEINSTANCEW inst, LPVOID ref) {
  EnumCtx* ctx = (EnumCtx*)ref;
  DWORD vidpid = inst->guidProduct.Data1;
  for (DWORD x : ctx->xinputIds)
    if (x == vidpid) return DIENUM_CONTINUE;
  ctx->found.push_back({inst->guidInstance, {inst->tszProductName, vidpid}});
  return DIENUM_CONTINUE;
}

BOOL CALLBACK diAxisEnum(LPCDIDEVICEOBJECTINSTANCEW obj, LPVOID ref) {
  IDirectInputDevice8W* dev = (IDirectInputDevice8W*)ref;
  DIPROPRANGE r{};
  r.diph.dwSize = sizeof(DIPROPRANGE);
  r.diph.dwHeaderSize = sizeof(DIPROPHEADER);
  r.diph.dwHow = DIPH_BYID;
  r.diph.dwObj = obj->dwType;
  r.lMin = -32768;
  r.lMax = 32767;
  dev->SetProperty(DIPROP_RANGE, &r.diph);
  return DIENUM_CONTINUE;
}

float normAxis(LONG v) {
  float f = v / 32767.0f;
  if (f < -1) f = -1;
  if (f > 1) f = 1;
  return f;
}

float normXAxis(SHORT v) { return v < 0 ? v / 32768.0f : v / 32767.0f; }

}  // namespace

bool Input::init(HWND hwnd) {
  hwnd_ = hwnd;
  HRESULT hr = DirectInput8Create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8W, (void**)&di_,
                                  nullptr);
  if (FAILED(hr)) di_ = nullptr;
  rescan_ = true;
  return true;
}

void Input::releaseDirectInput() {
  for (auto& p : diPads_) {
    if (p.dev) {
      p.dev->Unacquire();
      p.dev->Release();
    }
  }
  diPads_.clear();
}

void Input::shutdown() {
  releaseDirectInput();
  if (di_) di_->Release();
  di_ = nullptr;
}

void Input::enumerateDirectInput() {
  releaseDirectInput();
  if (!di_) return;
  EnumCtx ctx;
  ctx.di = di_;
  ctx.xinputIds = xinputVidPids();
  di_->EnumDevices(DI8DEVCLASS_GAMECTRL, diEnum, &ctx, DIEDFL_ATTACHEDONLY);
  for (auto& f : ctx.found) {
    IDirectInputDevice8W* dev = nullptr;
    if (FAILED(di_->CreateDevice(f.first, &dev, nullptr))) continue;
    if (FAILED(dev->SetDataFormat(&c_dfDIJoystick2))) {
      dev->Release();
      continue;
    }
    dev->SetCooperativeLevel(hwnd_, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
    dev->EnumObjects(diAxisEnum, dev, DIDFT_AXIS);
    dev->Acquire();
    DiPad p;
    p.dev = dev;
    p.name = f.second.first;
    wchar_t guid[64] = L"";
    StringFromGUID2(f.first, guid, 64);
    p.key = std::wstring(L"di:") + guid;
    p.vid = LOWORD(f.second.second);
    p.pid = HIWORD(f.second.second);
    if (p.vid == 0x054c) p.layout = 1;  // Sony
    else if (p.vid == 0x057e && (p.pid == 0x2009 || p.pid == 0x2006 || p.pid == 0x2007)) p.layout = 2;
    else p.layout = 0;
    diPads_.push_back(p);
  }
}

void Input::onKey(int vk, bool down) {
  if (vk >= 0 && vk < 256) keys_[vk] = down;
}

void Input::clearKeys() { std::memset(keys_, 0, sizeof(keys_)); }

void Input::poll() {
  // Hot-plugged pads are picked up via WM_DEVICECHANGE -> rescanDevices().
  // (DirectInput enumeration blocks for ~100 ms, so it must not run periodically.)
  bool probeXInput = false;
  if (rescan_) {
    rescan_ = false;
    enumerateDirectInput();
    probeXInput = true;
  }
  // XInputGetState on an empty slot is slow; probe empty slots only now and then.
  if (++xinputProbeTimer_ >= 120) {
    xinputProbeTimer_ = 0;
    probeXInput = true;
  }
  // Modifier keys: Windows reports generic VK_SHIFT etc.; derive sides.
  keys_[VK_LSHIFT] = (GetKeyState(VK_LSHIFT) & 0x8000) && GetForegroundWindow() == hwnd_;
  keys_[VK_RSHIFT] = (GetKeyState(VK_RSHIFT) & 0x8000) && GetForegroundWindow() == hwnd_;
  keys_[VK_LCONTROL] = (GetKeyState(VK_LCONTROL) & 0x8000) && GetForegroundWindow() == hwnd_;
  keys_[VK_RCONTROL] = (GetKeyState(VK_RCONTROL) & 0x8000) && GetForegroundWindow() == hwnd_;

  std::vector<std::pair<std::wstring, PadState>> next;
  for (DWORD i = 0; i < XUSER_MAX_COUNT; i++) {
    XINPUT_STATE st{};
    if (!xinputConnected_[i] && !probeXInput) continue;
    xinputConnected_[i] = XInputGetState(i, &st) == ERROR_SUCCESS;
    if (!xinputConnected_[i]) continue;
    PadState p;
    p.connected = true;
    p.xinput = true;
    p.name = L"Xbox Controller " + std::to_wstring(i + 1);
    WORD b = st.Gamepad.wButtons;
    p.buttons[kA] = b & XINPUT_GAMEPAD_A;
    p.buttons[kB] = b & XINPUT_GAMEPAD_B;
    p.buttons[kX] = b & XINPUT_GAMEPAD_X;
    p.buttons[kY] = b & XINPUT_GAMEPAD_Y;
    p.buttons[kLB] = b & XINPUT_GAMEPAD_LEFT_SHOULDER;
    p.buttons[kRB] = b & XINPUT_GAMEPAD_RIGHT_SHOULDER;
    p.buttons[kBack] = b & XINPUT_GAMEPAD_BACK;
    p.buttons[kStart] = b & XINPUT_GAMEPAD_START;
    p.buttons[kLS] = b & XINPUT_GAMEPAD_LEFT_THUMB;
    p.buttons[kRS] = b & XINPUT_GAMEPAD_RIGHT_THUMB;
    p.buttons[kUp] = b & XINPUT_GAMEPAD_DPAD_UP;
    p.buttons[kDown] = b & XINPUT_GAMEPAD_DPAD_DOWN;
    p.buttons[kLeft] = b & XINPUT_GAMEPAD_DPAD_LEFT;
    p.buttons[kRight] = b & XINPUT_GAMEPAD_DPAD_RIGHT;
    p.buttons[kLT] = st.Gamepad.bLeftTrigger > 100;
    p.buttons[kRT] = st.Gamepad.bRightTrigger > 100;
    p.axes[0] = normXAxis(st.Gamepad.sThumbLX);
    p.axes[1] = -normXAxis(st.Gamepad.sThumbLY);  // down = positive
    p.axes[2] = normXAxis(st.Gamepad.sThumbRX);
    p.axes[3] = -normXAxis(st.Gamepad.sThumbRY);
    next.push_back({L"xinput:" + std::to_wstring(i), p});
  }
  for (auto& d : diPads_) {
    if (FAILED(d.dev->Poll())) {
      d.dev->Acquire();
      if (FAILED(d.dev->Poll())) continue;
    }
    DIJOYSTATE2 js{};
    if (FAILED(d.dev->GetDeviceState(sizeof(js), &js))) continue;
    PadState p;
    p.connected = true;
    p.name = d.name;
    for (int i = 0; i < 32; i++) p.buttons[16 + i] = js.rgbButtons[i] & 0x80;
    auto raw = [&](int i) { return (js.rgbButtons[i] & 0x80) != 0; };
    if (d.layout == 1) {  // PlayStation
      p.buttons[kX] = raw(0);
      p.buttons[kA] = raw(1);
      p.buttons[kB] = raw(2);
      p.buttons[kY] = raw(3);
      p.buttons[kLB] = raw(4);
      p.buttons[kRB] = raw(5);
      p.buttons[kLT] = raw(6);
      p.buttons[kRT] = raw(7);
      p.buttons[kBack] = raw(8);
      p.buttons[kStart] = raw(9);
      p.buttons[kLS] = raw(10);
      p.buttons[kRS] = raw(11);
      p.axes[2] = normAxis(js.lZ);
      p.axes[3] = normAxis(js.lRz);
    } else if (d.layout == 2) {  // Switch Pro
      p.buttons[kA] = raw(0);
      p.buttons[kB] = raw(1);
      p.buttons[kX] = raw(2);
      p.buttons[kY] = raw(3);
      p.buttons[kLB] = raw(4);
      p.buttons[kRB] = raw(5);
      p.buttons[kLT] = raw(6);
      p.buttons[kRT] = raw(7);
      p.buttons[kBack] = raw(8);
      p.buttons[kStart] = raw(9);
      p.buttons[kLS] = raw(10);
      p.buttons[kRS] = raw(11);
      p.axes[2] = normAxis(js.lRx);
      p.axes[3] = normAxis(js.lRy);
    } else {  // generic / SNES-style USB pad
      p.buttons[kB] = raw(0);
      p.buttons[kA] = raw(1);
      p.buttons[kY] = raw(2);
      p.buttons[kX] = raw(3);
      p.buttons[kLB] = raw(4);
      p.buttons[kRB] = raw(5);
      p.buttons[kBack] = raw(6);
      p.buttons[kStart] = raw(7);
      p.buttons[kLS] = raw(8);
      p.buttons[kRS] = raw(9);
      p.axes[2] = normAxis(js.lZ);
      p.axes[3] = normAxis(js.lRz);
    }
    p.axes[0] = normAxis(js.lX);
    p.axes[1] = normAxis(js.lY);
    p.axes[4] = normAxis(js.lRx);
    p.axes[5] = normAxis(js.lRy);
    DWORD pov = js.rgdwPOV[0];
    if (LOWORD(pov) != 0xffff) {
      // 0 = up, 9000 = right, 18000 = down, 27000 = left (hundredths of degrees)
      p.buttons[kUp] = pov >= 31500 || pov <= 4500;
      p.buttons[kRight] = pov >= 4500 && pov <= 13500;
      p.buttons[kDown] = pov >= 13500 && pov <= 22500;
      p.buttons[kLeft] = pov >= 22500 && pov <= 31500;
    }
    next.push_back({d.key, p});
  }
  assignSlots(next);
}

void Input::assignSlots(std::vector<std::pair<std::wstring, PadState>>& found) {
  std::vector<bool> seen(pads_.size(), false);
  auto place = [&](size_t slot, std::pair<std::wstring, PadState>& f) {
    padKeys_[slot] = f.first;
    pads_[slot] = f.second;
    seen[slot] = true;
  };
  std::vector<std::pair<std::wstring, PadState>*> fresh;
  for (auto& f : found) {  // returning pads first, into their own slots
    auto it = std::find(padKeys_.begin(), padKeys_.end(), f.first);
    if (it != padKeys_.end()) place(it - padKeys_.begin(), f);
    else fresh.push_back(&f);
  }
  for (auto* f : fresh) {  // new pads: a slot left by an unplugged pad, else a new one
    size_t slot = 0;
    while (slot < pads_.size() && (seen[slot] || pads_[slot].connected)) slot++;
    if (slot == pads_.size()) {
      if ((int)pads_.size() >= kMaxPads) continue;
      pads_.emplace_back();
      padKeys_.emplace_back();
      seen.push_back(false);
    }
    place(slot, *f);
  }
  for (size_t i = 0; i < pads_.size(); i++) {
    if (seen[i]) continue;
    PadState gone;  // unplugged: no input, keep the name for the menu
    gone.name = pads_[i].name;
    gone.xinput = pads_[i].xinput;
    pads_[i] = gone;
  }
}

bool Input::active(const Binding& b, int padIdx, float dz) const {
  switch (b.type) {
    case Binding::Key: return keyDown(b.code);
    case Binding::PadButton: {
      const PadState* p = pad(padIdx);
      return p && b.code < kPadButtons && p->buttons[b.code];
    }
    case Binding::PadAxisPos: {
      const PadState* p = pad(padIdx);
      return p && b.code < 6 && p->axes[b.code] > dz;
    }
    case Binding::PadAxisNeg: {
      const PadState* p = pad(padIdx);
      return p && b.code < 6 && p->axes[b.code] < -dz;
    }
    case Binding::PadHat: {
      const PadState* p = pad(padIdx);
      return p && p->buttons[kUp + (b.code & 3)];
    }
    default: return false;
  }
}

void Input::resetCapture() {
  std::memcpy(prevKeys_, keys_, sizeof(keys_));
  prevPads_ = pads_;
}

bool Input::capture(Binding* out, bool wantPads, int padIdx, float dz) {
  bool got = false;
  if (!wantPads) {
    for (int k = 1; k < 256; k++) {
      if (k == VK_SHIFT || k == VK_CONTROL || k == VK_MENU) continue;
      if (keys_[k] && !prevKeys_[k]) {
        out->type = Binding::Key;
        out->code = (uint16_t)k;
        got = true;
        break;
      }
    }
  } else {
    const PadState* p = pad(padIdx);
    const PadState* q = (padIdx >= 0 && padIdx < (int)prevPads_.size()) ? &prevPads_[padIdx] : nullptr;
    if (p) {
      for (int i = 0; i < kPadButtons && !got; i++) {
        if (p->buttons[i] && !(q && q->buttons[i])) {
          out->type = Binding::PadButton;
          out->code = (uint16_t)i;
          got = true;
        }
      }
      for (int a = 0; a < 6 && !got; a++) {
        float prev = q ? q->axes[a] : 0;
        if (p->axes[a] > 0.6f && prev <= 0.6f) {
          out->type = Binding::PadAxisPos;
          out->code = (uint16_t)a;
          got = true;
        } else if (p->axes[a] < -0.6f && prev >= -0.6f) {
          out->type = Binding::PadAxisNeg;
          out->code = (uint16_t)a;
          got = true;
        }
      }
    }
  }
  std::memcpy(prevKeys_, keys_, sizeof(keys_));
  prevPads_ = pads_;
  (void)dz;
  return got;
}
