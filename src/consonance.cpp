#include "scalatrix/consonance.hpp"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace scalatrix {

// Plomp-Levelt constants
static constexpr double DSTAR = 0.24;
static constexpr double S1 = 0.0207;
static constexpr double S2 = 18.96;
static constexpr double C1_PL = 5.0;
static constexpr double C2_PL = -5.0;
static constexpr double A1 = -3.51;
static constexpr double A2 = -5.75;

// sf_max: position of maximum dissonance in the atomic PL curve
// g(sf) = C1*exp(A1*sf) + C2*exp(A2*sf), g'(sf) = 0 at sf_max
// sf_max = ln(-C2*A2 / (C1*A1)) / (A1 - A2)
static const double SF_MAX = std::log((-C2_PL * A2) / (C1_PL * A1)) / (A1 - A2);

// d_flat: atomic dissonance value at sf_max (the plateau value)
static const double D_FLAT = C1_PL * std::exp(A1 * SF_MAX) + C2_PL * std::exp(A2 * SF_MAX);

// Generation 3 analytic decomposition constant:
// Decompose PL atom as L + H where H'(0) = 0 -> alpha = C2 + C1*A1/A2.
// LOCAL_CONS_AMP = -alpha is the per-pair local-consonance peak at sf = 0.
static constexpr double LOCAL_CONS_AMP = -(C2_PL + C1_PL * A1 / A2);

static double computeDissonanceAtCents(const Spectrum& spectrum, double f0, double cents) {
    double ratio = std::pow(2.0, cents / 1200.0);
    size_t np = spectrum.partials.size();
    size_t total = 2 * np;

    std::vector<std::pair<double, double>> fa(total);
    for (size_t i = 0; i < np; ++i) {
        fa[i] = {f0 * spectrum.partials[i].ratio, spectrum.partials[i].amplitude};
        fa[i + np] = {f0 * ratio * spectrum.partials[i].ratio, spectrum.partials[i].amplitude};
    }
    std::sort(fa.begin(), fa.end());

    double diss = 0.0;
    for (size_t i = 0; i < total; ++i) {
        for (size_t j = i + 1; j < total; ++j) {
            double f_low = fa[i].first;
            double f_high = fa[j].first;
            double a_min = std::min(fa[i].second, fa[j].second);
            double fdif = f_high - f_low;
            double s = DSTAR / (S1 * f_low + S2);
            double sf = s * fdif;
            diss += a_min * (C1_PL * std::exp(A1 * sf) + C2_PL * std::exp(A2 * sf));
        }
    }
    return diss;
}

PLCurve computePLCurve(const Spectrum& spectrum, double f0,
                       double cents_min, double cents_max, double resolution) {
    int n_points = static_cast<int>((cents_max - cents_min) / resolution) + 1;
    PLCurve result;
    result.cents.resize(n_points);
    result.pl.resize(n_points);

    for (int i = 0; i < n_points; ++i) {
        double c = cents_min + i * (cents_max - cents_min) / (n_points - 1);
        result.cents[i] = c;
        result.pl[i] = computeDissonanceAtCents(spectrum, f0, c);
    }
    return result;
}

