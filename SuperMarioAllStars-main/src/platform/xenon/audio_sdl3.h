// Sound on the Xbox 360 (hito 4): SDL3's audio stream (SDL-xenon feeds xsdk's audio driver: 6 channels of floats at
// 48 kHz, docs/README-xsdk.md), with the same resampling (32040 Hz -> the device's rate) and latency control as the
// Android and Windows outputs (platform/android/audio_sdl.*, app/audio.h): the same class, for the app to use as is.
#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

struct SDL_AudioStream;

class Audio {
public:
  bool init(int latencyMs);
  void shutdown();
  // Push interleaved stereo samples at the SNES rate.
  void push(const int16_t* samples, size_t frames, double inputRate);
  void setVolume(float v) { volume_ = v; }
  void setLatency(int ms) { latencyMs_ = ms; }
  void flush();
  void setPaused(bool paused);
  int queuedFrames() const;
  bool ok() const { return stream_ != nullptr; }
  float level() const { return level_; }
  int deviceFrames() const { return deviceFrames_; }
  int rate() const { return rate_; }
  // (for the log: times the queue ran dry, and its lowest level, since the last call)
  void takeStats(int* underruns, int* minQueued) {
    *underruns = underruns_;
    *minQueued = minQueued_ == 1 << 30 ? 0 : minQueued_;
    underruns_ = 0;
    minQueued_ = 1 << 30;
  }

private:
  SDL_AudioStream* stream_ = nullptr;
  int rate_ = 48000;
  int latencyMs_ = 64;
  int deviceFrames_ = 256;
  int underruns_ = 0, minQueued_ = 1 << 30;
  double pos_ = 0.0;
  int16_t hist_[4][2] = {};
  std::vector<int16_t> out_;
  float level_ = 0;
  float volume_ = 1.0f;
};
