/**
 * RimeWrapper - librime API 封装实现
 */

#include "rime_wrapper.h"
#include <cstring>
#include <iostream>

namespace suyan {

// ========== 单例实现 ==========

RimeWrapper& RimeWrapper::instance() {
    static RimeWrapper instance;
    return instance;
}

RimeWrapper::RimeWrapper() {
    api_ = rime_get_api();
}

RimeWrapper::~RimeWrapper() {
    if (initialized_) {
        finalize();
    }
}

// ========== 初始化与关闭 ==========

bool RimeWrapper::initialize(const std::string& userDataDir,
                              const std::string& sharedDataDir,
                              const std::string& appName) {
    if (initialized_) {
        return true;
    }

    if (!api_) {
        std::cerr << "RimeWrapper: Failed to get RIME API" << std::endl;
        return false;
    }

    // 设置 traits
    RIME_STRUCT(RimeTraits, traits);
    traits.shared_data_dir = sharedDataDir.c_str();
    traits.user_data_dir = userDataDir.c_str();
    traits.distribution_name = "SuYan";
    traits.distribution_code_name = "SuYan";
    traits.distribution_version = "1.0.0";
    traits.app_name = appName.c_str();
    traits.min_log_level = 1;  // WARNING

    // 设置通知回调
    api_->set_notification_handler(notificationHandler, this);

    // 初始化
    api_->setup(&traits);
    api_->initialize(&traits);

    initialized_ = true;
    return true;
}

void RimeWrapper::finalize() {
    if (!initialized_ || !api_) {
        return;
    }

    api_->finalize();
    initialized_ = false;
}

bool RimeWrapper::startMaintenance(bool fullCheck) {
    if (!initialized_ || !api_) {
        return false;
    }
    return api_->start_maintenance(fullCheck ? True : False);
}

void RimeWrapper::joinMaintenanceThread() {
    if (initialized_ && api_) {
        api_->join_maintenance_thread();
    }
}

bool RimeWrapper::isMaintenanceMode() const {
    if (!initialized_ || !api_) {
        return false;
    }
    return api_->is_maintenance_mode();
}

// ========== 会话管理 ==========

RimeSessionId RimeWrapper::createSession() {
    if (!initialized_ || !api_) {
        return 0;
    }
    return api_->create_session();
}

void RimeWrapper::destroySession(RimeSessionId sessionId) {
    if (initialized_ && api_ && sessionId != 0) {
        api_->destroy_session(sessionId);
    }
}

bool RimeWrapper::findSession(RimeSessionId sessionId) const {
    if (!initialized_ || !api_) {
        return false;
    }
    return api_->find_session(sessionId);
}

// ========== 输入处理 ==========

bool RimeWrapper::processKey(RimeSessionId sessionId, int keyCode, int modifiers) {
    if (!initialized_ || !api_ || sessionId == 0) {
        return false;
    }
    return api_->process_key(sessionId, keyCode, modifiers);
}

bool RimeWrapper::simulateKeySequence(RimeSessionId sessionId, const std::string& keySequence) {
    if (!initialized_ || !api_ || sessionId == 0) {
        return false;
    }
    return api_->simulate_key_sequence(sessionId, keySequence.c_str());
}

void RimeWrapper::clearComposition(RimeSessionId sessionId) {
    if (initialized_ && api_ && sessionId != 0) {
        api_->clear_composition(sessionId);
    }
}

bool RimeWrapper::commitComposition(RimeSessionId sessionId) {
    if (!initialized_ || !api_ || sessionId == 0) {
        return false;
    }
    return api_->commit_composition(sessionId);
}

// ========== 候选词操作 ==========

CandidateMenu RimeWrapper::getCandidateMenu(RimeSessionId sessionId) {
    CandidateMenu menu;
    menu.pageSize = 0;
    menu.pageIndex = 0;
    menu.isLastPage = true;
    menu.highlightedIndex = 0;

    if (!initialized_ || !api_ || sessionId == 0) {
        return menu;
    }

    RIME_STRUCT(RimeContext, context);
    if (!api_->get_context(sessionId, &context)) {
        return menu;
    }

    // 填充菜单信息
    menu.pageSize = context.menu.page_size;
    menu.pageIndex = context.menu.page_no;
    menu.isLastPage = context.menu.is_last_page;
    menu.highlightedIndex = context.menu.highlighted_candidate_index;

    if (context.menu.select_keys) {
        menu.selectKeys = context.menu.select_keys;
    }

    // 填充候选词
    for (int i = 0; i < context.menu.num_candidates; ++i) {
        Candidate candidate;
        candidate.index = i;
        if (context.menu.candidates[i].text) {
            candidate.text = context.menu.candidates[i].text;
        }
        if (context.menu.candidates[i].comment) {
            candidate.comment = context.menu.candidates[i].comment;
        }
        menu.candidates.push_back(candidate);
    }

    api_->free_context(&context);
    return menu;
}

bool RimeWrapper::selectCandidateOnCurrentPage(RimeSessionId sessionId, size_t index) {
    if (!initialized_ || !api_ || sessionId == 0) {
        return false;
    }
    return api_->select_candidate_on_current_page(sessionId, index);
}

bool RimeWrapper::selectCandidate(RimeSessionId sessionId, size_t index) {
    if (!initialized_ || !api_ || sessionId == 0) {
        return false;
    }
    return api_->select_candidate(sessionId, index);
}

bool RimeWrapper::highlightCandidate(RimeSessionId sessionId, size_t index) {
    if (!initialized_ || !api_ || sessionId == 0) {
        return false;
    }
    if (RIME_API_AVAILABLE(api_, highlight_candidate)) {
        return api_->highlight_candidate(sessionId, index);
    }
    return false;
}

bool RimeWrapper::changePage(RimeSessionId sessionId, bool backward) {
    if (!initialized_ || !api_ || sessionId == 0) {
        return false;
    }
    if (RIME_API_AVAILABLE(api_, change_page)) {
        return api_->change_page(sessionId, backward ? True : False);
    }
    return false;
}

bool RimeWrapper::deleteCandidate(RimeSessionId sessionId, size_t index) {
    if (!initialized_ || !api_ || sessionId == 0) {
        return false;
    }
    if (RIME_API_AVAILABLE(api_, delete_candidate)) {
        return api_->delete_candidate(sessionId, index);
    }
    return false;
}

// ========== 输出获取 ==========

std::string RimeWrapper::getCommitText(RimeSessionId sessionId) {
    if (!initialized_ || !api_ || sessionId == 0) {
        return "";
    }

    RIME_STRUCT(RimeCommit, commit);
    if (!api_->get_commit(sessionId, &commit)) {
        return "";
    }

    std::string text;
    if (commit.text) {
        text = commit.text;
    }

    api_->free_commit(&commit);
    return text;
}

Composition RimeWrapper::getComposition(RimeSessionId sessionId) {
    Composition comp;
    comp.cursorPos = 0;
    comp.selStart = 0;
    comp.selEnd = 0;

    if (!initialized_ || !api_ || sessionId == 0) {
        return comp;
    }

    RIME_STRUCT(RimeContext, context);
    if (!api_->get_context(sessionId, &context)) {
        return comp;
    }

    if (context.composition.preedit) {
        comp.preedit = context.composition.preedit;
    }
    comp.cursorPos = context.composition.cursor_pos;
    comp.selStart = context.composition.sel_start;
    comp.selEnd = context.composition.sel_end;

    api_->free_context(&context);
    return comp;
}

std::string RimeWrapper::getRawInput(RimeSessionId sessionId) {
    if (!initialized_ || !api_ || sessionId == 0) {
        return "";
    }

    const char* input = api_->get_input(sessionId);
    return input ? input : "";
}

size_t RimeWrapper::getCaretPos(RimeSessionId sessionId) {
    if (!initialized_ || !api_ || sessionId == 0) {
        return 0;
    }
    return api_->get_caret_pos(sessionId);
}

void RimeWrapper::setCaretPos(RimeSessionId sessionId, size_t pos) {
    if (initialized_ && api_ && sessionId != 0) {
        api_->set_caret_pos(sessionId, pos);
    }
}

// ========== 状态查询 ==========

RimeState RimeWrapper::getState(RimeSessionId sessionId) {
    RimeState state;
    state.isComposing = false;
    state.isAsciiMode = false;
    state.isDisabled = false;

    if (!initialized_ || !api_ || sessionId == 0) {
        return state;
    }

    RIME_STRUCT(RimeStatus, status);
    if (!api_->get_status(sessionId, &status)) {
        return state;
    }

    if (status.schema_id) {
        state.schemaId = status.schema_id;
    }
    if (status.schema_name) {
        state.schemaName = status.schema_name;
    }
    state.isComposing = status.is_composing;
    state.isAsciiMode = status.is_ascii_mode;
    state.isDisabled = status.is_disabled;

    api_->free_status(&status);
    return state;
}

bool RimeWrapper::getOption(RimeSessionId sessionId, const std::string& option) {
    if (!initialized_ || !api_ || sessionId == 0) {
        return false;
    }
    return api_->get_option(sessionId, option.c_str());
}

void RimeWrapper::setOption(RimeSessionId sessionId, const std::string& option, bool value) {
    if (initialized_ && api_ && sessionId != 0) {
        api_->set_option(sessionId, option.c_str(), value ? True : False);
    }
}

// ========== 方案管理 ==========

std::string RimeWrapper::getCurrentSchemaId(RimeSessionId sessionId) {
    if (!initialized_ || !api_ || sessionId == 0) {
        return "";
    }

    char buffer[256] = {0};
    if (api_->get_current_schema(sessionId, buffer, sizeof(buffer))) {
        return buffer;
    }
    return "";
}

bool RimeWrapper::selectSchema(RimeSessionId sessionId, const std::string& schemaId) {
    if (!initialized_ || !api_ || sessionId == 0) {
        return false;
    }
    return api_->select_schema(sessionId, schemaId.c_str());
}

std::vector<std::pair<std::string, std::string>> RimeWrapper::getSchemaList() {
    std::vector<std::pair<std::string, std::string>> result;

    if (!initialized_ || !api_) {
        return result;
    }

    RimeSchemaList schemaList;
    if (!api_->get_schema_list(&schemaList)) {
        return result;
    }

    for (size_t i = 0; i < schemaList.size; ++i) {
        std::string id = schemaList.list[i].schema_id ? schemaList.list[i].schema_id : "";
        std::string name = schemaList.list[i].name ? schemaList.list[i].name : "";
        result.emplace_back(id, name);
    }

    api_->free_schema_list(&schemaList);
    return result;
}

// ========== 通知回调 ==========

void RimeWrapper::setNotificationCallback(NotificationCallback callback) {
    notificationCallback_ = std::move(callback);
}

void RimeWrapper::notificationHandler(void* contextObject,
                                       RimeSessionId sessionId,
                                       const char* messageType,
                                       const char* messageValue) {
    auto* wrapper = static_cast<RimeWrapper*>(contextObject);
    if (wrapper && wrapper->notificationCallback_) {
        wrapper->notificationCallback_(
            sessionId,
            messageType ? messageType : "",
            messageValue ? messageValue : ""
        );
    }
}

// ========== 版本信息 ==========

std::string RimeWrapper::getVersion() const {
    if (!api_) {
        return "";
    }
    const char* version = api_->get_version();
    return version ? version : "";
}

} // namespace suyan
