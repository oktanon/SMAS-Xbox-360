// Controllers on the Xbox 360: see input_sdl3.h. platform/android/input_sdl.cpp (SDL2's GameController) moved to
// SDL3's Gamepad API: the same slots (a pad keeps the slot it first got; unplugged, it leaves it disconnected), the
// same button and axis numbers, the same bindings.
#include "platform/xenon/input_sdl3.h"

#include <SDL3/SDL.h>

#include <cstring>

#include "app/canvas.h"  // (widenUtf8)

bool Input::init() {
  if (!SDL_WasInit(SDL_INIT_GAMEPAD)) SDL_InitSubSystem(SDL_INIT_GAMEPAD);
  rescan_ = true;
  return true;
}

void Input::shutdown() {
  for (SDL_Gamepad*& c : ctl_)
    if (c) SDL_CloseGamepad(c);
  ctl_.clear();
  pads_.clear();
  clearKeys();
}

void Input::onKey(int vk, bool down) {
  if (vk <= 0 || vk >= 256) return;
  keys_[vk] = down;
  if (down) sticky_[vk] = 2;
}

void Input::clearKeys() {
  std::memset(keys_, 0, sizeof(keys_));
  std::memset(sticky_, 0, sizeof(sticky_));
}

bool Input::anyPad() const {
  for (const PadState& p : pads_)
    if (p.connected) return true;
  return false;
}

void Input::scan() {
  rescan_ = false;
  for (size_t i = 0; i < ctl_.size(); i++)
    if (ctl_[i] && !SDL_GamepadConnected(ctl_[i])) {
      SDL_CloseGamepad(ctl_[i]);
      ctl_[i] = nullptr;
      pads_[i].connected = false;
    }
  int count = 0;
  SDL_JoystickID* ids = SDL_GetGamepads(&count);
  for (int j = 0; ids && j < count; j++) {
    bool open = false;
    for (SDL_Gamepad* c : ctl_)
      if (c && SDL_GetGamepadID(c) == ids[j]) open = true;
    if (open) continue;
    SDL_Gamepad* c = SDL_OpenGamepad(ids[j]);
    if (!c) continue;
    size_t slot = ctl_.size();
    for (size_t i = 0; i < ctl_.size(); i++)
      if (!ctl_[i]) {
        slot = i;
        break;
      }
    if (slot == ctl_.size()) {
      if ((int)slot >= kMaxPads) {
        SDL_CloseGamepad(c);
        continue;
      }
      ctl_.push_back(nullptr);
      pads_.push_back(PadState());
    }
    ctl_[slot] = c;
    PadState& p = pads_[slot];
    p = PadState();
    p.connected = true;
    p.xinput = true;
    const char* n = SDL_GetGamepadName(c);
    p.name = widenUtf8(n ? n : "Controller");
  }
  SDL_free(ids);
}

void Input::poll() {
  for (uint8_t& s : sticky_)
    if (s) s--;
  if (rescan_) scan();
  // (SDL3 names the face buttons by position: south = A on an Xbox pad)
  static const SDL_GamepadButton kMap[14] = {
      SDL_GAMEPAD_BUTTON_SOUTH,      SDL_GAMEPAD_BUTTON_EAST,           SDL_GAMEPAD_BUTTON_WEST,
      SDL_GAMEPAD_BUTTON_NORTH,      SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,  SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,
      SDL_GAMEPAD_BUTTON_BACK,       SDL_GAMEPAD_BUTTON_START,          SDL_GAMEPAD_BUTTON_LEFT_STICK,
      SDL_GAMEPAD_BUTTON_RIGHT_STICK, SDL_GAMEPAD_BUTTON_DPAD_UP,       SDL_GAMEPAD_BUTTON_DPAD_DOWN,
      SDL_GAMEPAD_BUTTON_DPAD_LEFT,  SDL_GAMEPAD_BUTTON_DPAD_RIGHT,
  };
  for (size_t i = 0; i < ctl_.size(); i++) {
    SDL_Gamepad* c = ctl_[i];
    PadState& p = pads_[i];
    if (!c) {
      std::memset(p.buttons, 0, sizeof(p.buttons));
      std::memset(p.axes, 0, sizeof(p.axes));
      continue;
    }
    for (int b = 0; b < 14; b++) p.buttons[b] = SDL_GetGamepadButton(c, kMap[b]);
    p.buttons[14] = SDL_GetGamepadAxis(c, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > 16000;
    p.buttons[15] = SDL_GetGamepadAxis(c, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > 16000;
    p.buttons[16] = SDL_GetGamepadButton(c, SDL_GAMEPAD_BUTTON_GUIDE);
    p.axes[0] = SDL_GetGamepadAxis(c, SDL_GAMEPAD_AXIS_LEFTX) / 32767.0f;
    p.axes[1] = SDL_GetGamepadAxis(c, SDL_GAMEPAD_AXIS_LEFTY) / 32767.0f;
    p.axes[2] = SDL_GetGamepadAxis(c, SDL_GAMEPAD_AXIS_RIGHTX) / 32767.0f;
    p.axes[3] = SDL_GetGamepadAxis(c, SDL_GAMEPAD_AXIS_RIGHTY) / 32767.0f;
    p.axes[4] = SDL_GetGamepadAxis(c, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) / 32767.0f;
    p.axes[5] = SDL_GetGamepadAxis(c, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) / 32767.0f;
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
      return p && p->buttons[10 + (b.code & 3)];
    }
    default: return false;
  }
}

void Input::resetCapture() {
  std::memcpy(prevKeys_, keys_, sizeof(keys_));
  prevPads_ = pads_;
}

bool Input::capture(Binding* out, bool wantPads, int padIdx, float) {
  bool got = false;
  if (!wantPads) {
    for (int k = 1; k < 256 && !got; k++)
      if (keyDown(k) && !prevKeys_[k]) {
        out->type = Binding::Key;
        out->code = (uint16_t)k;
        got = true;
      }
  } else if (const PadState* p = pad(padIdx)) {
    const PadState* q = padIdx < (int)prevPads_.size() ? &prevPads_[padIdx] : nullptr;
    for (int i = 0; i < kPadButtons && !got; i++)
      if (p->buttons[i] && !(q && q->buttons[i])) {
        out->type = Binding::PadButton;
        out->code = (uint16_t)i;
        got = true;
      }
    for (int a = 0; a < 4 && !got; a++) {  // (the sticks; the triggers bind as buttons 14 and 15)
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
  resetCapture();
  return got;
}

void Input::rumble(int padIdx, float low, float high, int ms) {
  if (padIdx < 0 || padIdx >= (int)ctl_.size() || !ctl_[padIdx]) return;
  auto u16 = [](float v) { return (Uint16)(v <= 0 ? 0 : v >= 1 ? 0xffff : v * 65535.0f); };
  SDL_RumbleGamepad(ctl_[padIdx], u16(low), u16(high), (Uint32)(ms < 0 ? 0 : ms));
}
