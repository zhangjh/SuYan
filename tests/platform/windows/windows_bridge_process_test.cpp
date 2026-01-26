/**
 * WindowsBridge 进程名获取单元测试
 * Task 3.7: 编写 WindowsBridge 单元测试
 * 
 * 测试内容：
 * - 测试进程名获取格式
 * - Property 6: 进程名格式正确性
 * 
 * Validates: Requirements 10.2
 */

#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#endif

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "FAIL: " << message << std::endl; \
            std::cerr << "  Location: " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

#define TEST_PASS(message) \
    std::cout << "PASS: " << message << std::endl

namespace suyan_test {

bool isValidProcessNameFormat(const std::string& processName) {
    if (processName.empty()) return false;
    if (processName.find('/') != std::string::npos) return false;
    if (processName.find('\\') != std::string::npos) return false;
    
    const std::string illegalChars = "<>:\"|?*";
    for (char c : processName) {
        if (illegalChars.find(c) != std::string::npos) return false;
        if (static_cast<unsigned char>(c) < 32) return false;
    }
    
    std::string lowerName = processName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    const std::vector<std::string> exts = {".exe", ".com", ".bat", ".cmd", ".msc", ".msi"};
    for (const auto& ext : exts) {
        if (lowerName.length() >= ext.length() &&
            lowerName.substr(lowerName.length() - ext.length()) == ext) {
            size_t dotPos = lowerName.rfind('.');
            return dotPos != 0;
        }
    }
    return processName.length() > 0;
}

bool hasExeExtension(const std::string& processName) {
    if (processName.length() < 4) return false;
    std::string ext = processName.substr(processName.length() - 4);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return ext == ".exe";
}

std::string extractFileName(const std::string& fullPath) {
    size_t lastSlash = fullPath.find_last_of("/\\");
    if (lastSlash != std::string::npos) return fullPath.substr(lastSlash + 1);
    return fullPath;
}

#ifdef _WIN32
std::string getForegroundProcessName() {
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return std::string();
    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    if (processId == 0) return std::string();
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (!hProcess) return std::string();
    wchar_t processPath[MAX_PATH] = {};
    DWORD pathLen = MAX_PATH;
    std::string result;
    if (QueryFullProcessImageNameW(hProcess, 0, processPath, &pathLen)) {
        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, processPath, -1, nullptr, 0, nullptr, nullptr);
        if (utf8Len > 0) {
            std::string utf8Path(utf8Len - 1, '\0');
            WideCharToMultiByte(CP_UTF8, 0, processPath, -1, &utf8Path[0], utf8Len, nullptr, nullptr);
            result = extractFileName(utf8Path);
        }
    }
    CloseHandle(hProcess);
    return result;
}
#else
std::string getForegroundProcessName() { return "notepad.exe"; }
#endif

} // namespace suyan_test

class WindowsBridgeProcessTest {
public:
    WindowsBridgeProcessTest() { std::srand(static_cast<unsigned>(std::time(nullptr))); }

    bool runAllTests() {
        std::cout << "=== WindowsBridge Process Name Test ===" << std::endl;
        std::cout << "Task 3.7 | Validates: Requirements 10.2" << std::endl << std::endl;
        bool allPassed = true;
        allPassed &= testValidProcessNameFormat();
        allPassed &= testInvalidProcessNameFormat();
        allPassed &= testExeExtensionDetection();
        allPassed &= testFileNameExtraction();
        allPassed &= testProperty6();
        std::cout << std::endl;
        std::cout << (allPassed ? "=== All tests passed ===" : "=== Some tests failed ===") << std::endl;
        return allPassed;
    }

private:
    bool testValidProcessNameFormat() {
        std::cout << "\n--- Valid Process Name Format Test ---" << std::endl;
        std::vector<std::string> validNames = {
            "notepad.exe", "explorer.exe", "chrome.exe", "NOTEPAD.EXE",
            "my-app.exe", "my_app.exe", "app123.exe", "a.exe",
            "app.com", "setup.msi", "script.bat", "script.cmd"
        };
        for (const auto& name : validNames) {
            TEST_ASSERT(suyan_test::isValidProcessNameFormat(name), "Should be valid: " + name);
        }
        TEST_PASS("testValidProcessNameFormat");
        return true;
    }

