#ifndef SCALATRIX_LABEL_CALCULATOR_HPP
#define SCALATRIX_LABEL_CALCULATOR_HPP

#include <string>
#include <cmath>
#include <cstdio>
#include "scalatrix/mos.hpp"
#include "scalatrix/node.hpp"

namespace scalatrix {

class LabelCalculator {
public:
    static std::string nodeLabelDigit(const MOS& mos, Vector2i v);
    static std::string nodeLabelLetter(const MOS& mos, Vector2i v);
    static std::string nodeLabelLetterWithOctaveNumber(const MOS& mos, Vector2i v, int middle_C_octave = 4);
    
    /**
     * Generate a label showing the pitch with optional deviation in cents.
     * 
     * @param node The node to generate label for
     * @param thresholdCents If deviation is less than this, show plain label (default 0.1)
     * @param compareWithTempered If true, compare tempered pitch with closest; if false, compare tuning_coord with closest
     * @return String with format "label" or "label+/-XX.Xct" depending on deviation
     */
    static std::string deviationLabel(const Node& node, double thresholdCents = 0.1, 
                                      bool compareWithTempered = false);

    std::string noteLabelNormalized(MOS& mos, Vector2i v, bool override_letter_labels = false) {
        if (mos.generator > 4.0/7 && mos.generator < 3.0/5 && mos.equave > 0.9 && mos.equave < 1.2 && !override_letter_labels)
        {
            Vector2i diatonic_coord = diatonic_mos.mapFromMOS (mos, v);
            return nodeLabelLetter(diatonic_mos, diatonic_coord);
        }
        return nodeLabelDigit(mos, v);
    }

    LabelCalculator() : diatonic_mos(MOS::fromParams (5, 2, 1, 1.0, .585)) {}

private:
    MOS diatonic_mos;
    
    // Helper method to calculate accidental string
    static std::string accidentalString(const MOS& mos, Vector2i v);
};

}

#endif