// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_COIN_H
#define BITCOIN_NODE_COIN_H

#include <map>

class COutPoint; // 输出点类
class Coin;      // 硬币类

namespace node {
struct NodeContext; // 节点上下文结构体

/**
 * Look up unspent output information. Returns coins in the mempool and in the
 * current chain UTXO set. Iterates through all the keys in the map and
 * populates the values.
 *
 * 查找未花费输出信息。返回内存池和当前链UTXO集合中的硬币。
 * 遍历映射中的所有键并填充值。
 *
 * @param[in] node The node context to use for lookup - 用于查找的节点上下文
 * @param[in,out] coins map to fill - 要填充的硬币映射
 */
void FindCoins(const node::NodeContext& node, std::map<COutPoint, Coin>& coins);

} // namespace node

#endif // BITCOIN_NODE_COIN_H