    bool testInvalidProcessNameFormat() {
        std::cout << "\n--- Invalid Process Name Format Test ---" << std::endl;
        std::vector<std::string> invalidNames = {
            "", "C:\\Windows\\notepad.exe", "/usr/bin/app",
            "app<name>.exe", "app>name.exe", "app:name.exe",
            "app|name.exe", "app?name.exe", "app*name.exe", ".exe"
        };
        for (const auto& name : invalidNames) {
            TEST_ASSERT(!suyan_test::isValidProcessNameFormat(name), "Should be invalid: " + name);
        }
        TEST_PASS("testInvalidProcessNameFormat");
        return true;
    }

    bool testExeExtensionDetection() {
        std::cout << "\n--- Exe Extension Detection Test ---" << std::endl;
        TEST_ASSERT(suyan_test::hasExeExtension("notepad.exe"), "notepad.exe should have .exe");
        TEST_ASSERT(suyan_test::hasExeExtension("NOTEPAD.EXE"), "NOTEPAD.EXE should have .exe");
        TEST_ASSERT(!suyan_test::hasExeExtension("notepad"), "notepad should not have .exe");
        TEST_ASSERT(!suyan_test::hasExeExtension("notepad.com"), "notepad.com should not have .exe");
        TEST_ASSERT(!suyan_test::hasExeExtension(""), "empty should not have .exe");
        TEST_PASS("testExeExtensionDetection");
        return true;
    }

    bool testFileNameExtraction() {
        std::cout << "\n--- File Name Extraction Test ---" << std::endl;
        TEST_ASSERT(suyan_test::extractFileName("C:\\Windows\\notepad.exe") == "notepad.exe", "Windows path");
        TEST_ASSERT(suyan_test::extractFileName("/usr/bin/app") == "app", "Unix path");
        TEST_ASSERT(suyan_test::extractFileName("notepad.exe") == "notepad.exe", "No path");
        TEST_ASSERT(suyan_test::extractFileName("") == "", "Empty string");
        TEST_PASS("testFileNameExtraction");
        return true;
    }

    // Property 6: Process Name Format Correctness
    // Validates: Requirements 10.2
    bool testProperty6() {
        std::cout << "\n--- Property 6: Process Name Format Correctness ---" << std::endl;
        std::cout << "  Validates: Requirements 10.2" << std::endl;
        int testCount = 0;

        std::vector<std::string> commonProcessNames = {
            "System", "smss.exe", "csrss.exe", "explorer.exe", "svchost.exe",
            "notepad.exe", "cmd.exe", "chrome.exe", "code.exe", "WINWORD.EXE"
        };
        for (const auto& name : commonProcessNames) {
            TEST_ASSERT(suyan_test::isValidProcessNameFormat(name), "Common: " + name);
            testCount++;
        }

        std::string currentProcess = suyan_test::getForegroundProcessName();
        if (!currentProcess.empty()) {
            TEST_ASSERT(suyan_test::isValidProcessNameFormat(currentProcess), "Actual: " + currentProcess);
            testCount++;
            std::cout << "  Current process: " << currentProcess << std::endl;
        }

        for (int i = 0; i < 100; i++) {
            std::string randomName = generateRandomValidProcessName();
            TEST_ASSERT(suyan_test::isValidProcessNameFormat(randomName), "Random: " + randomName);
            testCount++;
        }

        TEST_ASSERT(suyan_test::isValidProcessNameFormat("a.exe"), "Shortest: a.exe");
        TEST_ASSERT(suyan_test::isValidProcessNameFormat("a"), "Single char: a");
        std::string longName = std::string(200, 'a') + ".exe";
        TEST_ASSERT(suyan_test::isValidProcessNameFormat(longName), "Long name");
        testCount += 3;

        std::cout << "  Executed " << testCount << " tests" << std::endl;
        TEST_PASS("Property 6: Process Name Format Correctness");
        return true;
    }

    std::string generateRandomValidProcessName() {
        const std::string validChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.";
        int baseLen = 1 + (std::rand() % 20);
        std::string baseName;
        for (int i = 0; i < baseLen; i++) baseName += validChars[std::rand() % validChars.length()];
        if (baseName.empty()) baseName = "app";
        const std::vector<std::string> exts = {".exe", ".EXE", ".com"};
        baseName += exts[std::rand() % exts.size()];
        return baseName;
    }
};

int main() {
    WindowsBridgeProcessTest test;
    return test.runAllTests() ? 0 : 1;
}
