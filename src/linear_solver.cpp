#include "scalatrix/linear_solver.hpp"
#include <cmath>
#include <stdexcept>
#include <algorithm>

namespace scalatrix {

std::array<double, 6> LinearSolver6x6::solve(const std::array<std::array<double, 6>, 6>& A, 
                                              const std::array<double, 6>& b) {
    // Create working copies
    std::array<std::array<double, 6>, 6> matrix = A;
    std::array<double, 6> rhs = b;
    
    // Gaussian elimination with partial pivoting
    for (int k = 0; k < 6; ++k) {
        // Find pivot
        int pivot_row = k;
        double max_val = std::abs(matrix[k][k]);
        
        for (int i = k + 1; i < 6; ++i) {
            if (std::abs(matrix[i][k]) > max_val) {
                max_val = std::abs(matrix[i][k]);
                pivot_row = i;
            }
        }
        
        // Check for singular matrix. Desktop builds throw (unchanged); embedded
        // builds compiled with -fno-exceptions return a zero fallback so the
        // library links on MCUs (e.g. Teensy 4) where C++ exceptions are not
        // available. A degenerate matrix arises only from degenerate inputs.
        if (max_val < 1e-10) {
#if defined(__cpp_exceptions) && __cpp_exceptions
            throw std::runtime_error("Matrix is singular or nearly singular");
#else
            return std::array<double, 6>{};
#endif
        }
        
        // Swap rows if needed
        if (pivot_row != k) {
            std::swap(matrix[k], matrix[pivot_row]);
            std::swap(rhs[k], rhs[pivot_row]);
        }
        
        // Eliminate column k below diagonal
        for (int i = k + 1; i < 6; ++i) {
            double factor = matrix[i][k] / matrix[k][k];
            for (int j = k + 1; j < 6; ++j) {
                matrix[i][j] -= factor * matrix[k][j];
            }
            rhs[i] -= factor * rhs[k];
            matrix[i][k] = 0; // For numerical stability
        }
    }
    
    // Back substitution
    std::array<double, 6> solution;
    for (int i = 5; i >= 0; --i) {
        solution[i] = rhs[i];
        for (int j = i + 1; j < 6; ++j) {
            solution[i] -= matrix[i][j] * solution[j];
        }
        solution[i] /= matrix[i][i];
    }
    
    return solution;
}

} // namespace scalatrix