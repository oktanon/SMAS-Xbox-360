// A small OS layer for the app: paths, files and folders, links. Paths are
// std::wstring everywhere (Windows' file APIs take them as they are; elsewhere
// they're UTF-8 when a file is opened). sys_win.cpp (Windows), sys_posix.cpp
// (Android).
#pragma once
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

std::wstring pathJoin(const std::wstring& dir, const std::wstring& name);
std::wstring pathDir(const std::wstring& path);   // all but the last part ("." if none)
std::wstring pathName(const std::wstring& path);  // the last part

FILE* openFile(const std::wstring& path, const char* mode);
bool readFile(const std::wstring& path, std::vector<uint8_t>& out, size_t maxBytes = 64u << 20);
bool readText(const std::wstring& path, std::string& out);
bool fileExists(const std::wstring& path);
// When it was last written, local time (false if it doesn't exist).
bool fileModified(const std::wstring& path, int* day, int* month, int* hour, int* minute);
void makeDir(const std::wstring& path);
// The names of the files in a folder (not folders); false if it can't be read.
bool listFiles(const std::wstring& dir, std::vector<std::wstring>& names);

// Local time now, strftime-formatted.
std::string timeText(const char* fmt);

void openUrl(const char* url);
