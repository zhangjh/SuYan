/**
 * LayoutManager 实现
 */

#include "layout_manager.h"
#include <iostream>

namespace suyan {

// ========== 辅助函数实现 ==========

QString layoutTypeToQString(LayoutType type) {
    switch (type) {
        case LayoutType::Horizontal: return QStringLiteral("horizontal");
        case LayoutType::Vertical: return QStringLiteral("vertical");
        default: return QStringLiteral("horizontal");
    }
}

LayoutType qstringToLayoutType(const QString& str) {
    if (str == QStringLiteral("vertical")) return LayoutType::Vertical;
    return LayoutType::Horizontal;  // 默认横排
}

// ========== LayoutManager 实现 ==========

LayoutManager& LayoutManager::instance() {
    static LayoutManager instance;
    return instance;
}

LayoutManager::LayoutManager() : QObject(nullptr) {
    // 默认值
    layoutType_ = LayoutType::Horizontal;
    pageSize_ = 9;
}

LayoutManager::~LayoutManager() = default;

bool LayoutManager::initialize() {
    if (initialized_) {
        return true;
    }

    // 从 ConfigManager 加载配置
    loadFromConfig();

    // 监听 ConfigManager 的配置变更
    connect(&ConfigManager::instance(), &ConfigManager::layoutConfigChanged,
            this, [this](const LayoutConfig& config) {
        if (layoutType_ != config.type) {
            layoutType_ = config.type;
            emit layoutTypeChanged(layoutType_);
        }
        
        if (pageSize_ != config.pageSize) {
            pageSize_ = config.pageSize;
            emit pageSizeChanged(pageSize_);
        }
    });

    initialized_ = true;
    return true;
}

void LayoutManager::loadFromConfig() {
    auto& configMgr = ConfigManager::instance();
    
    if (!configMgr.isInitialized()) {
        std::cerr << "LayoutManager: ConfigManager 未初始化，使用默认配置" << std::endl;
        return;
    }

    LayoutConfig config = configMgr.getLayoutConfig();
    layoutType_ = config.type;
    pageSize_ = config.pageSize;
}

void LayoutManager::saveToConfig() {
    auto& configMgr = ConfigManager::instance();
    
    if (!configMgr.isInitialized()) {
        std::cerr << "LayoutManager: ConfigManager 未初始化，无法保存配置" << std::endl;
        return;
    }

    configMgr.setLayoutType(layoutType_);
    configMgr.setPageSize(pageSize_);
}

void LayoutManager::setLayoutType(LayoutType type) {
    if (layoutType_ != type) {
        layoutType_ = type;
        saveToConfig();
        emit layoutTypeChanged(type);
    }
}

void LayoutManager::toggleLayout() {
    if (layoutType_ == LayoutType::Horizontal) {
        setLayoutType(LayoutType::Vertical);
    } else {
        setLayoutType(LayoutType::Horizontal);
    }
}

void LayoutManager::setPageSize(int size) {
    // 限制范围 1-10
    if (size < 1) size = 1;
    if (size > 10) size = 10;

    if (pageSize_ != size) {
        pageSize_ = size;
        saveToConfig();
        emit pageSizeChanged(size);
    }
}

} // namespace suyan
