// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_CONTEXT_H
#define BITCOIN_NODE_CONTEXT_H

#include <atomic>
#include <cstdlib>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

// 前向声明各种类，避免包含头文件
class ArgsManager;           // 参数管理器
class AddrMan;               // 地址管理器
class BanMan;                // 封禁管理器
class BaseIndex;             // 基础索引
class CBlockPolicyEstimator; // 区块策略估算器
class CConnman;              // 连接管理器
class ValidationSignals;     // 验证信号
class CScheduler;            // 调度器
class CTxMemPool;            // 交易内存池
class ChainstateManager;     // 链状态管理器
class ECC_Context;           // 椭圆曲线上下文
class NetGroupManager;       // 网络组管理器
class PeerManager;           // 对等节点管理器

namespace interfaces {
class Chain;        // 链接口
class ChainClient;  // 链客户端接口
class Mining;       // 挖矿接口
class Init;         // 初始化接口
class WalletLoader; // 钱包加载器接口
} // namespace interfaces

namespace kernel {
struct Context;     // 内核上下文
}

namespace util {
class SignalInterrupt; // 信号中断
}

namespace node {
class KernelNotifications; // 内核通知
class Warnings;            // 警告管理

//! NodeContext struct containing references to chain state and connection
//! state.
//!
//! 包含链状态和连接状态引用的NodeContext结构体。
//!
//! This is used by init, rpc, and test code to pass object references around
//! without needing to declare the same variables and parameters repeatedly, or
//! to use globals. More variables could be added to this struct (particularly
//! references to validation objects) to eliminate use of globals
//! and make code more modular and testable. The struct isn't intended to have
//! any member functions. It should just be a collection of references that can
//! be used without pulling in unwanted dependencies or functionality.
//!
//! 这被init、rpc和测试代码用来传递对象引用，而不需要重复声明相同的变量和参数，
//! 或使用全局变量。可以向此结构体添加更多变量（特别是对验证对象的引用）
//! 以消除全局变量的使用，并使代码更加模块化和可测试。该结构体不打算有任何
//! 成员函数。它应该只是一个引用的集合，可以在不引入不需要的依赖项或功能的情况下使用。
struct NodeContext {
    //! libbitcoin_kernel context
    //! libbitcoin_kernel上下文
    std::unique_ptr<kernel::Context> kernel;

    //! 椭圆曲线加密上下文
    std::unique_ptr<ECC_Context> ecc_context;

    //! Init interface for initializing current process and connecting to other processes.
    //! 用于初始化当前进程并连接到其他进程的Init接口。
    interfaces::Init* init{nullptr};

    //! Function to request a shutdown.
    //! 请求关闭的函数。
    std::function<bool()> shutdown_request;

    //! Interrupt object used to track whether node shutdown was requested.
    //! 用于跟踪是否请求节点关闭的中断对象。
    util::SignalInterrupt* shutdown_signal{nullptr};

    //! 地址管理器 - 管理P2P网络地址
    std::unique_ptr<AddrMan> addrman;

    //! 连接管理器 - 管理网络连接
    std::unique_ptr<CConnman> connman;

    //! 交易内存池 - 存储待确认的交易
    std::unique_ptr<CTxMemPool> mempool;

    //! 网络组管理器 - 管理网络分组
    std::unique_ptr<const NetGroupManager> netgroupman;

    //! 费用估算器 - 估算交易费用
    std::unique_ptr<CBlockPolicyEstimator> fee_estimator;

    //! 对等节点管理器 - 管理P2P网络中的对等节点
    std::unique_ptr<PeerManager> peerman;

    //! 链状态管理器 - 管理区块链状态
    std::unique_ptr<ChainstateManager> chainman;

    //! 封禁管理器 - 管理被封禁的节点
    std::unique_ptr<BanMan> banman;

    //! 参数管理器 - 管理命令行和配置文件参数
    ArgsManager* args{nullptr}; // Currently a raw pointer because the memory is not managed by this struct
    // 目前是原始指针，因为内存不由此结构体管理

    //! 索引列表 - 各种区块链索引
    std::vector<BaseIndex*> indexes; // raw pointers because memory is not managed by this struct
    // 原始指针，因为内存不由此结构体管理

    //! 链接口 - 提供链操作的统一接口
    std::unique_ptr<interfaces::Chain> chain;

    //! List of all chain clients (wallet processes or other client) connected to node.
    //! 连接到节点的所有链客户端（钱包进程或其他客户端）的列表。
    std::vector<std::unique_ptr<interfaces::ChainClient>> chain_clients;

    //! Reference to chain client that should used to load or create wallets
    //! opened by the gui.
    //! 应该用于加载或创建由GUI打开的钱包的链客户端引用。
    std::unique_ptr<interfaces::Mining> mining;

    //! 钱包加载器接口
    interfaces::WalletLoader* wallet_loader{nullptr};

    //! 调度器 - 管理后台任务调度
    std::unique_ptr<CScheduler> scheduler;

    //! RPC中断点函数 - 用于RPC调用的中断检查
    std::function<void()> rpc_interruption_point = [] {};

    //! Issues blocking calls about sync status, errors and warnings
    //! 发出关于同步状态、错误和警告的阻塞调用
    std::unique_ptr<KernelNotifications> notifications;

    //! Issues calls about blocks and transactions
    //! 发出关于区块和交易的调用
    std::unique_ptr<ValidationSignals> validation_signals;

    //! 退出状态 - 原子变量，线程安全
    std::atomic<int> exit_status{EXIT_SUCCESS};

    //! Manages all the node warnings
    //! 管理所有节点警告
    std::unique_ptr<node::Warnings> warnings;

    //! 后台初始化线程
    std::thread background_init_thread;

    //! Declare default constructor and destructor that are not inline, so code
    //! instantiating the NodeContext struct doesn't need to #include class
    //! definitions for all the unique_ptr members.
    //!
    //! 声明非内联的默认构造函数和析构函数，这样实例化NodeContext结构体的代码
    //! 不需要为所有unique_ptr成员包含类定义。
    NodeContext();
    ~NodeContext();
};
} // namespace node

#endif // BITCOIN_NODE_CONTEXT_H
