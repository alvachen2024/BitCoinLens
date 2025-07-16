// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_TIMEOFFSETS_H
#define BITCOIN_NODE_TIMEOFFSETS_H

#include <sync.h>

#include <chrono>
#include <cstddef>
#include <deque>

namespace node {
class Warnings; // 警告管理类
} // namespace node

/**
 * 时间偏移管理类
 * 用于跟踪和管理本地时钟与网络对等节点时钟之间的时间差异
 * 帮助检测系统时钟是否与网络时间同步
 */
class TimeOffsets
{
public:
    /**
     * 构造函数
     * @param warnings 警告管理器的引用，用于发出时间同步警告
     */
    TimeOffsets(node::Warnings& warnings) : m_warnings{warnings} {}

private:
    //! Maximum number of timeoffsets stored.
    //! 存储的最大时间偏移数量
    static constexpr size_t MAX_SIZE{50};

    //! Minimum difference between system and network time for a warning to be raised.
    //! 发出警告的系统时间和网络时间之间的最小差异
    static constexpr std::chrono::minutes WARN_THRESHOLD{10};

    mutable Mutex m_mutex; // 互斥锁，保护时间偏移数据

    /** The observed time differences between our local clock and those of our outbound peers. A
     * positive offset means our peer's clock is ahead of our local clock.
     *
     * 我们本地时钟与出站对等节点时钟之间观察到的时间差异。
     * 正偏移意味着对等节点的时钟领先于我们的本地时钟。 */
    std::deque<std::chrono::seconds> m_offsets GUARDED_BY(m_mutex){}; // 时间偏移队列

    node::Warnings& m_warnings; // 警告管理器引用

public:
    /** Add a new time offset sample.
     *
     * 添加新的时间偏移样本。
     *
     * @param offset 时间偏移值（秒）
     */
    void Add(std::chrono::seconds offset) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Compute and return the median of the collected time offset samples. The median is returned
     * as 0 when there are less than 5 samples.
     *
     * 计算并返回收集的时间偏移样本的中位数。当样本少于5个时，中位数返回为0。
     *
     * @return 时间偏移的中位数（秒）
     */
    std::chrono::seconds Median() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Raise warnings if the median time offset exceeds the warnings threshold. Returns true if
     * warnings were raised.
     *
     * 如果中位数时间偏移超过警告阈值，则发出警告。如果发出警告则返回true。
     *
     * @return 是否发出了警告
     */
    bool WarnIfOutOfSync() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
};

#endif // BITCOIN_NODE_TIMEOFFSETS_H
