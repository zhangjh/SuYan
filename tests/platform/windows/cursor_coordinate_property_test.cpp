/**
 * Cursor Coordinate Transformation Property Tests
 * 
 * Feature: windows-platform-support
 * Property 8: Cursor Coordinate Transformation
 * Validates: Requirements 7.5
 * 
 * Tests that cursor coordinates are correctly handled across the full range
 * of possible screen positions, including negative coordinates for extended
 * displays positioned to the left or above the primary monitor.
 */

#include "shared/ipc_protocol.h"
#include <iostream>
#include <random>
#include <limits>
#include <cmath>

using namespace suyan::ipc;

static std::mt19937 rng(42);

static int16_t randomScreenCoord() {
    return std::uniform_int_distribution<int16_t>(-4096, 8192)(rng);
}

static int16_t randomDimension() {
    return std::uniform_int_distribution<int16_t>(1, 500)(rng);
}

/**
 * Property 8.1: Screen coordinate preservation through IPC encoding
 * 
 * For any screen coordinate (including negative values from extended displays),
 * the coordinate should be preserved exactly through the IPC encoding/decoding cycle.
 */
static bool testScreenCoordinatePreservation() {
    std::cout << "Property 8.1: Screen Coordinate Preservation... ";
    
    for (int i = 0; i < 100; ++i) {
        int16_t screenX = randomScreenCoord();
        int16_t screenY = randomScreenCoord();
        int16_t width = randomDimension();
        int16_t height = randomDimension();
        
        uint32_t param1, param2;
        CursorPosition::encode(screenX, screenY, width, height, param1, param2);
        
        int16_t decodedX, decodedY, decodedW, decodedH;
        CursorPosition::decode(param1, param2, decodedX, decodedY, decodedW, decodedH);
        
        if (screenX != decodedX || screenY != decodedY ||
            width != decodedW || height != decodedH) {
            std::cout << "FAILED at iteration " << i << std::endl;
            std::cout << "  Original: (" << screenX << ", " << screenY 
                      << ") " << width << "x" << height << std::endl;
            std::cout << "  Decoded:  (" << decodedX << ", " << decodedY 
                      << ") " << decodedW << "x" << decodedH << std::endl;
            return false;
        }
    }
    
    std::cout << "PASSED (100 iterations)" << std::endl;
    return true;
}

/**
 * Property 8.2: Extended display negative coordinates
 * 
 * Coordinates from extended displays positioned to the left or above
 * the primary monitor (negative X or Y) must be handled correctly.
 */
