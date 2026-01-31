/**
 * VirtualKey to RimeKey Conversion Property Tests
 * Task 9.3: 编写键码转换属性测试
 * 
 * Feature: windows-platform-support
 * Property 5: VirtualKey to RimeKey Conversion
 * 
 * Validates: Requirements 4.10
 */

#include <iostream>
#include <random>
#include <vector>
#include <string>
#include <cstdint>

#ifndef _WIN32

#define VK_BACK           0x08
#define VK_TAB            0x09
#define VK_RETURN         0x0D
#define VK_ESCAPE         0x1B
#define VK_SPACE          0x20
#define VK_PRIOR          0x21
#define VK_NEXT           0x22
#define VK_END            0x23
#define VK_HOME           0x24
#define VK_LEFT           0x25
#define VK_UP             0x26
#define VK_RIGHT          0x27
#define VK_DOWN           0x28
#define VK_INSERT         0x2D
#define VK_DELETE         0x2E
#define VK_F1             0x70
#define VK_F2             0x71
#define VK_F3             0x72
#define VK_F4             0x73
#define VK_F5             0x74
#define VK_F6             0x75
#define VK_F7             0x76
#define VK_F8             0x77
#define VK_F9             0x78
#define VK_F10            0x79
#define VK_F11            0x7A
#define VK_F12            0x7B
#define VK_OEM_1          0xBA
#define VK_OEM_PLUS       0xBB
#define VK_OEM_COMMA      0xBC
#define VK_OEM_MINUS      0xBD
#define VK_OEM_PERIOD     0xBE
#define VK_OEM_2          0xBF
#define VK_OEM_3          0xC0
#define VK_OEM_4          0xDB
#define VK_OEM_5          0xDC
#define VK_OEM_6          0xDD
#define VK_OEM_7          0xDE

#else
#include <windows.h>
#endif

