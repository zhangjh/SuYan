// tests/unit/dictionary_manager_test.cpp
// 多词库管理器单元测试

#include <gtest/gtest.h>

#include <memory>

#include "core/dictionary/dictionary_manager.h"
#include "core/dictionary/dictionary_manager_impl.h"
#include "core/storage/sqlite_storage.h"

using namespace ime::dictionary;
using namespace ime::storage;

class DictionaryManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 使用内存数据库进行测试
        storage_ = std::make_unique<SqliteStorage>(":memory:");
        ASSERT_TRUE(storage_->Initialize());

        manager_ = std::make_unique<DictionaryManagerImpl>(storage_.get());
        ASSERT_TRUE(manager_->Initialize());
    }

    void TearDown() override {
        manager_->Shutdown();
        storage_->Close();
    }

    // 创建测试词库信息
    DictionaryInfo CreateTestDictInfo(const std::string& id, int priority) {
        DictionaryInfo info;
        info.id = id;
        info.name = "Test Dictionary " + id;
        info.type = DictionaryType::Base;
        info.version = "1.0.0";
        info.wordCount = 0;
        info.filePath = "";  // 不使用文件
        info.priority = priority;
        info.isEnabled = true;
        info.isLoaded = false;
        return info;
    }

    std::unique_ptr<SqliteStorage> storage_;
    std::unique_ptr<DictionaryManagerImpl> manager_;
};

// 测试：初始化
TEST_F(DictionaryManagerTest, Initialize) {
    EXPECT_TRUE(manager_->IsInitialized());
}

// 测试：注册词库
TEST_F(DictionaryManagerTest, RegisterDictionary) {
    auto info = CreateTestDictInfo("dict1", 100);
    EXPECT_TRUE(manager_->RegisterDictionary(info));

    auto retrieved = manager_->GetDictionaryInfo("dict1");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->id, "dict1");
    EXPECT_EQ(retrieved->priority, 100);
}

// 测试：注销词库
TEST_F(DictionaryManagerTest, UnregisterDictionary) {
    auto info = CreateTestDictInfo("dict1", 100);
    EXPECT_TRUE(manager_->RegisterDictionary(info));

    EXPECT_TRUE(manager_->UnregisterDictionary("dict1"));

    auto retrieved = manager_->GetDictionaryInfo("dict1");
    EXPECT_FALSE(retrieved.has_value());
}

// 测试：设置词库启用状态
TEST_F(DictionaryManagerTest, SetDictionaryEnabled) {
    auto info = CreateTestDictInfo("dict1", 100);
    EXPECT_TRUE(manager_->RegisterDictionary(info));

    EXPECT_TRUE(manager_->SetDictionaryEnabled("dict1", false));

    auto retrieved = manager_->GetDictionaryInfo("dict1");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_FALSE(retrieved->isEnabled);
}

// 测试：设置词库优先级
TEST_F(DictionaryManagerTest, SetDictionaryPriority) {
    auto info = CreateTestDictInfo("dict1", 100);
    EXPECT_TRUE(manager_->RegisterDictionary(info));

    EXPECT_TRUE(manager_->SetDictionaryPriority("dict1", 200));

    auto retrieved = manager_->GetDictionaryInfo("dict1");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->priority, 200);
}

// 测试：获取所有词库（按优先级排序）
TEST_F(DictionaryManagerTest, GetAllDictionariesSortedByPriority) {
    EXPECT_TRUE(manager_->RegisterDictionary(CreateTestDictInfo("dict1", 50)));
    EXPECT_TRUE(manager_->RegisterDictionary(CreateTestDictInfo("dict2", 100)));
    EXPECT_TRUE(manager_->RegisterDictionary(CreateTestDictInfo("dict3", 75)));

    auto dicts = manager_->GetAllDictionaries();
    ASSERT_EQ(dicts.size(), 3);

    // 应该按优先级降序排列
    EXPECT_EQ(dicts[0].id, "dict2");  // priority 100
    EXPECT_EQ(dicts[1].id, "dict3");  // priority 75
    EXPECT_EQ(dicts[2].id, "dict1");  // priority 50
}

// 测试：获取已启用的词库
TEST_F(DictionaryManagerTest, GetEnabledDictionaries) {
    EXPECT_TRUE(manager_->RegisterDictionary(CreateTestDictInfo("dict1", 100)));
    EXPECT_TRUE(manager_->RegisterDictionary(CreateTestDictInfo("dict2", 50)));

    EXPECT_TRUE(manager_->SetDictionaryEnabled("dict2", false));

    auto enabled = manager_->GetEnabledDictionaries();
    ASSERT_EQ(enabled.size(), 1);
    EXPECT_EQ(enabled[0].id, "dict1");
}

