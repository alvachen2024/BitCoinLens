// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_CHAINSTATE_H
#define BITCOIN_NODE_CHAINSTATE_H

#include <util/translation.h>
#include <validation.h>

#include <cstdint>
#include <functional>
#include <tuple>

class CTxMemPool;

namespace kernel {
struct CacheSizes; // 缓存大小结构体
} // namespace kernel

namespace node {

/**
 * 链状态加载选项结构体
 * 定义了加载链状态时的各种配置选项
 */
struct ChainstateLoadOptions {
    CTxMemPool* mempool{nullptr};        // 交易内存池指针
    bool coins_db_in_memory{false};      // 是否将硬币数据库放在内存中

    // Whether to wipe the chainstate database when loading it. If set, this
    // will cause the chainstate database to be rebuilt starting from genesis.
    // 加载时是否清除链状态数据库。如果设置，这将导致链状态数据库从创世区块开始重建。
    bool wipe_chainstate_db{false};

    bool prune{false};                   // 是否启用修剪模式

    //! Setting require_full_verification to true will require all checks at
    //! check_level (below) to succeed for loading to succeed. Setting it to
    //! false will skip checks if cache is not big enough to run them, so may be
    //! helpful for running with a small cache.
    //!
    //! 将require_full_verification设置为true将要求check_level（下面）的所有检查
    //! 都成功才能使加载成功。将其设置为false将在缓存不够大时跳过检查，
    //! 所以可能有助于在小缓存下运行。
    bool require_full_verification{true};

    int64_t check_blocks{DEFAULT_CHECKBLOCKS};  // 要检查的区块数量
    int64_t check_level{DEFAULT_CHECKLEVEL};    // 检查级别

    std::function<void()> coins_error_cb;       // 硬币错误回调函数
};

//! Chainstate load status. Simple applications can just check for the success
//! case, and treat other cases as errors. More complex applications may want to
//! try reindexing in the generic failure case, and pass an interrupt callback
//! and exit cleanly in the interrupted case.
//!
//! 链状态加载状态。简单应用程序可以只检查成功情况，并将其他情况视为错误。
//! 更复杂的应用程序可能希望在一般失败情况下尝试重新索引，并在中断情况下
//! 传递中断回调并干净地退出。
enum class ChainstateLoadStatus {
    SUCCESS,                    // 成功加载
    FAILURE,                    //!< Generic failure which reindexing may fix - 一般失败，重新索引可能修复
    FAILURE_FATAL,              //!< Fatal error which should not prompt to reindex - 致命错误，不应提示重新索引
    FAILURE_INCOMPATIBLE_DB,    // 数据库不兼容
    FAILURE_INSUFFICIENT_DBCACHE, // 数据库缓存不足
    INTERRUPTED,                // 被中断
};

//! Chainstate load status code and optional error string.
//! 链状态加载状态代码和可选的错误字符串。
using ChainstateLoadResult = std::tuple<ChainstateLoadStatus, bilingual_str>;

/** This sequence can have 4 types of outcomes:
 *
 * 此序列可以有4种结果：
 *
 *  1. Success
 *     1. 成功
 *  2. Shutdown requested
 *    - nothing failed but a shutdown was triggered in the middle of the
 *      sequence
 *     2. 请求关闭
 *     - 没有失败，但在序列中间触发了关闭
 *  3. Soft failure
 *    - a failure that might be recovered from with a reindex
 *     3. 软失败
 *     - 可能通过重新索引恢复的失败
 *  4. Hard failure
 *    - a failure that definitively cannot be recovered from with a reindex
 *     4. 硬失败
 *     - 绝对无法通过重新索引恢复的失败
 *
 *  LoadChainstate returns a (status code, error string) tuple.
 *  LoadChainstate返回一个（状态代码，错误字符串）元组。
 */

/**
 * 加载链状态
 * 主要的链状态加载函数，处理区块链状态的初始化
 *
 * @param chainman 链状态管理器引用
 * @param cache_sizes 缓存大小配置
 * @param options 加载选项
 * @return 加载结果（状态代码和错误信息）
 */
ChainstateLoadResult LoadChainstate(ChainstateManager& chainman, const kernel::CacheSizes& cache_sizes,
                                    const ChainstateLoadOptions& options);

/**
 * 验证已加载的链状态
 * 验证链状态是否正确加载和初始化
 *
 * @param chainman 链状态管理器引用
 * @param options 验证选项
 * @return 验证结果（状态代码和错误信息）
 */
ChainstateLoadResult VerifyLoadedChainstate(ChainstateManager& chainman, const ChainstateLoadOptions& options);

} // namespace node

#endif // BITCOIN_NODE_CHAINSTATE_H
