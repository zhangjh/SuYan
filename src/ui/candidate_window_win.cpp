/**
 * CandidateWindow Windows 特定实现
 *
 * 使用 Windows API 设置窗口属性，确保候选词窗口
 * 在所有应用窗口之上显示，包括全屏应用。
 *
 * 关键技术点：
 * 1. 窗口置顶：使用 HWND_TOPMOST 确保在所有窗口之上
 * 2. 焦点控制：设置 WS_EX_NOACTIVATE 防止获取焦点
 * 3. 全屏支持：检测全屏应用并确保窗口可见
 * 4. 多显示器支持：使用 MonitorFromPoint 获取光标所在显示器
 * 5. 负坐标支持：正确处理扩展屏幕在主屏幕左侧/上方的情况
 */

#include "candidate_window.h"

#ifdef Q_OS_WIN

#include <windows.h>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

namespace suyan {

/**
 * 检测指定窗口是否为全屏窗口
 */
static bool isWindowFullScreen(HWND hwnd) {
    if (!hwnd || !IsWindowVisible(hwnd)) {
        return false;
    }
    
    RECT windowRect;
    if (!GetWindowRect(hwnd, &windowRect)) {
        return false;
    }
    
    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    if (!monitor) {
        return false;
    }
    
    MONITORINFO monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFO);
    if (!GetMonitorInfo(monitor, &monitorInfo)) {
        return false;
    }
    
    return (windowRect.left <= monitorInfo.rcMonitor.left &&
            windowRect.top <= monitorInfo.rcMonitor.top &&
            windowRect.right >= monitorInfo.rcMonitor.right &&
            windowRect.bottom >= monitorInfo.rcMonitor.bottom);
}

/**
 * 检测是否有全屏应用正在运行
 */
static bool isFullScreenAppRunning() {
    HWND foreground = GetForegroundWindow();
    return isWindowFullScreen(foreground);
}

/**
 * 计算候选窗口位置（Windows 特定实现）
 * 
 * 使用 MonitorFromPoint 获取光标所在显示器，
 * 处理屏幕边缘情况，支持负坐标（扩展屏幕）。
 * 
 * @param cursorX 光标 X 坐标（屏幕坐标，可为负数）
 * @param cursorY 光标 Y 坐标（屏幕坐标，可为负数）
 * @param cursorHeight 光标高度
 * @param windowWidth 候选窗口宽度
 * @param windowHeight 候选窗口高度
 * @param outX 输出：窗口 X 坐标
 * @param outY 输出：窗口 Y 坐标
 */
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

/**
 * 设置 Windows 窗口属性
 *
 * 配置窗口以满足输入法候选词窗口的特殊需求：
 * 1. 显示在所有普通窗口之上（包括全屏应用）
 * 2. 不获取键盘焦点
 * 3. 不在任务栏显示
 */
void CandidateWindow::setupWindowsWindowLevel() {
    WId winId = this->winId();
    if (winId == 0) {
        return;
    }
    
    HWND hwnd = reinterpret_cast<HWND>(winId);
    if (!hwnd) {
        return;
    }
    
    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    exStyle |= WS_EX_NOACTIVATE | WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);
    
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    
    BOOL disableTransitions = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_TRANSITIONS_FORCEDISABLED,
                          &disableTransitions, sizeof(disableTransitions));
}

/**
 * 确保窗口在全屏应用中正确显示
 */
void CandidateWindow::ensureVisibleInFullScreen() {
    WId winId = this->winId();
    if (winId == 0) {
        return;
    }
    
    HWND hwnd = reinterpret_cast<HWND>(winId);
    if (!hwnd) {
        return;
    }
    
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    
    if (isFullScreenAppRunning()) {
        InvalidateRect(hwnd, nullptr, FALSE);
        UpdateWindow(hwnd);
    }
}

/**
 * 使用 Native Windows API 显示窗口在指定位置
 */
void CandidateWindow::showAtNativeImpl(const QPoint& pos) {
    WId winId = this->winId();
    if (winId == 0) {
        return;
    }
    
    HWND hwnd = reinterpret_cast<HWND>(winId);
    if (!hwnd) {
        return;
    }
    
    SetWindowPos(hwnd, HWND_TOPMOST, pos.x(), pos.y(), 0, 0,
                 SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

} // namespace suyan

#endif // Q_OS_WIN
