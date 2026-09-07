#pragma once
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace rat::benchmark {
// Nearest-rank: sort N samples; select ceil(p*N)-1 (p in (0,1]).
inline double quantile(std::vector<double> samples, double p) {
  if (samples.empty() || !std::isfinite(p) || p <= 0 || p > 1)
    throw std::invalid_argument("quantile requires samples and p in (0,1]");
  std::sort(samples.begin(), samples.end());
  return samples[static_cast<std::size_t>(std::ceil(p * static_cast<double>(samples.size()))) - 1];
}
}  // namespace rat::benchmark
