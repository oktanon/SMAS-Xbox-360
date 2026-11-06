#include "monitors.h"
#include <map>

static BOOL CALLBACK monEnum(HMONITOR h, HDC, LPRECT, LPARAM lp) {
  auto* list = (std::vector<MonitorInfo>*)lp;
  MONITORINFOEXW mi{};
  mi.cbSize = sizeof(mi);
  if (!GetMonitorInfoW(h, &mi)) return TRUE;
  MonitorInfo m;
  m.handle = h;
  m.device = mi.szDevice;
  m.rect = mi.rcMonitor;
  m.work = mi.rcWork;
  m.primary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
  DEVMODEW dm{};
  dm.dmSize = sizeof(dm);
  if (EnumDisplaySettingsW(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm) && dm.dmDisplayFrequency > 1)
    m.refresh = (int)dm.dmDisplayFrequency;
  // GDI rounds down fractional rates (59.94 -> 59); the exact value comes
  // from DisplayConfig below when available.
  m.hz = (m.refresh % 60 == 59) ? (m.refresh + 1) / 1.001 : m.refresh;
  list->push_back(m);
  return TRUE;
}

std::vector<MonitorInfo> enumerateMonitors() {
  std::vector<MonitorInfo> list;
  EnumDisplayMonitors(nullptr, nullptr, monEnum, (LPARAM)&list);

  // Friendly names and exact refresh rates through the DisplayConfig API
  std::map<std::wstring, std::wstring> names;
  std::map<std::wstring, double> rates;
  UINT32 np = 0, nm = 0;
  if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &np, &nm) == ERROR_SUCCESS) {
    std::vector<DISPLAYCONFIG_PATH_INFO> paths(np);
    std::vector<DISPLAYCONFIG_MODE_INFO> modes(nm);
    if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &np, paths.data(), &nm, modes.data(), nullptr) == ERROR_SUCCESS) {
      for (UINT32 i = 0; i < np; i++) {
        DISPLAYCONFIG_SOURCE_DEVICE_NAME src{};
        src.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
        src.header.size = sizeof(src);
        src.header.adapterId = paths[i].sourceInfo.adapterId;
        src.header.id = paths[i].sourceInfo.id;
        if (DisplayConfigGetDeviceInfo(&src.header) != ERROR_SUCCESS) continue;
        const DISPLAYCONFIG_RATIONAL& rr = paths[i].targetInfo.refreshRate;
        if (rr.Numerator && rr.Denominator) rates[src.viewGdiDeviceName] = (double)rr.Numerator / rr.Denominator;
        DISPLAYCONFIG_TARGET_DEVICE_NAME tgt{};
        tgt.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
        tgt.header.size = sizeof(tgt);
        tgt.header.adapterId = paths[i].targetInfo.adapterId;
        tgt.header.id = paths[i].targetInfo.id;
        if (DisplayConfigGetDeviceInfo(&tgt.header) != ERROR_SUCCESS) continue;
        if (tgt.monitorFriendlyDeviceName[0]) names[src.viewGdiDeviceName] = tgt.monitorFriendlyDeviceName;
      }
    }
  }
  int idx = 1;
  for (auto& m : list) {
    auto rt = rates.find(m.device);
    if (rt != rates.end() && rt->second > 20 && rt->second < 1000) m.hz = rt->second;
    auto it = names.find(m.device);
    std::wstring base = it != names.end() ? it->second : L"Display";
    int w = m.rect.right - m.rect.left, h = m.rect.bottom - m.rect.top;
    m.name = std::to_wstring(idx++) + L": " + base + L" (" + std::to_wstring(w) + L"x" + std::to_wstring(h) + L" @" +
             std::to_wstring(m.refresh) + L"Hz)" + (m.primary ? L" - Primary" : L"");
  }
  return list;
}

MonitorInfo findMonitor(const std::vector<MonitorInfo>& list, const std::wstring& device) {
  for (auto& m : list)
    if (!device.empty() && m.device == device) return m;
  for (auto& m : list)
    if (m.primary) return m;
  return list.empty() ? MonitorInfo() : list[0];
}

MonitorInfo monitorForWindow(const std::vector<MonitorInfo>& list, HWND hwnd) {
  HMONITOR h = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
  for (auto& m : list)
    if (m.handle == h) return m;
  return findMonitor(list, L"");
}
