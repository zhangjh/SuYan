/**
 * 编码转换单元测试
 * Task 3.2: 编写编码转换单元测试
 * 
 * 测试内容：
 * - 测试中文、英文、混合文本的转换
 * - Property 4: UTF-8/UTF-16 编码转换往返一致性
 * 
 * Validates: Requirements 3.2
 * 
 * 注意：此测试文件设计为跨平台编译，在非 Windows 平台上
 * 使用标准库实现进行测试。
 */

#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <cstring>

// ========== 平台兼容层 ==========
// 在非 Windows 平台上使用标准库实现编码转换

#ifdef _WIN32
#include <windows.h>
#else

// 非 Windows 平台：使用 codecvt 或手动实现 UTF-8/UTF-16 转换
#include <locale>
#include <codecvt>

#endif // _WIN32

// ========== 编码转换函数实现 ==========
// 这是 WindowsBridge 中编码转换函数的独立实现，用于跨平台测试

namespace suyan_test {

#ifdef _WIN32

// Windows 平台：使用 Windows API
std::wstring utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) {
        return std::wstring();
    }
    
    int wideLen = MultiByteToWideChar(
        CP_UTF8, 
        0, 
        utf8.c_str(), 
        static_cast<int>(utf8.length()), 
        nullptr, 
        0
    );
    
    if (wideLen == 0) {
        return std::wstring();
    }
    
    std::wstring wide(wideLen, L'\0');
    MultiByteToWideChar(
        CP_UTF8, 
        0, 
        utf8.c_str(), 
        static_cast<int>(utf8.length()), 
        &wide[0], 
        wideLen
    );
    
    return wide;
}

std::string wideToUtf8(const std::wstring& wide) {
    if (wide.empty()) {
        return std::string();
    }
    
    int utf8Len = WideCharToMultiByte(
        CP_UTF8, 
        0, 
        wide.c_str(), 
        static_cast<int>(wide.length()), 
        nullptr, 
        0, 
        nullptr, 
        nullptr
    );
    
    if (utf8Len == 0) {
        return std::string();
    }
    
    std::string utf8(utf8Len, '\0');
    WideCharToMultiByte(
        CP_UTF8, 
        0, 
        wide.c_str(), 
        static_cast<int>(wide.length()), 
        &utf8[0], 
        utf8Len, 
        nullptr, 
        nullptr
    );
    
    return utf8;
}

#else

// 非 Windows 平台：使用标准库 codecvt（C++11/14/17）
// 注意：codecvt 在 C++17 中被弃用，但仍可使用

std::wstring utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) {
        return std::wstring();
    }
    
    try {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes(utf8);
    } catch (const std::exception&) {
        return std::wstring();
    }
}

std::string wideToUtf8(const std::wstring& wide) {
    if (wide.empty()) {
        return std::string();
    }
    
    try {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(wide);
    } catch (const std::exception&) {
        return std::string();
    }
}

#endif // _WIN32

/**
 * 验证 UTF-8 字符串是否有效
 */
