// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_EVICTION_H
#define BITCOIN_NODE_EVICTION_H

#include <node/connection_types.h>
#include <net_permissions.h>

#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

typedef int64_t NodeId; // 节点ID类型定义

/**
 * 节点驱逐候选者结构体
 * 包含用于评估是否驱逐对等节点的各种属性
 */
struct NodeEvictionCandidate {
    NodeId id;                                    // 节点ID
    std::chrono::seconds m_connected;             // 连接时长
    std::chrono::microseconds m_min_ping_time;    // 最小ping时间
    std::chrono::seconds m_last_block_time;       // 最后区块时间
    std::chrono::seconds m_last_tx_time;          // 最后交易时间
    bool fRelevantServices;                       // 是否有相关服务
    bool m_relay_txs;                             // 是否中继交易
    bool fBloomFilter;                            // 是否使用布隆过滤器
    uint64_t nKeyedNetGroup;                      // 网络组密钥
    bool prefer_evict;                            // 是否优先驱逐
    bool m_is_local;                              // 是否为本地连接
    Network m_network;                            // 网络类型
    bool m_noban;                                 // 是否禁止封禁
    ConnectionType m_conn_type;                   // 连接类型
};

/**
 * Select an inbound peer to evict after filtering out (protecting) peers having
 * distinct, difficult-to-forge characteristics. The protection logic picks out
 * fixed numbers of desirable peers per various criteria, followed by (mostly)
 * ratios of desirable or disadvantaged peers. If any eviction candidates
 * remain, the selection logic chooses a peer to evict.
 *
 * 在过滤掉（保护）具有独特、难以伪造特征的对等节点后，选择一个入站对等节点进行驱逐。
 * 保护逻辑根据各种标准挑选固定数量的理想对等节点，然后（主要是）理想或劣势对等节点的比例。
 * 如果还有任何驱逐候选者，选择逻辑会选择一个对等节点进行驱逐。
 *
 * @param vEvictionCandidates 驱逐候选者列表
 * @return 要驱逐的节点ID，如果没有合适的候选者则返回空值
 */
[[nodiscard]] std::optional<NodeId> SelectNodeToEvict(std::vector<NodeEvictionCandidate>&& vEvictionCandidates);

/** Protect desirable or disadvantaged inbound peers from eviction by ratio.
 *
 * 按比例保护理想或劣势的入站对等节点免于驱逐。
 *
 * This function protects half of the peers which have been connected the
 * longest, to replicate the non-eviction implicit behavior and preclude attacks
 * that start later.
 *
 * 此函数保护连接时间最长的一半对等节点，以复制非驱逐的隐式行为并防止稍后开始的攻击。
 *
 * Half of these protected spots (1/4 of the total) are reserved for the
 * following categories of peers, sorted by longest uptime, even if they're not
 * longest uptime overall:
 *
 * 这些受保护位置的一半（总数的1/4）为以下类别的对等节点保留，按最长运行时间排序，
 * 即使它们不是总体最长运行时间：
 *
 * - onion peers connected via our tor control service
 * - 通过我们的tor控制服务连接的洋葱对等节点
 *
 * - localhost peers, as manually configured hidden services not using
 *   `-bind=addr[:port]=onion` will not be detected as inbound onion connections
 * - 本地主机对等节点，因为不使用`-bind=addr[:port]=onion`手动配置的隐藏服务
 *   不会被检测为入站洋葱连接
 *
 * - I2P peers
 * - I2P对等节点
 *
 * - CJDNS peers
 * - CJDNS对等节点
 *
 * This helps protect these privacy network peers, which tend to be otherwise
 * disadvantaged under our eviction criteria for their higher min ping times
 * relative to IPv4/IPv6 peers, and favorise the diversity of peer connections.
 *
 * 这有助于保护这些隐私网络对等节点，它们在我们的驱逐标准下往往处于劣势，
 * 因为相对于IPv4/IPv6对等节点，它们的最小ping时间更高，并促进对等节点连接的多样性。
 *
 * @param vEvictionCandidates 驱逐候选者列表（会被修改）
 */
void ProtectEvictionCandidatesByRatio(std::vector<NodeEvictionCandidate>& vEvictionCandidates);

#endif // BITCOIN_NODE_EVICTION_H
