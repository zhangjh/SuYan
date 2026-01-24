/**
 * IMKBridge - macOS Input Method Kit 桥接层
 *
 * 实现 IMKInputController 子类，处理 macOS 输入法框架的事件。
 * 将 IMK 事件转换为 InputEngine 可处理的格式。
 */

#ifndef SUYAN_PLATFORM_MACOS_IMKBRIDGE_H
#define SUYAN_PLATFORM_MACOS_IMKBRIDGE_H

#import <Cocoa/Cocoa.h>
#import <InputMethodKit/InputMethodKit.h>

// C++ 前向声明
#ifdef __cplusplus
namespace suyan {
    class InputEngine;
    class CandidateWindow;
}
#endif

/**
 * SuYanInputController - IMK 输入控制器
 *
 * 继承自 IMKInputController，处理：
 * - 键盘事件 (handleEvent:client:)
 * - 输入法激活/停用 (activateServer:/deactivateServer:)
 * - 输入提交 (commitComposition:)
 * - 状态栏菜单 (menu)
 */
@interface SuYanInputController : IMKInputController

// ========== IMKInputController 重写方法 ==========

/**
 * 处理键盘事件
 *
 * @param event NSEvent 键盘事件
 * @param sender 当前输入客户端
 * @return YES 表示事件已处理，NO 表示传递给应用
 */
- (BOOL)handleEvent:(NSEvent *)event client:(id)sender;

/**
 * 输入法激活
 *
 * 当用户切换到此输入法时调用
 *
 * @param sender 当前输入客户端
 */
- (void)activateServer:(id)sender;

/**
 * 输入法停用
 *
 * 当用户切换到其他输入法时调用
 *
 * @param sender 当前输入客户端
 */
- (void)deactivateServer:(id)sender;

/**
 * 提交当前输入
 *
 * 当应用请求提交当前输入时调用（如点击其他位置）
 *
 * @param sender 当前输入客户端
 */
- (void)commitComposition:(id)sender;

/**
 * 获取状态栏菜单
 *
 * @return 状态栏右键菜单
 */
- (NSMenu *)menu;

// ========== 候选词窗口相关 ==========

/**
 * 获取候选词窗口位置
 *
 * @param sender 当前输入客户端
 * @return 候选词窗口应显示的位置
 */
- (NSRect)candidateWindowFrameForClient:(id)sender;

// ========== 内部方法 ==========

/**
 * 更新候选词窗口
 */
- (void)updateCandidateWindow;

/**
 * 隐藏候选词窗口
 */
- (void)hideCandidateWindow;

/**
 * 提交文本到客户端
 *
 * @param text 要提交的文本
 */
- (void)commitText:(NSString *)text;

/**
 * 更新 preedit 显示
 *
 * @param preedit 预编辑文本
 * @param caretPos 光标位置
 */
- (void)updatePreedit:(NSString *)preedit caretPosition:(NSInteger)caretPos;

/**
 * 清除 preedit
 */
- (void)clearPreedit;

@end

// ========== C++ 桥接函数 ==========

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 设置全局 InputEngine 实例
 *
 * 由 main.mm 在初始化时调用
 */
void SuYanIMK_SetInputEngine(void* engine);

/**
 * 获取全局 InputEngine 实例
 */
void* SuYanIMK_GetInputEngine(void);

/**
 * 设置全局 CandidateWindow 实例
 *
 * 由 main.mm 在初始化时调用
 */
void SuYanIMK_SetCandidateWindow(void* window);

/**
 * 获取全局 CandidateWindow 实例
 */
void* SuYanIMK_GetCandidateWindow(void);

/**
 * 将 NSEvent keyCode 转换为 RIME keyCode
 *
 * @param keyCode macOS 虚拟键码
 * @param characters 按键字符
 * @param modifiers 修饰键
 * @return RIME 键码
 */
int SuYanIMK_ConvertKeyCode(unsigned short keyCode, 
                            NSString* characters,
                            NSEventModifierFlags modifiers);

/**
 * 将 NSEvent modifierFlags 转换为 RIME modifiers
 *
 * @param modifierFlags macOS 修饰键标志
 * @return RIME 修饰键掩码
 */
int SuYanIMK_ConvertModifiers(NSEventModifierFlags modifierFlags);

#ifdef __cplusplus
}
#endif

#endif // SUYAN_PLATFORM_MACOS_IMKBRIDGE_H