bool isValidUtf8(const std::string& str) {
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(str.c_str());
    size_t len = str.length();
    size_t i = 0;
    
    while (i < len) {
        if (bytes[i] <= 0x7F) {
            // ASCII 字符
            i++;
        } else if ((bytes[i] & 0xE0) == 0xC0) {
            // 2 字节序列
            if (i + 1 >= len || (bytes[i + 1] & 0xC0) != 0x80) {
                return false;
            }
            // 检查过长编码
            if ((bytes[i] & 0x1E) == 0) {
                return false;
            }
            i += 2;
        } else if ((bytes[i] & 0xF0) == 0xE0) {
            // 3 字节序列
            if (i + 2 >= len || 
                (bytes[i + 1] & 0xC0) != 0x80 || 
                (bytes[i + 2] & 0xC0) != 0x80) {
                return false;
            }
            // 检查过长编码和代理对
            unsigned int codepoint = ((bytes[i] & 0x0F) << 12) |
                                     ((bytes[i + 1] & 0x3F) << 6) |
                                     (bytes[i + 2] & 0x3F);
            if (codepoint < 0x0800 || (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
                return false;
            }
            i += 3;
        } else if ((bytes[i] & 0xF8) == 0xF0) {
            // 4 字节序列
            if (i + 3 >= len || 
                (bytes[i + 1] & 0xC0) != 0x80 || 
                (bytes[i + 2] & 0xC0) != 0x80 ||
                (bytes[i + 3] & 0xC0) != 0x80) {
                return false;
            }
            // 检查过长编码和有效范围
            unsigned int codepoint = ((bytes[i] & 0x07) << 18) |
                                     ((bytes[i + 1] & 0x3F) << 12) |
                                     ((bytes[i + 2] & 0x3F) << 6) |
                                     (bytes[i + 3] & 0x3F);
            if (codepoint < 0x10000 || codepoint > 0x10FFFF) {
                return false;
            }
            i += 4;
        } else {
            return false;
        }
    }
    
    return true;
}

/**
 * 生成随机有效 UTF-8 字符串
 */
std::string generateRandomUtf8(size_t maxLength) {
    std::string result;
    size_t targetLen = std::rand() % (maxLength + 1);
    
    while (result.length() < targetLen) {
        int type = std::rand() % 4;
        
        switch (type) {
            case 0: {
                // ASCII 字符 (0x20-0x7E，可打印字符)
                char c = static_cast<char>(0x20 + (std::rand() % 95));
                result += c;
                break;
            }
            case 1: {
                // 2 字节 UTF-8 (0x80-0x7FF)
                unsigned int cp = 0x80 + (std::rand() % (0x7FF - 0x80 + 1));
                result += static_cast<char>(0xC0 | (cp >> 6));
                result += static_cast<char>(0x80 | (cp & 0x3F));
                break;
            }
            case 2: {
                // 3 字节 UTF-8 (0x800-0xFFFF，排除代理对 0xD800-0xDFFF)
                unsigned int cp;
                do {
                    cp = 0x800 + (std::rand() % (0xFFFF - 0x800 + 1));
                } while (cp >= 0xD800 && cp <= 0xDFFF);
                result += static_cast<char>(0xE0 | (cp >> 12));
                result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (cp & 0x3F));
                break;
            }
            case 3: {
                // 4 字节 UTF-8 (0x10000-0x10FFFF)
                unsigned int cp = 0x10000 + (std::rand() % (0x10FFFF - 0x10000 + 1));
                result += static_cast<char>(0xF0 | (cp >> 18));
                result += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (cp & 0x3F));
                break;
            }
        }
    }
    
    return result;
}

} // namespace suyan_test


// ========== 测试辅助宏 ==========

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "✗ 断言失败: " << message << std::endl; \
            std::cerr << "  位置: " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

#define TEST_PASS(message) \
    std::cout << "✓ " << message << std::endl


// ========== 测试类 ==========

class WindowsBridgeEncodingTest {
public:
    WindowsBridgeEncodingTest() {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
    }

    bool runAllTests() {
        std::cout << "=== 编码转换单元测试 ===" << std::endl;
        std::cout << "Task 3.2: 编写编码转换单元测试" << std::endl;
        std::cout << "Validates: Requirements 3.2" << std::endl;
        std::cout << std::endl;

        bool allPassed = true;

        // 基础功能测试
        allPassed &= testEmptyString();
        allPassed &= testEnglishText();
        allPassed &= testChineseText();
        allPassed &= testMixedText();
        allPassed &= testSpecialCharacters();
        allPassed &= testEmoji();
        
        // 属性测试
        allPassed &= testProperty4_RoundTripConsistency();

        std::cout << std::endl;
        if (allPassed) {
            std::cout << "=== 所有测试通过 ===" << std::endl;
        } else {
            std::cout << "=== 部分测试失败 ===" << std::endl;
        }

        return allPassed;
    }

private:
    // ========== 空字符串测试 ==========
    
    bool testEmptyString() {
        std::cout << "\n--- 空字符串测试 ---" << std::endl;
        
        // UTF-8 空字符串转 UTF-16
        std::string emptyUtf8 = "";
        std::wstring wideResult = suyan_test::utf8ToWide(emptyUtf8);
        TEST_ASSERT(wideResult.empty(), "空 UTF-8 字符串应转换为空 UTF-16 字符串");
        
        // UTF-16 空字符串转 UTF-8
        std::wstring emptyWide = L"";
        std::string utf8Result = suyan_test::wideToUtf8(emptyWide);
        TEST_ASSERT(utf8Result.empty(), "空 UTF-16 字符串应转换为空 UTF-8 字符串");
        
        TEST_PASS("testEmptyString: 空字符串转换正确");
        return true;
    }

    // ========== 英文文本测试 ==========
    
