/**
 * FrequencyManager 单元测试
 *
 * 测试词频管理器的 SQLite 存储、更新、查询和排序合并功能。
 */

#include <iostream>
#include <cassert>
#include <filesystem>
#include <thread>
#include <chrono>
#include <QCoreApplication>
#include <QSignalSpy>
#include "frequency_manager.h"

namespace fs = std::filesystem;

// 测试辅助宏
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

class FrequencyManagerTest {
public:
    FrequencyManagerTest() {
        // 使用临时目录进行测试
        testDataDir_ = fs::temp_directory_path().string() + "/suyan_freq_test";
        
        // 清理之前的测试数据
        fs::remove_all(testDataDir_);
    }
    
    ~FrequencyManagerTest() {
        // 关闭管理器
        suyan::FrequencyManager::instance().shutdown();
        // 清理测试数据
        fs::remove_all(testDataDir_);
    }
    
    bool runAllTests() {
        std::cout << "=== FrequencyManager 单元测试 ===" << std::endl;
        std::cout << "测试数据目录: " << testDataDir_ << std::endl;
        std::cout << std::endl;
        
        bool allPassed = true;
        
        // 基础测试
        allPassed &= testGetInstance();
        allPassed &= testInitialize();
        
        // 词频更新测试
        allPassed &= testUpdateFrequency();
        allPassed &= testUpdateFrequencyIncrement();
        allPassed &= testSetFrequency();
        allPassed &= testUpdateFrequencyBatch();
        
        // 词频查询测试
        allPassed &= testGetFrequency();
        allPassed &= testGetWordFrequency();
        allPassed &= testQueryByPinyin();
        allPassed &= testGetHighFrequencyWords();
        
        // 排序合并测试
        allPassed &= testMergeSortCandidates();
        allPassed &= testMergeSortWithNoUserFrequency();
        
        // 数据管理测试
        allPassed &= testDeleteFrequency();
        allPassed &= testGetRecordCount();
        allPassed &= testCleanupLowFrequency();
        allPassed &= testClearAll();
        
        // 导入导出测试
        allPassed &= testExportImport();
        
        // 信号测试
        allPassed &= testSignals();
        
        std::cout << std::endl;
        if (allPassed) {
            std::cout << "=== 所有测试通过 ===" << std::endl;
        } else {
            std::cout << "=== 部分测试失败 ===" << std::endl;
        }
        
        return allPassed;
    }
    
private:
    std::string testDataDir_;
    
    // 重置测试环境
    void resetTestEnvironment() {
        auto& fm = suyan::FrequencyManager::instance();
        if (fm.isInitialized()) {
            fm.clearAll();
        }
    }
    
    // ========== 基础测试 ==========
    
    bool testGetInstance() {
        auto& fm1 = suyan::FrequencyManager::instance();
        auto& fm2 = suyan::FrequencyManager::instance();
        TEST_ASSERT(&fm1 == &fm2, "单例实例应该相同");
        TEST_PASS("testGetInstance: 单例模式正常");
        return true;
    }
    
    bool testInitialize() {
        auto& fm = suyan::FrequencyManager::instance();
        
        // 首次初始化
        bool result = fm.initialize(testDataDir_);
        TEST_ASSERT(result, "初始化应该成功");
        TEST_ASSERT(fm.isInitialized(), "初始化后 isInitialized 应该返回 true");
        
        // 检查数据目录已创建
        TEST_ASSERT(fs::exists(testDataDir_), "数据目录应该已创建");
        
        // 检查数据库文件已创建
        std::string dbPath = fm.getDatabasePath();
        TEST_ASSERT(fs::exists(dbPath), "数据库文件应该已创建");
        std::cout << "  数据库路径: " << dbPath << std::endl;
        
        // 重复初始化应该返回 true（幂等）
        result = fm.initialize(testDataDir_);
        TEST_ASSERT(result, "重复初始化应该返回 true");
        
        TEST_PASS("testInitialize: 初始化成功");
        return true;
    }
    
    // ========== 词频更新测试 ==========
    
    bool testUpdateFrequency() {
        resetTestEnvironment();
        auto& fm = suyan::FrequencyManager::instance();
        
        // 更新一个新词
        bool result = fm.updateFrequency("你好", "nihao");
        TEST_ASSERT(result, "更新词频应该成功");
        
        // 验证词频为 1
        int freq = fm.getFrequency("你好", "nihao");
        TEST_ASSERT(freq == 1, "新词的词频应该是 1");
        
        TEST_PASS("testUpdateFrequency: 更新词频正常");
        return true;
    }
    
