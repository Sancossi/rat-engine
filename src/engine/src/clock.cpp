#include "rat/clock.hpp"

namespace rat {

SteadyClock::SteadyClock() : start_(std::chrono::steady_clock::now()) {}

double SteadyClock::now_seconds() const {
  return std::chrono::duration<double>(std::chrono::steady_clock::now() - start_).count();
}

}  // namespace rat