ConsonanceCurve computeConsonanceCurve(const Spectrum& spectrum, double f0,
    double cents_min, double cents_max, double resolution, double logBaseline)
{
    int n_points = static_cast<int>((cents_max - cents_min) / resolution) + 1;

    ConsonanceCurve result;
    result.cents.resize(n_points);
    result.pl.resize(n_points);
    result.spiky.resize(n_points, 0.0);
    result.consonance.resize(n_points, 0.0);

    // Build cents grid and precompute ratios
    std::vector<double> cents_ratios(n_points);
    for (int i = 0; i < n_points; ++i) {
        result.cents[i] = cents_min + i * (cents_max - cents_min) / (n_points - 1);
        cents_ratios[i] = std::pow(2.0, result.cents[i] / 1200.0);
    }

    // Compute PL curve
    for (int i = 0; i < n_points; ++i) {
        result.pl[i] = computeDissonanceAtCents(spectrum, f0, result.cents[i]);
    }

    // Compute exact asymmetric pyramids (spiky = d_flat - d for each pair where sf < SF_MAX)
    // For each pair of (base partial i, transposed partial j):
    const auto& partials = spectrum.partials;
    size_t np = partials.size();

    for (size_t pi = 0; pi < np; ++pi) {
        for (size_t pj = 0; pj < np; ++pj) {
            double f_base = f0 * partials[pi].ratio;
            double a_min = std::min(partials[pi].amplitude, partials[pj].amplitude);

            for (int ci = 0; ci < n_points; ++ci) {
                double f_trans = f0 * cents_ratios[ci] * partials[pj].ratio;
                double fdif = std::abs(f_base - f_trans);
                double f_low = std::min(f_base, f_trans);
                double s = DSTAR / (S1 * f_low + S2);
                double sf = s * fdif;

                if (sf < SF_MAX) {
                    double d_normal = a_min * (C1_PL * std::exp(A1 * sf) + C2_PL * std::exp(A2 * sf));
                    result.spiky[ci] += a_min * D_FLAT - d_normal;
                }
            }
        }
    }

    // Hull = PL + spiky (flat-topped PL)
    result.hull.resize(n_points);
    for (int i = 0; i < n_points; ++i) {
        result.hull[i] = result.pl[i] + result.spiky[i];
    }

    // Find peak spiky (at unison)
    result.peak = *std::max_element(result.spiky.begin(), result.spiky.end());

    // Auto-compute logBaseline if <= 0: find value where |logBaseline| fraction maps to zero
    result.logBaseline = logBaseline;
    double effectiveLogBaseline = logBaseline;
    if (logBaseline <= 0.0 && result.peak > 0.0) {
        double targetFraction = (logBaseline < 0.0) ? -logBaseline : 0.5;
        targetFraction = std::clamp(targetFraction, 0.01, 0.99);

        // Find the threshold: the value at the targetFraction percentile
        // Using nth_element for O(n) median finding
        std::vector<double> sorted_spiky(result.spiky);
        int target_idx = static_cast<int>(targetFraction * (n_points - 1));
        std::nth_element(sorted_spiky.begin(), sorted_spiky.begin() + target_idx, sorted_spiky.end());
        double threshold = sorted_spiky[target_idx];

        if (threshold > 0.0) {
            // logBaseline = -1 / log10(threshold / peak)
            double ratio = threshold / result.peak;
            effectiveLogBaseline = -1.0 / std::log10(ratio);
        } else {
            // More than targetFraction of points are already zero
            // Find the smallest nonzero spiky value above the target
            int n_zero = 0;
            for (int i = 0; i < n_points; ++i)
                if (result.spiky[i] <= 0.0) ++n_zero;

            if (n_zero < n_points) {
                // Collect nonzero values
                std::vector<double> nonzero;
                nonzero.reserve(n_points - n_zero);
                for (int i = 0; i < n_points; ++i)
                    if (result.spiky[i] > 0.0) nonzero.push_back(result.spiky[i]);

                // We need (targetFraction * n_points - n_zero) additional points to be cut
                int additional_cut = static_cast<int>(targetFraction * n_points) - n_zero;
                if (additional_cut > 0 && additional_cut < static_cast<int>(nonzero.size())) {
                    std::nth_element(nonzero.begin(), nonzero.begin() + additional_cut, nonzero.end());
                    double t = nonzero[additional_cut];
                    double ratio = t / result.peak;
                    effectiveLogBaseline = -1.0 / std::log10(ratio);
                } else {
                    // Already exceeds target or no nonzero values to cut
                    effectiveLogBaseline = 0.5; // fallback
                }
            } else {
                effectiveLogBaseline = 0.5; // fallback: all zero
            }
        }
        result.logBaseline = effectiveLogBaseline;
    }

    // Compute consonance: max(0, 1 + logBaseline * log10(spiky/peak))
    if (result.peak > 0.0) {
        for (int i = 0; i < n_points; ++i) {
            if (result.spiky[i] > 0.0) {
                double norm = result.spiky[i] / result.peak;
                double c = 1.0 + effectiveLogBaseline * std::log10(norm);
                result.consonance[i] = std::max(0.0, c);
            }
        }
    }

    return result;
}

