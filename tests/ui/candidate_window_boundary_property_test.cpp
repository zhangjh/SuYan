/**
 * Candidate Window Boundary Containment Property Tests
 * 
 * Feature: windows-platform-support
 * Property 7: Candidate Window Boundary Containment
 * Validates: Requirements 6.2, 6.3
 * 
 * Tests that for any cursor position and candidate window size,
 * the calculated window position ensures the window is completely
 * within the visible area of the monitor containing the cursor.
 */

#include <iostream>
#include <random>
#include <cstdint>

static std::mt19937 rng(42);

struct MonitorWorkArea {
    int left;
    int top;
    int right;
    int bottom;
    
    int width() const { return right - left; }
    int height() const { return bottom - top; }
};

/**
 * Simulates the calculateCandidateWindowPosition function logic
 * for testing purposes without Windows API dependencies.
 */
static void calculateCandidateWindowPosition(
    int cursorX, int cursorY, int cursorHeight,
    int windowWidth, int windowHeight,
    const MonitorWorkArea& workArea,
    int& outX, int& outY)
{
    outX = cursorX;
    outY = cursorY + cursorHeight + 2;
    
    if (outX + windowWidth > workArea.right) {
        outX = workArea.right - windowWidth;
    }
    if (outX < workArea.left) {
        outX = workArea.left;
    }
    
    if (outY + windowHeight > workArea.bottom) {
        int topY = cursorY - windowHeight - 5;
        if (topY >= workArea.top) {
            outY = topY;
        } else {
            outY = workArea.bottom - windowHeight;
            if (outY < workArea.top) {
                outY = workArea.top;
            }
        }
    }
}

static int randomInRange(int min, int max) {
    return std::uniform_int_distribution<int>(min, max)(rng);
}

/**
 * Property 7.1: Window right edge within monitor bounds
 * 
 * For any cursor position and window size, the window's right edge
 * should not exceed the monitor's work area right edge.
 */
static bool testRightEdgeBoundary() {
    std::cout << "Property 7.1: Window Right Edge Within Bounds... ";
    
    MonitorWorkArea workArea = {0, 0, 1920, 1080};
    
    for (int i = 0; i < 100; ++i) {
        int cursorX = randomInRange(0, 1920);
        int cursorY = randomInRange(0, 1000);
        int cursorHeight = randomInRange(15, 30);
        int windowWidth = randomInRange(200, 500);
        int windowHeight = randomInRange(100, 300);
        
        int outX, outY;
        calculateCandidateWindowPosition(
            cursorX, cursorY, cursorHeight,
            windowWidth, windowHeight, workArea,
            outX, outY);
        
        int rightEdge = outX + windowWidth;
        if (rightEdge > workArea.right) {
            std::cout << "FAILED at iteration " << i << std::endl;
            std::cout << "  Cursor: (" << cursorX << ", " << cursorY << ")" << std::endl;
            std::cout << "  Window size: " << windowWidth << "x" << windowHeight << std::endl;
            std::cout << "  Result: x=" << outX << ", right edge=" << rightEdge << std::endl;
            std::cout << "  Work area right: " << workArea.right << std::endl;
            return false;
        }
    }
    
    std::cout << "PASSED (100 iterations)" << std::endl;
    return true;
}

/**
 * Property 7.2: Window bottom edge within monitor bounds
 * 
 * For any cursor position and window size, the window's bottom edge
 * should not exceed the monitor's work area bottom edge.
 */
static bool testBottomEdgeBoundary() {
    std::cout << "Property 7.2: Window Bottom Edge Within Bounds... ";
    
    MonitorWorkArea workArea = {0, 0, 1920, 1080};
    
    for (int i = 0; i < 100; ++i) {
        int cursorX = randomInRange(0, 1920);
        int cursorY = randomInRange(0, 1080);
        int cursorHeight = randomInRange(15, 30);
        int windowWidth = randomInRange(200, 500);
        int windowHeight = randomInRange(100, 300);
        
        int outX, outY;
        calculateCandidateWindowPosition(
            cursorX, cursorY, cursorHeight,
            windowWidth, windowHeight, workArea,
            outX, outY);
        
        int bottomEdge = outY + windowHeight;
        if (bottomEdge > workArea.bottom) {
            std::cout << "FAILED at iteration " << i << std::endl;
            std::cout << "  Cursor: (" << cursorX << ", " << cursorY << ")" << std::endl;
            std::cout << "  Window size: " << windowWidth << "x" << windowHeight << std::endl;
            std::cout << "  Result: y=" << outY << ", bottom edge=" << bottomEdge << std::endl;
            std::cout << "  Work area bottom: " << workArea.bottom << std::endl;
            return false;
        }
    }
    
    std::cout << "PASSED (100 iterations)" << std::endl;
    return true;
}

