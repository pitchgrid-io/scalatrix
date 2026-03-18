#ifndef SCALATRIX_CONSONANCE_HPP
#define SCALATRIX_CONSONANCE_HPP

#include "scalatrix/spectrum.hpp"
#include <vector>
#include <string>
#include <utility>

namespace scalatrix {

struct PLCurve {
    std::vector<double> cents;
    std::vector<double> pl;
};

struct ConsonanceCurve {
    std::vector<double> cents;
    std::vector<double> pl;         // PL dissonance curve
    std::vector<double> hull;       // flat-topped PL (dips filled)
    std::vector<double> spiky;      // hull - pl (exact pyramids)
    std::vector<double> consonance; // log-transformed: max(0, 1 + logBaseline * log10(spiky/peak))
    double peak;                    // peak spiky value (at unison)
};

struct IntervalConsonance {
    std::string name;
    double cents;
    double consonance;
};

struct ConsonanceResult {
    std::vector<IntervalConsonance> intervals;
    double mean_consonance;
    double total_consonance;
};

/// Compute PL dissonance curve
PLCurve computePLCurve(const Spectrum& spectrum, double f0,
                       double cents_min, double cents_max, double resolution = 0.5);

/// Compute consonance curve: PL, hull (flat-topped PL), spiky (exact pyramids), consonance
/// Uses exact asymmetric pyramids derived from PL atomic curve shape.
/// Each partial pair's dip is analytically filled to produce the hull.
ConsonanceCurve computeConsonanceCurve(const Spectrum& spectrum, double f0,
    double cents_min, double cents_max, double resolution = 0.5,
    double logBaseline = 0.5);

/// Full scale analysis: compute consonance at each interval
ConsonanceResult analyzeScale(const Spectrum& spectrum, double f0,
    const std::vector<std::pair<std::string, double>>& intervals,
    double max_cents = 2000.0, double max_interval_cents = 1950.0,
    double logBaseline = 0.5);

} // namespace scalatrix

#endif
