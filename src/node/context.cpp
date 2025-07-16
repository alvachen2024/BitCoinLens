// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/context.h>

// 包含各种必要的头文件，提供NodeContext中使用的类的完整定义
#include <addrman.h>              // 地址管理器
#include <banman.h>               // 封禁管理器
#include <interfaces/chain.h>     // 链接口
#include <interfaces/mining.h>    // 挖矿接口
#include <kernel/context.h>       // 内核上下文
#include <key.h>                  // 密钥相关
#include <net.h>                  // 网络基础
#include <net_processing.h>       // 网络处理
#include <netgroup.h>             // 网络组
#include <node/kernel_notifications.h> // 内核通知
#include <node/warnings.h>        // 警告管理
#include <policy/fees.h>          // 费用策略
#include <scheduler.h>            // 调度器
#include <txmempool.h>            // 交易内存池
#include <validation.h>           // 验证
#include <validationinterface.h>  // 验证接口

namespace node {

/**
 * NodeContext默认构造函数
 * 使用编译器生成的默认构造函数，因为所有成员都有默认初始化值
 * 这样可以避免在头文件中包含所有类的定义，提高编译效率
 */
NodeContext::NodeContext() = default;

/**
 * NodeContext默认析构函数
 * 使用编译器生成的默认析构函数，因为所有智能指针成员会自动清理
 * 这确保了资源的正确释放，防止内存泄漏
 */
NodeContext::~NodeContext() = default;

} // namespace node
