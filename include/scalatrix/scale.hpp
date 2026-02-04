#ifndef SCALATRIX_SCALE_HPP
#define SCALATRIX_SCALE_HPP

#include "lattice.hpp"
#include "affine_transform.hpp"
#include "pitchset.hpp"
#include "node.hpp"
#include <string>
#include <vector>

const double DEFAULT_12TET_C_PITCH = 261.6255653006;

namespace scalatrix {

/**
 * Scale represents a collection of musical notes generated from a 2D lattice.
 * 
 * The fundamental concept: "A scale is a path on a 2D lattice"
 * 
 * Scales are generated using the fromAffine method which implements this concept by:
 * 1. Applying an affine transform to redistribute 2D integer lattice nodes in space
 * 2. Selecting all nodes that fall within the horizontal strip 0 ≤ y < 1 after transformation
 * 3. Ordering these nodes by increasing x-coordinate to form the sequential scale path
 * 4. Normalizing by constraining transforms so origin (0,0) maps to line segment (x=0, 0≤y<1)
 * 
 * This approach generates scales by "slicing" through the transformed lattice with a 
 * horizontal strip, creating a sequential path that respects the underlying mathematical structure.
 */
class Scale {
private:
    std::vector<Node> nodes_;
    double base_freq_;
    int root_idx_;
    void initNodes(int N);
public:
    Scale(double base_freq = DEFAULT_12TET_C_PITCH, int N = 128, int root_node_idx = 60);
    
    /**
     * Core method implementing "scale as path on lattice"
     * 
     * Generates a scale by applying an affine transformation to a 2D integer lattice,
     * then selecting nodes within the horizontal strip 0 ≤ y < 1 and ordering them
     * by x-coordinate to form a sequential path.
     * 
     * @param M Affine transform that redistributes lattice nodes (must map origin to (x=0, 0≤y<1))
     * @param base_freq Base frequency for the scale
     * @param N Number of nodes to generate
     * @param n_root Index of the root node in the generated scale
     * @return Scale object containing the generated path of nodes
     */
    static Scale fromAffine(const AffineTransform& M, const double base_freq, int N, int n_root);

    void print(int first = 58, int num = 5) const;
    std::vector<Node>& getNodes();
    const std::vector<Node>& getNodes() const;
    void recalcWithAffine(const AffineTransform& A, int N, int n_root);
    void retuneWithAffine(const AffineTransform& A);
    int getRootIdx() const { return root_idx_; }
    void temperToPitchSet(PitchSet& pitchset);
    double getBaseFreq() const { return base_freq_; }

    ~Scale();

};

} // namespace scalatrix

#endif // SCALATRIX_SCALE_HPP
