#include "audio.h"
#include <windows.h>
#include <xaudio2.h>
#include <algorithm>
#include <cmath>

bool Audio::init(int latencyMs) {
  latencyMs_ = latencyMs;
  if (FAILED(XAudio2Create(&xa_, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
    xa_ = nullptr;
    return false;
  }
  if (FAILED(xa_->CreateMasteringVoice(&master_))) {
    shutdown();
    return false;
  }
  XAUDIO2_VOICE_DETAILS det{};
  master_->GetVoiceDetails(&det);
  rate_ = det.InputSampleRate ? (int)det.InputSampleRate : 48000;
  if (rate_ < 22050 || rate_ > 192000) rate_ = 48000;
  WAVEFORMATEX wf{};
  wf.wFormatTag = WAVE_FORMAT_PCM;
  wf.nChannels = 2;
  wf.nSamplesPerSec = rate_;
  wf.wBitsPerSample = 16;
  wf.nBlockAlign = 4;
  wf.nAvgBytesPerSec = rate_ * 4;
  if (FAILED(xa_->CreateSourceVoice(&voice_, &wf, 0, 2.0f))) {
    shutdown();
    return false;
  }
  voice_->Start();
  return true;
}

void Audio::shutdown() {
  if (voice_) {
    voice_->Stop();
    voice_->FlushSourceBuffers();
    voice_->DestroyVoice();
    voice_ = nullptr;
  }
  if (master_) {
    master_->DestroyVoice();
    master_ = nullptr;
  }
  if (xa_) {
    xa_->Release();
    xa_ = nullptr;
  }
}

void Audio::setVolume(float v) {
  volume_ = v;
  if (voice_) voice_->SetVolume(v);
}

int Audio::queuedFrames() const {
  if (!voice_) return 0;
  XAUDIO2_VOICE_STATE st;
  voice_->GetState(&st, 0);
  if (st.BuffersQueued == 0) return 0;
  int64_t q = (int64_t)submitted_ - (int64_t)st.SamplesPlayed;
  return q < 0 ? 0 : (int)q;
}

void Audio::flush() {
  if (voice_) {
    voice_->FlushSourceBuffers();
    XAUDIO2_VOICE_STATE st;
    voice_->GetState(&st, 0);
    submitted_ = st.SamplesPlayed;
  }
  pos_ = 0;
}

void Audio::push(const int16_t* in, size_t frames, double inputRate) {
  if (tap && frames) tap(tapCtx, in, frames);
  if (!voice_ || frames == 0) return;
  XAUDIO2_VOICE_STATE st;
  voice_->GetState(&st, 0);
  if (st.BuffersQueued >= kBuffers - 1) return;  // overrun: drop

  int queued = queuedFrames();
  int target = rate_ * latencyMs_ / 1000;
  // Dynamic rate control: nudge the resampling ratio by up to 0.5%.
  double err = (double)(queued - target) / (double)std::max(target, 1);
  err = std::clamp(err, -1.0, 1.0);
  double ratio = inputRate / rate_ * (1.0 + err * 0.005);
  if (queued == 0) ratio = inputRate / rate_ * 0.995;

  out_.clear();
  float peak = 0;
  // Hermite interpolation across the input stream; hist_ holds the 4 most
  // recent input frames, pos_ is the fraction between hist_[1] and hist_[2].
  size_t i = 0;
  while (true) {
    while (pos_ >= 1.0) {
      if (i >= frames) goto done;
      for (int k = 0; k < 3; k++) {
        hist_[k][0] = hist_[k + 1][0];
        hist_[k][1] = hist_[k + 1][1];
      }
      hist_[3][0] = in[i * 2];
      hist_[3][1] = in[i * 2 + 1];
      i++;
      pos_ -= 1.0;
    }
    {
      float t = (float)pos_;
      for (int c = 0; c < 2; c++) {
        float y0 = hist_[0][c], y1 = hist_[1][c], y2 = hist_[2][c], y3 = hist_[3][c];
        float c0 = y1;
        float c1 = 0.5f * (y2 - y0);
        float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        float v = ((c3 * t + c2) * t + c1) * t + c0;
        v = std::clamp(v, -32768.0f, 32767.0f);
        peak = std::max(peak, std::fabs(v) / 32768.0f);
        out_.push_back((int16_t)v);
      }
      pos_ += ratio;
    }
  }
done:
  level_ = level_ * 0.8f + peak * volume_ * 0.2f;
  if (out_.empty()) return;
  XAUDIO2_BUFFER xb{};
  if (st.BuffersQueued == 0) {
    // Start or underrun: resync, and prime the queue with silence so the rate
    // control starts near its latency target instead of at zero.
    submitted_ = st.SamplesPlayed;
    std::vector<int16_t>& pad = bufs_[nextBuf_];
    pad.assign((size_t)(rate_ * latencyMs_ / 1000 / 2) * 2, 0);
    xb.AudioBytes = (UINT32)(pad.size() * sizeof(int16_t));
    xb.pAudioData = (const BYTE*)pad.data();
    voice_->SubmitSourceBuffer(&xb);
    submitted_ += pad.size() / 2;
    nextBuf_ = (nextBuf_ + 1) % kBuffers;
  }
  std::vector<int16_t>& b = bufs_[nextBuf_];
  b = out_;
  xb.AudioBytes = (UINT32)(b.size() * sizeof(int16_t));
  xb.pAudioData = (const BYTE*)b.data();
  voice_->SubmitSourceBuffer(&xb);
  submitted_ += b.size() / 2;
  nextBuf_ = (nextBuf_ + 1) % kBuffers;
}
