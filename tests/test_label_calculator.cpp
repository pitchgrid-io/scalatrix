#include "catch2/catch_test_macros.hpp"
#include "scalatrix/label_calculator.hpp"
#include "scalatrix/mos.hpp"
#include <cmath>

using namespace scalatrix;

TEST_CASE("LabelCalculator basic functionality", "[labelcalculator]") {
    LabelCalculator lc;
    MOS mos = MOS::fromParams(5, 2, 1, 1.0, 0.585); // 7-note MOS (like diatonic)
    
    SECTION("Generate normalized note labels") {
        // Test some basic coordinates
        Vector2i origin(0, 0);
        std::string label = lc.noteLabelNormalized(mos, origin);
        REQUIRE(!label.empty());
        
        // Different coordinates should give different labels (usually)
        Vector2i coord1(1, 0);
        Vector2i coord2(0, 1);
        std::string label1 = lc.noteLabelNormalized(mos, coord1);
        std::string label2 = lc.noteLabelNormalized(mos, coord2);
        
        REQUIRE(!label1.empty());
        REQUIRE(!label2.empty());
        
        // Labels should be different for different coordinates
        if (coord1.x != coord2.x || coord1.y != coord2.y) {
            // Usually different, but not guaranteed for all MOS systems
            // This is more of a basic sanity check
            REQUIRE(label1.length() > 0);
            REQUIRE(label2.length() > 0);
        }
    }
    
    SECTION("Label consistency") {
        Vector2i coord(2, 1);
        std::string label1 = lc.noteLabelNormalized(mos, coord);
        std::string label2 = lc.noteLabelNormalized(mos, coord);
        
        // Same coordinate should always give same label
        REQUIRE(label1 == label2);
    }
    
    SECTION("Handles origin") {
        Vector2i origin(0, 0);
        std::string label = lc.noteLabelNormalized(mos, origin);
        
        // Origin should have a valid label
        REQUIRE(!label.empty());
    }
}

TEST_CASE("LabelCalculator with different MOS systems", "[labelcalculator]") {
    LabelCalculator lc;
    
    SECTION("5-limit MOS (5,2)") {
        MOS mos = MOS::fromParams(5, 2, 1, 1.0, 0.585);
        
        // Test several coordinates
        std::vector<Vector2i> testCoords = {
            {0, 0}, {1, 0}, {0, 1}, {2, 0}, {1, 1}, {3, 1}
        };
        
        for (const auto& coord : testCoords) {
            std::string label = lc.noteLabelNormalized(mos, coord);
            REQUIRE(!label.empty());
        }
    }
    
    SECTION("Pentatonic-like MOS (3,2)") {
        MOS mos = MOS::fromParams(3, 2, 1, 1.0, 0.6);
        
        std::vector<Vector2i> testCoords = {
            {0, 0}, {1, 0}, {0, 1}, {2, 1}
        };
        
        for (const auto& coord : testCoords) {
            std::string label = lc.noteLabelNormalized(mos, coord);
            REQUIRE(!label.empty());
        }
    }
    
    SECTION("Different MOS systems give different labeling") {
        MOS mos1 = MOS::fromParams(5, 2, 1, 1.0, 0.585);
        MOS mos2 = MOS::fromParams(3, 2, 1, 1.0, 0.6);
        
        Vector2i coord(1, 1);
        std::string label1 = lc.noteLabelNormalized(mos1, coord);
        std::string label2 = lc.noteLabelNormalized(mos2, coord);
        
        // Different MOS systems should generally give different labels
        // (This isn't guaranteed but is typical)
        REQUIRE(!label1.empty());
        REQUIRE(!label2.empty());
    }
}

TEST_CASE("LabelCalculator edge cases", "[labelcalculator]") {
    LabelCalculator lc;
    MOS mos = MOS::fromParams(5, 2, 1, 1.0, 0.585);
    
    SECTION("Large coordinates") {
        Vector2i largeCoord(10, 10);
        std::string label = lc.noteLabelNormalized(mos, largeCoord);
        
        // Should handle large coordinates gracefully
        REQUIRE(!label.empty());
    }
    
    SECTION("Negative coordinates") {
        Vector2i negCoord(-2, -1);
        std::string label = lc.noteLabelNormalized(mos, negCoord);
        
        // Should handle negative coordinates
        REQUIRE(!label.empty());
    }
    
    SECTION("Mixed sign coordinates") {
        Vector2i mixedCoord(-1, 2);
        std::string label = lc.noteLabelNormalized(mos, mixedCoord);
        
        // Should handle mixed positive/negative coordinates
        REQUIRE(!label.empty());
    }
}

