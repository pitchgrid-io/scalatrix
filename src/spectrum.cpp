#include "scalatrix/spectrum.hpp"
#include <cmath>
#include <vector>

namespace scalatrix {

Spectrum Spectrum::harmonic(int n_partials, double decay) {
    std::vector<Partial> p;
    p.reserve(n_partials);
    for (int i = 1; i <= n_partials; ++i) {
        p.push_back({static_cast<double>(i), std::pow(decay, i - 1)});
    }
    return Spectrum(std::move(p));
}

Spectrum Spectrum::oddHarmonic(int max_harmonic, double decay) {
    std::vector<Partial> p;
    for (int h = 1; h <= max_harmonic; h += 2) {
        p.push_back({static_cast<double>(h), std::pow(decay, h - 1)});
    }
    return Spectrum(std::move(p));
}

static std::vector<int> primeFactors(int n) {
    std::vector<int> factors;
    int i = 2;
    while (i * i <= n) {
        if (n % i == 0) {
            factors.push_back(i);
            n /= i;
        } else {
            ++i;
        }
    }
    if (n > 1) factors.push_back(n);
    return factors;
}

Spectrum Spectrum::pseudoharmonic(int n_partials, double decay,
    const std::map<int, double>& prime_cents)
{
    // Convert prime cents to adjustment ratios (adjusted/just)
    std::map<int, double> prime_adj;
    for (auto& [p, cents] : prime_cents) {
        double adjusted_ratio = std::pow(2.0, cents / 1200.0);
        prime_adj[p] = adjusted_ratio / p;
    }

    std::vector<Partial> parts;
    parts.reserve(n_partials);
    for (int n = 1; n <= n_partials; ++n) {
        auto factors = primeFactors(n);
        double ratio = static_cast<double>(n);
        for (int f : factors) {
            auto it = prime_adj.find(f);
            if (it != prime_adj.end()) {
                ratio *= it->second;
            }
        }
        parts.push_back({ratio, std::pow(decay, n - 1)});
    }
    return Spectrum(std::move(parts));
}

} // namespace scalatrix
