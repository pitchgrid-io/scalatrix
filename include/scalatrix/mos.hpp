#ifndef SCALATRIX_MOS_HPP
#define SCALATRIX_MOS_HPP

#include "scalatrix/affine_transform.hpp"
#include "scalatrix/scale.hpp"
#include "scalatrix/pitchset.hpp"

namespace scalatrix {

class MOS {
public:

    MOS(int a, int b, int m, double e, double g);

    int a, b, n, a0, b0, n0, mode, nL, nS;
    int repetitions, depth;
    double equave; // log2(frequency ratio) of equave (interval of equivalence)
    double period; // log2(frequency ratio) of MOS period
    double generator;

    Vector2i L_vec, s_vec, chroma_vec;
    double L_fr, s_fr, chroma_fr;

    std::vector<bool> path;
    AffineTransform impliedAffine;
    IntegerAffineTransform mosTransform;
    Vector2i v_gen;
    Scale base_scale;


    static MOS fromParams(int a, int b, int m, double e, double g);
    //static MOS fromImpliedAffine(const AffineTransform& A, int repetitions);
    static MOS fromG(int depth, int m, double g, double e, int repetitions = 1);
    void adjustG(int depth, int m, double g, double e, int repetitions = 1);
    void adjustParams(int a, int b, int m, double e, double g);
    //void adjustParamsFromImpliedAffine(const AffineTransform& A);

    double coordToFreq(double x, double y, double base_freq);

    double angle() const;
    double angleStd() const;

    AffineTransform calcImpliedAffine() const;
    void updateVectors();

    double gFromAngle(double angle);

    // Deprecated: Label methods - use LabelCalculator instead
    // These are kept for backward compatibility
    std::string nodeLabelDigit(Vector2i v) const;
    std::string nodeLabelLetter(Vector2i v) const;
    std::string nodeLabelLetterWithOctaveNumber(Vector2i v, int middle_C_octave=4) const;

    void _recalcOnRetuneUsingAffine(AffineTransform& A);

    void retuneZeroPoint();
    void retuneOnePoint(Vector2i v, double log2fr);
    void retuneTwoPoints(Vector2i fixed, Vector2i v, double log2fr);
    void retuneThreePoints(Vector2i fixed1, Vector2i fixed2, Vector2i v, double log2fr);

    Scale generateScaleFromMOS(double base_freq, int n, int root);
    void retuneScaleWithMOS(Scale& scale, double base_freq);

    Vector2i mapFromMOS(MOS& other, Vector2i v);

    int nodeEquaveNr(Vector2i v) const {return (v.x + v.y + 256*n) / n - 256;}
    bool nodeInScale(Vector2i v) const;
    
    // Scale degree (0 to n-1) within the period
    int nodeScaleDegree(Vector2i v) const {return (v.x + v.y + 256*n) % n;}
    bool nodeInScale(Vector2i v) const;

    // Accidental count (positive=sharp, negative=flat in bright-generator convention)
    int nodeAccidental(Vector2i v) const;
    
    // Convert notation (step, alter, octave) to MOS coordinates (a, b)
    // step: scale degree 0 to n-1
    // alter: accidental count (positive=sharp, negative=flat)
    // octave: octave number (0 = base octave)
    Vector2i mosCoordFromNotation(int step, int alter, int octave) const;

};

} // namespace scalatrix

#endif // SCALATRIX_MOS_HPP