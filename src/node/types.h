// Copyright (c) 2010-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//! @file node/types.h is a home for public enum and struct type definitions
//! that are used internally by node code, but also used externally by wallet,
//! mining or GUI code.
//!
//! 该文件是node代码内部使用，同时也被钱包、挖矿或GUI代码外部使用的
//! 公共枚举和结构体类型定义的家园。
//!
//! This file is intended to define only simple types that do not have external
//! dependencies. More complicated types should be defined in dedicated header
//! files.
//!
//! 该文件旨在只定义没有外部依赖的简单类型。更复杂的类型应该在专门的
//! 头文件中定义。

#ifndef BITCOIN_NODE_TYPES_H
#define BITCOIN_NODE_TYPES_H

#include <consensus/amount.h>
#include <cstddef>
#include <policy/policy.h>
#include <script/script.h>
#include <uint256.h>
#include <util/time.h>

namespace node {

/**
 * 交易错误枚举类
 * 定义了交易处理过程中可能出现的各种错误类型
 */
enum class TransactionError {
    OK, //!< No error - 无错误
    MISSING_INPUTS,        //!< 缺少输入 - 交易缺少必要的输入
    ALREADY_IN_UTXO_SET,   //!< 已在UTXO集合中 - 交易输出已经存在于UTXO集合中
    MEMPOOL_REJECTED,      //!< 内存池拒绝 - 交易被内存池拒绝
    MEMPOOL_ERROR,         //!< 内存池错误 - 内存池处理过程中出现错误
    MAX_FEE_EXCEEDED,      //!< 超过最大费用 - 交易费用超过了允许的最大值
    MAX_BURN_EXCEEDED,     //!< 超过最大销毁量 - 交易销毁的比特币超过了允许的最大值
    INVALID_PACKAGE,       //!< 无效包 - 交易包无效
};

/**
 * 区块创建选项结构体
 * 定义了创建新区块时的各种配置选项
 */
struct BlockCreateOptions {
    /**
     * Set false to omit mempool transactions
     * 设置为false以省略内存池交易
     */
    bool use_mempool{true};

    /**
     * The default reserved weight for the fixed-size block header,
     * transaction count and coinbase transaction.
     * 固定大小区块头部、交易数量和创币交易的默认保留权重
     */
    size_t block_reserved_weight{DEFAULT_BLOCK_RESERVED_WEIGHT};

    /**
     * The maximum additional sigops which the pool will add in coinbase
     * transaction outputs.
     * 池将在创币交易输出中添加的最大额外签名操作数
     */
    size_t coinbase_output_max_additional_sigops{400};

    /**
     * Script to put in the coinbase transaction. The default is an
     * anyone-can-spend dummy.
     * 放入创币交易的脚本。默认是一个任何人都可以花费的虚拟脚本。
     *
     * Should only be used for tests, when the default doesn't suffice.
     * 应该只在测试中使用，当默认值不够用时。
     *
     * Note that higher level code like the getblocktemplate RPC may omit the
     * coinbase transaction entirely. It's instead constructed by pool software
     * using fields like coinbasevalue, coinbaseaux and default_witness_commitment.
     * This software typically also controls the payout outputs, even for solo
     * mining.
     * 注意，像getblocktemplate RPC这样的高级代码可能会完全省略创币交易。
     * 相反，它由池软件使用coinbasevalue、coinbaseaux和default_witness_commitment
     * 等字段构建。该软件通常也控制支付输出，即使是单独挖矿。
     *
     * The size and sigops are not checked against
     * coinbase_max_additional_weight and coinbase_output_max_additional_sigops.
     * 大小和签名操作不会根据coinbase_max_additional_weight和
     * coinbase_output_max_additional_sigops进行检查。
     */
    CScript coinbase_output_script{CScript() << OP_TRUE};
};

/**
 * 区块等待选项结构体
 * 定义了等待新区块模板时的配置选项
 */
struct BlockWaitOptions {
    /**
     * How long to wait before returning nullptr instead of a new template.
     * Default is to wait forever.
     * 在返回nullptr而不是新模板之前等待多长时间。
     * 默认是永远等待。
     */
    MillisecondsDouble timeout{MillisecondsDouble::max()};

    /**
     * The wait method will not return a new template unless it has fees at
     * least fee_threshold sats higher than the current template, or unless
     * the chain tip changes and the previous template is no longer valid.
     * 等待方法不会返回新模板，除非它的费用至少比当前模板高fee_threshold聪，
     * 或者链尖发生变化且之前的模板不再有效。
     *
     * A caller may not be interested in templates with higher fees, and
     * determining whether fee_threshold is reached is also expensive. So as
     * an optimization, when fee_threshold is set to MAX_MONEY (default), the
     * implementation is able to be much more efficient, skipping expensive
     * checks and only returning new templates when the chain tip changes.
     * 调用者可能对费用更高的模板不感兴趣，确定是否达到fee_threshold也很昂贵。
     * 因此作为优化，当fee_threshold设置为MAX_MONEY（默认）时，实现可以更高效，
     * 跳过昂贵的检查，只在链尖变化时返回新模板。
     */
    CAmount fee_threshold{MAX_MONEY};
};

/**
 * 区块检查选项结构体
 * 定义了检查区块时的配置选项
 */
struct BlockCheckOptions {
    /**
     * Set false to omit the merkle root check
     * 设置为false以省略默克尔根检查
     */
    bool check_merkle_root{true};

    /**
     * Set false to omit the proof-of-work check
     * 设置为false以省略工作量证明检查
     */
    bool check_pow{true};
};
} // namespace node

#endif // BITCOIN_NODE_TYPES_H
