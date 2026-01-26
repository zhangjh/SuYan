/**
 * 键码转换实现
 * Task 2.1: 实现键码转换函数
 * 
 * Windows 虚拟键码到 RIME 键码的转换实现
 * 
 * Requirements: 2.1, 2.2
 */

#ifdef _WIN32

#include "key_converter.h"

namespace suyan {

// ========== X11 Keysym 常量定义 ==========
// 参考: deps/librime-windows/rime-deps-de4700e-Windows-msvc-x64/include/X11/keysymdef.h

namespace XK {
    // TTY 功能键
    constexpr int BackSpace     = 0xff08;
    constexpr int Tab           = 0xff09;
    constexpr int Return        = 0xff0d;
    constexpr int Pause         = 0xff13;
    constexpr int Scroll_Lock   = 0xff14;
    constexpr int Escape        = 0xff1b;
    constexpr int Delete        = 0xffff;

    // 光标控制键
    constexpr int Home          = 0xff50;
    constexpr int Left          = 0xff51;
    constexpr int Up            = 0xff52;
    constexpr int Right         = 0xff53;
    constexpr int Down          = 0xff54;
    constexpr int Page_Up       = 0xff55;
    constexpr int Page_Down     = 0xff56;
    constexpr int End           = 0xff57;

    // 杂项功能键
    constexpr int Print         = 0xff61;
    constexpr int Insert        = 0xff63;
    constexpr int Menu          = 0xff67;
    constexpr int Num_Lock      = 0xff7f;

    // 小键盘功能键
    constexpr int KP_Enter      = 0xff8d;
    constexpr int KP_Home       = 0xff95;
    constexpr int KP_Left       = 0xff96;
    constexpr int KP_Up         = 0xff97;
    constexpr int KP_Right      = 0xff98;
    constexpr int KP_Down       = 0xff99;
    constexpr int KP_Page_Up    = 0xff9a;
    constexpr int KP_Page_Down  = 0xff9b;
    constexpr int KP_End        = 0xff9c;
    constexpr int KP_Insert     = 0xff9e;
    constexpr int KP_Delete     = 0xff9f;
    constexpr int KP_Multiply   = 0xffaa;
    constexpr int KP_Add        = 0xffab;
    constexpr int KP_Subtract   = 0xffad;
    constexpr int KP_Decimal    = 0xffae;
    constexpr int KP_Divide     = 0xffaf;
    constexpr int KP_0          = 0xffb0;
    constexpr int KP_1          = 0xffb1;
    constexpr int KP_2          = 0xffb2;
    constexpr int KP_3          = 0xffb3;
    constexpr int KP_4          = 0xffb4;
    constexpr int KP_5          = 0xffb5;
    constexpr int KP_6          = 0xffb6;
    constexpr int KP_7          = 0xffb7;
    constexpr int KP_8          = 0xffb8;
    constexpr int KP_9          = 0xffb9;

    // 功能键 F1-F24
    constexpr int F1            = 0xffbe;
    constexpr int F2            = 0xffbf;
    constexpr int F3            = 0xffc0;
    constexpr int F4            = 0xffc1;
    constexpr int F5            = 0xffc2;
    constexpr int F6            = 0xffc3;
    constexpr int F7            = 0xffc4;
    constexpr int F8            = 0xffc5;
    constexpr int F9            = 0xffc6;
    constexpr int F10           = 0xffc7;
    constexpr int F11           = 0xffc8;
    constexpr int F12           = 0xffc9;
    constexpr int F13           = 0xffca;
    constexpr int F14           = 0xffcb;
    constexpr int F15           = 0xffcc;
    constexpr int F16           = 0xffcd;
    constexpr int F17           = 0xffce;
    constexpr int F18           = 0xffcf;
    constexpr int F19           = 0xffd0;
    constexpr int F20           = 0xffd1;
    constexpr int F21           = 0xffd2;
    constexpr int F22           = 0xffd3;
    constexpr int F23           = 0xffd4;
    constexpr int F24           = 0xffd5;