    bool testUpdateFrequencyIncrement() {
        resetTestEnvironment();
        auto& fm = suyan::FrequencyManager::instance();
        
        // 多次更新同一个词
        fm.updateFrequency("世界", "shijie");
        fm.updateFrequency("世界", "shijie");
        fm.updateFrequency("世界", "shijie");
        
        // 验证词频累加
        int freq = fm.getFrequency("世界", "shijie");
        TEST_ASSERT(freq == 3, "多次更新后词频应该累加");
        
        TEST_PASS("testUpdateFrequencyIncrement: 词频累加正常");
        return true;
    }
    
    bool testSetFrequency() {
        resetTestEnvironment();
        auto& fm = suyan::FrequencyManager::instance();
        
        // 直接设置词频
        bool result = fm.setFrequency("测试", "ceshi", 10);
        TEST_ASSERT(result, "设置词频应该成功");
        
        int freq = fm.getFrequency("测试", "ceshi");
        TEST_ASSERT(freq == 10, "词频应该是 10");
        
        // 再次设置不同的值
        fm.setFrequency("测试", "ceshi", 5);
        freq = fm.getFrequency("测试", "ceshi");
        TEST_ASSERT(freq == 5, "词频应该被更新为 5");
        
        TEST_PASS("testSetFrequency: 设置词频正常");
        return true;
    }
    
    bool testUpdateFrequencyBatch() {
        resetTestEnvironment();
        auto& fm = suyan::FrequencyManager::instance();
        
        std::vector<std::pair<std::string, std::string>> words = {
            {"苹果", "pingguo"},
            {"香蕉", "xiangjiao"},
            {"橘子", "juzi"}
        };
        
        int count = fm.updateFrequencyBatch(words);
        TEST_ASSERT(count == 3, "批量更新应该成功 3 个");
        
        // 验证每个词都被添加
        TEST_ASSERT(fm.getFrequency("苹果", "pingguo") == 1, "苹果词频应该是 1");
        TEST_ASSERT(fm.getFrequency("香蕉", "xiangjiao") == 1, "香蕉词频应该是 1");
        TEST_ASSERT(fm.getFrequency("橘子", "juzi") == 1, "橘子词频应该是 1");
        
        TEST_PASS("testUpdateFrequencyBatch: 批量更新正常");
        return true;
    }
    
    // ========== 词频查询测试 ==========
    
    bool testGetFrequency() {
        resetTestEnvironment();
        auto& fm = suyan::FrequencyManager::instance();
        
        // 查询不存在的词
        int freq = fm.getFrequency("不存在", "bucunzai");
        TEST_ASSERT(freq == 0, "不存在的词频应该返回 0");
        
        // 添加词后查询
        fm.setFrequency("存在", "cunzai", 5);
        freq = fm.getFrequency("存在", "cunzai");
        TEST_ASSERT(freq == 5, "存在的词频应该返回正确值");
        
        TEST_PASS("testGetFrequency: 获取词频正常");
        return true;
    }
    
    bool testGetWordFrequency() {
        resetTestEnvironment();
        auto& fm = suyan::FrequencyManager::instance();
        
        // 查询不存在的词
        auto wf = fm.getWordFrequency("不存在", "bucunzai");
        TEST_ASSERT(!wf.has_value(), "不存在的词应该返回空 optional");
        
        // 添加词后查询
        fm.setFrequency("详细", "xiangxi", 8);
        wf = fm.getWordFrequency("详细", "xiangxi");
        TEST_ASSERT(wf.has_value(), "存在的词应该返回有效 optional");
        TEST_ASSERT(wf->word == "详细", "词文本应该正确");
        TEST_ASSERT(wf->pinyin == "xiangxi", "拼音应该正确");
        TEST_ASSERT(wf->frequency == 8, "词频应该正确");
        TEST_ASSERT(wf->id > 0, "ID 应该大于 0");
        TEST_ASSERT(wf->lastUsedAt > 0, "最后使用时间应该大于 0");
        TEST_ASSERT(wf->createdAt > 0, "创建时间应该大于 0");
        
        TEST_PASS("testGetWordFrequency: 获取词频记录正常");
        return true;
    }
    
