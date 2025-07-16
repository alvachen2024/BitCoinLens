// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_WARNINGS_H
#define BITCOIN_NODE_WARNINGS_H

#include <sync.h>
#include <util/translation.h>

#include <map>
#include <variant>
#include <vector>

class UniValue;

namespace kernel {
enum class Warning; // 内核警告枚举
} // namespace kernel

namespace node {

/**
 * 节点警告枚举
 * 定义了节点可能产生的各种警告类型
 */
enum class Warning {
    CLOCK_OUT_OF_SYNC,        // 时钟不同步
    PRE_RELEASE_TEST_BUILD,   // 预发布测试版本
    FATAL_INTERNAL_ERROR,     // 致命内部错误
};

/**
 * @class Warnings
 * @brief Manages warning messages within a node.
 *
 * 管理节点内的警告消息。
 *
 * The Warnings class provides mechanisms to set, unset, and retrieve
 * warning messages. It updates the GUI when warnings are changed.
 *
 * Warnings类提供了设置、取消设置和检索警告消息的机制。
 * 当警告发生变化时，它会更新GUI。
 *
 * This class is designed to be non-copyable to ensure warnings
 * are managed centrally.
 *
 * 此类设计为不可复制，以确保警告集中管理。
 */
class Warnings
{
    typedef std::variant<kernel::Warning, node::Warning> warning_type; // 警告类型变体

    mutable Mutex m_mutex;    // 互斥锁，保护警告映射
    std::map<warning_type, bilingual_str> m_warnings GUARDED_BY(m_mutex); // 警告映射，存储警告ID和消息

public:
    Warnings();

    //! A warnings instance should always be passed by reference, never copied.
    //! 警告实例应该始终通过引用传递，绝不能复制。
    Warnings(const Warnings&) = delete;
    Warnings& operator=(const Warnings&) = delete;

        /**
     * @brief Set a warning message. If a warning with the specified
     *        `id` is already active, false is returned and the new
     *        warning is ignored. If `id` does not yet exist, the
     *        warning is set, the UI is updated, and true is returned.
     *
     * 设置警告消息。如果具有指定`id`的警告已经激活，
     * 则返回false并忽略新警告。如果`id`尚不存在，
     * 则设置警告，更新UI，并返回true。
     *
     * @param[in]   id  Unique identifier of the warning - 警告的唯一标识符
     * @param[in]   message Warning message to be shown - 要显示的警告消息
     *
     * @returns true if the warning was indeed set (i.e. there is no
     *          active warning with this `id`), otherwise false.
     * @returns 如果警告确实被设置（即没有具有此`id`的活动警告），
     *          则返回true，否则返回false。
     */
    bool Set(warning_type id, bilingual_str message) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

        /**
     * @brief Unset a warning message. If a warning with the specified
     *        `id` is active, it is unset, the UI is updated, and true
     *        is returned. Otherwise, no warning is unset and false is
     *        returned.
     *
     * 取消设置警告消息。如果具有指定`id`的警告处于活动状态，
     * 则取消设置，更新UI，并返回true。否则，不取消设置任何警告，
     * 并返回false。
     *
     * @param[in]   id  Unique identifier of the warning - 警告的唯一标识符
     *
     * @returns true if the warning was indeed unset (i.e. there is an
     *          active warning with this `id`), otherwise false.
     * @returns 如果警告确实被取消设置（即存在具有此`id`的活动警告），
     *          则返回true，否则返回false。
     */
    bool Unset(warning_type id) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Return potential problems detected by the node, sorted by the
     * warning_type id
     *
     * 返回节点检测到的潜在问题，按warning_type id排序 */
    std::vector<bilingual_str> GetMessages() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
};

/**
 * RPC helper function that wraps warnings.GetMessages().
 *
 * RPC辅助函数，包装warnings.GetMessages()。
 *
 * Returns a UniValue::VSTR with the latest warning if use_deprecated is
 * set to true, or a UniValue::VARR with all warnings otherwise.
 *
 * 如果use_deprecated设置为true，则返回包含最新警告的UniValue::VSTR，
 * 否则返回包含所有警告的UniValue::VARR。
 */
UniValue GetWarningsForRpc(const Warnings& warnings, bool use_deprecated);

} // namespace node

#endif // BITCOIN_NODE_WARNINGS_H