    // 修饰键
    constexpr int Shift_L       = 0xffe1;
    constexpr int Shift_R       = 0xffe2;
    constexpr int Control_L     = 0xffe3;
    constexpr int Control_R     = 0xffe4;
    constexpr int Caps_Lock     = 0xffe5;
    constexpr int Meta_L        = 0xffe7;  // Win 键
    constexpr int Meta_R        = 0xffe8;
    constexpr int Alt_L         = 0xffe9;
    constexpr int Alt_R         = 0xffea;
    constexpr int Super_L       = 0xffeb;
    constexpr int Super_R       = 0xffec;
}

// ========== 修饰键掩码（与 input_engine.h 中的 KeyModifier 一致）==========

namespace KeyModifier {
    constexpr int None    = 0;
    constexpr int Shift   = 1 << 0;
    constexpr int Control = 1 << 2;
    constexpr int Alt     = 1 << 3;
    constexpr int Super   = 1 << 6;
}

// ========== 辅助函数 ==========

/**
 * 检查按键是否按下
 */
static inline bool isKeyDown(int vk) {
    return (GetKeyState(vk) & 0x8000) != 0;
}

/**
 * 检查 Caps Lock 是否开启
 */
static inline bool isCapsLockOn() {
    return (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
}

// ========== 公共函数实现 ==========

int convertVirtualKeyToRime(WPARAM vk, UINT scanCode, bool extended) {
    // 1. 功能键 F1-F24
    if (vk >= VK_F1 && vk <= VK_F24) {
        return XK::F1 + (static_cast<int>(vk) - VK_F1);
    }

    // 2. 修饰键
    switch (vk) {
        case VK_SHIFT:
            // 区分左右 Shift
            if (scanCode == 0x36) {
                return XK::Shift_R;
            }
            return XK::Shift_L;
        case VK_LSHIFT:
            return XK::Shift_L;
        case VK_RSHIFT:
            return XK::Shift_R;
        case VK_CONTROL:
            // 区分左右 Control
            if (extended) {
                return XK::Control_R;
            }
            return XK::Control_L;
        case VK_LCONTROL:
            return XK::Control_L;
        case VK_RCONTROL:
            return XK::Control_R;
        case VK_MENU:  // Alt 键
            // 区分左右 Alt
            if (extended) {
                return XK::Alt_R;
            }
            return XK::Alt_L;
        case VK_LMENU:
            return XK::Alt_L;
        case VK_RMENU:
            return XK::Alt_R;
        case VK_LWIN:
            return XK::Super_L;
        case VK_RWIN:
            return XK::Super_R;
        case VK_CAPITAL:
            return XK::Caps_Lock;
    }

    // 3. 导航键和编辑键
    switch (vk) {
        case VK_RETURN:
            // 区分主键盘 Enter 和小键盘 Enter
            if (extended) {
                return XK::KP_Enter;
            }
            return XK::Return;
        case VK_TAB:
            return XK::Tab;
        case VK_BACK:
            return XK::BackSpace;
        case VK_ESCAPE:
            return XK::Escape;
        case VK_SPACE:
            return 0x0020;  // 空格直接返回 ASCII
        case VK_DELETE:
            if (extended) {
                return XK::Delete;
            }
            return XK::KP_Delete;
        case VK_INSERT:
            if (extended) {
                return XK::Insert;
            }
            return XK::KP_Insert;
        case VK_HOME:
            if (extended) {
                return XK::Home;
            }
            return XK::KP_Home;
        case VK_END:
            if (extended) {
                return XK::End;
            }
            return XK::KP_End;
        case VK_PRIOR:  // Page Up
            if (extended) {
                return XK::Page_Up;
            }
            return XK::KP_Page_Up;
        case VK_NEXT:   // Page Down
            if (extended) {
                return XK::Page_Down;
            }
            return XK::KP_Page_Down;
        case VK_LEFT:
            if (extended) {
                return XK::Left;
            }
            return XK::KP_Left;
        case VK_RIGHT:
            if (extended) {
                return XK::Right;
            }
            return XK::KP_Right;
        case VK_UP:
            if (extended) {
                return XK::Up;
            }
            return XK::KP_Up;
        case VK_DOWN:
            if (extended) {
                return XK::Down;
            }
            return XK::KP_Down;
    }

    // 4. 其他功能键
    switch (vk) {
        case VK_PAUSE:
            return XK::Pause;
        case VK_SCROLL:
            return XK::Scroll_Lock;
        case VK_SNAPSHOT:  // Print Screen
            return XK::Print;
        case VK_NUMLOCK:
            return XK::Num_Lock;
        case VK_APPS:  // 应用程序键（右键菜单键）
            return XK::Menu;
    }

    // 5. 小键盘数字和运算符
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) {
        return XK::KP_0 + (static_cast<int>(vk) - VK_NUMPAD0);
    }
    switch (vk) {
        case VK_MULTIPLY:
            return XK::KP_Multiply;
        case VK_ADD:
            return XK::KP_Add;
        case VK_SUBTRACT:
            return XK::KP_Subtract;
        case VK_DECIMAL:
            return XK::KP_Decimal;
        case VK_DIVIDE:
            return XK::KP_Divide;
    }

    // 6. 字母键 (VK_A - VK_Z)
    // 返回小写字母的 ASCII 值，RIME 会根据修饰键处理大小写
    if (vk >= 'A' && vk <= 'Z') {
        // 检查是否应该返回大写
        bool shiftDown = isKeyDown(VK_SHIFT);
        bool capsLock = isCapsLockOn();
        // Shift XOR CapsLock 决定大小写
        if (shiftDown != capsLock) {
            return static_cast<int>(vk);  // 大写 'A'-'Z'
        } else {
            return static_cast<int>(vk) + 32;  // 小写 'a'-'z'
        }
    }

    // 7. 数字键 (VK_0 - VK_9)
    // 主键盘数字键，需要考虑 Shift 状态
    if (vk >= '0' && vk <= '9') {
        if (isKeyDown(VK_SHIFT)) {
            // Shift + 数字键 = 符号
            // 美式键盘布局: )!@#$%^&*(
            static const char shiftedDigits[] = ")!@#$%^&*(";
            return static_cast<int>(shiftedDigits[vk - '0']);
        }
        return static_cast<int>(vk);  // 数字 '0'-'9'
    }

    // 8. OEM 键（标点符号等）
    // 使用 ToUnicode 获取实际字符
    wchar_t ch = getCharacterFromKey(vk, scanCode);
    if (ch >= 0x20 && ch <= 0x7e) {
        return static_cast<int>(ch);
    }

    // 无法转换
    return 0;
}

int convertModifiersToRime() {
    int modifiers = KeyModifier::None;

    if (isKeyDown(VK_SHIFT) || isKeyDown(VK_LSHIFT) || isKeyDown(VK_RSHIFT)) {
        modifiers |= KeyModifier::Shift;
    }

    if (isKeyDown(VK_CONTROL) || isKeyDown(VK_LCONTROL) || isKeyDown(VK_RCONTROL)) {
        modifiers |= KeyModifier::Control;
    }

    if (isKeyDown(VK_MENU) || isKeyDown(VK_LMENU) || isKeyDown(VK_RMENU)) {
        modifiers |= KeyModifier::Alt;
    }

    if (isKeyDown(VK_LWIN) || isKeyDown(VK_RWIN)) {
        modifiers |= KeyModifier::Super;
    }

    return modifiers;
}

bool isCharacterKey(WPARAM vk) {
    // 字母键
    if (vk >= 'A' && vk <= 'Z') {
        return true;
    }

    // 数字键
    if (vk >= '0' && vk <= '9') {
        return true;
    }

    // 空格键
    if (vk == VK_SPACE) {
        return true;
    }

    // OEM 键（标点符号等）
    switch (vk) {
        case VK_OEM_1:      // ;:
        case VK_OEM_PLUS:   // =+
        case VK_OEM_COMMA:  // ,<
        case VK_OEM_MINUS:  // -_
        case VK_OEM_PERIOD: // .>
        case VK_OEM_2:      // /?
        case VK_OEM_3:      // `~
        case VK_OEM_4:      // [{
        case VK_OEM_5:      // \|
        case VK_OEM_6:      // ]}
        case VK_OEM_7:      // '"
        case VK_OEM_102:    // 欧洲键盘的额外键
            return true;
    }

    return false;
}

wchar_t getCharacterFromKey(WPARAM vk, UINT scanCode) {
    // 获取当前键盘状态
    BYTE keyboardState[256];
    if (!GetKeyboardState(keyboardState)) {
        return 0;
    }

    // 使用 ToUnicode 转换
    wchar_t buffer[4] = {0};
    int result = ToUnicode(
        static_cast<UINT>(vk),
        scanCode,
        keyboardState,
        buffer,
        4,
        0  // 不使用菜单标志
    );

    if (result == 1) {
        return buffer[0];
    }

    // 如果 ToUnicode 返回 0 或负数（死键），尝试手动映射
    // 这是为了处理某些特殊情况
    if (result <= 0) {
        // 对于 OEM 键，尝试使用 MapVirtualKey 获取字符
        UINT ch = MapVirtualKey(static_cast<UINT>(vk), MAPVK_VK_TO_CHAR);
        if (ch != 0) {
            // 检查 Shift 状态
            if (isKeyDown(VK_SHIFT)) {
                // 对于某些键，需要返回 Shift 后的字符
                // 这里只处理常见的 OEM 键
                switch (vk) {
                    case VK_OEM_1:      return ':';
                    case VK_OEM_PLUS:   return '+';
                    case VK_OEM_COMMA:  return '<';
                    case VK_OEM_MINUS:  return '_';
                    case VK_OEM_PERIOD: return '>';
                    case VK_OEM_2:      return '?';
                    case VK_OEM_3:      return '~';
                    case VK_OEM_4:      return '{';
                    case VK_OEM_5:      return '|';
                    case VK_OEM_6:      return '}';
                    case VK_OEM_7:      return '"';
                }
            } else {
                switch (vk) {
                    case VK_OEM_1:      return ';';
                    case VK_OEM_PLUS:   return '=';
                    case VK_OEM_COMMA:  return ',';
                    case VK_OEM_MINUS:  return '-';
                    case VK_OEM_PERIOD: return '.';
                    case VK_OEM_2:      return '/';
                    case VK_OEM_3:      return '`';
                    case VK_OEM_4:      return '[';
                    case VK_OEM_5:      return '\\';
                    case VK_OEM_6:      return ']';
                    case VK_OEM_7:      return '\'';
                }
            }
            return static_cast<wchar_t>(ch);
        }
    }

    return 0;
}

bool isModifierKey(WPARAM vk) {
    switch (vk) {
        case VK_SHIFT:
        case VK_LSHIFT:
        case VK_RSHIFT:
        case VK_CONTROL:
        case VK_LCONTROL:
        case VK_RCONTROL:
        case VK_MENU:
        case VK_LMENU:
        case VK_RMENU:
        case VK_LWIN:
        case VK_RWIN:
        case VK_CAPITAL:
            return true;
        default:
            return false;
    }
}

bool isFunctionKey(WPARAM vk) {
    return vk >= VK_F1 && vk <= VK_F24;
}

bool isNumpadKey(WPARAM vk) {
    // 小键盘数字
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) {
        return true;
    }
    // 小键盘运算符
    switch (vk) {
        case VK_MULTIPLY:
        case VK_ADD:
        case VK_SEPARATOR:
        case VK_SUBTRACT:
        case VK_DECIMAL:
        case VK_DIVIDE:
        case VK_NUMLOCK:
            return true;
        default:
            return false;
    }
}

bool isNavigationKey(WPARAM vk) {
    switch (vk) {
        case VK_LEFT:
        case VK_RIGHT:
        case VK_UP:
        case VK_DOWN:
        case VK_HOME:
        case VK_END:
        case VK_PRIOR:  // Page Up
        case VK_NEXT:   // Page Down
            return true;
        default:
            return false;
    }
}

} // namespace suyan

#endif // _WIN32
