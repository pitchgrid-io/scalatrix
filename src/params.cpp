#include "scalatrix/params.hpp"
#include "scalatrix/linear_solver.hpp"
#include <cassert>

namespace scalatrix {


AffineTransform affineFromThreeDots(
    const Vector2d& a1, const Vector2d& a2, const Vector2d& a3,
    const Vector2d& b1, const Vector2d& b2, const Vector2d& b3) {
    // Set up the 6x6 matrix
    std::array<std::array<double, 6>, 6> M = {{
        {a1.x, a1.y, 1, 0, 0, 0},
        {0, 0, 0, a1.x, a1.y, 1},
        {a2.x, a2.y, 1, 0, 0, 0},
        {0, 0, 0, a2.x, a2.y, 1},
        {a3.x, a3.y, 1, 0, 0, 0},
        {0, 0, 0, a3.x, a3.y, 1}
    }};

    // Set up the right-hand side vector
    std::array<double, 6> b = {b1.x, b1.y, b2.x, b2.y, b3.x, b3.y};

    // Solve the system
    std::array<double, 6> sol = LinearSolver6x6::solve(M, b);

    // Extract the affine transformation parameters
    AffineTransform res = AffineTransform(sol[0], sol[1], sol[3], sol[4], sol[2], sol[5]);

    return res;

};

AffineTransform affineFromMOSParams(int /*a*/, int /*b*/, int /*m*/, double /*e*/, double /*r*/) {
    // Stub — not yet implemented
    return AffineTransform(1, 0, 0, 1, 0, 0);
}

} // namespace scalatrix