    bool testEnglishText() {
        std::cout << "\n--- 英文文本测试 ---" << std::endl;
        
        // 简单英文
        std::string english = "Hello, World!";
        std::wstring wide = suyan_test::utf8ToWide(english);
        TEST_ASSERT(wide == L"Hello, World!", "英文文本 UTF-8 转 UTF-16 应正确");
        
        std::string back = suyan_test::wideToUtf8(wide);
        TEST_ASSERT(back == english, "英文文本往返转换应一致");
        
        // 包含数字和标点
        std::string alphanumeric = "Test123!@#$%^&*()";
        wide = suyan_test::utf8ToWide(alphanumeric);
        back = suyan_test::wideToUtf8(wide);
        TEST_ASSERT(back == alphanumeric, "字母数字混合文本往返转换应一致");
        
        // 长文本
        std::string longText = "The quick brown fox jumps over the lazy dog. "
                               "Pack my box with five dozen liquor jugs. "
                               "How vexingly quick daft zebras jump!";
        wide = suyan_test::utf8ToWide(longText);
        back = suyan_test::wideToUtf8(wide);
        TEST_ASSERT(back == longText, "长英文文本往返转换应一致");
        
        TEST_PASS("testEnglishText: 英文文本转换正确");
        return true;
    }

    // ========== 中文文本测试 ==========
    
    bool testChineseText() {
        std::cout << "\n--- 中文文本测试 ---" << std::endl;
        
        // 简体中文
        std::string chinese = u8"你好，世界！";
        std::wstring wide = suyan_test::utf8ToWide(chinese);
        std::string back = suyan_test::wideToUtf8(wide);
        TEST_ASSERT(back == chinese, "简体中文往返转换应一致");
        
        // 常用汉字
        std::string commonChinese = u8"素言输入法是一款优秀的中文输入法";
        wide = suyan_test::utf8ToWide(commonChinese);
        back = suyan_test::wideToUtf8(wide);
        TEST_ASSERT(back == commonChinese, "常用汉字往返转换应一致");
        
        // 繁体中文
        std::string traditional = u8"繁體中文測試";
        wide = suyan_test::utf8ToWide(traditional);
        back = suyan_test::wideToUtf8(wide);
        TEST_ASSERT(back == traditional, "繁体中文往返转换应一致");
        
        // 生僻字（CJK 扩展区）
        std::string rareChinese = u8"龘靐齉";
        wide = suyan_test::utf8ToWide(rareChinese);
        back = suyan_test::wideToUtf8(wide);
        TEST_ASSERT(back == rareChinese, "生僻字往返转换应一致");
        
        TEST_PASS("testChineseText: 中文文本转换正确");
        return true;
    }

    // ========== 混合文本测试 ==========
    
    bool testMixedText() {
        std::cout << "\n--- 混合文本测试 ---" << std::endl;
        
        // 中英混合
        std::string mixed1 = u8"Hello你好World世界";
        std::wstring wide = suyan_test::utf8ToWide(mixed1);
        std::string back = suyan_test::wideToUtf8(wide);
        TEST_ASSERT(back == mixed1, "中英混合文本往返转换应一致");
        
        // 中文、英文、数字、标点混合
        std::string mixed2 = u8"素言IME v1.0 - 中文输入法 (Chinese Input Method)";
        wide = suyan_test::utf8ToWide(mixed2);
        back = suyan_test::wideToUtf8(wide);
        TEST_ASSERT(back == mixed2, "复杂混合文本往返转换应一致");
        
        // 日文假名混合
        std::string japanese = u8"こんにちは世界Hello";
        wide = suyan_test::utf8ToWide(japanese);
        back = suyan_test::wideToUtf8(wide);
        TEST_ASSERT(back == japanese, "日文混合文本往返转换应一致");
        
        // 韩文混合
        std::string korean = u8"안녕하세요Hello你好";
        wide = suyan_test::utf8ToWide(korean);
        back = suyan_test::wideToUtf8(wide);
        TEST_ASSERT(back == korean, "韩文混合文本往返转换应一致");
        
        TEST_PASS("testMixedText: 混合文本转换正确");
        return true;
    }

    // ========== 特殊字符测试 ==========
    