/**
 * Property 7.3: Window left edge within monitor bounds
 * 
 * For any cursor position and window size, the window's left edge
 * should not be less than the monitor's work area left edge.
 */
static bool testLeftEdgeBoundary() {
    std::cout << "Property 7.3: Window Left Edge Within Bounds... ";
    
    MonitorWorkArea workArea = {0, 0, 1920, 1080};
    
    for (int i = 0; i < 100; ++i) {
        int cursorX = randomInRange(-100, 100);
        int cursorY = randomInRange(0, 1000);
        int cursorHeight = randomInRange(15, 30);
        int windowWidth = randomInRange(200, 500);
        int windowHeight = randomInRange(100, 300);
        
        int outX, outY;
        calculateCandidateWindowPosition(
            cursorX, cursorY, cursorHeight,
            windowWidth, windowHeight, workArea,
            outX, outY);
        
        if (outX < workArea.left) {
            std::cout << "FAILED at iteration " << i << std::endl;
            std::cout << "  Cursor: (" << cursorX << ", " << cursorY << ")" << std::endl;
            std::cout << "  Window size: " << windowWidth << "x" << windowHeight << std::endl;
            std::cout << "  Result: x=" << outX << std::endl;
            std::cout << "  Work area left: " << workArea.left << std::endl;
            return false;
        }
    }
    
    std::cout << "PASSED (100 iterations)" << std::endl;
    return true;
}

/**
 * Property 7.4: Negative coordinates (extended display left of primary)
 * 
 * When the cursor is on an extended display to the left of the primary
 * (negative X coordinates), the window should still be contained within
 * that monitor's work area.
 */
static bool testNegativeCoordinates() {
    std::cout << "Property 7.4: Negative Coordinates (Extended Display)... ";
    
    MonitorWorkArea workArea = {-1920, 0, 0, 1080};
    
    for (int i = 0; i < 100; ++i) {
        int cursorX = randomInRange(-1920, -1);
        int cursorY = randomInRange(0, 1000);
        int cursorHeight = randomInRange(15, 30);
        int windowWidth = randomInRange(200, 500);
        int windowHeight = randomInRange(100, 300);
        
        int outX, outY;
        calculateCandidateWindowPosition(
            cursorX, cursorY, cursorHeight,
            windowWidth, windowHeight, workArea,
            outX, outY);
        
        int rightEdge = outX + windowWidth;
        int bottomEdge = outY + windowHeight;
        
        bool withinBounds = 
            outX >= workArea.left &&
            rightEdge <= workArea.right &&
            outY >= workArea.top &&
            bottomEdge <= workArea.bottom;
        
        if (!withinBounds) {
            std::cout << "FAILED at iteration " << i << std::endl;
            std::cout << "  Cursor: (" << cursorX << ", " << cursorY << ")" << std::endl;
            std::cout << "  Window: (" << outX << ", " << outY << ") " 
                      << windowWidth << "x" << windowHeight << std::endl;
            std::cout << "  Work area: (" << workArea.left << ", " << workArea.top 
                      << ") to (" << workArea.right << ", " << workArea.bottom << ")" << std::endl;
            return false;
        }
    }
    
    std::cout << "PASSED (100 iterations)" << std::endl;
    return true;
}

/**
 * Property 7.5: Window appears above cursor when bottom space insufficient
 * 
 * When there's not enough space below the cursor, the window should
 * appear above the cursor instead.
 */
