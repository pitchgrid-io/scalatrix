// The coords-only mapped scale, the shared strip walk and the label-free
// pitch sets (pitchgrid-plugin card #525) against the code they replace,
// copied here unchanged from 99d027c as the reference. Every field is
// compared bit for bit, over a sweep of MOS structures, tuning generators,
// steps, offsets and node windows, and over the pitch set inputs.

#include "catch2/catch_test_macros.hpp"
#include "scalatrix/mos.hpp"
#include "scalatrix/scale.hpp"
#include "scalatrix/lattice.hpp"
#include "scalatrix/pitchset.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <numeric>
#include <string>
#include <vector>

using namespace scalatrix;

namespace {

bool sameBits(double a, double b) { return std::memcmp(&a, &b, sizeof a) == 0; }

// Scale::recalcWithAffine before the shared walk.
std::vector<Node> refFromAffine(const AffineTransform& A, double base_freq_, int N, int root_node_idx) {
    std::vector<Node> nodes_(static_cast<size_t>(N));
    AffineTransform M = AffineTransform(A);
    M.tx = 0;
    M.ty = 0;
    auto [r, s] = findClosestWithinStrip(M);
    Vector2d zr = M * r;
    Vector2d zs = M * s;
    int n_min = -root_node_idx, n_max = N - root_node_idx;
    Node root;
    root.natural_coord = Vector2i(0, 0);
    root.tuning_coord = A * root.natural_coord;
    root.pitch = base_freq_;
    nodes_[root_node_idx] = root;
    Node last = root;
    for (int n = 1; n < n_max; ++n) {
        if (0 <= last.tuning_coord.y + zr.y && last.tuning_coord.y + zr.y < 1) {
            last.natural_coord += r;
        } else if (0 <= last.tuning_coord.y + zs.y && last.tuning_coord.y + zs.y < 1) {
            last.natural_coord += s;
        } else {
            last.natural_coord += (r + s);
        }
        last.tuning_coord = A * last.natural_coord;
        last.pitch = base_freq_ * std::exp2(last.tuning_coord.x);
        nodes_[root_node_idx + n] = last;
    }
    last = root;
    for (int n = -1; n >= n_min; --n) {
        if (0 <= last.tuning_coord.y - zr.y && last.tuning_coord.y - zr.y < 1) {
            last.natural_coord -= r;
        } else if (0 <= last.tuning_coord.y - zs.y && last.tuning_coord.y - zs.y < 1) {
            last.natural_coord -= s;
        } else {
            last.natural_coord -= (r + s);
        }
        last.tuning_coord = A * last.natural_coord;
        last.pitch = base_freq_ * std::exp2(last.tuning_coord.x);
        nodes_[root_node_idx + n] = last;
    }
    return nodes_;
}

// MOS::generateMappedScale before the coords-only first pass.
std::vector<Node> refMappedScale(const MOS& mos, int steps, double offset, double base_freq, int n_nodes, int root) {
    double mos_offset = (offset + 0.5) / steps;
    double mos_scale_factor = static_cast<double>(mos.n) / steps;
    AffineTransform stretched_t(1, 0, 0, mos_scale_factor, 0, 0);
    AffineTransform squeezed_t = stretched_t * mos.structureImpliedAffine;
    squeezed_t.ty = mos_offset;
    std::vector<Node> nodes = refFromAffine(squeezed_t, base_freq, n_nodes, root);
    for (auto& node : nodes) {
        node.tuning_coord.x = (mos.impliedAffine * node.natural_coord).x;
        node.pitch = base_freq * std::exp2(node.tuning_coord.x);
    }
    return nodes;
}

bool sameNode(const Node& a, const Node& b) {
    return a.natural_coord == b.natural_coord
        && sameBits(a.tuning_coord.x, b.tuning_coord.x) && sameBits(a.tuning_coord.y, b.tuning_coord.y)
        && sameBits(a.pitch, b.pitch) && a.isTempered == b.isTempered
        && a.temperedPitch.label == b.temperedPitch.label && sameBits(a.temperedPitch.log2fr, b.temperedPitch.log2fr)
        && a.closestPitch.label == b.closestPitch.label && sameBits(a.closestPitch.log2fr, b.closestPitch.log2fr);
}

// The pitch set builders before the label-free variants.
static PitchSet refETPitchSet(unsigned int n_et, double equave_log2fr, double min_log2fr, double max_log2fr) {
    PitchSet pitchset;
    
    // Calculate the range of steps to generate
    // For min_log2fr: step * equave_log2fr / n_et >= min_log2fr, so step >= min_log2fr * n_et / equave_log2fr
    // For max_log2fr: step * equave_log2fr / n_et <= max_log2fr, so step <= max_log2fr * n_et / equave_log2fr
    int min_step = (int)ceil(min_log2fr * n_et / equave_log2fr);
    int max_step = (int)floor(max_log2fr * n_et / equave_log2fr);
    
    for (int i = min_step; i <= max_step; i++) {
        PitchSetPitch p;
        p.label = std::to_string(i) + "\\" + std::to_string(n_et);
        p.log2fr = i * equave_log2fr / n_et;
        
        // Filter to ensure the pitch is within the specified range
        if (p.log2fr >= min_log2fr - 1e-6 && p.log2fr <= max_log2fr + 1e-6) {
            pitchset.push_back(p);
        }
    }
    
    // Sort by log2fr (should already be sorted, but ensure it)
    std::sort(pitchset.begin(), pitchset.end(), 
        [](const PitchSetPitch& a, const PitchSetPitch& b) {
            return a.log2fr < b.log2fr;
        }
    );
    
    return pitchset;
}

static PitchSet refJIPitchSet(PrimeList primes, int max_numorden, double min_log2fr, double max_log2fr) {
    PitchSet pitchset;
    PrimeList nums;
    // generate all numbers with prime factors in primes up to max_numorden
    for (int i = 1; i < max_numorden; i++) {
        int r = i;
        double log2fr = 0.0;
        for (auto p : primes) {
            while (r % p.number == 0) {
                r /= p.number;
                log2fr += p.log2fr;
            }
        }
        bool include = r == 1;
        if(include){
            PseudoPrimeInt p;
            p.label = std::to_string(i);
            p.number = i;
            p.log2fr = log2fr;
            nums.push_back(p);
        }
    }
    for (auto num: nums){
        for (auto den: nums){
            if (std::gcd(num.number, den.number) > 1){
                continue;
            }
            if (static_cast<int>(num.number) <= max_numorden && static_cast<int>(den.number) <= max_numorden) {
                PitchSetPitch pitch;
                pitch.label = num.label + ":" + den.label;
                pitch.log2fr = num.log2fr - den.log2fr;
                pitchset.push_back(pitch);
            }
        }
    }
    // filter out pitches with log2fr outside of min_log2fr and max_log2fr,
    // in place: one set alive at a time
    pitchset.erase(std::remove_if(pitchset.begin(), pitchset.end(),
        [&](const PitchSetPitch& pitch) {
            return !(pitch.log2fr > min_log2fr - 1e-6 && pitch.log2fr < max_log2fr + 1e-6);
        }), pitchset.end());
    pitchset.shrink_to_fit();
    // sort
    std::sort(pitchset.begin(), pitchset.end(), 
        [](PitchSetPitch a, PitchSetPitch b) {
            return a.log2fr < b.log2fr;
        }
    );
    return pitchset;
}


static PitchSet refHarmonicSeriesPitchSet(PrimeList primes, int base, double min_log2fr, double max_log2fr) {
    PitchSet pitchset;
    double base_log2fr = log2(base);
    
    // Calculate the range of numerators to generate
    // For min_log2fr: num/base >= 2^min_log2fr, so num >= base * 2^min_log2fr
    // For max_log2fr: num/base <= 2^max_log2fr, so num <= base * 2^max_log2fr
    int min_num = std::max(1, (int)ceil(base * exp2(min_log2fr)));
    int max_num = (int)floor(base * exp2(max_log2fr));
    
    for (int num = min_num; num <= max_num; num++) {
        PitchSetPitch pitch;
        // Simplify the fraction by dividing by GCD
        int gcd = std::gcd(num, base);
        int simplified_num = num / gcd;
        int simplified_base = base / gcd;
        pitch.label = std::to_string(simplified_num) + ":" + std::to_string(simplified_base);
        pitch.log2fr = -base_log2fr;
        int r = num;
        for (auto p : primes) {
            while (r % p.number == 0) {
                r /= p.number;
                pitch.log2fr += p.log2fr;
            }
        }
        pitch.log2fr += log2(r);
        
        // Filter pitches to ensure they're within the specified range
        if (pitch.log2fr >= min_log2fr - 1e-6 && pitch.log2fr <= max_log2fr + 1e-6) {
            pitchset.push_back(pitch);
        }
    }
    
    std::sort(pitchset.begin(), pitchset.end(), 
        [](PitchSetPitch a, PitchSetPitch b) {
            return a.log2fr < b.log2fr;
        }
    );
    return pitchset;
}


bool sameSet(const PitchSet& a, const PitchSet& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (a[i].label != b[i].label || !sameBits(a[i].log2fr, b[i].log2fr)) return false;
    return true;
}

bool sameLog2s(const std::vector<double>& a, const PitchSet& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (!sameBits(a[i], b[i].log2fr)) return false;
    return true;
}

// The node windows the plugin asks for, and smaller ones.
struct Window { int n_nodes, root; };
const Window kWindows[] = { {128, 60}, {24, 12}, {13, 0}, {7, 6}, {1, 0} };

} // namespace