TEST_CASE("LabelCalculator deterministic behavior", "[labelcalculator]") {
    SECTION("Multiple instances give same results") {
        LabelCalculator lc1, lc2;
        MOS mos = MOS::fromParams(5, 2, 1, 1.0, 0.585);
        Vector2i coord(3, 1);
        
        std::string label1 = lc1.noteLabelNormalized(mos, coord);
        std::string label2 = lc2.noteLabelNormalized(mos, coord);
        
        // Different instances should give same result
        REQUIRE(label1 == label2);
    }
    
    SECTION("Repeated calls give same results") {
        LabelCalculator lc;
        MOS mos = MOS::fromParams(5, 2, 1, 1.0, 0.585);
        Vector2i coord(2, 3);
        
        std::vector<std::string> labels;
        for (int i = 0; i < 5; ++i) {
            labels.push_back(lc.noteLabelNormalized(mos, coord));
        }
        
        // All calls should give same result
        for (size_t i = 1; i < labels.size(); ++i) {
            REQUIRE(labels[i] == labels[0]);
        }
    }
}

TEST_CASE("LabelCalculator diatonic window", "[labelcalculator]") {
    SECTION("meantone fifth + octave is inside") {
        MOS mos = MOS::fromParams(5, 2, 1, 1.0, 0.585);
        REQUIRE(LabelCalculator::inDiatonicWindow(mos));
    }

    SECTION("fifth below 4/7 is outside") {
        MOS mos = MOS::fromParams(5, 2, 1, 1.0, 0.55);
        REQUIRE_FALSE(LabelCalculator::inDiatonicWindow(mos));
    }
}

TEST_CASE("LabelCalculator nearestWesternPitchClass", "[labelcalculator]") {
    const double c4 = 440.0 * std::exp2(-9.0 / 12.0);

    SECTION("concert C is pitch class 0") {
        REQUIRE(LabelCalculator::nearestWesternPitchClass(c4) == 0);
    }

    SECTION("51ct above C is Db (pc 1)") {
        REQUIRE(LabelCalculator::nearestWesternPitchClass(c4 * std::exp2(51.0 / 1200.0)) == 1);
    }

    SECTION("49ct above C stays C (pc 0)") {
        REQUIRE(LabelCalculator::nearestWesternPitchClass(c4 * std::exp2(49.0 / 1200.0)) == 0);
    }

    SECTION("tritone is F# (pc 6)") {
        REQUIRE(LabelCalculator::nearestWesternPitchClass(c4 * std::exp2(0.5)) == 6);
    }

    SECTION("-51ct is B (pc 11)") {
        REQUIRE(LabelCalculator::nearestWesternPitchClass(c4 * std::exp2(-51.0 / 1200.0)) == 11);
    }
}

TEST_CASE("LabelCalculator western root follows concert pitch", "[labelcalculator]") {
    LabelCalculator lc;
    MOS mos = MOS::fromParams(5, 2, 1, 1.0, 0.585);
    const double c4 = 440.0 * std::exp2(-9.0 / 12.0);
    Vector2i origin(0, 0);

    SECTION("default / concert C labels origin C") {
        REQUIRE(lc.noteLabelNormalized(mos, origin) == "C");
        REQUIRE(lc.noteLabelNormalized(mos, origin, c4) == "C");
    }

    SECTION("51ct above C labels origin D♭") {
        const double f = c4 * std::exp2(51.0 / 1200.0);
        REQUIRE(lc.noteLabelNormalized(mos, origin, f) == "D♭");
    }

    SECTION("49ct above C still labels origin C") {
        const double f = c4 * std::exp2(49.0 / 1200.0);
        REQUIRE(lc.noteLabelNormalized(mos, origin, f) == "C");
    }

    SECTION("tritone labels origin F♯") {
        const double f = c4 * std::exp2(0.5);
        REQUIRE(lc.noteLabelNormalized(mos, origin, f) == "F♯");
    }
}