// 测试：多词库合并查询（使用测试辅助方法）
TEST_F(DictionaryManagerTest, MergeQueryFromMultipleDictionaries) {
    // 注册两个词库
    auto info1 = CreateTestDictInfo("dict1", 100);  // 高优先级
    auto info2 = CreateTestDictInfo("dict2", 50);   // 低优先级
    EXPECT_TRUE(manager_->RegisterDictionary(info1));
    EXPECT_TRUE(manager_->RegisterDictionary(info2));

    // 手动创建已加载的词库（模拟加载）
    // 由于没有实际文件，我们使用测试辅助方法直接添加词条

    // 首先需要"加载"词库（创建空的已加载词库）
    // 这里我们通过直接操作来模拟

    // 创建一个新的管理器实例用于测试
    auto testManager = std::make_unique<DictionaryManagerImpl>(storage_.get());
    ASSERT_TRUE(testManager->Initialize());

    // 注册词库
    EXPECT_TRUE(testManager->RegisterDictionary(info1));
    EXPECT_TRUE(testManager->RegisterDictionary(info2));

    // 由于无法直接加载（没有文件），我们测试空查询
    auto result = testManager->Query("ni", 10);
    EXPECT_TRUE(result.entries.empty());
}

// 测试：词库优先级影响查询结果
TEST_F(DictionaryManagerTest, PriorityAffectsQueryOrder) {
    // 这个测试验证优先级排序逻辑
    // 由于需要实际词库文件，这里只测试排序逻辑

    auto info1 = CreateTestDictInfo("dict1", 100);
    auto info2 = CreateTestDictInfo("dict2", 200);
    auto info3 = CreateTestDictInfo("dict3", 50);

    EXPECT_TRUE(manager_->RegisterDictionary(info1));
    EXPECT_TRUE(manager_->RegisterDictionary(info2));
    EXPECT_TRUE(manager_->RegisterDictionary(info3));

    auto dicts = manager_->GetAllDictionaries();
    ASSERT_EQ(dicts.size(), 3);

    // 验证按优先级降序排列
    EXPECT_EQ(dicts[0].priority, 200);
    EXPECT_EQ(dicts[1].priority, 100);
    EXPECT_EQ(dicts[2].priority, 50);
}

// 测试：空拼音查询
TEST_F(DictionaryManagerTest, EmptyPinyinQuery) {
    auto result = manager_->Query("", 10);
    EXPECT_TRUE(result.entries.empty());
    EXPECT_EQ(result.totalCount, 0);
}

// 测试：检查词条是否存在（空词库）
TEST_F(DictionaryManagerTest, ContainsWordEmptyDictionary) {
    EXPECT_FALSE(manager_->ContainsWord("你", "ni"));
}

// 测试：获取词频（空词库）
TEST_F(DictionaryManagerTest, GetWordFrequencyEmptyDictionary) {
    EXPECT_EQ(manager_->GetWordFrequency("你", "ni"), -1);
}

// 测试：卸载所有词库
TEST_F(DictionaryManagerTest, UnloadAllDictionaries) {
    EXPECT_TRUE(manager_->RegisterDictionary(CreateTestDictInfo("dict1", 100)));
    EXPECT_TRUE(manager_->RegisterDictionary(CreateTestDictInfo("dict2", 50)));

    manager_->UnloadAllDictionaries();

    auto loaded = manager_->GetLoadedDictionaries();
    EXPECT_TRUE(loaded.empty());
}

// 测试：词库类型转换
TEST_F(DictionaryManagerTest, DictionaryTypeConversion) {
    EXPECT_EQ(DictionaryTypeUtils::ToString(DictionaryType::Base), "base");
    EXPECT_EQ(DictionaryTypeUtils::ToString(DictionaryType::Extended),
              "extended");
    EXPECT_EQ(DictionaryTypeUtils::ToString(DictionaryType::Industry),
              "industry");
    EXPECT_EQ(DictionaryTypeUtils::ToString(DictionaryType::User), "user");

    EXPECT_EQ(DictionaryTypeUtils::FromString("base"), DictionaryType::Base);
    EXPECT_EQ(DictionaryTypeUtils::FromString("extended"),
              DictionaryType::Extended);
    EXPECT_EQ(DictionaryTypeUtils::FromString("industry"),
              DictionaryType::Industry);
    EXPECT_EQ(DictionaryTypeUtils::FromString("user"), DictionaryType::User);
    EXPECT_EQ(DictionaryTypeUtils::FromString("unknown"), DictionaryType::Base);
}