    bool testSpecialCharacters() {
        std::cout << "\n--- 特殊字符测试 ---" << std::endl;
        
        // 换行符和制表符
        std::string whitespace = "Line1\nLine2\tTabbed";
        std::wstring wide = suyan_test::utf8ToWide(whitespace);
        std::string back = suyan_test::wideToUtf8(wide);
        TEST_ASSERT(back == whitespace, "空白字符往返转换应一致");
        
        // 特殊标点
        std::string punctuation = u8"「」『』【】〖〗《》〈〉";
        wide = suyan_test::utf8ToWide(punctuation);
        back = suyan_test::wideToUtf8(wide);
        TEST_ASSERT(back == punctuation, "中文标点往返转换应一致");
        
        // 数学符号
        std::string math = u8"∑∏∫∂∇√∞≈≠≤≥";
        wide = suyan_test::utf8ToWide(math);
        back = suyan_test::wideToUtf8(wide);
        TEST_ASSERT(back == math, "数学符号往返转换应一致");
        
        // 货币符号
        std::string currency = u8"$€£¥₹₽";
        wide = suyan_test::utf8ToWide(currency);
        back = suyan_test::wideToUtf8(wide);
        TEST_ASSERT(back == currency, "货币符号往返转换应一致");
        
        TEST_PASS("testSpecialCharacters: 特殊字符转换正确");
        return true;
    }

    // ========== Emoji 测试 ==========
    
    bool testEmoji() {
        std::cout << "\n--- Emoji 测试 ---" << std::endl;
        
        // 基本 Emoji（BMP 范围内）
        std::string basicEmoji = u8"☺☻♥♦♣♠";
        std::wstring wide = suyan_test::utf8ToWide(basicEmoji);
        std::string back = suyan_test::wideToUtf8(wide);
        TEST_ASSERT(back == basicEmoji, "基本 Emoji 往返转换应一致");
        
        // 现代 Emoji（需要代理对）
        std::string modernEmoji = u8"😀😁😂🤣😃😄";
        wide = suyan_test::utf8ToWide(modernEmoji);
        back = suyan_test::wideToUtf8(wide);
        TEST_ASSERT(back == modernEmoji, "现代 Emoji 往返转换应一致");
        
        // Emoji 与文本混合
        std::string mixedEmoji = u8"Hello 👋 你好 🇨🇳";
        wide = suyan_test::utf8ToWide(mixedEmoji);
        back = suyan_test::wideToUtf8(wide);
        TEST_ASSERT(back == mixedEmoji, "Emoji 混合文本往返转换应一致");
        
        TEST_PASS("testEmoji: Emoji 转换正确");
        return true;
    }

