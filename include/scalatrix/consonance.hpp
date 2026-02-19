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

struct HullResult {
    std::vector<double> cents;
    std::vector<double> pl;
    std::vector<double> hull;
    std::vector<double> spiky;
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

/// Compute Hull₃ from PL curve
HullResult computeHull3(const PLCurve& pl_curve, int order = 3, double spike_threshold = 0.005);

/// Consonance value from normalized spiky
double consonanceValue(double spiky_normalized);

/// Full scale analysis
ConsonanceResult analyzeScale(const Spectrum& spectrum, double f0,
    const std::vector<std::pair<std::string, double>>& intervals,
    double max_cents = 2000.0, double max_interval_cents = 1950.0);

} // namespace scalatrix

#endif
