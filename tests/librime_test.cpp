/**
 * librime API 验证测试
 * 
 * 验证 librime 预编译库可以正确链接和调用
 */

#include <iostream>
#include <cstring>
#include "rime_api.h"

int main() {
    std::cout << "=== librime API 验证测试 ===" << std::endl;
    
    // 1. 获取 RIME API
    RimeApi* rime = rime_get_api();
    if (!rime) {
        std::cerr << "错误: 无法获取 RIME API" << std::endl;
        return 1;
    }
    std::cout << "✓ 成功获取 RIME API" << std::endl;
    
    // 2. 检查 API 版本
    std::cout << "  RIME API 数据大小: " << rime->data_size << std::endl;
    
    // 3. 设置 RIME traits（不实际初始化，只验证 API 可调用）
    RIME_STRUCT(RimeTraits, traits);
    traits.app_name = "librime_test";
    
    // 4. 验证关键 API 函数指针存在
    bool api_ok = true;
    
    if (!rime->setup) {
        std::cerr << "错误: rime->setup 为空" << std::endl;
        api_ok = false;
    }
    if (!rime->initialize) {
        std::cerr << "错误: rime->initialize 为空" << std::endl;
        api_ok = false;
    }
    if (!rime->finalize) {
        std::cerr << "错误: rime->finalize 为空" << std::endl;
        api_ok = false;
    }
    if (!rime->create_session) {
        std::cerr << "错误: rime->create_session 为空" << std::endl;
        api_ok = false;
    }
    if (!rime->destroy_session) {
        std::cerr << "错误: rime->destroy_session 为空" << std::endl;
        api_ok = false;
    }
    if (!rime->process_key) {
        std::cerr << "错误: rime->process_key 为空" << std::endl;
        api_ok = false;
    }
    if (!rime->get_context) {
        std::cerr << "错误: rime->get_context 为空" << std::endl;
        api_ok = false;
    }
    if (!rime->get_commit) {
        std::cerr << "错误: rime->get_commit 为空" << std::endl;
        api_ok = false;
    }
    
    if (api_ok) {
        std::cout << "✓ 所有关键 API 函数指针验证通过" << std::endl;
    } else {
        std::cerr << "✗ 部分 API 函数指针验证失败" << std::endl;
        return 1;
    }
    
    std::cout << std::endl;
    std::cout << "=== librime 预编译库验证成功 ===" << std::endl;
    std::cout << "库文件可以正确链接，API 可以正常调用。" << std::endl;
    
    return 0;
}
