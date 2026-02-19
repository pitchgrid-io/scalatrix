#ifndef SCALATRIX_SPECTRUM_HPP
#define SCALATRIX_SPECTRUM_HPP

#include <vector>
#include <map>

namespace scalatrix {

struct Partial {
    double ratio;
    double amplitude;
};

class Spectrum {
public:
    std::vector<Partial> partials;

    Spectrum() = default;
    explicit Spectrum(std::vector<Partial> p) : partials(std::move(p)) {}

    /// Harmonic spectrum: partials 1..n with geometric decay
    static Spectrum harmonic(int n_partials, double decay = 0.88);

    /// Odd harmonic spectrum: partials 1,3,5,...  with decay^(h-1)
    static Spectrum oddHarmonic(int max_harmonic, double decay = 0.88);

    /// Pseudoharmonic: primes tuned to specific cent values
    static Spectrum pseudoharmonic(int n_partials, double decay = 0.88,
        const std::map<int, double>& prime_cents = {{2, 1200.0}, {3, 1900.0}, {5, 2800.0}});
};

} // namespace scalatrix

#endif
