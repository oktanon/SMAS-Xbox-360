#include "core/rewind.h"
#include <algorithm>
#include <cstring>

namespace {
void put32(std::vector<uint8_t>& o, uint32_t v) {
  for (int i = 0; i < 4; i++) o.push_back((uint8_t)(v >> (8 * i)));
}
bool get32(const uint8_t*& p, const uint8_t* end, uint32_t* v) {
  if (end - p < 4) return false;
  *v = p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24;
  p += 4;
  return true;
}
}  // namespace

// Format: older size, newer size, then (zero run, literal count, literals)
// pairs, each count a 32-bit value, covering max(sizes) bytes of
// older XOR newer (the shorter one padded with zeros).
std::vector<uint8_t> Rewind::encode(const std::vector<uint8_t>& older, const std::vector<uint8_t>& newer) {
  std::vector<uint8_t> o;
  size_t n = std::max(older.size(), newer.size());
  o.reserve(64);
  put32(o, (uint32_t)older.size());
  put32(o, (uint32_t)newer.size());
  auto x = [&](size_t i) -> uint8_t {
    return (i < older.size() ? older[i] : 0) ^ (i < newer.size() ? newer[i] : 0);
  };
  size_t i = 0;
  while (i < n) {
    size_t z = i;
    while (z < n && x(z) == 0) z++;
    size_t l = z;
    // (a literal run ends at 8 zero bytes in a row)
    size_t zeros = 0;
    while (l < n && zeros < 8) {
      zeros = x(l) == 0 ? zeros + 1 : 0;
      l++;
    }
    if (zeros == 8) l -= 8;
    put32(o, (uint32_t)(z - i));
    put32(o, (uint32_t)(l - z));
    for (size_t k = z; k < l; k++) o.push_back(x(k));
    i = l;
  }
  return o;
}

bool Rewind::decode(const std::vector<uint8_t>& delta, const std::vector<uint8_t>& newer,
                    std::vector<uint8_t>* older) {
  const uint8_t* p = delta.data();
  const uint8_t* end = p + delta.size();
  uint32_t oldN, newN;
  if (!get32(p, end, &oldN) || !get32(p, end, &newN) || newN != newer.size()) return false;
  size_t n = std::max<size_t>(oldN, newN);
  std::vector<uint8_t> out(n, 0);
  std::memcpy(out.data(), newer.data(), newer.size());
  size_t i = 0;
  while (p < end) {
    uint32_t z, l;
    if (!get32(p, end, &z) || !get32(p, end, &l)) return false;
    i += z;
    if (i + l > n || (size_t)(end - p) < l) return false;
    for (uint32_t k = 0; k < l; k++) out[i + k] ^= p[k];
    p += l;
    i += l;
  }
  out.resize(oldN);
  *older = std::move(out);
  return true;
}

#ifdef SMAS_XENON
void (*Rewind::threadStart)() = nullptr;

Rewind::~Rewind() {
  if (!worker_.joinable()) return;
  {
    std::lock_guard<std::mutex> g(lock_);
    quit_ = true;
  }
  cv_.notify_all();
  worker_.join();
}

// Till the thread has taken in what it was given.
void Rewind::wait() {
  std::unique_lock<std::mutex> g(lock_);
  cv_.wait(g, [this] { return !hasPending_ && !busy_; });
}

void Rewind::push(const std::vector<uint8_t>& state) { push(std::vector<uint8_t>(state)); }

// The snapshot handed over (waiting if the last one isn't taken in yet), its difference worked out on the thread.
void Rewind::push(std::vector<uint8_t>&& state) {
  if (!worker_.joinable()) {
    worker_ = std::thread([this] {
      if (threadStart) threadStart();
      std::unique_lock<std::mutex> g(lock_);
      for (;;) {
        cv_.wait(g, [this] { return hasPending_ || quit_; });
        if (quit_) return;
        std::vector<uint8_t> s;
        s.swap(pending_);
        hasPending_ = false;
        busy_ = true;
        g.unlock();
        const auto at = std::chrono::steady_clock::now();
        pushNow(s);
        workUs_.fetch_add((uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(
                              std::chrono::steady_clock::now() - at).count(), std::memory_order_relaxed);
        works_.fetch_add(1, std::memory_order_relaxed);
        g.lock();
        busy_ = false;
        cv_.notify_all();
      }
    });
  }
  std::unique_lock<std::mutex> g(lock_);
  cv_.wait(g, [this] { return !hasPending_; });
  pending_ = std::move(state);
  hasPending_ = true;
  g.unlock();
  cv_.notify_all();
}

const std::vector<uint8_t>& Rewind::newest() {
  wait();
  return cur_;
}
#endif

void Rewind::clear() {
#ifdef SMAS_XENON
  wait();
#endif
  cur_.clear();
  deltas_.clear();
  bytes_ = 0;
#ifdef SMAS_XENON
  counted();
#endif
}

#ifdef SMAS_XENON
void Rewind::pushNow(const std::vector<uint8_t>& state) {
#else
void Rewind::push(const std::vector<uint8_t>& state) {
#endif
  if (!cur_.empty()) {
    deltas_.push_back(encode(cur_, state));
    bytes_ += deltas_.back().size();
    while (!deltas_.empty() && (bytes_ > maxBytes_ || deltas_.size() + 1 > maxSnapshots_)) {
      bytes_ -= deltas_.front().size();
      deltas_.pop_front();
    }
  }
  cur_ = state;
#ifdef SMAS_XENON
  counted();
#endif
}

bool Rewind::stepBack(std::vector<uint8_t>* out) {
#ifdef SMAS_XENON
  wait();
#endif
  if (deltas_.empty()) return false;
  std::vector<uint8_t> older;
  if (!decode(deltas_.back(), cur_, &older)) {
    clear();
    return false;
  }
  bytes_ -= deltas_.back().size();
  deltas_.pop_back();
  cur_ = std::move(older);
  *out = cur_;
#ifdef SMAS_XENON
  counted();
#endif
  return true;
}
