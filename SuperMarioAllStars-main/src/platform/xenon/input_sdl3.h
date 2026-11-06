// Controllers on the Xbox 360 (hito 4): SDL3's gamepads (SDL-xenon reads up to four Xbox 360 controllers with
// XamInputGetState, docs/README-xsdk.md), the same model as Windows' app/input.h and Android's
// platform/android/input_sdl.h, for the app to use as is: buttons in the standard layout (0-15 = A B X Y LB RB Back
// Start LS RS Up Down Left Right LT RT, 16 = Guide), axes 0-3 = LX LY RX RY. There's no keyboard on the console: the
// key calls are kept for the app and do nothing but remember what the app tells them.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "app/config.h"

constexpr int kPadButtons = 48;

struct PadState {
  bool connected = false;
  bool xinput = false;
  std::wstring name;
  bool buttons[kPadButtons] = {};
  float axes[6] = {};
};

struct SDL_Gamepad;

class Input {
public:
  bool init();
  void shutdown();
  void poll();
  void onKey(int vk, bool down);
  void clearKeys();
  void rescanDevices() { rescan_ = true; }

  bool keyDown(int vk) const { return vk >= 0 && vk < 256 && (keys_[vk] || sticky_[vk] > 0); }
  static constexpr int kMaxPads = 4;  // (the console's)
  int padCount() const { return (int)pads_.size(); }
  const PadState* pad(int i) const { return (i >= 0 && i < (int)pads_.size()) ? &pads_[i] : nullptr; }
  bool anyPad() const;  // a controller is connected

  bool active(const Binding& b, int padIdx, float deadzone) const;
  bool capture(Binding* out, bool pads, int padIdx, float deadzone);
  void resetCapture();

  // Rumble on a pad, low and high motors 0..1, for `ms` (0 stops it). (The PC port doesn't use it yet.)
  void rumble(int padIdx, float low, float high, int ms);

private:
  bool keys_[256] = {};
  bool prevKeys_[256] = {};
  uint8_t sticky_[256] = {};
  std::vector<PadState> pads_;
  std::vector<PadState> prevPads_;
  std::vector<SDL_Gamepad*> ctl_;
  bool rescan_ = true;
  void scan();
};