TEST_CASE("generateMappedScale, mappedNaturalCoords and fromAffine equal the reference path", "[scale][mos][reference]") {
    const double gens[] = { 0.585, 0.5833333, 0.415, 0.4, 0.6, 0.2924, 0.737, 0.1, 0.9, 0.5 };
    const double stretches[] = { 1.0, 1.0016, 0.98, 1.585 };
    int compared = 0;
    for (int depth = 1; depth <= 7; ++depth)
    for (double g : gens)
    for (double e : stretches)
    for (int mode = 0; mode < 4; ++mode)
    for (double tuningShift : { 0.0, 0.003, -0.011 }) {
        MOS mos = MOS::fromG(depth, mode, g, e);
        if (mode > mos.n - 1) continue;
        if (tuningShift != 0.0)
            // Tuning generator off the structure one: impliedAffine and
            // structureImpliedAffine differ.
            mos.adjustTuningG(depth, mode, g + tuningShift, e);
        for (int steps : { 5, 7, 12, 17, 19, 22, 31, 41, 53 })
        for (double offsetFrac : { 0.0, 5.5 / 12, 0.5, 0.97 })
        for (const Window& w : kWindows) {
            const double offset = (steps - 1) * offsetFrac;
            const double base = 261.6255653006;
            const std::vector<Node> ref = refMappedScale(mos, steps, offset, base, w.n_nodes, w.root);
            const Scale mapped = mos.generateMappedScale(steps, offset, base, w.n_nodes, w.root);
            const std::vector<Vector2i> coords = mos.mappedNaturalCoords(steps, offset, w.n_nodes, w.root);
            REQUIRE(mapped.getNodes().size() == ref.size());
            REQUIRE(coords.size() == ref.size());
            bool same = true;
            for (size_t i = 0; i < ref.size(); ++i)
                same = same && sameNode(mapped.getNodes()[i], ref[i]) && coords[i] == ref[i].natural_coord;
            REQUIRE(same);

            // fromAffine (recalcWithAffine) on the same selection transform.
            double mos_offset = (offset + 0.5) / steps;
            AffineTransform squeezed(1, 0, 0, static_cast<double>(mos.n) / steps, 0, 0);
            squeezed = squeezed * mos.structureImpliedAffine;
            squeezed.ty = mos_offset;
            const std::vector<Node> refA = refFromAffine(squeezed, base, w.n_nodes, w.root);
            const Scale fromA = Scale::fromAffine(squeezed, base, w.n_nodes, w.root);
            const std::vector<Vector2i> coordsA = Scale::naturalCoordsFromAffine(squeezed, w.n_nodes, w.root);
            for (size_t i = 0; i < refA.size(); ++i)
                same = same && sameNode(fromA.getNodes()[i], refA[i]) && coordsA[i] == refA[i].natural_coord;
            REQUIRE(same);
            ++compared;
        }
    }
    // The base scale every MOS builds goes through fromAffine as well.
    for (int depth = 1; depth <= 7; ++depth)
    for (double g : gens) {
        MOS mos = MOS::fromG(depth, 1, g, 1.0);
        const std::vector<Node> refBase = refFromAffine(mos.impliedAffine, 1.0, mos.n + 1, 0);
        bool same = mos.base_scale.getNodes().size() == refBase.size();
        for (size_t i = 0; same && i < refBase.size(); ++i)
            same = sameNode(mos.base_scale.getNodes()[i], refBase[i]);
        REQUIRE(same);
    }
    WARN("compared " << compared);
    REQUIRE(compared > 100000);
}