    // ========== Property 4: UTF-8/UTF-16 编码转换往返一致性 ==========
    /**
     * Property 4: UTF-8/UTF-16 编码转换往返一致性
     * 
     * For any 有效的 UTF-8 字符串，经过 utf8ToWide 转换为 UTF-16 后，
     * 再经过 wideToUtf8 转换回 UTF-8，应得到与原始字符串相同的结果
     * （round-trip property）。
     * 
     * Validates: Requirements 3.2
     */
    bool testProperty4_RoundTripConsistency() {
        std::cout << "\n--- Property 4: UTF-8/UTF-16 编码转换往返一致性 ---" << std::endl;
        std::cout << "  验证: 任意有效 UTF-8 字符串往返转换后应与原始字符串相同" << std::endl;
        
        const int NUM_ITERATIONS = 100;
        int testCount = 0;
        int skippedCount = 0;
        
        // 测试预定义的字符串
        std::vector<std::string> predefinedStrings = {
            "",
            "a",
            "Hello",
            u8"你",
            u8"你好",
            u8"Hello你好",
            u8"😀",
            u8"Hello 😀 你好",
            u8"The quick brown fox",
            u8"素言输入法",
            u8"こんにちは",
            u8"안녕하세요",
            u8"Привет",
            u8"مرحبا",
            u8"שלום",
            u8"🎉🎊🎁🎀",
            u8"∑∏∫∂∇",
            u8"①②③④⑤",
            u8"ⅠⅡⅢⅣⅤ",
        };
        
        for (const auto& original : predefinedStrings) {
            if (!suyan_test::isValidUtf8(original)) {
                skippedCount++;
                continue;
            }
            
            std::wstring wide = suyan_test::utf8ToWide(original);
            std::string roundTrip = suyan_test::wideToUtf8(wide);
            
            TEST_ASSERT(roundTrip == original,
                "预定义字符串往返转换应一致: \"" + original + "\"");
            testCount++;
        }
        
        // 测试随机生成的字符串
        for (int i = 0; i < NUM_ITERATIONS; i++) {
            std::string original = suyan_test::generateRandomUtf8(100);
            
            if (!suyan_test::isValidUtf8(original)) {
                skippedCount++;
                continue;
            }
            
            std::wstring wide = suyan_test::utf8ToWide(original);
            std::string roundTrip = suyan_test::wideToUtf8(wide);
            
            TEST_ASSERT(roundTrip == original,
                "随机字符串往返转换应一致 (迭代 " + std::to_string(i) + ")");
            testCount++;
        }
        
        // 测试边界情况
        
        // 单字节 ASCII 边界
        for (int c = 0x20; c <= 0x7E; c++) {
            std::string single(1, static_cast<char>(c));
            std::wstring wide = suyan_test::utf8ToWide(single);
            std::string back = suyan_test::wideToUtf8(wide);
            TEST_ASSERT(back == single,
                "ASCII 字符 " + std::to_string(c) + " 往返转换应一致");
            testCount++;
        }
        
        // 2 字节 UTF-8 边界
        {
            // 最小 2 字节字符 (U+0080)
            std::string minTwoByte = "\xC2\x80";
            if (suyan_test::isValidUtf8(minTwoByte)) {
                std::wstring wide = suyan_test::utf8ToWide(minTwoByte);
                std::string back = suyan_test::wideToUtf8(wide);
                TEST_ASSERT(back == minTwoByte, "最小 2 字节字符往返转换应一致");
                testCount++;
            }
            
            // 最大 2 字节字符 (U+07FF)
            std::string maxTwoByte = "\xDF\xBF";
            if (suyan_test::isValidUtf8(maxTwoByte)) {
                std::wstring wide = suyan_test::utf8ToWide(maxTwoByte);
                std::string back = suyan_test::wideToUtf8(wide);
                TEST_ASSERT(back == maxTwoByte, "最大 2 字节字符往返转换应一致");
                testCount++;
            }
        }
        
        // 3 字节 UTF-8 边界
        {
            // 最小 3 字节字符 (U+0800)
            std::string minThreeByte = "\xE0\xA0\x80";
            if (suyan_test::isValidUtf8(minThreeByte)) {
                std::wstring wide = suyan_test::utf8ToWide(minThreeByte);
                std::string back = suyan_test::wideToUtf8(wide);
                TEST_ASSERT(back == minThreeByte, "最小 3 字节字符往返转换应一致");
                testCount++;
            }
            
            // 最大 3 字节字符 (U+FFFF，排除代理对)
            std::string maxThreeByte = "\xEF\xBF\xBF";
            if (suyan_test::isValidUtf8(maxThreeByte)) {
                std::wstring wide = suyan_test::utf8ToWide(maxThreeByte);
                std::string back = suyan_test::wideToUtf8(wide);
                TEST_ASSERT(back == maxThreeByte, "最大 3 字节字符往返转换应一致");
                testCount++;
            }
        }
        
        // 4 字节 UTF-8 边界（需要代理对）
        {
            // 最小 4 字节字符 (U+10000)
            std::string minFourByte = "\xF0\x90\x80\x80";
            if (suyan_test::isValidUtf8(minFourByte)) {
                std::wstring wide = suyan_test::utf8ToWide(minFourByte);
                std::string back = suyan_test::wideToUtf8(wide);
                TEST_ASSERT(back == minFourByte, "最小 4 字节字符往返转换应一致");
                testCount++;
            }
            
            // 最大 4 字节字符 (U+10FFFF)
            std::string maxFourByte = "\xF4\x8F\xBF\xBF";
            if (suyan_test::isValidUtf8(maxFourByte)) {
                std::wstring wide = suyan_test::utf8ToWide(maxFourByte);
                std::string back = suyan_test::wideToUtf8(wide);
                TEST_ASSERT(back == maxFourByte, "最大 4 字节字符往返转换应一致");
                testCount++;
            }
        }
        
        std::cout << "  执行了 " << testCount << " 次测试";
        if (skippedCount > 0) {
            std::cout << " (跳过 " << skippedCount << " 个无效字符串)";
        }
        std::cout << std::endl;
        
        TEST_PASS("Property 4: UTF-8/UTF-16 编码转换往返一致性验证通过");
        return true;
    }
};


// ========== 主函数 ==========

int main() {
    WindowsBridgeEncodingTest test;
    return test.runAllTests() ? 0 : 1;
}
