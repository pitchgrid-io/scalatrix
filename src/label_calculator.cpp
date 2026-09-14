#include "scalatrix/label_calculator.hpp"

namespace scalatrix {

namespace {

constexpr double kConcertA4 = 440.0;
// C4 is 9 semitones below A4.
inline double concertC4() {
    return kConcertA4 * std::exp2(-9.0 / 12.0);
}

// Natural scale-degree (C=0) and accidental of each 12-TET class, spelled
// uniquely on the fifths chain from Db (−5) through C to F# (+6).
constexpr int kWesternStep[12]  = {0, 1, 1, 2, 2, 3, 3, 4, 5, 5, 6, 6};
constexpr int kWesternAlter[12] = {0,-1, 0,-1, 0, 0, 1, 0,-1, 0,-1, 0};

} // namespace

bool LabelCalculator::inDiatonicWindow(const MOS& mos) {
    return mos.generator > 4.0 / 7 && mos.generator < 3.0 / 5
        && mos.equave > 0.9 && mos.equave < 1.2;
}

int LabelCalculator::nearestWesternPitchClass(double freqHz) {
    const double semis = 12.0 * std::log2(freqHz / concertC4());
    int n = static_cast<int>(std::floor(semis + 0.5));
    int pc = n % 12;
    if (pc < 0) pc += 12;
    return pc;
}

std::string LabelCalculator::noteLabelNormalized(MOS& mos, Vector2i v, bool override_letter_labels) {
    return noteLabelNormalized(mos, v, concertC4(), override_letter_labels);
}

std::string LabelCalculator::noteLabelNormalized(MOS& mos, Vector2i v, double baseFreq, bool override_letter_labels) {
    if (!override_letter_labels && inDiatonicWindow(mos)) {
        Vector2i diatonic_coord = diatonic_mos.mapFromMOS(mos, v);
        const int pc = nearestWesternPitchClass(baseFreq);
        if (pc != 0) {
            diatonic_coord += diatonic_mos.mosCoordFromNotation(kWesternStep[pc], kWesternAlter[pc], 0);
        }
        return nodeLabelLetter(diatonic_mos, diatonic_coord);
    }
    return nodeLabelDigit(mos, v);
}

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