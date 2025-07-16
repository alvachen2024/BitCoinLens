// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_TRANSACTION_H
#define BITCOIN_NODE_TRANSACTION_H

#include <common/messages.h>
#include <policy/feerate.h>
#include <primitives/transaction.h>

class CBlockIndex;
class CTxMemPool;
namespace Consensus {
struct Params;
}

namespace node {
class BlockManager;
struct NodeContext;

/** Maximum fee rate for sendrawtransaction and testmempoolaccept RPC calls.
 * Also used by the GUI when broadcasting a completed PSBT.
 * By default, a transaction with a fee rate higher than this will be rejected
 * by these RPCs and the GUI. This can be overridden with the maxfeerate argument.
 *
 * sendrawtransaction和testmempoolaccept RPC调用的最大费用率。
 * 在广播已完成的PSBT时也由GUI使用。
 * 默认情况下，费用率高于此的交易将被这些RPC和GUI拒绝。
 * 这可以通过maxfeerate参数覆盖。
 */
static const CFeeRate DEFAULT_MAX_RAW_TX_FEE_RATE{COIN / 10}; // 默认最大原始交易费用率：0.1 BTC

/** Maximum burn value for sendrawtransaction, submitpackage, and testmempoolaccept RPC calls.
 * By default, a transaction with a burn value higher than this will be rejected
 * by these RPCs and the GUI. This can be overridden with the maxburnamount argument.
 *
 * sendrawtransaction、submitpackage和testmempoolaccept RPC调用的最大销毁值。
 * 默认情况下，销毁值高于此的交易将被这些RPC和GUI拒绝。
 * 这可以通过maxburnamount参数覆盖。
 */
static const CAmount DEFAULT_MAX_BURN_AMOUNT{0}; // 默认最大销毁金额：0（不允许销毁）

/**
 * Submit a transaction to the mempool and (optionally) relay it to all P2P peers.
 *
 * 将交易提交到内存池并（可选地）中继到所有P2P对等节点。
 *
 * Mempool submission can be synchronous (will await mempool entry notification
 * over the CValidationInterface) or asynchronous (will submit and not wait for
 * notification), depending on the value of wait_callback. wait_callback MUST
 * NOT be set while cs_main, cs_mempool or cs_wallet are held to avoid
 * deadlock.
 *
 * 内存池提交可以是同步的（将通过CValidationInterface等待内存池条目通知）
 * 或异步的（将提交但不等待通知），具体取决于wait_callback的值。
 * 在持有cs_main、cs_mempool或cs_wallet时绝不能设置wait_callback以避免死锁。
 *
 * @param[in]  node reference to node context - 节点上下文引用
 * @param[in]  tx the transaction to broadcast - 要广播的交易
 * @param[out] err_string reference to std::string to fill with error string if available - 如果可用，填充错误字符串的std::string引用
 * @param[in]  max_tx_fee reject txs with fees higher than this (if 0, accept any fee) - 拒绝费用高于此的交易（如果为0，接受任何费用）
 * @param[in]  relay flag if both mempool insertion and p2p relay are requested - 如果请求内存池插入和p2p中继，则为标志
 * @param[in]  wait_callback wait until callbacks have been processed to avoid stale result due to a sequentially RPC - 等待回调处理完成，避免由于顺序RPC导致的过时结果
 * return error - 返回错误
 */
[[nodiscard]] TransactionError BroadcastTransaction(NodeContext& node, CTransactionRef tx, std::string& err_string, const CAmount& max_tx_fee, bool relay, bool wait_callback);

/**
 * Return transaction with a given hash.
 * If mempool is provided and block_index is not provided, check it first for the tx.
 * If -txindex is available, check it next for the tx.
 * Finally, if block_index is provided, check for tx by reading entire block from disk.
 *
 * 返回具有给定哈希的交易。
 * 如果提供了mempool且未提供block_index，首先在其中检查交易。
 * 如果-txindex可用，接下来在其中检查交易。
 * 最后，如果提供了block_index，通过从磁盘读取整个区块来检查交易。
 *
 * @param[in]  block_index     The block to read from disk, or nullptr - 要从磁盘读取的区块，或nullptr
 * @param[in]  mempool         If provided, check mempool for tx - 如果提供，在内存池中检查交易
 * @param[in]  hash            The txid - 交易ID
 * @param[out] hashBlock       The block hash, if the tx was found via -txindex or block_index - 如果通过-txindex或block_index找到交易，则为区块哈希
 * @returns                    The tx if found, otherwise nullptr - 如果找到则返回交易，否则返回nullptr
 */
CTransactionRef GetTransaction(const CBlockIndex* const block_index, const CTxMemPool* const mempool, const uint256& hash, uint256& hashBlock, const BlockManager& blockman);

} // namespace node

#endif // BITCOIN_NODE_TRANSACTION_H
