// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_CACHES_H
#define BITCOIN_NODE_CACHES_H

#include <kernel/caches.h>
#include <util/byte_units.h>

#include <cstddef>

class ArgsManager; // 参数管理器类

//! min. -dbcache (bytes)
//! 最小数据库缓存大小（字节）
static constexpr size_t MIN_DB_CACHE{4_MiB};

//! -dbcache default (bytes)
//! 数据库缓存默认大小（字节）
static constexpr size_t DEFAULT_DB_CACHE{DEFAULT_KERNEL_CACHE};

namespace node {

/**
 * 索引缓存大小结构体
 * 定义各种索引的缓存大小配置
 */
struct IndexCacheSizes {
    size_t tx_index{0};      // 交易索引缓存大小
    size_t filter_index{0};  // 过滤器索引缓存大小
};

/**
 * 缓存大小结构体
 * 包含索引缓存和内核缓存的配置
 */
struct CacheSizes {
    IndexCacheSizes index;   // 索引缓存大小
    kernel::CacheSizes kernel; // 内核缓存大小
};

/**
 * 计算缓存大小
 * 根据命令行参数和索引数量计算各种缓存的大小
 *
 * @param args 参数管理器，包含用户配置的缓存参数
 * @param n_indexes 索引数量，默认为0
 * @return 计算出的缓存大小配置
 */
CacheSizes CalculateCacheSizes(const ArgsManager& args, size_t n_indexes = 0);

} // namespace node

#endif // BITCOIN_NODE_CACHES_H
