#include "scalatrix/consonance.hpp"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <limits>
#include <cassert>
#include <set>

namespace scalatrix {

// Plomp-Levelt constants
static constexpr double DSTAR = 0.24;
static constexpr double S1 = 0.0207;
static constexpr double S2 = 18.96;
static constexpr double C1_PL = 5.0;
static constexpr double C2_PL = -5.0;
static constexpr double A1 = -3.51;
static constexpr double A2 = -5.75;

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

// ─── Not-a-knot cubic spline (matches scipy.interpolate.CubicSpline default) ───
//
// Solves (n+1)x(n+1) system for c[0..n] where n = number of segments.
// Interior: h[i-1]*c[i-1] + 2(h[i-1]+h[i])*c[i] + h[i]*c[i+1] = rhs[i]
// Not-a-knot left:  h[1]*c[0] - (h[0]+h[1])*c[1] + h[0]*c[2] = 0
// Not-a-knot right: h[n-1]*c[n-2] - (h[n-2]+h[n-1])*c[n-1] + h[n-2]*c[n] = 0

class CubicSpline {
    std::vector<double> xs;
    std::vector<double> a_coef, b_coef, c_coef, d_coef;
    int n_seg;
public:
    CubicSpline(const std::vector<double>& x, const std::vector<double>& y) {
        n_seg = static_cast<int>(x.size()) - 1;
        xs = x;
        if (n_seg < 1) return;
        if (n_seg == 1) {
            a_coef = {y[0]};
            b_coef = {(y[1] - y[0]) / (x[1] - x[0])};
            c_coef = {0.0, 0.0};
            d_coef = {0.0};
            return;
        }

        int n = n_seg;
        std::vector<double> h(n);
        for (int i = 0; i < n; ++i) h[i] = x[i + 1] - x[i];

        // Build (n+1)x(n+2) augmented matrix
        int sz = n + 1;
        std::vector<std::vector<double>> mat(sz, std::vector<double>(sz + 1, 0.0));

        // Row 0: not-a-knot left
        mat[0][0] = h[1];
        mat[0][1] = -(h[0] + h[1]);
        mat[0][2] = h[0];
        mat[0][sz] = 0.0;

        // Interior rows 1..n-1
        for (int i = 1; i < n; ++i) {
            mat[i][i - 1] = h[i - 1];
            mat[i][i] = 2.0 * (h[i - 1] + h[i]);
            mat[i][i + 1] = h[i];
            mat[i][sz] = 3.0 * ((y[i + 1] - y[i]) / h[i] - (y[i] - y[i - 1]) / h[i - 1]);
        }

        // Row n: not-a-knot right
        mat[n][n - 2] = h[n - 1];
        mat[n][n - 1] = -(h[n - 2] + h[n - 1]);
        mat[n][n] = h[n - 2];
        mat[n][sz] = 0.0;

        // Gaussian elimination with partial pivoting
        for (int col = 0; col < sz; ++col) {
            int pivot = col;
            for (int row = col + 1; row < sz; ++row) {
                if (std::abs(mat[row][col]) > std::abs(mat[pivot][col])) pivot = row;
            }
            std::swap(mat[col], mat[pivot]);
            if (std::abs(mat[col][col]) < 1e-15) continue;
            for (int row = col + 1; row < sz; ++row) {
                double factor = mat[row][col] / mat[col][col];
                for (int k = col; k <= sz; ++k) {
                    mat[row][k] -= factor * mat[col][k];
                }
            }
        }

        // Back substitution
        std::vector<double> c(sz);
        for (int i = sz - 1; i >= 0; --i) {
            c[i] = mat[i][sz];
            for (int j = i + 1; j < sz; ++j) {
                c[i] -= mat[i][j] * c[j];
            }
            c[i] /= mat[i][i];
        }

        a_coef.resize(n);
        b_coef.resize(n);
        c_coef = c;
        d_coef.resize(n);
        for (int i = 0; i < n; ++i) {
            a_coef[i] = y[i];
            b_coef[i] = (y[i + 1] - y[i]) / h[i] - h[i] * (c[i + 1] + 2.0 * c[i]) / 3.0;
            d_coef[i] = (c[i + 1] - c[i]) / (3.0 * h[i]);
        }
    }

