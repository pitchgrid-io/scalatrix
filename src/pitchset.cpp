#include "scalatrix/pitchset.hpp"
#include <numeric>
#include <sstream>
#include <algorithm>
#include <cmath>

const int PRIMES[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97};


namespace scalatrix {

PseudoPrimeInt pseudoPrimeFromIndexNumber(unsigned int index) {
    PseudoPrimeInt p;
    p.label = std::to_string(PRIMES[index]);
    p.number = PRIMES[index];
    p.log2fr = log2(p.number);
    return p;
};


PrimeList generateDefaultPrimeList(int n_primes) {
    n_primes = std::min(n_primes, 25);
    PrimeList primes;
    for (int i = 0; i < n_primes; i++) {
        PseudoPrimeInt p = pseudoPrimeFromIndexNumber(i);
        primes.push_back(p);
    }
    return primes;    
};


PitchSet generateETPitchSet(unsigned int n_et, double equave_log2fr, double min_log2fr, double max_log2fr) {
    PitchSet pitchset;
    
    // Calculate the range of steps to generate
    // For min_log2fr: step * equave_log2fr / n_et >= min_log2fr, so step >= min_log2fr * n_et / equave_log2fr
    // For max_log2fr: step * equave_log2fr / n_et <= max_log2fr, so step <= max_log2fr * n_et / equave_log2fr
    int min_step = (int)ceil(min_log2fr * n_et / equave_log2fr);
    int max_step = (int)floor(max_log2fr * n_et / equave_log2fr);
    
    for (int i = min_step; i <= max_step; i++) {
        PitchSetPitch p;
        p.label = std::to_string(i) + "\\" + std::to_string(n_et);
        p.log2fr = i * equave_log2fr / n_et;
        
        // Filter to ensure the pitch is within the specified range
        if (p.log2fr >= min_log2fr - 1e-6 && p.log2fr <= max_log2fr + 1e-6) {
            pitchset.push_back(p);
        }
    }
    
    // Sort by log2fr (should already be sorted, but ensure it)
    std::sort(pitchset.begin(), pitchset.end(), 
        [](const PitchSetPitch& a, const PitchSetPitch& b) {
            return a.log2fr < b.log2fr;
        }
    );
    
    return pitchset;
};

PitchSet generateJIPitchSet(PrimeList primes, int max_numorden, double min_log2fr, double max_log2fr) {
    PitchSet pitchset;
    PrimeList nums;
    // generate all numbers with prime factors in primes up to max_numorden
    for (int i = 1; i < max_numorden; i++) {
        int r = i;
        double log2fr = 0.0;
        for (auto p : primes) {
            while (r % p.number == 0) {
                r /= p.number;
                log2fr += p.log2fr;
            }
        }
        bool include = r == 1;
        if(include){
            PseudoPrimeInt p;
            p.label = std::to_string(i);
            p.number = i;
            p.log2fr = log2fr;
            nums.push_back(p);
        }
    }
    for (auto num: nums){
        for (auto den: nums){
            if (std::gcd(num.number, den.number) > 1){
                continue;
            }
            if (static_cast<int>(num.number) <= max_numorden && static_cast<int>(den.number) <= max_numorden) {
                PitchSetPitch pitch;
                pitch.label = num.label + ":" + den.label;
                pitch.log2fr = num.log2fr - den.log2fr;
                pitchset.push_back(pitch);
            }
        }
    }
    // filter out pitches with log2fr outside of min_log2fr and max_log2fr
    PitchSet filtered_pitchset;
    for (auto pitch : pitchset) {
        if (pitch.log2fr > min_log2fr - 1e-6 && pitch.log2fr < max_log2fr + 1e-6) {
            filtered_pitchset.push_back(pitch);
        }
    }
    // sort
    std::sort(filtered_pitchset.begin(), filtered_pitchset.end(), 
        [](PitchSetPitch a, PitchSetPitch b) {
            return a.log2fr < b.log2fr;
        }
    );
    return filtered_pitchset;
};


PitchSet generateHarmonicSeriesPitchSet(PrimeList primes, int base, double min_log2fr, double max_log2fr) {
    PitchSet pitchset;
    double base_log2fr = log2(base);
    
    // Calculate the range of numerators to generate
    // For min_log2fr: num/base >= 2^min_log2fr, so num >= base * 2^min_log2fr
    // For max_log2fr: num/base <= 2^max_log2fr, so num <= base * 2^max_log2fr
    int min_num = std::max(1, (int)ceil(base * exp2(min_log2fr)));
    int max_num = (int)floor(base * exp2(max_log2fr));
    
    for (int num = min_num; num <= max_num; num++) {
        PitchSetPitch pitch;
        // Simplify the fraction by dividing by GCD
        int gcd = std::gcd(num, base);
        int simplified_num = num / gcd;
        int simplified_base = base / gcd;
        pitch.label = std::to_string(simplified_num) + ":" + std::to_string(simplified_base);
        pitch.log2fr = -base_log2fr;
        int r = num;
        for (auto p : primes) {
            while (r % p.number == 0) {
                r /= p.number;
                pitch.log2fr += p.log2fr;
            }
        }
        pitch.log2fr += log2(r);
        
        // Filter pitches to ensure they're within the specified range
        if (pitch.log2fr >= min_log2fr - 1e-6 && pitch.log2fr <= max_log2fr + 1e-6) {
            pitchset.push_back(pitch);
        }
    }
    
    std::sort(pitchset.begin(), pitchset.end(), 
        [](PitchSetPitch a, PitchSetPitch b) {
            return a.log2fr < b.log2fr;
        }
    );
    return pitchset;
};

PitchSetPitch operator+(const PitchSetPitch& a, const PitchSetPitch& b) {
    PitchSetPitch result;
    result.log2fr = a.log2fr + b.log2fr;
    
    // Parse label formats
    auto parseRatio = [](const std::string& label) -> std::pair<bool, std::pair<int, int>> {
        size_t colonPos = label.find(':');
        if (colonPos != std::string::npos) {
            try {
                int num = std::stoi(label.substr(0, colonPos));
                int den = std::stoi(label.substr(colonPos + 1));
                return {true, {num, den}};
            } catch (...) {
                return {false, {0, 0}};
            }
        }
        return {false, {0, 0}};
    };
    
    auto parseET = [](const std::string& label) -> std::pair<bool, std::pair<int, int>> {
        size_t backslashPos = label.find('\\');
        if (backslashPos != std::string::npos) {
            try {
                int num = std::stoi(label.substr(0, backslashPos));
                int den = std::stoi(label.substr(backslashPos + 1));
                return {true, {num, den}};
            } catch (...) {
                return {false, {0, 0}};
            }
        }
        return {false, {0, 0}};
    };
    
    auto [isRatioA, ratioA] = parseRatio(a.label);
    auto [isRatioB, ratioB] = parseRatio(b.label);
    auto [isETA, etA] = parseET(a.label);
    auto [isETB, etB] = parseET(b.label);
    
    if (isRatioA && isRatioB) {
        // Both are ratios: multiply fractions and simplify
        int newNum = ratioA.first * ratioB.first;
        int newDen = ratioA.second * ratioB.second;
        int gcd = std::gcd(newNum, newDen);
        newNum /= gcd;
        newDen /= gcd;
        result.label = std::to_string(newNum) + ":" + std::to_string(newDen);
    } else if (isETA && isETB && etA.second == etB.second) {
        // Both are ET fractions with same denominator: add numerators
        int newNum = etA.first + etB.first;
        int den = etA.second;
        result.label = std::to_string(newNum) + "\\" + std::to_string(den);
    } else {
        // Incompatible formats or different denominators - return empty label
        result.label = "";
    }
    
    return result;
}

PitchSetPitch operator*(int multiplier, const PitchSetPitch& pitch) {
    return pitch * multiplier;
}

PitchSetPitch operator*(const PitchSetPitch& pitch, int multiplier) {
    PitchSetPitch result;
    result.log2fr = multiplier * pitch.log2fr;
    
    // Parse label formats
    auto parseRatio = [](const std::string& label) -> std::pair<bool, std::pair<int, int>> {
        size_t colonPos = label.find(':');
        if (colonPos != std::string::npos) {
            try {
                int num = std::stoi(label.substr(0, colonPos));
                int den = std::stoi(label.substr(colonPos + 1));
                return {true, {num, den}};
            } catch (...) {
                return {false, {0, 0}};
            }
        }
        return {false, {0, 0}};
    };
    
    auto parseET = [](const std::string& label) -> std::pair<bool, std::pair<int, int>> {
        size_t backslashPos = label.find('\\');
        if (backslashPos != std::string::npos) {
            try {
                int num = std::stoi(label.substr(0, backslashPos));
                int den = std::stoi(label.substr(backslashPos + 1));
                return {true, {num, den}};
            } catch (...) {
                return {false, {0, 0}};
            }
        }
        return {false, {0, 0}};
    };
    
    auto [isRatio, ratio] = parseRatio(pitch.label);
    auto [isET, et] = parseET(pitch.label);
    
    if (isRatio) {
        // For ratios: integer is the power
        // multiplier * (num:den) = num^multiplier : den^multiplier
        if (multiplier >= 0) {
            long long newNum = 1;
            long long newDen = 1;
            
            // Calculate num^multiplier and den^multiplier
            for (int i = 0; i < multiplier; i++) {
                newNum *= ratio.first;
                newDen *= ratio.second;
            }
            
            // Simplify with GCD
            long long gcd = std::gcd(newNum, newDen);
            newNum /= gcd;
            newDen /= gcd;
            
            result.label = std::to_string(newNum) + ":" + std::to_string(newDen);
        } else {
            // Negative multiplier: invert and raise to positive power
            int posMultiplier = -multiplier;
            long long newNum = 1;
            long long newDen = 1;
            
            // Calculate den^|multiplier| : num^|multiplier| (inverted)
            for (int i = 0; i < posMultiplier; i++) {
                newNum *= ratio.second;  // denominator becomes numerator
                newDen *= ratio.first;   // numerator becomes denominator
            }
            
            // Simplify with GCD
            long long gcd = std::gcd(newNum, newDen);
            newNum /= gcd;
            newDen /= gcd;
            
            result.label = std::to_string(newNum) + ":" + std::to_string(newDen);
        }
    } else if (isET) {
        // For ET: multiply the numerator
        // multiplier * (num\\den) = (multiplier * num)\\den
        int newNum = multiplier * et.first;
        int den = et.second;
        result.label = std::to_string(newNum) + "\\" + std::to_string(den);
    } else {
        // Unknown format - return empty label
        result.label = "";
    }
    
    return result;
}


};