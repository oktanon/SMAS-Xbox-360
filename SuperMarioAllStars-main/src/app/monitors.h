#pragma once
#include <windows.h>
#include <string>
#include <vector>

struct MonitorInfo {
  HMONITOR handle = nullptr;
  std::wstring device;  // \\.\DISPLAYn
  std::wstring name;    // friendly name
  RECT rect{};
  RECT work{};
  bool primary = false;
  int refresh = 60;    // rounded, for display
  double hz = 60.0;    // exact refresh rate (e.g. 59.94, 143.998)
};

std::vector<MonitorInfo> enumerateMonitors();
// Find by device name; falls back to primary.
MonitorInfo findMonitor(const std::vector<MonitorInfo>& list, const std::wstring& device);
// The monitor a window is (mostly) on.
MonitorInfo monitorForWindow(const std::vector<MonitorInfo>& list, HWND hwnd);
