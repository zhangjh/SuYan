/**
 * @file main.cpp
 * @brief 属性测试入口
 * 
 * 使用 RapidCheck 进行属性测试 (Property-Based Testing)
 * 每个属性测试运行 100+ 次迭代
 */

#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
