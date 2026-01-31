/**
 * Candidate Window Position Visualization Test Tool
 * 
 * Feature: windows-platform-support
 * Task 10.4: 实现候选窗口位置可视化测试工具
 * Validates: Requirements 6.1, 6.2, 6.3
 * 
 * This tool provides visual verification that the candidate window
 * appears correctly near the input cursor in various scenarios.
 */

#ifdef _WIN32

#include <windows.h>
#include <iostream>
#include <string>
#include <cstdint>

namespace {

struct MonitorWorkArea {
    int left;
    int top;
    int right;
    int bottom;
};

void calculateCandidateWindowPosition(
    int cursorX, int cursorY, int cursorHeight,
    int windowWidth, int windowHeight,
    int& outX, int& outY)
{
    POINT cursorPt = { cursorX, cursorY };
    HMONITOR hMonitor = MonitorFromPoint(cursorPt, MONITOR_DEFAULTTONEAREST);
    
    MONITORINFO mi;
    mi.cbSize = sizeof(MONITORINFO);
    
    if (!hMonitor || !GetMonitorInfoW(hMonitor, &mi)) {
        outX = cursorX;
        outY = cursorY + cursorHeight + 2;
        return;
    }
    
    const RECT& workArea = mi.rcWork;
    
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

const int MAX_HORIZONTAL_OFFSET = 50;
const int MAX_VERTICAL_GAP = 30;

bool verifyCandidateNearCursor(int windowX, int windowY, int windowWidth, int windowHeight,
                               int cursorX, int cursorY, int cursorHeight,
                               const MONITORINFO& mi,
                               std::string& errorMsg) {
    int horizontalOffset = abs(windowX - cursorX);
    
    int spaceOnRight = mi.rcWork.right - cursorX;
    bool atRightEdge = spaceOnRight < windowWidth;
    
    if (!atRightEdge && horizontalOffset > MAX_HORIZONTAL_OFFSET) {
        errorMsg = "Horizontal offset too large: " + std::to_string(horizontalOffset) + "px (max: " + std::to_string(MAX_HORIZONTAL_OFFSET) + "px)";
        return false;
    }
    
    int windowBottom = windowY + windowHeight;
    int cursorBottom = cursorY + cursorHeight;
    
    bool belowCursor = windowY >= cursorBottom;
    bool aboveCursor = windowBottom <= cursorY;
    
    if (belowCursor) {
        int gap = windowY - cursorBottom;
        if (gap > MAX_VERTICAL_GAP) {
            errorMsg = "Window too far below cursor: " + std::to_string(gap) + "px (max: " + std::to_string(MAX_VERTICAL_GAP) + "px)";
            return false;
        }
    } else if (aboveCursor) {
        int gap = cursorY - windowBottom;
        if (gap > MAX_VERTICAL_GAP) {
            errorMsg = "Window too far above cursor: " + std::to_string(gap) + "px (max: " + std::to_string(MAX_VERTICAL_GAP) + "px)";
            return false;
        }
    } else {
        errorMsg = "Window overlaps with cursor";
        return false;
    }
    
    return true;
}

struct TestScenario {
    const char* name;
    int cursorX;
    int cursorY;
    int cursorHeight;
    int windowWidth;
    int windowHeight;
};

bool runScenario(const TestScenario& scenario, const MONITORINFO& mi) {
    std::cout << "  " << scenario.name << "... ";
    
    int outX, outY;
    calculateCandidateWindowPosition(
        scenario.cursorX, scenario.cursorY, scenario.cursorHeight,
        scenario.windowWidth, scenario.windowHeight,
        outX, outY);
    
    int rightEdge = outX + scenario.windowWidth;
    int bottomEdge = outY + scenario.windowHeight;
    
    bool withinBounds = 
        outX >= mi.rcWork.left &&
        rightEdge <= mi.rcWork.right &&
        outY >= mi.rcWork.top &&
        bottomEdge <= mi.rcWork.bottom;
    
    if (!withinBounds) {
        std::cout << "FAILED (out of bounds)" << std::endl;
        std::cout << "    Window: (" << outX << ", " << outY << ") " 
                  << scenario.windowWidth << "x" << scenario.windowHeight << std::endl;
        std::cout << "    Work area: (" << mi.rcWork.left << ", " << mi.rcWork.top 
                  << ") to (" << mi.rcWork.right << ", " << mi.rcWork.bottom << ")" << std::endl;
        return false;
    }
    
    std::string errorMsg;
    bool nearCursor = verifyCandidateNearCursor(
        outX, outY, scenario.windowWidth, scenario.windowHeight,
        scenario.cursorX, scenario.cursorY, scenario.cursorHeight,
        mi, errorMsg);
    
    if (!nearCursor) {
        std::cout << "FAILED (" << errorMsg << ")" << std::endl;
        std::cout << "    Cursor: (" << scenario.cursorX << ", " << scenario.cursorY << ")" << std::endl;
        std::cout << "    Window: (" << outX << ", " << outY << ")" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    std::cout << "    Cursor: (" << scenario.cursorX << ", " << scenario.cursorY 
              << ") -> Window: (" << outX << ", " << outY << ")" << std::endl;
    return true;
}

}  // namespace

int main() {
    std::cout << "=== Candidate Window Position Visualization Test ===" << std::endl;
    std::cout << "Feature: windows-platform-support" << std::endl;
    std::cout << "Task 10.4: Candidate Window Position Test Tool" << std::endl;
    std::cout << "Validates: Requirements 6.1, 6.2, 6.3" << std::endl;
    std::cout << std::endl;
    
    POINT pt = {0, 0};
    HMONITOR hPrimary = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO primaryMi;
    primaryMi.cbSize = sizeof(MONITORINFO);
    
    if (!hPrimary || !GetMonitorInfoW(hPrimary, &primaryMi)) {
        std::cerr << "Failed to get primary monitor info" << std::endl;
        return 1;
    }
    
    std::cout << "Primary Monitor:" << std::endl;
    std::cout << "  Monitor: (" << primaryMi.rcMonitor.left << ", " << primaryMi.rcMonitor.top 
              << ") to (" << primaryMi.rcMonitor.right << ", " << primaryMi.rcMonitor.bottom << ")" << std::endl;
    std::cout << "  Work Area: (" << primaryMi.rcWork.left << ", " << primaryMi.rcWork.top 
              << ") to (" << primaryMi.rcWork.right << ", " << primaryMi.rcWork.bottom << ")" << std::endl;
    std::cout << std::endl;
    
    int workWidth = primaryMi.rcWork.right - primaryMi.rcWork.left;
    int workHeight = primaryMi.rcWork.bottom - primaryMi.rcWork.top;
    
    TestScenario scenarios[] = {
        {"Normal position (center)", workWidth / 2, workHeight / 2, 20, 300, 150},
        {"Near right edge", workWidth - 50, workHeight / 2, 20, 300, 150},
        {"Near bottom edge", workWidth / 2, workHeight - 50, 20, 300, 150},
        {"Bottom-right corner", workWidth - 50, workHeight - 50, 20, 300, 150},
        {"Top-left corner", 10, 10, 20, 300, 150},
        {"Top-right corner", workWidth - 50, 10, 20, 300, 150},
        {"Large window near edge", workWidth - 100, workHeight - 100, 20, 500, 300},
    };
    
    std::cout << "Running test scenarios on primary monitor:" << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    for (const auto& scenario : scenarios) {
        if (runScenario(scenario, primaryMi)) {
            ++passed;
        } else {
            ++failed;
        }
    }
    
    std::cout << std::endl;
    std::cout << "Results: " << passed << " passed, " << failed << " failed" << std::endl;
    
    return failed > 0 ? 1 : 0;
}

#else

#include <iostream>

int main() {
    std::cout << "This test is Windows-only." << std::endl;
    return 0;
}

#endif