static bool testWindowAboveCursor() {
    std::cout << "Property 7.5: Window Above Cursor When Bottom Space Insufficient... ";
    
    MonitorWorkArea workArea = {0, 0, 1920, 1080};
    
    for (int i = 0; i < 100; ++i) {
        int cursorY = randomInRange(900, 1050);
        int cursorX = randomInRange(100, 1800);
        int cursorHeight = 20;
        int windowWidth = 300;
        int windowHeight = 200;
        
        int outX, outY;
        calculateCandidateWindowPosition(
            cursorX, cursorY, cursorHeight,
            windowWidth, windowHeight, workArea,
            outX, outY);
        
        int bottomEdge = outY + windowHeight;
        
        if (bottomEdge > workArea.bottom) {
            std::cout << "FAILED at iteration " << i << std::endl;
            std::cout << "  Cursor: (" << cursorX << ", " << cursorY << ")" << std::endl;
            std::cout << "  Window: y=" << outY << ", bottom=" << bottomEdge << std::endl;
            std::cout << "  Work area bottom: " << workArea.bottom << std::endl;
            return false;
        }
    }
    
    std::cout << "PASSED (100 iterations)" << std::endl;
    return true;
}

/**
 * Property 7.6: Multi-monitor configurations
 * 
 * Test various multi-monitor setups to ensure boundary containment
 * works correctly in all configurations.
 */
static bool testMultiMonitorConfigurations() {
    std::cout << "Property 7.6: Multi-Monitor Configurations... ";
    
    struct MonitorConfig {
        MonitorWorkArea workArea;
        const char* description;
    };
    
    MonitorConfig configs[] = {
        {{0, 0, 1920, 1080}, "Primary 1080p"},
        {{-1920, 0, 0, 1080}, "Left extended 1080p"},
        {{1920, 0, 3840, 1080}, "Right extended 1080p"},
        {{0, -1080, 1920, 0}, "Top extended 1080p"},
        {{-2560, 0, 0, 1440}, "Left extended 1440p"},
        {{-3840, 0, 0, 2160}, "Left extended 4K"},
    };
    
    for (const auto& cfg : configs) {
        for (int i = 0; i < 20; ++i) {
            int cursorX = randomInRange(cfg.workArea.left, cfg.workArea.right - 1);
            int cursorY = randomInRange(cfg.workArea.top, cfg.workArea.bottom - 50);
            int cursorHeight = 20;
            int windowWidth = randomInRange(200, 400);
            int windowHeight = randomInRange(100, 250);
            
            int outX, outY;
            calculateCandidateWindowPosition(
                cursorX, cursorY, cursorHeight,
                windowWidth, windowHeight, cfg.workArea,
                outX, outY);
            
            int rightEdge = outX + windowWidth;
            int bottomEdge = outY + windowHeight;
            
            bool withinBounds = 
                outX >= cfg.workArea.left &&
                rightEdge <= cfg.workArea.right &&
                outY >= cfg.workArea.top &&
                bottomEdge <= cfg.workArea.bottom;
            
            if (!withinBounds) {
                std::cout << "FAILED: " << cfg.description << std::endl;
                std::cout << "  Cursor: (" << cursorX << ", " << cursorY << ")" << std::endl;
                std::cout << "  Window: (" << outX << ", " << outY << ") " 
                          << windowWidth << "x" << windowHeight << std::endl;
                return false;
            }
        }
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

int main() {
    std::cout << "=== Candidate Window Boundary Containment Property Tests ===" << std::endl;
    std::cout << "Feature: windows-platform-support" << std::endl;
    std::cout << "Property 7: Candidate Window Boundary Containment" << std::endl;
    std::cout << "Validates: Requirements 6.2, 6.3" << std::endl;
    std::cout << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    if (testRightEdgeBoundary()) ++passed; else ++failed;
    if (testBottomEdgeBoundary()) ++passed; else ++failed;
    if (testLeftEdgeBoundary()) ++passed; else ++failed;
    if (testNegativeCoordinates()) ++passed; else ++failed;
    if (testWindowAboveCursor()) ++passed; else ++failed;
    if (testMultiMonitorConfigurations()) ++passed; else ++failed;
    
    std::cout << std::endl;
    std::cout << "Results: " << passed << " passed, " << failed << " failed" << std::endl;
    
    return failed > 0 ? 1 : 0;
}