static bool testExtendedDisplayCoordinates() {
    std::cout << "Property 8.2: Extended Display Negative Coordinates... ";
    
    struct TestCase {
        int16_t x, y, w, h;
        const char* description;
    };
    
    TestCase testCases[] = {
        {-1920, 540, 1, 20, "Left extended display"},
        {-3840, 540, 1, 20, "Two monitors left"},
        {960, -1080, 1, 20, "Top extended display"},
        {-1920, -1080, 1, 20, "Top-left extended display"},
        {-32768, -32768, 1, 1, "Extreme negative"},
        {32767, 32767, 100, 100, "Extreme positive"},
        {0, 0, 1, 20, "Origin"},
        {-1, -1, 1, 1, "Just negative"},
    };
    
    for (const auto& tc : testCases) {
        uint32_t param1, param2;
        CursorPosition::encode(tc.x, tc.y, tc.w, tc.h, param1, param2);
        
        int16_t decodedX, decodedY, decodedW, decodedH;
        CursorPosition::decode(param1, param2, decodedX, decodedY, decodedW, decodedH);
        
        if (tc.x != decodedX || tc.y != decodedY ||
            tc.w != decodedW || tc.h != decodedH) {
            std::cout << "FAILED: " << tc.description << std::endl;
            std::cout << "  Original: (" << tc.x << ", " << tc.y 
                      << ") " << tc.w << "x" << tc.h << std::endl;
            std::cout << "  Decoded:  (" << decodedX << ", " << decodedY 
                      << ") " << decodedW << "x" << decodedH << std::endl;
            return false;
        }
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

/**
 * Property 8.3: Coordinate sign preservation
 * 
 * The sign of coordinates must be preserved through encoding.
 * Negative coordinates should remain negative, positive should remain positive.
 */
static bool testCoordinateSignPreservation() {
    std::cout << "Property 8.3: Coordinate Sign Preservation... ";
    
    for (int i = 0; i < 100; ++i) {
        int16_t x = randomScreenCoord();
        int16_t y = randomScreenCoord();
        
        uint32_t param1, param2;
        CursorPosition::encode(x, y, 1, 1, param1, param2);
        
        int16_t decodedX, decodedY, decodedW, decodedH;
        CursorPosition::decode(param1, param2, decodedX, decodedY, decodedW, decodedH);
        
        bool xSignCorrect = (x < 0) == (decodedX < 0) || (x == 0 && decodedX == 0);
        bool ySignCorrect = (y < 0) == (decodedY < 0) || (y == 0 && decodedY == 0);
        
        if (!xSignCorrect || !ySignCorrect) {
            std::cout << "FAILED at iteration " << i << std::endl;
            std::cout << "  Original: (" << x << ", " << y << ")" << std::endl;
            std::cout << "  Decoded:  (" << decodedX << ", " << decodedY << ")" << std::endl;
            return false;
        }
    }
    
    std::cout << "PASSED (100 iterations)" << std::endl;
    return true;
}

/**
 * Property 8.4: Typical multi-monitor configurations
 * 
 * Test coordinates that would occur in common multi-monitor setups.
 */
static bool testTypicalMultiMonitorConfigs() {
    std::cout << "Property 8.4: Typical Multi-Monitor Configurations... ";
    
    struct MonitorConfig {
        int16_t primaryWidth, primaryHeight;
        int16_t extendedOffsetX, extendedOffsetY;
        const char* description;
    };
    
    MonitorConfig configs[] = {
        {1920, 1080, -1920, 0, "1080p + 1080p left"},
        {2560, 1440, -2560, 0, "1440p + 1440p left"},
        {3840, 2160, -3840, 0, "4K + 4K left"},
        {1920, 1080, 0, -1080, "1080p + 1080p top"},
        {1920, 1080, -1920, -1080, "1080p + 1080p top-left diagonal"},
    };
    
    for (const auto& cfg : configs) {
        for (int i = 0; i < 10; ++i) {
            int16_t x = cfg.extendedOffsetX + std::uniform_int_distribution<int16_t>(0, cfg.primaryWidth - 1)(rng);
            int16_t y = cfg.extendedOffsetY + std::uniform_int_distribution<int16_t>(0, cfg.primaryHeight - 1)(rng);
            
            uint32_t param1, param2;
            CursorPosition::encode(x, y, 1, 20, param1, param2);
            
            int16_t decodedX, decodedY, decodedW, decodedH;
            CursorPosition::decode(param1, param2, decodedX, decodedY, decodedW, decodedH);
            
            if (x != decodedX || y != decodedY) {
                std::cout << "FAILED: " << cfg.description << std::endl;
                std::cout << "  Original: (" << x << ", " << y << ")" << std::endl;
                std::cout << "  Decoded:  (" << decodedX << ", " << decodedY << ")" << std::endl;
                return false;
            }
        }
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

int main() {
    std::cout << "=== Cursor Coordinate Transformation Property Tests ===" << std::endl;
    std::cout << "Feature: windows-platform-support" << std::endl;
    std::cout << "Property 8: Cursor Coordinate Transformation" << std::endl;
    std::cout << "Validates: Requirements 7.5" << std::endl;
    std::cout << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    if (testScreenCoordinatePreservation()) ++passed; else ++failed;
    if (testExtendedDisplayCoordinates()) ++passed; else ++failed;
    if (testCoordinateSignPreservation()) ++passed; else ++failed;
    if (testTypicalMultiMonitorConfigs()) ++passed; else ++failed;
    
    std::cout << std::endl;
    std::cout << "Results: " << passed << " passed, " << failed << " failed" << std::endl;
    
    return failed > 0 ? 1 : 0;
}
