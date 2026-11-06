// Keyboard + joypad input (XInput and DirectInput), normalized to a standard
// gamepad layout: buttons 0-15 = A B X Y LB RB Back Start LS RS Up Down Left
// Right LT RT, 16+ = raw DirectInput buttons. Axes 0-3 = LX LY RX RY.
#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "config.h"

struct IDirectInput8W;
struct IDirectInputDevice8W;

constexpr int kPadButtons = 48;

struct PadState {
  bool connected = false;
  bool xinput = false;
  std::wstring name;
  bool buttons[kPadButtons] = {};
  float axes[6] = {};
};

class Input {
public:
  bool init(HWND hwnd);
  void shutdown();
  void poll();
  void onKey(int vk, bool down);
  void clearKeys();
  void rescanDevices() { rescan_ = true; }

  bool keyDown(int vk) const { return vk >= 0 && vk < 256 && keys_[vk]; }
  // Pads keep the slot they first got (a player's choice of "Pad 2" stays
  // with that controller); an unplugged pad leaves its slot disconnected and
  // takes it back when it returns.
  static constexpr int kMaxPads = 8;
  int padCount() const { return (int)pads_.size(); }
  const PadState* pad(int i) const { return (i >= 0 && i < (int)pads_.size()) ? &pads_[i] : nullptr; }

  bool active(const Binding& b, int padIdx, float deadzone) const;
  // Detect a newly pressed control since the last call (for rebinding).
  bool capture(Binding* out, bool pads, int padIdx, float deadzone);
  void resetCapture();

  // Rumble is not used; placeholder for API completeness.
private:
  HWND hwnd_ = nullptr;
  bool keys_[256] = {};
  bool prevKeys_[256] = {};
  std::vector<PadState> pads_;
  std::vector<std::wstring> padKeys_;  // device identity per slot
  std::vector<PadState> prevPads_;
  void assignSlots(std::vector<std::pair<std::wstring, PadState>>& found);
  bool rescan_ = true;
  int xinputProbeTimer_ = 0;
  bool xinputConnected_[4] = {true, true, true, true};  // probed on the first poll

  IDirectInput8W* di_ = nullptr;
  struct DiPad {
    IDirectInputDevice8W* dev = nullptr;
    std::wstring name;
    std::wstring key;  // instance GUID
    uint16_t vid = 0, pid = 0;
    int layout = 0;  // 0 generic SNES-style, 1 PlayStation, 2 Switch Pro
  };
  std::vector<DiPad> diPads_;

  void enumerateDirectInput();
  void releaseDirectInput();
  static BOOL CALLBACK enumCallback(const void* inst, void* ctx);
};