namespace {

constexpr uint32_t ModNone    = 0x00;
constexpr uint32_t ModShift   = 0x01;
constexpr uint32_t ModControl = 0x02;
constexpr uint32_t ModAlt     = 0x04;

struct KeyMapping {
    int vk;
    int rimeKey;
};

const KeyMapping SPECIAL_KEYS[] = {
    {VK_SPACE,   0x20},
    {VK_RETURN,  0xff0d},
    {VK_BACK,    0xff08},
    {VK_ESCAPE,  0xff1b},
    {VK_TAB,     0xff09},
    {VK_LEFT,    0xff51},
    {VK_UP,      0xff52},
    {VK_RIGHT,   0xff53},
    {VK_DOWN,    0xff54},
    {VK_PRIOR,   0xff55},
    {VK_NEXT,    0xff56},
    {VK_HOME,    0xff50},
    {VK_END,     0xff57},
    {VK_DELETE,  0xffff},
    {VK_INSERT,  0xff63},
};

struct OEMKeyMapping {
    int vk;
    char normal;
    char shifted;
};

const OEMKeyMapping OEM_KEYS[] = {
    {VK_OEM_1,      ';', ':'},
    {VK_OEM_PLUS,   '=', '+'},
    {VK_OEM_COMMA,  ',', '<'},
    {VK_OEM_MINUS,  '-', '_'},
    {VK_OEM_PERIOD, '.', '>'},
    {VK_OEM_2,      '/', '?'},
    {VK_OEM_3,      '`', '~'},
    {VK_OEM_4,      '[', '{'},
    {VK_OEM_5,      '\\', '|'},
    {VK_OEM_6,      ']', '}'},
    {VK_OEM_7,      '\'', '"'},
};

int convertVirtualKeyToRimeKey(int vk, int modifiers) {
    for (const auto& mapping : SPECIAL_KEYS) {
        if (mapping.vk == vk) {
            return mapping.rimeKey;
        }
    }

    if (vk >= 'A' && vk <= 'Z') {
        bool shift = (modifiers & ModShift) != 0;
        if (shift) {
            return vk;
        } else {
            return vk + ('a' - 'A');
        }
    }

    if (vk >= '0' && vk <= '9') {
        return vk;
    }

    for (const auto& mapping : OEM_KEYS) {
        if (mapping.vk == vk) {
            bool shift = (modifiers & ModShift) != 0;
            return shift ? mapping.shifted : mapping.normal;
        }
    }

    if (vk >= VK_F1 && vk <= VK_F12) {
        return 0xffbe + (vk - VK_F1);
    }

    return 0;
}

std::mt19937 g_rng(42);

/**
 * Property 5: VirtualKey to RimeKey Conversion
 * 
 * For any Windows VirtualKey and modifier combination, convertVirtualKeyToRimeKey
 * should return the correct RIME key code.
 * 
 * Sub-properties:
 * - Letter keys A-Z should return lowercase (a-z) without Shift, uppercase (A-Z) with Shift
 * - OEM keys should return correct character based on Shift state
 * - Special keys should return corresponding RIME key codes
 * 
 * Validates: Requirements 4.10
 */

bool testProperty5_LetterKeyConversion() {
    std::cout << "Property 5.1: Letter Key Conversion... ";

    for (int i = 0; i < 100; ++i) {
        int vk = 'A' + (g_rng() % 26);
        
        int rimeKeyNoShift = convertVirtualKeyToRimeKey(vk, ModNone);
        int expectedNoShift = vk + ('a' - 'A');
        
        if (rimeKeyNoShift != expectedNoShift) {
            std::cout << "FAILED at iteration " << i << std::endl;
            std::cout << "  VK=0x" << std::hex << vk << " without Shift: expected 0x" 
                      << expectedNoShift << ", got 0x" << rimeKeyNoShift << std::dec << std::endl;
            return false;
        }

        int rimeKeyWithShift = convertVirtualKeyToRimeKey(vk, ModShift);
        int expectedWithShift = vk;
        
        if (rimeKeyWithShift != expectedWithShift) {
            std::cout << "FAILED at iteration " << i << std::endl;
            std::cout << "  VK=0x" << std::hex << vk << " with Shift: expected 0x" 
                      << expectedWithShift << ", got 0x" << rimeKeyWithShift << std::dec << std::endl;
            return false;
        }
    }

    std::cout << "PASSED (100 iterations)" << std::endl;
    return true;
}

bool testProperty5_DigitKeyConversion() {
    std::cout << "Property 5.2: Digit Key Conversion... ";

    for (int i = 0; i < 100; ++i) {
        int vk = '0' + (g_rng() % 10);
        
        int rimeKey = convertVirtualKeyToRimeKey(vk, ModNone);
        
        if (rimeKey != vk) {
            std::cout << "FAILED at iteration " << i << std::endl;
            std::cout << "  Digit VK=0x" << std::hex << vk << ": expected 0x" 
                      << vk << ", got 0x" << rimeKey << std::dec << std::endl;
            return false;
        }
    }

    std::cout << "PASSED (100 iterations)" << std::endl;
    return true;
}

bool testProperty5_OEMKeyConversion() {
    std::cout << "Property 5.3: OEM Key Conversion... ";

    for (int i = 0; i < 100; ++i) {
        int idx = g_rng() % (sizeof(OEM_KEYS) / sizeof(OEM_KEYS[0]));
        const auto& mapping = OEM_KEYS[idx];
        
        int rimeKeyNoShift = convertVirtualKeyToRimeKey(mapping.vk, ModNone);
        if (rimeKeyNoShift != mapping.normal) {
            std::cout << "FAILED at iteration " << i << std::endl;
            std::cout << "  OEM VK=0x" << std::hex << mapping.vk << " without Shift: expected '" 
                      << mapping.normal << "' (0x" << static_cast<int>(mapping.normal) 
                      << "), got 0x" << rimeKeyNoShift << std::dec << std::endl;
            return false;
        }

        int rimeKeyWithShift = convertVirtualKeyToRimeKey(mapping.vk, ModShift);
        if (rimeKeyWithShift != mapping.shifted) {
            std::cout << "FAILED at iteration " << i << std::endl;
            std::cout << "  OEM VK=0x" << std::hex << mapping.vk << " with Shift: expected '" 
                      << mapping.shifted << "' (0x" << static_cast<int>(mapping.shifted) 
                      << "), got 0x" << rimeKeyWithShift << std::dec << std::endl;
            return false;
        }
    }

    std::cout << "PASSED (100 iterations)" << std::endl;
    return true;
}

bool testProperty5_SpecialKeyConversion() {
    std::cout << "Property 5.4: Special Key Conversion... ";

    for (int i = 0; i < 100; ++i) {
        int idx = g_rng() % (sizeof(SPECIAL_KEYS) / sizeof(SPECIAL_KEYS[0]));
        const auto& mapping = SPECIAL_KEYS[idx];
        
        int rimeKey = convertVirtualKeyToRimeKey(mapping.vk, ModNone);
        if (rimeKey != mapping.rimeKey) {
            std::cout << "FAILED at iteration " << i << std::endl;
            std::cout << "  Special VK=0x" << std::hex << mapping.vk << ": expected 0x" 
                      << mapping.rimeKey << ", got 0x" << rimeKey << std::dec << std::endl;
            return false;
        }
    }

    std::cout << "PASSED (100 iterations)" << std::endl;
    return true;
}

bool testProperty5_FunctionKeyConversion() {
    std::cout << "Property 5.5: Function Key Conversion... ";

    for (int i = 0; i < 100; ++i) {
        int fKeyIndex = g_rng() % 12;
        int vk = VK_F1 + fKeyIndex;
        int expectedRimeKey = 0xffbe + fKeyIndex;
        
        int rimeKey = convertVirtualKeyToRimeKey(vk, ModNone);
        if (rimeKey != expectedRimeKey) {
            std::cout << "FAILED at iteration " << i << std::endl;
            std::cout << "  F" << (fKeyIndex + 1) << " (VK=0x" << std::hex << vk 
                      << "): expected 0x" << expectedRimeKey << ", got 0x" << rimeKey 
                      << std::dec << std::endl;
            return false;
        }
    }

    std::cout << "PASSED (100 iterations)" << std::endl;
    return true;
}

}  // namespace

int main() {
    std::cout << "=== VirtualKey to RimeKey Conversion Property Tests ===" << std::endl;
    std::cout << "Feature: windows-platform-support" << std::endl;
    std::cout << "Task 9.3: 编写键码转换属性测试" << std::endl;
    std::cout << "Property 5: VirtualKey to RimeKey Conversion" << std::endl;
    std::cout << "Validates: Requirements 4.10" << std::endl;
    std::cout << std::endl;

    int passed = 0;
    int failed = 0;

    if (testProperty5_LetterKeyConversion()) ++passed; else ++failed;
    if (testProperty5_DigitKeyConversion()) ++passed; else ++failed;
    if (testProperty5_OEMKeyConversion()) ++passed; else ++failed;
    if (testProperty5_SpecialKeyConversion()) ++passed; else ++failed;
    if (testProperty5_FunctionKeyConversion()) ++passed; else ++failed;

    std::cout << std::endl;
    std::cout << "Results: " << passed << " passed, " << failed << " failed" << std::endl;

    return failed > 0 ? 1 : 0;
}
