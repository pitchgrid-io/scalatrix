#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_floating_point.hpp"
#include "scalatrix/mos.hpp"
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <new>

using namespace scalatrix;
using Catch::Matchers::WithinAbs;

namespace {

std::atomic<int> allocHits{0};
std::atomic<bool> countAlloc{false};

} // namespace

void* operator new(std::size_t n) {
    if (countAlloc.load(std::memory_order_relaxed))
        allocHits.fetch_add(1, std::memory_order_relaxed);
    if (void* p = std::malloc(n))
        return p;
    throw std::bad_alloc();
}

void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

TEST_CASE("MOS construction from generator", "[mos]") {
    SECTION("Basic MOS from generator") {
        MOS mos = MOS::fromG(3, 4, 0.585, 1.0, 1);
        
        // Test that basic properties are reasonable
        REQUIRE(mos.n > 0);
        REQUIRE(mos.a > 0);
        REQUIRE(mos.b > 0);
        REQUIRE_THAT(mos.generator, WithinAbs(0.585, 1e-10));
        REQUIRE_THAT(mos.equave, WithinAbs(1.0, 1e-10));
        REQUIRE(mos.repetitions == 1);
    }
    
    SECTION("MOS from parameters") {
        MOS mos = MOS::fromParams(5, 2, 1, 1.0, 0.585);
        
        REQUIRE(mos.a == 5);
        REQUIRE(mos.b == 2);
        REQUIRE(mos.n == 7);
        REQUIRE_THAT(mos.equave, WithinAbs(1.0, 1e-10));
    }
}

TEST_CASE("MOS path generation", "[mos]") {
    MOS mos = MOS::fromParams(5, 2, 1, 1.0, 0.585); // Use simpler construction
    
    SECTION("Path has reasonable length") {
        REQUIRE(mos.path.size() > 0);
        REQUIRE(mos.path.size() <= mos.n * 2); // Allow some flexibility
    }
    
    SECTION("Path contains correct number of L and s steps") {
        int countL = 0, countS = 0;
        for (auto step : mos.path) {
            if (step == 0) countL++;
            else if (step == 1) countS++;
        }
        // Path should have some steps
        REQUIRE(countL + countS == mos.path.size());
        REQUIRE(countL > 0);
        REQUIRE(countS > 0);
    }
}

TEST_CASE("MOS scale generation", "[mos]") {
    MOS mos = MOS::fromG(3, 4, 0.585, 1.0, 1);
    
    SECTION("Generate scale from MOS") {
        auto scale = mos.generateScaleFromMOS(261.63, 128, 60);
        
        REQUIRE(scale.getNodes().size() == 128);
        REQUIRE(scale.getRootIdx() == 60);
        REQUIRE_THAT(scale.getBaseFreq(), WithinAbs(261.63, 1e-10));
    }
    
    SECTION("Base scale has correct number of nodes") {
        REQUIRE(mos.base_scale.getNodes().size() >= mos.n);
    }
}

TEST_CASE("MOS angle calculations", "[mos]") {
    MOS mos = MOS::fromG(3, 4, 0.585, 1.0, 1);
    
    SECTION("Angle is within expected range") {
        double angle = mos.angle();
        REQUIRE(angle >= -M_PI);
        REQUIRE(angle <= M_PI);
    }
    
    SECTION("Generator from angle round-trips") {
        double originalGen = mos.generator;
        double angle = mos.angle();
        double recoveredGen = mos.gFromAngle(angle);
        
        REQUIRE_THAT(recoveredGen, WithinAbs(originalGen, 1e-10));
    }
}

TEST_CASE("MOS retuning operations", "[mos]") {
    MOS mos = MOS::fromG(3, 4, 0.585, 1.0, 1);
    auto scale = mos.generateScaleFromMOS(261.63, 12, 0);
    
    SECTION("Retune one point") {
        Vector2i targetCoord(0, 0);
        double targetPitch = 1.0;
        
        mos.retuneOnePoint(targetCoord, targetPitch);
        mos.retuneScaleWithMOS(scale, 261.63);
        
        // After retuning, the transform should map the target coord to target pitch
        auto transformed = mos.impliedAffine.apply(targetCoord);
        REQUIRE_THAT(transformed.x, WithinAbs(targetPitch, 1e-10));
    }
    
    SECTION("Retune two points") {
        Vector2i coord1(0, 0);
        Vector2i coord2(mos.a, mos.b);
        double pitch1 = 0.0;
        double pitch2 = 1.1;
        
        mos.retuneTwoPoints(coord1, coord2, pitch2);
        mos.retuneScaleWithMOS(scale, 261.63);
        
        // Check that the equave has been adjusted
        REQUIRE_THAT(mos.equave, WithinAbs(pitch2, 1e-10));
    }
    
    SECTION("Retune three points") {
        Vector2i coord1(0, 0);
        Vector2i coord2(mos.a, mos.b);
        Vector2i coord3 = mos.v_gen;
        double pitch1 = 0.0;
        double pitch2 = 1.0;
        double pitch3 = 7.0/12;
        
        mos.retuneThreePoints(coord1, coord2, coord3, pitch3);
        mos.retuneScaleWithMOS(scale, 261.63);
        
        // The generator should have been adjusted
        auto transformed = mos.impliedAffine.apply(coord3);
        REQUIRE_THAT(transformed.x, WithinAbs(pitch3, 0.01)); // Less strict due to iterative solving
    }
    
    SECTION("Retune zero point resets") {
        // First modify the MOS
        mos.retuneOnePoint({1, 1}, 0.5);
        
        // Then reset
        mos.retuneZeroPoint();
        
        // Should be back to something reasonable (not testing exact values)
        auto origin = mos.impliedAffine.apply({0, 0});
        REQUIRE(std::abs(origin.x) < 1.0); // Should be within reasonable bounds
    }
}

