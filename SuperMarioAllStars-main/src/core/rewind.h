// Rewind buffer: the recent past as a chain of save states. Only the newest
// is kept whole; each older one is stored as its difference from the next
// (XOR, zero runs packed), a few KB instead of ~270 KB.
#pragma once
#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>
#ifdef SMAS_XENON
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#endif

class Rewind {
public:
  // At most `maxBytes` of differences and `maxSnapshots` states are kept;
  // the oldest go first.
  explicit Rewind(size_t maxBytes = 96u << 20, size_t maxSnapshots = 1800)
      : maxBytes_(maxBytes), maxSnapshots_(maxSnapshots) {}

  void clear();
  // Records `state` as the newest snapshot.
  void push(const std::vector<uint8_t>& state);
#ifdef SMAS_XENON
  // The Xbox 360: a snapshot's difference worked out on a thread of its own (5.8 ms a snapshot on the console's
  // main thread, docs/PERFORMANCE.md); every other call waits for it first. snapshots() doesn't wait.
  ~Rewind();
  void push(std::vector<uint8_t>&& state);
  const std::vector<uint8_t>& newest();
  size_t snapshots() const { return count_.load(std::memory_order_acquire); }
  static void (*threadStart)();  // (the thread placed: app_xenon.cpp, hardware thread 1)
#endif
  // Steps back one snapshot: drops the newest and puts the one before it in
  // `out`. False when there is nothing older.
  bool stepBack(std::vector<uint8_t>* out);
#ifndef SMAS_XENON
  // The newest snapshot (empty if none).
  const std::vector<uint8_t>& newest() const { return cur_; }
  size_t snapshots() const { return cur_.empty() ? 0 : deltas_.size() + 1; }
#endif
  size_t bytes() const { return bytes_; }

  // (Exposed for tests.)
  static std::vector<uint8_t> encode(const std::vector<uint8_t>& older, const std::vector<uint8_t>& newer);
  static bool decode(const std::vector<uint8_t>& delta, const std::vector<uint8_t>& newer,
                     std::vector<uint8_t>* older);

private:
  size_t maxBytes_, maxSnapshots_;
  std::vector<uint8_t> cur_;                 // newest snapshot, whole
  std::deque<std::vector<uint8_t>> deltas_;  // deltas_[i]: snapshot i from snapshot i + 1
  size_t bytes_ = 0;
#ifdef SMAS_XENON
  void pushNow(const std::vector<uint8_t>& state);
  void wait();
  void counted() { count_.store(cur_.empty() ? 0 : deltas_.size() + 1, std::memory_order_release); }
  std::thread worker_;
  std::mutex lock_;
  std::condition_variable cv_;
  std::vector<uint8_t> pending_;
  bool hasPending_ = false, busy_ = false, quit_ = false;
  std::atomic<size_t> count_{0};
#endif
};