TEST_CASE("The pitch sets and their label-free variants equal the reference builders", "[pitchset][reference]") {
    const std::pair<double, double> ranges[] = { {0.0, 1.0}, {0.0, 1.001}, {-6.0, 7.0416}, {-11.0, 13.0}, {0.25, 0.75} };
    int compared = 0;
    for (auto [lo, hi] : ranges) {
        for (unsigned int n_et = 1; n_et <= 72; ++n_et)
        for (double equave : { 1.0, 1.0016, 1.585, 0.98 }) {
            const PitchSet ref = refETPitchSet(n_et, equave, lo, hi);
            REQUIRE(sameSet(generateETPitchSet(n_et, equave, lo, hi), ref));
            REQUIRE(sameLog2s(generateETPitchLog2s(n_et, equave, lo, hi), ref));
            ++compared;
        }
        for (int base = 1; base <= 64; ++base)
        for (int nPrimes : { 0, 3, 6 }) {
            const PrimeList primes = generateDefaultPrimeList(nPrimes);
            const PitchSet ref = refHarmonicSeriesPitchSet(primes, base, lo, hi);
            REQUIRE(sameSet(generateHarmonicSeriesPitchSet(primes, base, lo, hi), ref));
            REQUIRE(sameLog2s(generateHarmonicSeriesPitchLog2s(primes, base, lo, hi), ref));
            ++compared;
        }
        // The plugin's JI prime choices: any subset of 2..13.
        for (unsigned int mask = 0; mask < 64; ++mask)
        for (int maxNumber : { 1, 2, 5, 8, 13, 20, 33, 50 }) {
            PrimeList primes;
            for (unsigned int i = 0; i < 6; ++i)
                if (mask & (1u << i)) primes.push_back(pseudoPrimeFromIndexNumber(i));
            const PitchSet ref = refJIPitchSet(primes, maxNumber, lo, hi);
            REQUIRE(sameSet(generateJIPitchSet(primes, maxNumber, lo, hi), ref));
            REQUIRE(sameLog2s(generateJIPitchLog2s(primes, maxNumber, lo, hi), ref));
            ++compared;
        }
    }
    WARN("compared " << compared);
    REQUIRE(compared > 4000);
}