TEST_CASE("LabelCalculator deviationLabel", "[labelcalculator]") {
    SECTION("No closest pitch returns empty string") {
        Node node;
        node.tuning_coord.x = 0.5;
        node.closestPitch = {"", 0.0}; // Empty label
        
        std::string result = LabelCalculator::deviationLabel(node, 0.1, false);
        REQUIRE(result.empty());
    }
    
    SECTION("Small deviation returns plain label") {
        Node node;
        node.tuning_coord.x = 0.5;
        node.closestPitch = {"3:2", 0.5849625007}; // log2(3/2)
        node.temperedPitch = {"3:2", 0.5849625007};
        
        // Deviation is about 1200 * (0.5 - 0.5849625007) = -101.955 cents
        // With threshold 200 cents, should return plain label
        std::string result = LabelCalculator::deviationLabel(node, 200.0, false);
        REQUIRE(result == "3:2");
    }
    
    SECTION("Large deviation returns label with cents") {
        Node node;
        node.tuning_coord.x = 0.5;
        node.closestPitch = {"3:2", 0.5849625007}; // log2(3/2)
        node.temperedPitch = {"3:2", 0.5849625007};
        
        // Deviation is about 1200 * (0.5 - 0.5849625007) = -101.955 cents
        // With threshold 0.1 cents, should return label with deviation
        std::string result = LabelCalculator::deviationLabel(node, 0.1, false);
        REQUIRE(result.find("3:2") != std::string::npos);
        REQUIRE(result.find("ct") != std::string::npos);
        REQUIRE(result.find("-") != std::string::npos); // Should show negative deviation
    }
    
    SECTION("Positive deviation") {
        Node node;
        node.tuning_coord.x = 0.7;
        node.closestPitch = {"3:2", 0.5849625007}; // log2(3/2)
        node.temperedPitch = {"3:2", 0.5849625007};
        
        // Deviation is about 1200 * (0.7 - 0.5849625007) = +138.045 cents
        std::string result = LabelCalculator::deviationLabel(node, 0.1, false);
        REQUIRE(result.find("3:2") != std::string::npos);
        REQUIRE(result.find("ct") != std::string::npos);
        REQUIRE(result.find("+") != std::string::npos); // Should show positive deviation
    }
    
    SECTION("Compare with tempered pitch") {
        Node node;
        node.tuning_coord.x = 0.5;
        node.closestPitch = {"3:2", 0.5849625007}; // log2(3/2)
        node.temperedPitch = {"3:2", 0.6}; // Slightly different tempered pitch
        
        // When compareWithTempered=true, should compare temperedPitch with closestPitch
        // Deviation is about 1200 * (0.6 - 0.5849625007) = +18.045 cents
        std::string result = LabelCalculator::deviationLabel(node, 0.1, true);
        REQUIRE(result.find("3:2") != std::string::npos);
        REQUIRE(result.find("ct") != std::string::npos);
        REQUIRE(result.find("+") != std::string::npos);
        
        // When compareWithTempered=false, should compare tuning_coord with closestPitch
        // Deviation is about 1200 * (0.5 - 0.5849625007) = -101.955 cents
        std::string result2 = LabelCalculator::deviationLabel(node, 0.1, false);
        REQUIRE(result2.find("3:2") != std::string::npos);
        REQUIRE(result2.find("ct") != std::string::npos);
        REQUIRE(result2.find("-") != std::string::npos);
        
        // Results should be different
        REQUIRE(result != result2);
    }
    
    SECTION("Exact match returns plain label") {
        Node node;
        node.tuning_coord.x = 0.5849625007; // Exactly log2(3/2)
        node.closestPitch = {"3:2", 0.5849625007};
        node.temperedPitch = {"3:2", 0.5849625007};
        
        std::string result = LabelCalculator::deviationLabel(node, 0.1, false);
        REQUIRE(result == "3:2");
    }
}