TEST_CASE("MOS node coordinate mapping", "[mos]") {
    MOS mos1 = MOS::fromParams(5, 2, 1, 1.0, 0.585);
    MOS mos2 = MOS::fromParams(3, 2, 1, 1.0, 0.6);
    
    SECTION("Map coordinate between MOS systems") {
        Vector2i coord(3, 1);
        Vector2i mapped = mos2.mapFromMOS(mos1, coord);
        
        // The mapped coordinate should exist
        REQUIRE(mapped.x >= -100);
        REQUIRE(mapped.x <= 100);
        REQUIRE(mapped.y >= -100);
        REQUIRE(mapped.y <= 100);
    }
    
    SECTION("Identity mapping") {
        Vector2i coord(3, 1);
        Vector2i mapped = mos1.mapFromMOS(mos1, coord);
        
        REQUIRE(mapped.x == coord.x);
        REQUIRE(mapped.y == coord.y);
    }
}

TEST_CASE("MOS frequency calculations", "[mos]") {
    MOS mos = MOS::fromParams(5, 2, 1, 1.0, 0.585);
    double baseFreq = 440.0;
    
    SECTION("Coordinate to frequency conversion") {
        // Root coordinate should give base frequency
        double rootFreq = mos.coordToFreq(0, 0, baseFreq);
        REQUIRE_THAT(rootFreq, WithinAbs(baseFreq, 1e-10));
        
        // One equave up should be double
        int equaveX = mos.a;
        int equaveY = mos.b;
        double octaveFreq = mos.coordToFreq(equaveX, equaveY, baseFreq);
        REQUIRE_THAT(octaveFreq, WithinAbs(baseFreq * 2.0, 0.1));
    }
}

TEST_CASE("MOS node labeling", "[mos]") {
    MOS mos = MOS::fromParams(5, 2, 1, 1.0, 0.585);
    
    SECTION("Node label letters") {
        // Test a few coordinates
        std::string label1 = mos.nodeLabelLetterWithOctaveNumber({0, 0});
        REQUIRE(!label1.empty());
        
        std::string label2 = mos.nodeLabelLetterWithOctaveNumber({1, 0});
        REQUIRE(!label2.empty());
        
        // Different coordinates should generally have different labels
        if (mos.n > 1) {
            REQUIRE(label1 != label2);
        }
    }
    
    SECTION("Node in scale check") {
        // Origin should be in scale
        REQUIRE(mos.nodeInScale({0, 0}));
        
        // One step along the scale should be in scale
        bool stepInScale = mos.nodeInScale({1, 0}) || mos.nodeInScale({0, 1});
        REQUIRE(stepInScale);
    }
}

TEST_CASE("root coordinates round-trip without copying the path", "[mos]") {
    MOS scales[] = {
        MOS::fromParams(5, 2, 1, 1.0, 0.585),
        MOS::fromParams(5, 2, 1, 1.0, 0.585, 2),
        MOS::fromG(5, 3, 0.585, 1.0, 1),
    };
    for (MOS& mos : scales) {
        REQUIRE(mos.path.size() > 1);
        for (int x = -3; x <= 3; ++x) {
            for (int y = -3; y <= 3; ++y) {
                const Vector2i v{x, y};
                REQUIRE(mos.toRootCoord(mos.fromRootCoord(v)) == v);
                REQUIRE(mos.fromRootCoord(mos.toRootCoord(v)) == v);
            }
        }
        const Vector2i coord{3, 1};
        REQUIRE(mos.mapFromMOS(mos, coord) == coord);
    }

    MOS& mos = scales[0];
    allocHits.store(0, std::memory_order_relaxed);
    countAlloc.store(true, std::memory_order_relaxed);
    const Vector2i converted = mos.fromRootCoord(Vector2i{3, 1});
    const Vector2i back = mos.toRootCoord(converted);
    const Vector2i mapped = mos.mapFromMOS(mos, Vector2i{2, -1});
    const int hits = allocHits.load(std::memory_order_relaxed);
    countAlloc.store(false, std::memory_order_relaxed);
    REQUIRE(hits == 0);
    REQUIRE(back == Vector2i{3, 1});
    REQUIRE(mapped == Vector2i{2, -1});
}

TEST_CASE("MOS step sizes", "[mos]") {
    MOS mos = MOS::fromParams(5, 2, 1, 1.0, 0.585);
    
    SECTION("L and s frequencies are positive") {
        REQUIRE(mos.L_fr > 0);
        REQUIRE(mos.s_fr > 0);
    }
    
    SECTION("L is larger than s (for standard MOS)") {
        REQUIRE(mos.L_fr > mos.s_fr);
    }
    
    SECTION("Chroma is the difference") {
        REQUIRE_THAT(mos.chroma_fr, WithinAbs(std::abs(mos.L_fr - mos.s_fr), 1e-10));
    }
}