    bool testQueryByPinyin() {
        resetTestEnvironment();
        auto& fm = suyan::FrequencyManager::instance();
        
        // 添加多个相同拼音的词
        fm.setFrequency("是", "shi", 10);
        fm.setFrequency("时", "shi", 5);
        fm.setFrequency("事", "shi", 8);
        fm.setFrequency("世", "shi", 3);
        
        // 查询
        auto results = fm.queryByPinyin("shi");
        TEST_ASSERT(results.size() == 4, "应该返回 4 个结果");
        
        // 验证按频率降序排列
        TEST_ASSERT(results[0].word == "是", "第一个应该是频率最高的");
        TEST_ASSERT(results[0].frequency == 10, "第一个词频应该是 10");
        TEST_ASSERT(results[1].word == "事", "第二个应该是频率第二高的");
        
        // 测试 limit
        results = fm.queryByPinyin("shi", 2);
        TEST_ASSERT(results.size() == 2, "限制后应该返回 2 个结果");
        
        TEST_PASS("testQueryByPinyin: 按拼音查询正常");
        return true;
    }
    
    bool testGetHighFrequencyWords() {
        resetTestEnvironment();
        auto& fm = suyan::FrequencyManager::instance();
        
        // 添加不同频率的词
        fm.setFrequency("高频", "gaopin", 100);
        fm.setFrequency("中频", "zhongpin", 50);
        fm.setFrequency("低频", "dipin", 2);
        
        // 查询高频词（阈值 10）
        auto results = fm.getHighFrequencyWords(10);
        TEST_ASSERT(results.size() == 2, "应该返回 2 个高频词");
        TEST_ASSERT(results[0].word == "高频", "第一个应该是最高频的");
        
        // 查询高频词（阈值 60）
        results = fm.getHighFrequencyWords(60);
        TEST_ASSERT(results.size() == 1, "应该返回 1 个高频词");
        
        TEST_PASS("testGetHighFrequencyWords: 获取高频词正常");
        return true;
    }
    
    // ========== 排序合并测试 ==========
    
    bool testMergeSortCandidates() {
        resetTestEnvironment();
        auto& fm = suyan::FrequencyManager::instance();
        
        // 设置用户词频
        fm.setFrequency("你好", "nihao", 10);  // 高频
        fm.setFrequency("拟好", "nihao", 2);   // 低频
        
        // 原始候选词列表（假设 RIME 返回的顺序）
        std::vector<std::pair<std::string, std::string>> candidates = {
            {"拟好", "nǐ hǎo"},   // 原始第一
            {"你好", "nǐ hǎo"},   // 原始第二
            {"泥好", "ní hǎo"},   // 原始第三（无用户词频）
        };
        
        // 合并排序
        auto sorted = fm.mergeSortCandidates(candidates, "nihao", 1);
        
        TEST_ASSERT(sorted.size() == 3, "排序后应该有 3 个候选词");
        
        // 验证高频词被提升
        TEST_ASSERT(sorted[0].text == "你好", "高频词应该排在第一");
        TEST_ASSERT(sorted[0].userFrequency == 10, "用户词频应该正确");
        
        TEST_PASS("testMergeSortCandidates: 排序合并正常");
        return true;
    }
    
    bool testMergeSortWithNoUserFrequency() {
        resetTestEnvironment();
        auto& fm = suyan::FrequencyManager::instance();
        
        // 不设置任何用户词频
        std::vector<std::pair<std::string, std::string>> candidates = {
            {"第一", "diyi"},
            {"第二", "dier"},
            {"第三", "disan"},
        };
        
        // 合并排序
        auto sorted = fm.mergeSortCandidates(candidates, "di", 1);
        
        TEST_ASSERT(sorted.size() == 3, "排序后应该有 3 个候选词");
        
        // 没有用户词频时，应该保持原始顺序
        TEST_ASSERT(sorted[0].text == "第一", "第一个应该保持不变");
        TEST_ASSERT(sorted[1].text == "第二", "第二个应该保持不变");
        TEST_ASSERT(sorted[2].text == "第三", "第三个应该保持不变");
        
        // 所有用户词频应该是 0
        for (const auto& c : sorted) {
            TEST_ASSERT(c.userFrequency == 0, "用户词频应该是 0");
        }
        
        TEST_PASS("testMergeSortWithNoUserFrequency: 无用户词频时排序正常");
        return true;
    }
    
    // ========== 数据管理测试 ==========
    
    bool testDeleteFrequency() {
        resetTestEnvironment();
        auto& fm = suyan::FrequencyManager::instance();
        
        // 添加词
        fm.setFrequency("删除", "shanchu", 5);
        TEST_ASSERT(fm.getFrequency("删除", "shanchu") == 5, "词应该存在");
        
        // 删除
        bool result = fm.deleteFrequency("删除", "shanchu");
        TEST_ASSERT(result, "删除应该成功");
        TEST_ASSERT(fm.getFrequency("删除", "shanchu") == 0, "删除后词频应该是 0");
        
        TEST_PASS("testDeleteFrequency: 删除词频正常");
        return true;
    }
    