ConsonanceCurve computeConsonanceCurveGen3(const Spectrum& spectrum, double f0,
    double cents_min, double cents_max, double resolution, double logBaseline)
{
    int n_points = static_cast<int>((cents_max - cents_min) / resolution) + 1;

    ConsonanceCurve result;
    result.cents.resize(n_points);
    result.pl.resize(n_points);
    result.spiky.resize(n_points, 0.0);
    result.consonance.resize(n_points, 0.0);

    // Build cents grid and precompute ratios
    std::vector<double> cents_ratios(n_points);
    for (int i = 0; i < n_points; ++i) {
        result.cents[i] = cents_min + i * (cents_max - cents_min) / (n_points - 1);
        cents_ratios[i] = std::pow(2.0, result.cents[i] / 1200.0);
    }

    // Compute PL curve
    for (int i = 0; i < n_points; ++i) {
        result.pl[i] = computeDissonanceAtCents(spectrum, f0, result.cents[i]);
    }

    // Generation 3: analytic decomposition of PL atom into local consonance + smooth hull.
    // Per-pair contribution: a_min * LOCAL_CONS_AMP * exp(A2 * sf).
    // No cutoff — the decomposition is global and continuous.
    const auto& partials = spectrum.partials;
    size_t np = partials.size();

    for (size_t pi = 0; pi < np; ++pi) {
        for (size_t pj = 0; pj < np; ++pj) {
            double f_base = f0 * partials[pi].ratio;
            double a_min = std::min(partials[pi].amplitude, partials[pj].amplitude);
            double a_scaled = a_min * LOCAL_CONS_AMP;

            for (int ci = 0; ci < n_points; ++ci) {
                double f_trans = f0 * cents_ratios[ci] * partials[pj].ratio;
                double fdif = std::abs(f_base - f_trans);
                double f_low = std::min(f_base, f_trans);
                double s = DSTAR / (S1 * f_low + S2);
                double sf = s * fdif;

                result.spiky[ci] += a_scaled * std::exp(A2 * sf);
            }
        }
    }

    // Hull = PL + spiky
    result.hull.resize(n_points);
    for (int i = 0; i < n_points; ++i) {
        result.hull[i] = result.pl[i] + result.spiky[i];
    }

    // Find peak spiky (at unison)
    result.peak = *std::max_element(result.spiky.begin(), result.spiky.end());

    // Auto-compute logBaseline if <= 0
    result.logBaseline = logBaseline;
    double effectiveLogBaseline = logBaseline;
    if (logBaseline <= 0.0 && result.peak > 0.0) {
        double targetFraction = (logBaseline < 0.0) ? -logBaseline : 0.5;
        targetFraction = std::clamp(targetFraction, 0.01, 0.99);

        std::vector<double> sorted_spiky(result.spiky);
        int target_idx = static_cast<int>(targetFraction * (n_points - 1));
        std::nth_element(sorted_spiky.begin(), sorted_spiky.begin() + target_idx, sorted_spiky.end());
        double threshold = sorted_spiky[target_idx];

        if (threshold > 0.0) {
            double ratio = threshold / result.peak;
            effectiveLogBaseline = -1.0 / std::log10(ratio);
        } else {
            int n_zero = 0;
            for (int i = 0; i < n_points; ++i)
                if (result.spiky[i] <= 0.0) ++n_zero;

            if (n_zero < n_points) {
                std::vector<double> nonzero;
                nonzero.reserve(n_points - n_zero);
                for (int i = 0; i < n_points; ++i)
                    if (result.spiky[i] > 0.0) nonzero.push_back(result.spiky[i]);

                int additional_cut = static_cast<int>(targetFraction * n_points) - n_zero;
                if (additional_cut > 0 && additional_cut < static_cast<int>(nonzero.size())) {
                    std::nth_element(nonzero.begin(), nonzero.begin() + additional_cut, nonzero.end());
                    double t = nonzero[additional_cut];
                    double ratio = t / result.peak;
                    effectiveLogBaseline = -1.0 / std::log10(ratio);
                } else {
                    effectiveLogBaseline = 0.5;
                }
            } else {
                effectiveLogBaseline = 0.5;
            }
        }
        result.logBaseline = effectiveLogBaseline;
    }

    // Compute consonance: max(0, 1 + logBaseline * log10(spiky/peak))
    if (result.peak > 0.0) {
        for (int i = 0; i < n_points; ++i) {
            if (result.spiky[i] > 0.0) {
                double norm = result.spiky[i] / result.peak;
                double c = 1.0 + effectiveLogBaseline * std::log10(norm);
                result.consonance[i] = std::max(0.0, c);
            }
        }
    }

    return result;
}

ConsonanceResult analyzeScale(const Spectrum& spectrum, double f0,
    const std::vector<std::pair<std::string, double>>& intervals,
    double max_cents, double max_interval_cents, double logBaseline)
{
    double margin = 300.0;
    double resolution = 0.5;

    // Compute consonance curve on extended range
    ConsonanceCurve curve = computeConsonanceCurve(spectrum, f0,
        0.0 - margin, max_cents + margin, resolution, logBaseline);

    // Evaluate consonance at each interval by linear interpolation
    ConsonanceResult result;
    result.total_consonance = 0.0;

    for (auto& [name, cents] : intervals) {
        if (cents > max_interval_cents) continue;

        double c = 0.0;
        int n = static_cast<int>(curve.cents.size());
        for (int i = 0; i + 1 < n; ++i) {
            if (curve.cents[i] <= cents && curve.cents[i + 1] >= cents) {
                double t = (cents - curve.cents[i]) / (curve.cents[i + 1] - curve.cents[i]);
                c = curve.consonance[i] + t * (curve.consonance[i + 1] - curve.consonance[i]);
                break;
            }
        }
        result.intervals.push_back({name, cents, c});
        result.total_consonance += c;
    }

    result.mean_consonance = result.intervals.empty() ? 0.0 :
        result.total_consonance / result.intervals.size();
    return result;
}

} // namespace scalatrix
