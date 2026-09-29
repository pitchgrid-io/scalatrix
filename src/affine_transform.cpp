#include "scalatrix/affine_transform.hpp"
#include <cassert>
#include <cmath>


namespace scalatrix {

namespace {

// `p*x + q*y + t`, rounded the way Apple clang does on arm64.
//
// FMA is in the arm64 baseline, so clang contracts that expression to
// fma(p, x, q*y) + t even at -O0. The x86-64 baseline has no FMA, and
// MSVC /fp:precise does not contract, so those platforms do two multiplies
// and an add. The two results differ by an ulp. The scale walk treats
// `0 <= y < 1` as a hard edge, and that ulp was enough to pick a different
// lattice step — Linux and Windows then assigned different MIDI notes than
// macOS. The mapper goldens were generated from the contracted rounding.
// `mul` is that two-product contraction. `mulAdd` adds t afterwards.
// A second fma, fma(p, x, fma(q, y, t)), does not match the goldens.
double mul(double p, double x, double q, double y) {
    return std::fma(p, x, q * y);
}

double mulAdd(double p, double x, double q, double y, double t) {
    return mul(p, x, q, y) + t;
}

} // namespace


IntegerAffineTransform::IntegerAffineTransform(int a_, int b_, int c_, int d_, int tx_, int ty_)
    : a(a_), b(b_), c(c_), d(d_), tx(tx_), ty(ty_) {}

IntegerAffineTransform IntegerAffineTransform::operator*(int s) const {
    return {a * s, b * s, c * s, d * s, tx * s, ty * s};
}

Vector2i IntegerAffineTransform::operator*(const Vector2i& v) const {
    return {a * v.x + b * v.y + tx, c * v.x + d * v.y + ty};
}

Vector2i IntegerAffineTransform::apply(const Vector2i& v) const {
    return {a * v.x + b * v.y + tx, c * v.x + d * v.y + ty};
}

IntegerAffineTransform IntegerAffineTransform::applyAffine(const IntegerAffineTransform& M) const {
    return {a * M.a + b * M.c, a * M.b + b * M.d, c * M.a + d * M.c, c * M.b + d * M.d, a * M.tx + b * M.ty + tx, c * M.tx + d * M.ty + ty};
}

IntegerAffineTransform IntegerAffineTransform::inverse() const {
    int det = a * d - b * c;
    assert(det != 0);
    return {d / det, -b / det, -c / det, a / det, -(d * tx - b * ty) / det, -(a * ty - c * tx) / det};
}

IntegerAffineTransform& IntegerAffineTransform::linearFromTwoDots(
    const Vector2i& a1, const Vector2i& a2,
    const Vector2i& b1, const Vector2i& b2) 
{
    // make sure a1 and a2 are not collinear
    assert(a1.x * a2.y - a1.y * a2.x != 0);
    // make sure b1 and b2 are not collinear
    assert(b1.x * b2.y - b1.y * b2.x != 0);

    static IntegerAffineTransform _self;
    _self.tx=0;
    _self.ty=0;
    // find linear transform that maps a1 to b1 and a2 to b2
    // find the linear transform that maps a1 to b1 and a2 to b2
    int det = a1.x * a2.y - a1.y * a2.x;
    _self.a = (b1.x * a2.y - b2.x * a1.y) / det;
    _self.b = (a1.x * b2.x - b1.x * a2.x) / det;
    _self.c = (b1.y * a2.y - a1.y * b2.y) / det;
    _self.d = (a1.x * b2.y - a2.x * b1.y) / det;

    return _self;
}

AffineTransform::AffineTransform(double a_, double b_, double c_, double d_, double tx_, double ty_)
    : a(a_), b(b_), c(c_), d(d_), tx(tx_), ty(ty_) {}

AffineTransform AffineTransform::operator*(double s) const {
    return {a * s, b * s, c * s, d * s, tx * s, ty * s};
}

Vector2d AffineTransform::operator*(const Vector2d& v) const {
    return {mulAdd(a, v.x, b, v.y, tx), mulAdd(c, v.x, d, v.y, ty)};
}

Vector2d AffineTransform::operator*(const Vector2i& v) const {
    const double x = static_cast<double>(v.x);
    const double y = static_cast<double>(v.y);
    return {mulAdd(a, x, b, y, tx), mulAdd(c, x, d, y, ty)};
}

AffineTransform AffineTransform::operator*(const AffineTransform& M) const {
    return {mul(a, M.a, b, M.c), mul(a, M.b, b, M.d),
            mul(c, M.a, d, M.c), mul(c, M.b, d, M.d),
            mulAdd(a, M.tx, b, M.ty, tx), mulAdd(c, M.tx, d, M.ty, ty)};
}

Vector2d AffineTransform::apply(const Vector2d& v) const {
    return {mulAdd(a, v.x, b, v.y, tx), mulAdd(c, v.x, d, v.y, ty)};
}

//Vector2d AffineTransform::applyInt(const Vector2i& v) const {
//    return {a * v.x + b * v.y + tx, c * v.x + d * v.y + ty};
//}

AffineTransform AffineTransform::applyAffine(const AffineTransform& M) const {
    return {mul(a, M.a, b, M.c), mul(a, M.b, b, M.d),
            mul(c, M.a, d, M.c), mul(c, M.b, d, M.d),
            mulAdd(a, M.tx, b, M.ty, tx), mulAdd(c, M.tx, d, M.ty, ty)};
}

AffineTransform AffineTransform::inverse() const {
    double det = a * d - b * c;
    assert(std::abs(det) > 1e-7);
    return {d / det, -b / det, -c / det, a / det, -(d * tx - b * ty) / det, -(a * ty - c * tx) / det};
}





} // namespace scalatrix