#include "scalatrix/mos.hpp"
#include "scalatrix/params.hpp" 
#include "scalatrix/label_calculator.hpp"

#include <cmath>
#include <vector>
#include <algorithm>
#include <cassert>
#include <iostream>

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

namespace scalatrix {


MOS::MOS(int a, int b, int m, double e, double g) {
    adjustParams(a, b, m, e, g);
}


int gcd(int a, int b) {
    if (b == 0) return a;
    return gcd(b, a % b);
}

double MOS::angleStd() const {
    // inverse of
    // double generator = 1.0 / (1.0 + tan((1 - angle) * M_PI * .5)); 
    

    double angle = 0.0;
    if (generator > 0.0) { 
        angle = M_PI_2 - atan2(1.0 / generator - 1.0, 1.0); 
    }
    return angle;
}

double MOS::angle() const {
    double angle = angleStd();
    for (int d = 0; d < path.size(); d++) {
        if (path[d]) {
            angle = atan2(tan(angle)-1, 1);
        }else{
            angle = atan2(1, 1/tan(angle)-1);
        }
    }
    return angle;
}

std::vector<bool> calcPath(int a, int b){
    std::vector<bool> path;
    int a_ = a;
    int b_ = b;
    while (a_ > 1 || b_ > 1) {
        if (a_ > b_) {
            a_ -= b_;
            path.push_back(false);
        } else {
            b_ -= a_;
            path.push_back(true);
        }
    }
    std::reverse(path.begin(), path.end());
    return path;
}


Vector2i applyPath(const std::vector<bool> path, const Vector2i& v) {
    int a = v.x;
    int b = v.y;
    for (bool p : path) {
        if (p) {
            b += a;
        } else {
            a += b;
        }
    }
    return {a,b};
}

Vector2i applyPathReverse(const std::vector<bool> path, const Vector2i& v) {
    int a = v.x;
    int b = v.y;
    std::vector<bool> reversed_path = path;
    std::reverse(reversed_path.begin(), reversed_path.end());
    for (bool p : reversed_path) {
        if (p) {
            b -= a;
        } else {
            a -= b;
        }
    }
    return {a,b};
}


MOS MOS::fromParams(int a, int b, int m, double e, double g){
    return MOS(a, b, m, e, g);
}

void MOS::updateVectors(){
    Vector2i v1 = {1, 0};
    Vector2i v2 = {0, 1};
    double v1_fr = this->impliedAffine.apply(v1).x;
    double v2_fr = this->impliedAffine.apply(v2).x;
    if (v1_fr > v2_fr) {
        this->L_vec = v1;
        this->s_vec = v2;
        this->L_fr = v1_fr;
        this->s_fr = v2_fr;
        this->nL = a;
        this->nS = b;
    } else {
        this->L_vec = v2;
        this->s_vec = v1;
        this->L_fr = v2_fr;
        this->s_fr = v1_fr;
        this->nL = b;
        this->nS = a;
    }
    this->chroma_vec = this->L_vec - this->s_vec;
    this->chroma_fr = this->L_fr - this->s_fr;
}

double MOS::coordToFreq(double x, double y, double base_freq){
    return base_freq * std::exp2((this->impliedAffine * Vector2d(x,y)).x);
}

void MOS::adjustParams(int a, int b, int m, double e, double g){
    assert(a > 0);
    assert(b > 0);
    assert(0.0 <= g && g <= 1.0);

    int n = a + b;
    int r = gcd(a, b);
    int a0 = a / r;
    int b0 = b / r;
    int n0 = a0 + b0;

    this->a = a;
    this->b = b;
    this->n = n;
    this->a0 = a0;
    this->b0 = b0;
    this->n0 = n0;
    this->mode = m;
    this->repetitions = r;
    
    this->equave = e;
    this->period = e / r;
    this->generator = g;

    this->path = calcPath(a0, b0);
    this->depth = this->path.size();
    this->v_gen = applyPath(this->path, {1, 0});
    this->impliedAffine = calcImpliedAffine();

    this->updateVectors();

    this->base_scale = Scale::fromAffine(this->impliedAffine, 1.0, n+1, 0);


    this->mosTransform = IntegerAffineTransform::linearFromTwoDots(
        {1, 0}, {1, 1},
        v_gen, {a0, b0}
    );
}

void MOS::adjustG(int depth, int m, double g, double e, int _repetitions){
    int a0 = 1;
    int b0 = 1;
    double a_len = g;
    double b_len = 1.0 - g;
    for (int i = 0; i < depth; i++) {
        if (a_len > b_len) {
            b0+=a0;
            a_len -= b_len;
        } else {
            a0+=b0;
            b_len -= a_len;
        }
    }
    this->adjustParams(a0*_repetitions, b0*_repetitions, m, e, g);
}

MOS MOS::fromG(int depth, int m, double g, double e, int repetitions){
    int a0 = 1;
    int b0 = 1;
    double a_len = g;
    double b_len = 1.0 - g;
    for (int i = 0; i < depth; i++) {
        if (a_len > b_len) {
            b0+=a0;
            a_len -= b_len;
        } else {
            a0+=b0;
            b_len -= a_len;
        }
    }
    return MOS(a0*repetitions, b0*repetitions, m, e, g);
}

double MOS::gFromAngle(double angle){
    std::vector<bool> reversed_path = path;
    std::reverse(reversed_path.begin(), reversed_path.end());

    double angle_ = angle;
    for (int d = 0; d < reversed_path.size(); d++) {
        if (reversed_path[d]) {
            angle_ = atan2(tan(angle_)+1, 1);
        }else{
            angle_ = atan2(1, 1/tan(angle_)+1);
        }
    }

    // print the angle
    //std::cout << "Calculated AngleSTD: " << M_2_PI * angle_ << "\n";

    return 1.0 / (1.0 + tan((M_PI_2 - angle_)));
}


//static MOS& fromImpliedAffine(const AffineTransform& A, int repetitions){
//    static MOS _self;
//    _self.repetitions = repetitions;
//    _self.adjustParamsFromImpliedAffine(A);
//    return _self;
//};
//
//void adjustParamsFromImpliedAffine(const AffineTransform& A, int repetitions){
//
//}

AffineTransform MOS::calcImpliedAffine() const {
    double q = 0.5/n0;
    return affineFromThreeDots(
        {0,0}, {(double)v_gen.x, (double)v_gen.y}, {(double)a0,(double)b0},
        {0,q*(2*mode+1)}, {generator * period, q*(2*mode+3)}, {period, q*(2*mode+1)}
    );
};




void MOS::_recalcOnRetuneUsingAffine(AffineTransform& A){
    this->base_scale.retuneWithAffine(A);
    this->impliedAffine = A;
    this->equave = A.apply(Vector2i(a,b)).x - A.apply(Vector2i(0,0)).x;
    this->period = A.apply(Vector2i(a0,b0)).x - A.apply(Vector2i(0,0)).x;
    this->generator = (A.apply(this->v_gen).x - A.apply(Vector2i(0,0)).x) / this->period;
    this->updateVectors();
};

void MOS::retuneZeroPoint(){
    // use to undo tempering
    _recalcOnRetuneUsingAffine(this->impliedAffine);
}
void MOS::retuneOnePoint(Vector2i v, double log2fr){
    // shift frequencies so v matches log2fr
    double shift = log2fr - (this->impliedAffine * v).x;
    AffineTransform A = this->impliedAffine;
    A.tx += shift;
    _recalcOnRetuneUsingAffine(A);
};
void MOS::retuneTwoPoints(Vector2i fixed, Vector2i v, double log2fr){
    // rescale frequencies so fixed matches its original frequency and v matches log2fr
    AffineTransform A = this->impliedAffine;
    AffineTransform B = AffineTransform();
    double fixed_log2fr = (A * fixed).x;
    double scale = (log2fr - fixed_log2fr)/((A * v).x - fixed_log2fr);
    B.a = scale;
    A = B * A;
    double shift = fixed_log2fr - (A * fixed).x;
    A.tx = shift;
    _recalcOnRetuneUsingAffine(A);
};
void MOS::retuneThreePoints(Vector2i fixed1, Vector2i fixed2, Vector2i v, double log2fr){
    // rescale frequencies so fixed1 and fixed2 match original frequencies, and v matches log2fr
    Vector2d fixed1_tuning = this->impliedAffine * fixed1;
    Vector2d fixed2_tuning = this->impliedAffine * fixed2;
    Vector2d v_tuning = this->impliedAffine * v;
    v_tuning.x = log2fr;
    AffineTransform A = affineFromThreeDots(
        Vector2d(fixed1), Vector2d(fixed2), Vector2d(v),
        fixed1_tuning, fixed2_tuning, v_tuning
    );
    _recalcOnRetuneUsingAffine(A);
};

Scale MOS::generateScaleFromMOS(double base_freq, int n_nodes, int root){
    Scale scale = Scale(base_freq, n_nodes, root);
    for (int i=-root; i<n_nodes-root; i++){
        int idx = (i +  128*n) % n;
        int octave_nr = (i + 128*n) / n - 128;
        Node& ref = this->base_scale.getNodes()[idx];
        Node& node = scale.getNodes()[i+root];
        node.natural_coord = (Vector2i(a,b) * octave_nr) + ref.natural_coord;
        node.tuning_coord = this->impliedAffine * node.natural_coord;
        node.tuning_coord.x = ref.tuning_coord.x + octave_nr * this->equave;
        node.pitch = base_freq * exp2(node.tuning_coord.x);
        node.isTempered = ref.isTempered;
        node.temperedPitch = ref.temperedPitch;
    }
    return scale;
};

void MOS::retuneScaleWithMOS(Scale& scale, double base_freq){
    //int n_nodes = scale.getNodes().size();
    int root_idx = scale.getRootIdx();
    for (int i = 0; i < scale.getNodes().size(); i++) {
        int idx = (i - root_idx + 128*n) % n;
        int octave_nr = (i - root_idx + 128*n) / n - 128;
        Node& ref = this->base_scale.getNodes()[idx];
        Node& node = scale.getNodes()[i];
        node.tuning_coord.x = ref.tuning_coord.x + octave_nr * this->equave;
        node.pitch = base_freq * std::exp2(node.tuning_coord.x);
        node.isTempered = ref.isTempered;
        node.temperedPitch = ref.temperedPitch;
    }
};


Vector2i MOS::mapFromMOS(MOS& other, Vector2i v){
    Vector2i result = applyPathReverse(other.path, v);
    result = applyPath(path, result);
    return result;
}

bool MOS::nodeInScale(Vector2i v) const{
    // check if the node is in the scale
    int d = v.x * b - v.y * a + mode;
    if (d < 0 || d >= n) {
        return false;
    }
    return true;
}

// Deprecated methods - forwarding to LabelCalculator
std::string MOS::nodeLabelDigit(Vector2i v) const {
    return LabelCalculator::nodeLabelDigit(*this, v);
}

std::string MOS::nodeLabelLetter(Vector2i v) const {
    return LabelCalculator::nodeLabelLetter(*this, v);
}

std::string MOS::nodeLabelLetterWithOctaveNumber(Vector2i v, int middle_C_octave) const {
    return LabelCalculator::nodeLabelLetterWithOctaveNumber(*this, v, middle_C_octave);
}

} // namespace scalatrix