    double eval(double xv) const {
        if (n_seg < 1) return 0.0;
        int seg = n_seg - 1;
        for (int i = 0; i < n_seg; ++i) {
            if (xv < xs[i + 1]) { seg = i; break; }
        }
        double dx = xv - xs[seg];
        return a_coef[seg] + b_coef[seg] * dx + c_coef[seg] * dx * dx + d_coef[seg] * dx * dx * dx;
    }
};

// Local maxima detection matching scipy.signal.argrelextrema(arr, np.greater, order=N)
static std::vector<int> findLocalMaxima(const std::vector<double>& arr, int order) {
    std::vector<int> maxima;
    int n = static_cast<int>(arr.size());
    for (int i = order; i < n - order; ++i) {
        bool is_max = true;
        for (int j = i - order; j <= i + order; ++j) {
            if (j != i && arr[j] >= arr[i]) { is_max = false; break; }
        }
        if (is_max) maxima.push_back(i);
    }
    return maxima;
}

// np.gradient matching (forward/backward at edges, central in middle)
static std::vector<double> npGradient(const std::vector<double>& f, double dx) {
    int n = static_cast<int>(f.size());
    std::vector<double> g(n);
    if (n < 2) return g;
    g[0] = (f[1] - f[0]) / dx;
    g[n - 1] = (f[n - 1] - f[n - 2]) / dx;
    for (int i = 1; i < n - 1; ++i) {
        g[i] = (f[i + 1] - f[i - 1]) / (2.0 * dx);
    }
    return g;
}

HullResult computeHull3(const PLCurve& pl_curve, int order, double spike_threshold) {
    int n = static_cast<int>(pl_curve.cents.size());
    if (n < 3) return {pl_curve.cents, pl_curve.pl, pl_curve.pl, std::vector<double>(n, 0.0)};

    double dx = pl_curve.cents[1] - pl_curve.cents[0];

    // 2nd derivative: np.gradient(np.gradient(pl, dx), dx)
    auto d1 = npGradient(pl_curve.pl, dx);
    auto d2 = npGradient(d1, dx);

    auto max_idx = findLocalMaxima(d2, order);

    if (max_idx.size() < 2) {
        return {pl_curve.cents, pl_curve.pl, pl_curve.pl, std::vector<double>(n, 0.0)};
    }

    std::vector<int> clean_idx;
    std::vector<std::pair<double, int>> all_vals;
    for (int idx : max_idx) {
        all_vals.push_back({d2[idx], idx});
        if (d2[idx] <= spike_threshold) {
            clean_idx.push_back(idx);
        }
    }

    if (clean_idx.size() < 2) {
        std::sort(all_vals.begin(), all_vals.end());
        clean_idx.clear();
        int keep = std::max(2, static_cast<int>(max_idx.size()) / 2);
        for (int i = 0; i < keep && i < static_cast<int>(all_vals.size()); ++i) {
            clean_idx.push_back(all_vals[i].second);
        }
        std::sort(clean_idx.begin(), clean_idx.end());
    }

    int ep_margin = static_cast<int>(50.0 / dx);
    if (clean_idx.front() > ep_margin) {
        clean_idx.insert(clean_idx.begin(), 0);
    }
    if (clean_idx.back() < n - ep_margin) {
        clean_idx.push_back(n - 1);
    }

    std::vector<double> kx, ky;
    for (int idx : clean_idx) {
        kx.push_back(pl_curve.cents[idx]);
        ky.push_back(pl_curve.pl[idx]);
    }

    CubicSpline spline(kx, ky);

    HullResult result;
    result.cents = pl_curve.cents;
    result.pl = pl_curve.pl;
    result.hull.resize(n);
    result.spiky.resize(n);
    for (int i = 0; i < n; ++i) {
        result.hull[i] = std::max(spline.eval(pl_curve.cents[i]), pl_curve.pl[i]);
        result.spiky[i] = result.hull[i] - pl_curve.pl[i];
    }
    return result;
}

// Local minima detection matching scipy.signal.argrelextrema(arr, np.less, order=N)
static std::vector<int> findLocalMinima(const std::vector<double>& arr, int order) {
    std::vector<int> minima;
    int n = static_cast<int>(arr.size());
    for (int i = order; i < n - order; ++i) {
        bool is_min = true;
        for (int j = i - order; j <= i + order; ++j) {
            if (j != i && arr[j] <= arr[i]) { is_min = false; break; }
        }
        if (is_min) minima.push_back(i);
    }
    return minima;
}

HullResult computeHull4(const PLCurve& pl_curve, int order, double sharpness_threshold) {
    int n = static_cast<int>(pl_curve.cents.size());
    if (n < 3) return {pl_curve.cents, pl_curve.pl, pl_curve.pl, std::vector<double>(n, 0.0)};

    double dx = pl_curve.cents[1] - pl_curve.cents[0];
    int ep_margin = static_cast<int>(50.0 / dx);

    // Find PL local minima and maxima
    auto min_indices = findLocalMinima(pl_curve.pl, order);
    auto max_indices = findLocalMaxima(pl_curve.pl, order);

    // Add endpoints to maxima if needed
    if (max_indices.empty() || max_indices.front() > ep_margin) {
        max_indices.insert(max_indices.begin(), 0);
    }
    if (max_indices.empty() || max_indices.back() < n - ep_margin) {
        max_indices.push_back(n - 1);
    }

    // Filter minima by sharpness: d2 > threshold (cusps where derivative jumps)
    auto d1 = npGradient(pl_curve.pl, dx);
    auto d2 = npGradient(d1, dx);

    std::vector<int> spike_indices;
    for (int mi : min_indices) {
        if (d2[mi] > sharpness_threshold) {
            spike_indices.push_back(mi);
        }
    }

    // For each spike, compute symmetric region and collect anchor points
    struct SpikeRegion { int a1, a2, si; };
    std::vector<SpikeRegion> spike_regions;
    std::set<int> all_anchor_set;

    for (int si : spike_indices) {
        // Find nearest flanking maxima
        int m1 = -1, m2 = -1;
        for (int k = static_cast<int>(max_indices.size()) - 1; k >= 0; --k) {
            if (max_indices[k] < si) { m1 = max_indices[k]; break; }
        }
        for (int k = 0; k < static_cast<int>(max_indices.size()); ++k) {
            if (max_indices[k] > si) { m2 = max_indices[k]; break; }
        }
        if (m1 < 0 || m2 < 0) continue;

        // Symmetric region: mirror the closer maximum
        int d_left = si - m1;
        int d_right = m2 - si;
        int d = std::min(d_left, d_right);

        int a1 = std::max(0, si - d);
        int a2 = std::min(n - 1, si + d);

        spike_regions.push_back({a1, a2, si});

        // Collect anchor points: symmetric endpoints + interior maxima
        all_anchor_set.insert(a1);
        all_anchor_set.insert(a2);
        for (int mi : max_indices) {
            if (mi > a1 && mi < a2) all_anchor_set.insert(mi);
        }
    }

    // Build global spline through all anchor points
    std::vector<int> all_anchors(all_anchor_set.begin(), all_anchor_set.end());
    std::sort(all_anchors.begin(), all_anchors.end());

    HullResult result;
    result.cents = pl_curve.cents;
    result.pl = pl_curve.pl;
    result.hull.resize(n);
    result.spiky.resize(n, 0.0);

    if (all_anchors.size() < 2) {
        // No spikes found — zero spiky everywhere
        result.hull = pl_curve.pl;
        return result;
    }

    std::vector<double> kx, ky;
    for (int idx : all_anchors) {
        kx.push_back(pl_curve.cents[idx]);
        ky.push_back(pl_curve.pl[idx]);
    }
    CubicSpline spline(kx, ky);

    // Compute hull from spline, clamped above PL
    std::vector<double> hull_full(n);
    std::vector<double> raw_spiky(n);
    for (int i = 0; i < n; ++i) {
        hull_full[i] = std::max(spline.eval(pl_curve.cents[i]), pl_curve.pl[i]);
        raw_spiky[i] = hull_full[i] - pl_curve.pl[i];
    }

    // Gate: only keep spiky within spike regions
    result.hull = pl_curve.pl; // default hull = PL (no elevation)
    for (auto& sr : spike_regions) {
        for (int i = sr.a1; i <= sr.a2; ++i) {
            if (raw_spiky[i] > result.spiky[i]) {
                result.spiky[i] = raw_spiky[i];
                result.hull[i] = hull_full[i];
            }
        }
    }

    return result;
}

double consonanceValue(double spiky_normalized) {
    return std::max(0.0, 1.0 + 0.5 * std::log10(std::max(spiky_normalized, 1e-10)));
}

ConsonanceResult analyzeScale(const Spectrum& spectrum, double f0,
    const std::vector<std::pair<std::string, double>>& intervals,
    double max_cents, double max_interval_cents)
{
    double margin = 300.0;
    double resolution = 0.5;
    
    // Step 1: compute_spiky_curve equivalent — PL on extended range
    double ext_min = 0.0 - margin;
    double ext_max = max_cents + margin;
    PLCurve pl_ext = computePLCurve(spectrum, f0, ext_min, ext_max, resolution);
    
    // Compute Hull₄ (selective spiky) on extended range
    HullResult hull4 = computeHull4(pl_ext);

    // Crop to [0, max_cents]
    int n_ext = static_cast<int>(pl_ext.cents.size());
    std::vector<double> cents_crop, spiky3_crop;
    for (int i = 0; i < n_ext; ++i) {
        if (pl_ext.cents[i] >= 0.0 && pl_ext.cents[i] <= max_cents) {
            cents_crop.push_back(pl_ext.cents[i]);
            spiky3_crop.push_back(hull4.spiky[i]);
        }
    }
    
    // Normalize: peak at 0¢
    double peak_at_0 = 0.0;
    for (size_t i = 0; i < cents_crop.size(); ++i) {
        if (cents_crop[i] >= -0.5 && cents_crop[i] <= 0.5) {
            peak_at_0 = std::max(peak_at_0, spiky3_crop[i]);
        }
    }
    if (peak_at_0 <= 0.0) {
        peak_at_0 = *std::max_element(spiky3_crop.begin(), spiky3_crop.end());
    }

    ConsonanceResult result;
    result.total_consonance = 0.0;

    for (auto& [name, cents] : intervals) {
        if (cents > max_interval_cents) continue;

        // Linear interpolation
        double sv = 0.0;
        for (size_t i = 0; i + 1 < cents_crop.size(); ++i) {
            if (cents_crop[i] <= cents && cents_crop[i + 1] >= cents) {
                double t = (cents - cents_crop[i]) / (cents_crop[i + 1] - cents_crop[i]);
                sv = spiky3_crop[i] + t * (spiky3_crop[i + 1] - spiky3_crop[i]);
                break;
            }
        }
        double sv_norm = sv / peak_at_0;
        double c = consonanceValue(sv_norm);
        result.intervals.push_back({name, cents, c});
        result.total_consonance += c;
    }

    result.mean_consonance = result.intervals.empty() ? 0.0 :
        result.total_consonance / result.intervals.size();
    return result;
}

} // namespace scalatrix
