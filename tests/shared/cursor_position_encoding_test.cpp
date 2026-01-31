/**
 * Cursor Position Encoding Property Tests
 * 
 * Feature: windows-platform-support
 * Property 10: Cursor Position Encoding Round-Trip
 * Validates: Requirements 3.6, 7.5
 */

#include "../../src/shared/ipc_protocol.h"
#include <iostream>
#include <random>
#include <limits>

using namespace suyan::ipc;

static std::mt19937 rng(42);

static int16_t randomI16() {
    return std::uniform_int_distribution<int16_t>(
        std::numeric_limits<int16_t>::min(),
        std::numeric_limits<int16_t>::max()
    )(rng);
}

static bool testCursorPositionRoundTrip() {
    std::cout << "Property 10: Cursor Position Encoding Round-Trip... ";
    
    for (int i = 0; i < 100; ++i) {
        int16_t origX = randomI16();
        int16_t origY = randomI16();
        int16_t origW = randomI16();
        int16_t origH = randomI16();
        
        uint32_t param1, param2;
        CursorPosition::encode(origX, origY, origW, origH, param1, param2);
        
        int16_t restoredX, restoredY, restoredW, restoredH;
        CursorPosition::decode(param1, param2, restoredX, restoredY, restoredW, restoredH);
        
        if (origX != restoredX || origY != restoredY ||
            origW != restoredW || origH != restoredH) {
            std::cout << "FAILED at iteration " << i << std::endl;
            std::cout << "  Original: x=" << origX << " y=" << origY
                      << " w=" << origW << " h=" << origH << std::endl;
            std::cout << "  Restored: x=" << restoredX << " y=" << restoredY
                      << " w=" << restoredW << " h=" << restoredH << std::endl;
            return false;
        }
    }
    
    std::cout << "PASSED (100 iterations)" << std::endl;
    return true;
}

static bool testNegativeCoordinates() {
    std::cout << "Property 10: Negative Coordinates (Extended Screen)... ";
    
    int16_t testCases[][4] = {
        {-1920, 0, 1, 20},
        {-100, -100, 2, 25},
        {-32768, -32768, 100, 100},
        {32767, 32767, 200, 200},
        {0, 0, 0, 0},
        {-1, -1, 1, 1},
    };
    
    for (auto& tc : testCases) {
        int16_t origX = tc[0], origY = tc[1], origW = tc[2], origH = tc[3];
        
        uint32_t param1, param2;
        CursorPosition::encode(origX, origY, origW, origH, param1, param2);
        
        int16_t restoredX, restoredY, restoredW, restoredH;
        CursorPosition::decode(param1, param2, restoredX, restoredY, restoredW, restoredH);
        
        if (origX != restoredX || origY != restoredY ||
            origW != restoredW || origH != restoredH) {
            std::cout << "FAILED" << std::endl;
            std::cout << "  Original: x=" << origX << " y=" << origY
                      << " w=" << origW << " h=" << origH << std::endl;
            std::cout << "  Restored: x=" << restoredX << " y=" << restoredY
                      << " w=" << restoredW << " h=" << restoredH << std::endl;
            return false;
        }
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

int main() {
    std::cout << "=== Cursor Position Encoding Property Tests ===" << std::endl;
    std::cout << "Feature: windows-platform-support" << std::endl;
    std::cout << "Validates: Requirements 3.6, 7.5" << std::endl;
    std::cout << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    if (testCursorPositionRoundTrip()) ++passed; else ++failed;
    if (testNegativeCoordinates()) ++passed; else ++failed;
    
    std::cout << std::endl;
    std::cout << "Results: " << passed << " passed, " << failed << " failed" << std::endl;
    
    return failed > 0 ? 1 : 0;
}
