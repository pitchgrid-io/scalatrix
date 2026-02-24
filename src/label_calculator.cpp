#include "scalatrix/label_calculator.hpp"

namespace scalatrix {

// ── Structure-based accidental (uses structure_L_vec) ────────────────────────

std::string LabelCalculator::accidentalString(const MOS& mos, Vector2i v) {
    int acc_sign = mos.structure_L_vec.x == 1 ? 1 : -1;
    int neutral_mode = mos.structure_L_vec.x == 1 ? 1 : mos.n0 - 2;
    int n_generators = v.x * mos.b0 - v.y * mos.a0;
    int acc = acc_sign * floor((n_generators + neutral_mode + 0.5) / mos.n0);
    std::string result = "";
    if (acc != 0) {
        while (acc < 0) {
            acc += 1;
            result += "\xe2\x99\xad"; // UTF-8 bytes for ♭ (U+266D)
        }
        while (acc > 0) {
            acc -= 1;
            result += "\xe2\x99\xaf"; // UTF-8 bytes for ♯ (U+266F)
        }
    }
    return result;
}

// ── Tuning-based accidental (uses L_vec) ─────────────────────────────────────

std::string LabelCalculator::accidentalStringTuning(const MOS& mos, Vector2i v) {
    int acc_sign = mos.L_vec.x == 1 ? 1 : -1;
    int neutral_mode = mos.L_vec.x == 1 ? 1 : mos.n0 - 2;
    int n_generators = v.x * mos.b0 - v.y * mos.a0;
    int acc = acc_sign * floor((n_generators + neutral_mode + 0.5) / mos.n0);
    std::string result = "";
    if (acc != 0) {
        while (acc < 0) {
            acc += 1;
            result += "\xe2\x99\xad"; // UTF-8 bytes for ♭ (U+266D)
        }
        while (acc > 0) {
            acc -= 1;
            result += "\xe2\x99\xaf"; // UTF-8 bytes for ♯ (U+266F)
        }
    }
    return result;
}

// ── Structure-based labels (default) ─────────────────────────────────────────

std::string LabelCalculator::nodeLabelDigit(const MOS& mos, Vector2i v, bool accidentalAfter) {
    int dia = (v.x + v.y + 128*mos.n) % mos.n;
    std::string deg = std::to_string(dia+1);
    std::string acc = accidentalString(mos, v);
    return accidentalAfter ? deg + acc : acc + deg;
}

std::string LabelCalculator::nodeLabelDigitZeroBased(const MOS& mos, Vector2i v, bool accidentalAfter) {
    int dia = (v.x + v.y + 128*mos.n) % mos.n;
    std::string deg = std::to_string(dia);
    std::string acc = accidentalString(mos, v);
    return accidentalAfter ? deg + acc : acc + deg;
}

std::string LabelCalculator::nodeLabelLetter(const MOS& mos, Vector2i v, bool accidentalAfter) {
    int dia = (v.x + v.y + 2 + 128*mos.n) % mos.n;
    std::string letter(1, 'A' + dia);
    std::string acc = accidentalString(mos, v);
    return accidentalAfter ? letter + acc : acc + letter;
}

std::string LabelCalculator::nodeLabelLetterWithOctaveNumber(const MOS& mos, Vector2i v, int middle_C_octave, bool accidentalAfter) {
    std::string result = nodeLabelLetter(mos, v, accidentalAfter);
    int octave = middle_C_octave + floor((.0 + v.x + v.y) / mos.n);
    result += std::to_string(octave);
    return result;
}

// ── Tuning-based labels ──────────────────────────────────────────────────────

std::string LabelCalculator::nodeLabelDigitTuning(const MOS& mos, Vector2i v, bool accidentalAfter) {
    int dia = (v.x + v.y + 128*mos.n) % mos.n;
    std::string deg = std::to_string(dia+1);
    std::string acc = accidentalStringTuning(mos, v);
    return accidentalAfter ? deg + acc : acc + deg;
}

std::string LabelCalculator::nodeLabelDigitTuningZeroBased(const MOS& mos, Vector2i v, bool accidentalAfter) {
    int dia = (v.x + v.y + 128*mos.n) % mos.n;
    std::string deg = std::to_string(dia);
    std::string acc = accidentalStringTuning(mos, v);
    return accidentalAfter ? deg + acc : acc + deg;
}

std::string LabelCalculator::nodeLabelLetterTuning(const MOS& mos, Vector2i v, bool accidentalAfter) {
    int dia = (v.x + v.y + 2 + 128*mos.n) % mos.n;
    std::string letter(1, 'A' + dia);
    std::string acc = accidentalStringTuning(mos, v);
    return accidentalAfter ? letter + acc : acc + letter;
}

std::string LabelCalculator::nodeLabelLetterWithOctaveNumberTuning(const MOS& mos, Vector2i v, int middle_C_octave, bool accidentalAfter) {
    std::string result = nodeLabelLetterTuning(mos, v, accidentalAfter);
    int octave = middle_C_octave + floor((.0 + v.x + v.y) / mos.n);
    result += std::to_string(octave);
    return result;
}

// ── Deviation label ──────────────────────────────────────────────────────────

std::string LabelCalculator::deviationLabel(const Node& node, double thresholdCents,
                                            bool compareWithTempered) {
    // Select which pitch to use as reference
    const PitchSetPitch& referencePitch = node.closestPitch;
    
    // If no reference pitch is set, return empty string
    if (referencePitch.label.empty()) {
        return "";
    }
    
    // Calculate the actual pitch of this node
    double actualPitchLog2fr = compareWithTempered ? node.temperedPitch.log2fr : node.tuning_coord.x;
    
    // Calculate deviation in cents
    double deviationCents = 1200.0 * (actualPitchLog2fr - referencePitch.log2fr);
    
    // If deviation is small enough, use plain label
    if (std::abs(deviationCents) < thresholdCents) {
        return referencePitch.label;
    }
    
    // Otherwise, append deviation
    char deviationStr[32];
    if (deviationCents > 0) {
        std::snprintf(deviationStr, sizeof(deviationStr), "%s+%.1fct", 
                     referencePitch.label.c_str(), deviationCents);
    } else {
        std::snprintf(deviationStr, sizeof(deviationStr), "%s%.1fct", 
                     referencePitch.label.c_str(), deviationCents);
    }
    
    return std::string(deviationStr);
}

} // namespace scalatrix