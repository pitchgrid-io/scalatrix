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
    // Structure-based labels (default) — stable when tuning changes
    // accidentalAfter: true = "C♯" (default), false = "♯C" (sheet music convention)
    static std::string nodeLabelDigit(const MOS& mos, Vector2i v, bool accidentalAfter = true);
    static std::string nodeLabelDigitZeroBased(const MOS& mos, Vector2i v, bool accidentalAfter = true);
    static std::string nodeLabelLetter(const MOS& mos, Vector2i v, bool accidentalAfter = true);
    static std::string nodeLabelLetterWithOctaveNumber(const MOS& mos, Vector2i v, int middle_C_octave = 4, bool accidentalAfter = true);

    // Tuning-based labels — follow tuning generator changes
    static std::string nodeLabelDigitTuning(const MOS& mos, Vector2i v, bool accidentalAfter = true);
    static std::string nodeLabelDigitTuningZeroBased(const MOS& mos, Vector2i v, bool accidentalAfter = true);
    static std::string nodeLabelLetterTuning(const MOS& mos, Vector2i v, bool accidentalAfter = true);
    static std::string nodeLabelLetterWithOctaveNumberTuning(const MOS& mos, Vector2i v, int middle_C_octave = 4, bool accidentalAfter = true);

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

    // Meantone-fifth + near-octave window where 5L2s Western spelling applies.
    static bool inDiatonicWindow(const MOS& mos);

    // 12-TET pitch class of the nearest concert pitch to freqHz. C=0 … B=11.
    // Spelling of that class is the unique name on the Db…F# fifths chain.
    static int nearestWesternPitchClass(double freqHz);

    // Map into 5L2s and label with C–G. If baseFreq is given, the origin is
    // spelled as the nearest concert pitch in the Db…F# fifths range.
    // Outside the diatonic window (or override_letter_labels), falls back to digits.
    std::string noteLabelNormalized(MOS& mos, Vector2i v, bool override_letter_labels = false);
    std::string noteLabelNormalized(MOS& mos, Vector2i v, double baseFreq, bool override_letter_labels = false);

    LabelCalculator() : diatonic_mos(MOS::fromParams (5, 2, 1, 1.0, .585)) {}

private:
    MOS diatonic_mos;

    // Helper methods to calculate accidental string
    static std::string accidentalString(const MOS& mos, Vector2i v);
    static std::string accidentalStringTuning(const MOS& mos, Vector2i v);
};

}

#endif