    bool testGetRecordCount() {
        resetTestEnvironment();
        auto& fm = suyan::FrequencyManager::instance();
        
        // 初始应该是 0
        TEST_ASSERT(fm.getRecordCount() == 0, "初始记录数应该是 0");
        
        // 添加几个词
        fm.setFrequency("一", "yi", 1);
        fm.setFrequency("二", "er", 2);
        fm.setFrequency("三", "san", 3);
        
        TEST_ASSERT(fm.getRecordCount() == 3, "记录数应该是 3");
        
        TEST_PASS("testGetRecordCount: 获取记录数正常");
        return true;
    }
    
    bool testCleanupLowFrequency() {
        resetTestEnvironment();
        auto& fm = suyan::FrequencyManager::instance();
        
        // 添加不同频率的词
        fm.setFrequency("高", "gao", 10);
        fm.setFrequency("中", "zhong", 5);
        fm.setFrequency("低", "di", 2);
        
        // 清理低频词（阈值 5）
        int deleted = fm.cleanupLowFrequency(5);
        TEST_ASSERT(deleted == 1, "应该删除 1 个低频词");
        
        // 验证
        TEST_ASSERT(fm.getFrequency("高", "gao") == 10, "高频词应该保留");
        TEST_ASSERT(fm.getFrequency("中", "zhong") == 5, "中频词应该保留");
        TEST_ASSERT(fm.getFrequency("低", "di") == 0, "低频词应该被删除");
        
        TEST_PASS("testCleanupLowFrequency: 清理低频词正常");
        return true;
    }
    
    bool testClearAll() {
        resetTestEnvironment();
        auto& fm = suyan::FrequencyManager::instance();
        
        // 添加一些词
        fm.setFrequency("清空", "qingkong", 1);
        fm.setFrequency("测试", "ceshi", 2);
        TEST_ASSERT(fm.getRecordCount() == 2, "应该有 2 条记录");
        
        // 清空
        bool result = fm.clearAll();
        TEST_ASSERT(result, "清空应该成功");
        TEST_ASSERT(fm.getRecordCount() == 0, "清空后记录数应该是 0");
        
        TEST_PASS("testClearAll: 清空数据正常");
        return true;
    }
    
    // ========== 导入导出测试 ==========
    
    bool testExportImport() {
        resetTestEnvironment();
        auto& fm = suyan::FrequencyManager::instance();
        
        // 添加测试数据
        fm.setFrequency("导出", "daochu", 10);
        fm.setFrequency("测试", "ceshi", 5);
        
        // 导出
        std::string exportPath = testDataDir_ + "/export_test.txt";
        bool result = fm.exportToFile(exportPath);
        TEST_ASSERT(result, "导出应该成功");
        TEST_ASSERT(fs::exists(exportPath), "导出文件应该存在");
        
        // 清空数据
        fm.clearAll();
        TEST_ASSERT(fm.getRecordCount() == 0, "清空后记录数应该是 0");
        
        // 导入
        int imported = fm.importFromFile(exportPath, false);
        TEST_ASSERT(imported == 2, "应该导入 2 条记录");
        
        // 验证数据
        TEST_ASSERT(fm.getFrequency("导出", "daochu") == 10, "导出词频应该正确");
        TEST_ASSERT(fm.getFrequency("测试", "ceshi") == 5, "测试词频应该正确");
        
        TEST_PASS("testExportImport: 导入导出正常");
        return true;
    }
    
    // ========== 信号测试 ==========
    
    bool testSignals() {
        resetTestEnvironment();
        auto& fm = suyan::FrequencyManager::instance();
        
        // 监听信号
        QSignalSpy updateSpy(&fm, &suyan::FrequencyManager::frequencyUpdated);
        QSignalSpy clearSpy(&fm, &suyan::FrequencyManager::dataCleared);
        
        // 更新词频
        fm.updateFrequency("信号", "xinhao");
        TEST_ASSERT(updateSpy.count() >= 1, "frequencyUpdated 信号应该被发送");
        
        // 清空数据
        fm.clearAll();
        TEST_ASSERT(clearSpy.count() >= 1, "dataCleared 信号应该被发送");
        
        TEST_PASS("testSignals: 信号发送正常");
        return true;
    }
};

int main(int argc, char* argv[]) {
    // 需要 QCoreApplication 来支持 Qt 信号
    QCoreApplication app(argc, argv);
    
    FrequencyManagerTest test;
    return test.runAllTests() ? 0 : 1;
}
