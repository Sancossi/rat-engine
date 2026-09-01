#pragma once

#include <chrono>

namespace rat {

class Clock {
 public:
  virtual ~Clock() = default;
  [[nodiscard]] virtual double now_seconds() const = 0;
};

class FakeClock final : public Clock {
 public:
  void set_seconds(double seconds) { now_ = seconds; }
  void advance_seconds(double delta) { now_ += delta; }
  [[nodiscard]] double now_seconds() const override { return now_; }

 private:
  double now_ = 0.0;
};

class SteadyClock final : public Clock {
 public:
  SteadyClock();
  [[nodiscard]] double now_seconds() const override;

 private:
  std::chrono::steady_clock::time_point start_{};
};

}  // namespace rat
