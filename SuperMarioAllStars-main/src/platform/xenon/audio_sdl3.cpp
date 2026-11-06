// Sound on the Xbox 360: see audio_sdl3.h. The resampling and rate control are platform/android/audio_sdl.cpp's
// (SDL2's audio queue) moved to SDL3's audio stream: a stream bound to the default device, in its rate, 16-bit
// stereo in the CPU's byte order (SDL_AUDIO_S16, docs/ENDIANNESS.md §3.3), which SDL converts to the device's format.
#include "platform/xenon/audio_sdl3.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>

bool Audio::init(int latencyMs) {
  shutdown();
  latencyMs_ = latencyMs;
#ifdef SDL_HINT_AUDIO_XSDK_HARDWARE_THREAD
  // SDL's device thread starts on the hardware thread of the thread that opens the device, at priority +1
  // (SDL-xenon docs/README-xsdk.md): on the emulation's 0 it took ~5 ms of each frame and made frames late (run 018).
  // Hardware thread 5, as SDL-xenon's sdl_check puts it (run 001); the render threads are on 2 and 4.
  SDL_SetHint(SDL_HINT_AUDIO_XSDK_HARDWARE_THREAD, "5");
#endif
  if (!SDL_WasInit(SDL_INIT_AUDIO) && !SDL_InitSubSystem(SDL_INIT_AUDIO)) return false;
  // the device's own rate (48 kHz on the console) and chunk (256 frames), so SDL's stream only converts the format
  SDL_AudioSpec dev{};
  int frames = 0;
  if (SDL_GetAudioDeviceFormat(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &dev, &frames)) {
    if (dev.freq > 0) rate_ = dev.freq;
    if (frames > 0) deviceFrames_ = frames;
  }
  const SDL_AudioSpec spec{SDL_AUDIO_S16, 2, rate_};
  stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
  if (!stream_) return false;
  SDL_ResumeAudioStreamDevice(stream_);
  return true;
}

void Audio::shutdown() {
  if (stream_) SDL_DestroyAudioStream(stream_);
  stream_ = nullptr;
}

void Audio::setPaused(bool paused) {
  if (!stream_) return;
  if (paused) SDL_PauseAudioStreamDevice(stream_);
  else SDL_ResumeAudioStreamDevice(stream_);
}

int Audio::queuedFrames() const { return stream_ ? SDL_GetAudioStreamQueued(stream_) / 4 : 0; }

void Audio::flush() {
  if (stream_) SDL_ClearAudioStream(stream_);
  pos_ = 0;
}

void Audio::push(const int16_t* in, size_t frames, double inputRate) {
  if (!stream_ || frames == 0) return;
  int queued = queuedFrames();
  // (at least two of the device's own chunks queued: it takes a whole one at once)
  int target = std::max(rate_ * latencyMs_ / 1000, deviceFrames_ * 2);
  minQueued_ = std::min(minQueued_, queued);
  if (queued > target * 3) return;  // overrun: drop
  // Dynamic rate control: nudge the resampling ratio by up to 0.5%.
  double err = std::clamp((double)(queued - target) / (double)std::max(target, 1), -1.0, 1.0);
  double ratio = inputRate / rate_ * (1.0 + err * 0.005);
  if (queued == 0) {
    underruns_++;
    // Start or underrun: prime the queue with silence so the rate control
    // starts near its latency target instead of at zero.
    std::vector<int16_t> pad((size_t)(target / 2) * 2, 0);
    SDL_PutAudioStreamData(stream_, pad.data(), (int)(pad.size() * sizeof(int16_t)));
    ratio = inputRate / rate_ * 0.995;
  }
  out_.clear();
  float peak = 0;
  // Hermite interpolation across the input stream; hist_ holds the 4 most
  // recent input frames, pos_ is the fraction between hist_[1] and hist_[2].
  size_t i = 0;
  for (;;) {
    bool more = true;
    while (pos_ >= 1.0) {
      if (i >= frames) {
        more = false;
        break;
      }
      for (int k = 0; k < 3; k++) {
        hist_[k][0] = hist_[k + 1][0];
        hist_[k][1] = hist_[k + 1][1];
      }
      hist_[3][0] = in[i * 2];
      hist_[3][1] = in[i * 2 + 1];
      i++;
      pos_ -= 1.0;
    }
    if (!more) break;
    float t = (float)pos_;
    for (int c = 0; c < 2; c++) {
      float y0 = hist_[0][c], y1 = hist_[1][c], y2 = hist_[2][c], y3 = hist_[3][c];
      float c1 = 0.5f * (y2 - y0);
      float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
      float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
      float v = (((c3 * t + c2) * t + c1) * t + y1) * volume_;
      v = std::clamp(v, -32768.0f, 32767.0f);
      peak = std::max(peak, std::fabs(v) / 32768.0f);
      out_.push_back((int16_t)v);
    }
    pos_ += ratio;
  }
  level_ = level_ * 0.8f + peak * 0.2f;
  if (!out_.empty()) SDL_PutAudioStreamData(stream_, out_.data(), (int)(out_.size() * sizeof(int16_t)));
}
