// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_KERNEL_NOTIFICATIONS_H
#define BITCOIN_NODE_KERNEL_NOTIFICATIONS_H

#include <kernel/notifications_interface.h>

#include <sync.h>
#include <threadsafety.h>
#include <uint256.h>

#include <atomic>
#include <cstdint>
#include <functional>

class ArgsManager;
class CBlockIndex;
enum class SynchronizationState;
struct bilingual_str;

namespace kernel {
enum class Warning; // 内核警告枚举
} // namespace kernel

namespace node {

class Warnings;
static constexpr int DEFAULT_STOPATHEIGHT{0}; // 默认停止高度

/**
 * 内核通知类
 * 实现内核通知接口，处理区块链同步过程中的各种通知事件
 * 包括区块提示、头部提示、进度更新、警告和错误等
 */
class KernelNotifications : public kernel::Notifications
{
public:
    /**
     * 构造函数
     * @param shutdown_request 关闭请求函数
     * @param exit_status 退出状态原子变量
     * @param warnings 警告管理器
     */
    KernelNotifications(const std::function<bool()>& shutdown_request, std::atomic<int>& exit_status, node::Warnings& warnings)
        : m_shutdown_request(shutdown_request), m_exit_status{exit_status}, m_warnings{warnings} {}

    /**
     * 区块提示通知
     * 当区块链尖端更新时调用，通知新的区块信息
     *
     * @param state 同步状态
     * @param index 区块索引
     * @param verification_progress 验证进度
     * @return 中断结果
     */
    [[nodiscard]] kernel::InterruptResult blockTip(SynchronizationState state, CBlockIndex& index, double verification_progress) override EXCLUSIVE_LOCKS_REQUIRED(!m_tip_block_mutex);

    /**
     * 头部提示通知
     * 当头部链尖端更新时调用
     *
     * @param state 同步状态
     * @param height 区块高度
     * @param timestamp 时间戳
     * @param presync 是否预同步
     */
    void headerTip(SynchronizationState state, int64_t height, int64_t timestamp, bool presync) override;

    /**
     * 进度通知
     * 报告同步或验证进度
     *
     * @param title 进度标题
     * @param progress_percent 进度百分比
     * @param resume_possible 是否可以恢复
     */
    void progress(const bilingual_str& title, int progress_percent, bool resume_possible) override;

    /**
     * 设置警告
     * 设置内核警告消息
     *
     * @param id 警告ID
     * @param message 警告消息
     */
    void warningSet(kernel::Warning id, const bilingual_str& message) override;

    /**
     * 取消警告
     * 取消内核警告消息
     *
     * @param id 警告ID
     */
    void warningUnset(kernel::Warning id) override;

    /**
     * 刷新错误
     * 报告刷新过程中的错误
     *
     * @param message 错误消息
     */
    void flushError(const bilingual_str& message) override;

    /**
     * 致命错误
     * 报告致命错误，可能导致节点关闭
     *
     * @param message 错误消息
     */
    void fatalError(const bilingual_str& message) override;

    //! Block height after which blockTip notification will return Interrupted{}, if >0.
    //! 如果大于0，在此区块高度之后blockTip通知将返回Interrupted{}。
    int m_stop_at_height{DEFAULT_STOPATHEIGHT};

    //! Useful for tests, can be set to false to avoid shutdown on fatal error.
    //! 对测试有用，可以设置为false以避免在致命错误时关闭。
    bool m_shutdown_on_fatal_error{true};

    Mutex m_tip_block_mutex; // 区块尖端互斥锁
    std::condition_variable m_tip_block_cv GUARDED_BY(m_tip_block_mutex); // 区块尖端条件变量

    //! The block for which the last blockTip notification was received.
    //! It's first set when the tip is connected during node initialization.
    //! Might be unset during an early shutdown.
    //!
    //! 收到最后一个blockTip通知的区块。
    //! 在节点初始化期间尖端连接时首次设置。
    //! 在早期关闭期间可能未设置。
    std::optional<uint256> TipBlock() EXCLUSIVE_LOCKS_REQUIRED(m_tip_block_mutex);

private:
    const std::function<bool()>& m_shutdown_request; // 关闭请求函数引用
    std::atomic<int>& m_exit_status;                 // 退出状态引用
    node::Warnings& m_warnings;                      // 警告管理器引用

    std::optional<uint256> m_tip_block GUARDED_BY(m_tip_block_mutex); // 当前尖端区块哈希
};

/**
 * 读取通知参数
 * 从参数管理器读取通知相关的配置参数
 *
 * @param args 参数管理器
 * @param notifications 内核通知对象
 */
void ReadNotificationArgs(const ArgsManager& args, KernelNotifications& notifications);

} // namespace node

#endif // BITCOIN_NODE_KERNEL_NOTIFICATIONS_H
