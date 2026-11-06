// sys.h on Windows.
#include "app/sys.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#include <ctime>

std::wstring pathJoin(const std::wstring& dir, const std::wstring& name) {
  if (dir.empty()) return name;
  wchar_t last = dir.back();
  return last == L'\\' || last == L'/' ? dir + name : dir + L"\\" + name;
}

std::wstring pathDir(const std::wstring& path) {
  size_t s = path.find_last_of(L"\\/");
  return s == std::wstring::npos ? L"." : path.substr(0, s);
}

std::wstring pathName(const std::wstring& path) {
  size_t s = path.find_last_of(L"\\/");
  return s == std::wstring::npos ? path : path.substr(s + 1);
}

FILE* openFile(const std::wstring& path, const char* mode) {
  std::wstring m(mode, mode + strlen(mode));
  return _wfopen(path.c_str(), m.c_str());
}

bool readFile(const std::wstring& path, std::vector<uint8_t>& out, size_t maxBytes) {
  FILE* f = openFile(path, "rb");
  if (!f) return false;
  _fseeki64(f, 0, SEEK_END);
  long long n = _ftelli64(f);
  _fseeki64(f, 0, SEEK_SET);
  if (n < 0 || (unsigned long long)n > maxBytes) {
    fclose(f);
    return false;
  }
  out.resize((size_t)n);
  size_t r = n ? fread(out.data(), 1, (size_t)n, f) : 0;
  fclose(f);
  return r == (size_t)n;
}

bool readText(const std::wstring& path, std::string& out) {
  std::vector<uint8_t> d;
  if (!readFile(path, d)) return false;
  out.assign(d.begin(), d.end());
  return true;
}

bool fileExists(const std::wstring& path) { return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES; }

bool fileModified(const std::wstring& path, int* day, int* month, int* hour, int* minute) {
  WIN32_FILE_ATTRIBUTE_DATA fa;
  if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fa)) return false;
  FILETIME lt;
  SYSTEMTIME st;
  FileTimeToLocalFileTime(&fa.ftLastWriteTime, &lt);
  FileTimeToSystemTime(&lt, &st);
  *day = st.wDay;
  *month = st.wMonth;
  *hour = st.wHour;
  *minute = st.wMinute;
  return true;
}

void makeDir(const std::wstring& path) { CreateDirectoryW(path.c_str(), nullptr); }

bool listFiles(const std::wstring& dir, std::vector<std::wstring>& names) {
  WIN32_FIND_DATAW fd;
  HANDLE h = FindFirstFileW(pathJoin(dir, L"*").c_str(), &fd);
  if (h == INVALID_HANDLE_VALUE) return false;
  do {
    if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) names.push_back(fd.cFileName);
  } while (FindNextFileW(h, &fd));
  FindClose(h);
  return true;
}

std::string timeText(const char* fmt) {
  time_t t = time(nullptr);
  tm lt;
  localtime_s(&lt, &t);
  char buf[64];
  strftime(buf, sizeof buf, fmt, &lt);
  return buf;
}

void openUrl(const char* url) {
  std::wstring w(url, url + strlen(url));
  ShellExecuteW(nullptr, L"open", w.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}
