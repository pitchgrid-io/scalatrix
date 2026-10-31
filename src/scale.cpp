#include "scalatrix/scale.hpp"
#include "scalatrix/lattice.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

namespace scalatrix {

void Scale::initNodes(int N){
    nodes_.clear();
    nodes_.reserve(N);
    for (int i = 0; i < N; ++i) {
        nodes_.push_back(Node());
    }
}

Scale::Scale(double base_freq, int N, int root_node_idx) : base_freq_(base_freq), root_idx_(root_node_idx) {
    initNodes(N);
}  

/**
 * Core implementation of "a scale is a path on a 2D lattice"
 * 
 * This method implements the fundamental scalatrix concept by:
 * 1. Applying the affine transform to redistribute lattice nodes in 2D space
 * 2. Finding nodes that fall within the horizontal strip 0 ≤ y < 1 after transformation
 * 3. Ordering these nodes by x-coordinate to create the sequential scale path
 * 4. The transform must be normalized so origin (0,0) maps to (x=0, 0≤y<1)
 */
/*static*/ 
Scale Scale::fromAffine(const AffineTransform& A, const double base_freq, int N, int root_node_idx) {
    Scale scale(base_freq, N, root_node_idx);
    scale.recalcWithAffine(A, N, root_node_idx);
    return scale;
}

/**
 * Generates scale nodes by applying affine transform and selecting from horizontal strip
 * 
 * This method implements the core algorithm:
 * 1. Apply transform A to redistribute lattice nodes
 * 2. Find lattice vectors r,s that map closest to strip boundaries
 * 3. Use 3-gap theorem to generate nodes within strip 0 ≤ y < 1
 * 4. Order resulting nodes by x-coordinate to form sequential scale path
 */
// The strip walk of recalcWithAffine: visit(index, natural_coord,
// tuning_coord) for the root, then the forward and the backward pass.
// recalcWithAffine and naturalCoordsFromAffine share it, so the nodes they
// select cannot drift apart.
template <typename Visit>
static void walkStrip(const AffineTransform& A, int N, int root_node_idx, Visit&& visit) {

    AffineTransform M = AffineTransform(A);
    M.tx = 0;
    M.ty = 0;
    auto [r, s] = findClosestWithinStrip(M);

    Vector2d zr = M * r;
    Vector2d zs = M * s;

    //printf("r = (%d, %d) -> (%f, %f)\n", r.x, r.y, zr.x, zr.y);
    //printf("s = (%d, %d) -> (%f, %f)\n", s.x, s.y, zs.x, zs.y);

    int n_min = -root_node_idx, n_max = N - root_node_idx;

    // Generate nodes within the strip 0 ≤ y < 1 using the 3-gap theorem
    // This creates the sequential scale path by selecting lattice nodes that
    // fall within the horizontal strip after transformation
    const Vector2i root_natural(0, 0);
    const Vector2d root_tuning = A * root_natural;
    visit(root_node_idx, root_natural, root_tuning);

    Vector2i natural = root_natural;
    Vector2d tuning = root_tuning;

    // forward pass
    for (int n = 1; n < n_max; ++n) {
        if (0 <= tuning.y + zr.y && tuning.y + zr.y < 1) {
            natural += r;
        } else if (0 <= tuning.y + zs.y && tuning.y + zs.y < 1) {
            natural += s;
        } else {
            natural += (r + s);
        }
        tuning = A * natural;
        visit(root_node_idx + n, natural, tuning);
    }
    //std::cout << "Length of right: " << right.size() << "\n";

    // backward pass
    natural = root_natural;
    tuning = root_tuning;
    for (int n = -1; n >= n_min; --n) {
        if (0 <= tuning.y - zr.y && tuning.y - zr.y < 1) {
            natural -= r;
        } else if (0 <= tuning.y - zs.y && tuning.y - zs.y < 1) {
            natural -= s;
        } else {
            natural -= (r + s);
        }
        tuning = A * natural;
        visit(root_node_idx + n, natural, tuning);
    }
}

void Scale::recalcWithAffine(const AffineTransform& A, int N, int root_node_idx) {
    walkStrip(A, N, root_node_idx, [&](int i, const Vector2i& natural, const Vector2d& tuning) {
        Node node;
        node.natural_coord = natural;
        node.tuning_coord = tuning;
        // The root keeps the base frequency itself, as before.
        node.pitch = i == root_node_idx ? base_freq_ : base_freq_ * std::exp2(tuning.x);
        nodes_[i] = node;
    });
}

void Scale::recalcCoordsWithAffine(const AffineTransform& A, int N, int root_node_idx) {
    walkStrip(A, N, root_node_idx, [&](int i, const Vector2i& natural, const Vector2d& tuning) {
        Node node;
        node.natural_coord = natural;
        node.tuning_coord = tuning;
        nodes_[i] = node;
    });
}

/*static*/
std::vector<Vector2i> Scale::naturalCoordsFromAffine(const AffineTransform& A, int N, int root_node_idx) {
    std::vector<Vector2i> coords(static_cast<size_t>(N));
    walkStrip(A, N, root_node_idx, [&](int i, const Vector2i& natural, const Vector2d&) {
        coords[static_cast<size_t>(i)] = natural;
    });
    return coords;
}

void Scale::retuneWithAffine(const AffineTransform& A) {
    for (size_t n = 0; n < nodes_.size(); ++n) {
        Node& node = nodes_[n];
        node.tuning_coord = A * node.natural_coord;
        node.pitch = base_freq_ * std::exp2(node.tuning_coord.x);
        node.isTempered = false;
    }
}


void Scale::print(int first, int num) const {
    for (int i = first; i < first + num; ++i) {
        if (i < 0 || static_cast<size_t>(i) >= nodes_.size()) {
            std::cout << "Node " << i << " out of range\n";
        } else {
            const Node& node = nodes_[i];
            
            // print natural coordinates, tuning coordinates, and pitch in single line
            std::cout.precision(3);
            std::cout << "Node " << i << ": (" << node.natural_coord.x << ", " << node.natural_coord.y << ") -> ("
                      << node.tuning_coord.x << ", " << node.tuning_coord.y << ") ";

            std::cout.precision(5);     
            std::cout << node.pitch << " Hz";

            if (node.isTempered) {
                std::cout << " (" << node.temperedPitch.label << ")";
            }
            std::cout << "\n";

        }
    }
}

void Scale::temperToPitchSet(PitchSet& pitchset){
    // find the closest pitch in pitchset to each node in base_scale
    for (auto& node : nodes_) {
        double node_pitch_log2fr = log2(node.pitch/base_freq_);
        double closest_pitch_log2fr = 0.0;
        double min_dist = 1e6;
        PitchSetPitch closest_pitch;
        for (auto& pitch : pitchset) {
            double dist = std::abs(pitch.log2fr - node_pitch_log2fr);
            if (dist < min_dist) {
                min_dist = dist;
                closest_pitch_log2fr = pitch.log2fr;
                closest_pitch = pitch;
            }
        }
        //node.tuning_coord.x = closest_pitch_log2fr;
        node.pitch = base_freq_ * exp2(closest_pitch_log2fr);
        node.isTempered = true;
        node.temperedPitch = closest_pitch;
        node.closestPitch = closest_pitch;
    }
};


std::vector<Node>& Scale::getNodes(){
    return nodes_;
}

const std::vector<Node>& Scale::getNodes() const {
    return nodes_;
}


} // namespace scalatrix