/**
 * Scalatrix - A C++ library for microtonal scale generation and tuning
 * 
 * Core Concept: "A scale is a path on a 2D lattice"
 * 
 * Scalatrix implements this fundamental idea through the Scale::fromAffine method:
 * 1. Apply an affine transform to redistribute 2D integer lattice nodes in space
 * 2. Select all nodes that fall within the horizontal strip 0 ≤ y < 1 after transformation  
 * 3. Order these nodes by increasing x-coordinate to form the sequential scale path
 * 4. Normalize by constraining transforms so origin (0,0) maps to line segment (x=0, 0≤y<1)
 * 
 * This approach generates scales by "slicing" through the transformed lattice, creating
 * sequential paths that respect the underlying mathematical structure while supporting
 * arbitrary regular temperaments and tuning systems.
 */
#ifndef SCALATRIX_HPP
#define SCALATRIX_HPP

#include "scalatrix/affine_transform.hpp"
#include "scalatrix/lattice.hpp"
#include "scalatrix/node.hpp"
#include "scalatrix/scale.hpp"
#include "scalatrix/params.hpp"
#include "scalatrix/mos.hpp"
#include "scalatrix/pitchset.hpp"
#include "scalatrix/label_calculator.hpp"
#include "scalatrix/spectrum.hpp"
#include "scalatrix/consonance.hpp"


#endif // SCALATRIX_HPP
