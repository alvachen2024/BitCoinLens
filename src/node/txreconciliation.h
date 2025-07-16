// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_TXRECONCILIATION_H
#define BITCOIN_NODE_TXRECONCILIATION_H

#include <net.h>
#include <sync.h>

#include <memory>
#include <tuple>

/** Supported transaction reconciliation protocol version
 * 支持的交易对账协议版本 */
static constexpr uint32_t TXRECONCILIATION_VERSION{1};

/**
 * 交易对账注册结果枚举
 * 定义了注册对等节点进行交易对账时的可能结果
 */
enum class ReconciliationRegisterResult {
    NOT_FOUND,           // 未找到
    SUCCESS,             // 成功
    ALREADY_REGISTERED,  // 已经注册
    PROTOCOL_VIOLATION,  // 协议违规
};

/**
 * Transaction reconciliation is a way for nodes to efficiently announce transactions.
 * This object keeps track of all txreconciliation-related communications with the peers.
 *
 * 交易对账是节点高效宣布交易的一种方式。
 * 此对象跟踪与对等节点的所有交易对账相关通信。
 *
 * The high-level protocol is:
 * 高级协议如下：
 *
 * 0.  Txreconciliation protocol handshake.
 * 0.  交易对账协议握手。
 *
 * 1.  Once we receive a new transaction, add it to the set instead of announcing immediately.
 * 1.  一旦我们收到新交易，将其添加到集合中而不是立即宣布。
 *
 * 2.  At regular intervals, a txreconciliation initiator requests a sketch from a peer, where a
 *     sketch is a compressed representation of short form IDs of the transactions in their set.
 * 2.  定期地，交易对账发起者向对等节点请求草图，其中草图是其集合中交易短格式ID的压缩表示。
 *
 * 3.  Once the initiator received a sketch from the peer, the initiator computes a local sketch,
 *     and combines the two sketches to attempt finding the difference in *sets*.
 * 3.  一旦发起者从对等节点收到草图，发起者计算本地草图，并组合两个草图以尝试找到*集合*中的差异。
 *
 * 4a. If the difference was not larger than estimated, see SUCCESS below.
 * 4a. 如果差异不大于估计值，请参阅下面的SUCCESS。
 *
 * 4b. If the difference was larger than estimated, initial txreconciliation fails. The initiator
 *     requests a larger sketch via an extension round (allowed only once).
 *     - If extension succeeds (a larger sketch is sufficient), see SUCCESS below.
 *     - If extension fails (a larger sketch is insufficient), see FAILURE below.
 * 4b. 如果差异大于估计值，初始交易对账失败。发起者通过扩展轮次请求更大的草图（仅允许一次）。
 *     - 如果扩展成功（更大的草图足够），请参阅下面的SUCCESS。
 *     - 如果扩展失败（更大的草图不足），请参阅下面的FAILURE。
 *
 * SUCCESS. The initiator knows full symmetrical difference and can request what the initiator is
 *          missing and announce to the peer what the peer is missing.
 * SUCCESS. 发起者知道完整的对称差异，可以请求发起者缺少的内容，并向对等节点宣布对等节点缺少的内容。
 *
 * FAILURE. The initiator notifies the peer about the failure and announces all transactions from
 *          the corresponding set. Once the peer received the failure notification, the peer
 *          announces all transactions from their set.
 * FAILURE. 发起者通知对等节点失败，并宣布来自相应集合的所有交易。一旦对等节点收到失败通知，
 *          对等节点宣布来自其集合的所有交易。

 * This is a modification of the Erlay protocol (https://arxiv.org/abs/1905.10518) with two
 * changes (sketch extensions instead of bisections, and an extra INV exchange round), both
 * are motivated in BIP-330.
 *
 * 这是Erlay协议（https://arxiv.org/abs/1905.10518）的修改版本，有两个变化
 * （草图扩展而不是二分法，以及额外的INV交换轮次），两者都在BIP-330中有动机。
 */
class TxReconciliationTracker
{
private:
    class Impl;                    // 实现类（PIMPL模式）
    const std::unique_ptr<Impl> m_impl; // 实现指针

public:
    explicit TxReconciliationTracker(uint32_t recon_version);
    ~TxReconciliationTracker();

    /**
     * Step 0. Generates initial part of the state (salt) required to reconcile txs with the peer.
     * The salt is used for short ID computation required for txreconciliation.
     * The function returns the salt.
     * A peer can't participate in future txreconciliations without this call.
     * This function must be called only once per peer.
     *
     * 步骤0. 生成与对等节点对账交易所需状态的初始部分（盐值）。
     * 盐值用于交易对账所需的短ID计算。
     * 函数返回盐值。
     * 没有此调用，对等节点无法参与未来的交易对账。
     * 此函数必须每个对等节点只调用一次。
     */
    uint64_t PreRegisterPeer(NodeId peer_id);

    /**
     * Step 0. Once the peer agreed to reconcile txs with us, generate the state required to track
     * ongoing reconciliations. Must be called only after pre-registering the peer and only once.
     *
     * 步骤0. 一旦对等节点同意与我们对账交易，生成跟踪正在进行的对账所需的状态。
     * 必须在对等节点预注册后且仅调用一次。
     */
    ReconciliationRegisterResult RegisterPeer(NodeId peer_id, bool is_peer_inbound,
                                              uint32_t peer_recon_version, uint64_t remote_salt);

    /**
     * Attempts to forget txreconciliation-related state of the peer (if we previously stored any).
     * After this, we won't be able to reconcile transactions with the peer.
     *
     * 尝试忘记对等节点的交易对账相关状态（如果我们之前存储了任何）。
     * 之后，我们将无法与对等节点对账交易。
     */
    void ForgetPeer(NodeId peer_id);

    /**
     * Check if a peer is registered to reconcile transactions with us.
     *
     * 检查对等节点是否已注册与我们进行交易对账。
     */
    bool IsPeerRegistered(NodeId peer_id) const;
};

#endif // BITCOIN_NODE_TXRECONCILIATION_H
