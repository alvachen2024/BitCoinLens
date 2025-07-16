# 交易处理专题教程 - 基于Bitcoin Core实践

## 1. 交易基础概念

### 1.1 交易结构

#### Bitcoin Core中的交易定义
```cpp
// 来自 primitives/transaction.h
class CTransaction {
public:
    // 交易版本
    int32_t nVersion;

    // 输入列表
    std::vector<CTxIn> vin;

    // 输出列表
    std::vector<CTxOut> vout;

    // 锁定时间
    uint32_t nLockTime;

    // 获取交易哈希
    Txid GetHash() const;

    // 获取见证哈希
    Wtxid GetWitnessHash() const;
};
```

#### 实践实现
```cpp
#include <vector>
#include <cstdint>
#include <string>

// 简化的交易输入结构
struct TransactionInput {
    uint256 previousTxHash;  // 前一个交易的哈希
    uint32_t outputIndex;    // 输出索引
    std::vector<uint8_t> scriptSig;  // 解锁脚本
    uint32_t sequence;       // 序列号
};

// 简化的交易输出结构
struct TransactionOutput {
    int64_t value;           // 输出金额（聪）
    std::vector<uint8_t> scriptPubKey;  // 锁定脚本
};

// 简化的交易结构
class SimpleTransaction {
public:
    int32_t version = 1;
    std::vector<TransactionInput> inputs;
    std::vector<TransactionOutput> outputs;
    uint32_t lockTime = 0;

    // 计算交易哈希
    uint256 getHash() const {
        // 简化的哈希计算
        std::string data = std::to_string(version);
        for (const auto& input : inputs) {
            data += input.previousTxHash.ToString();
            data += std::to_string(input.outputIndex);
        }
        for (const auto& output : outputs) {
            data += std::to_string(output.value);
        }
        data += std::to_string(lockTime);

        // 使用简单的哈希函数（实际应使用SHA256）
        return uint256::fromString(data);
    }

    // 计算交易大小
    size_t getSize() const {
        size_t size = 4; // 版本号
        size += 1; // 输入数量
        for (const auto& input : inputs) {
            size += 32 + 4 + input.scriptSig.size() + 4; // 哈希 + 索引 + 脚本 + 序列号
        }
        size += 1; // 输出数量
        for (const auto& output : outputs) {
            size += 8 + output.scriptPubKey.size(); // 金额 + 脚本
        }
        size += 4; // 锁定时间
        return size;
    }

    // 计算交易费用
    int64_t calculateFee(const std::vector<int64_t>& inputValues) const {
        int64_t inputTotal = 0;
        for (int64_t value : inputValues) {
            inputTotal += value;
        }

        int64_t outputTotal = 0;
        for (const auto& output : outputs) {
            outputTotal += output.value;
        }

        return inputTotal - outputTotal;
    }
};
```

### 1.2 交易验证

#### Bitcoin Core中的验证逻辑
```cpp
// 来自 transaction.cpp - 交易验证
TransactionError BroadcastTransaction(NodeContext& node, const CTransactionRef tx,
                                     std::string& err_string, const CAmount& max_tx_fee,
                                     bool relay, bool wait_callback) {
    // 检查交易是否已在链中确认
    CCoinsViewCache &view = node.chainman->ActiveChainstate().CoinsTip();
    for (size_t o = 0; o < tx->vout.size(); o++) {
        const Coin& existingCoin = view.AccessCoin(COutPoint(txid, o));
        if (!existingCoin.IsSpent()) {
            return TransactionError::ALREADY_IN_UTXO_SET;
        }
    }

    // 检查内存池中是否已存在
    if (auto mempool_tx = node.mempool->get(txid); mempool_tx) {
        // 交易已存在，只进行中继
    } else {
        // 验证交易并添加到内存池
        const MempoolAcceptResult result = node.chainman->ProcessTransaction(tx, false);
        if (result.m_result_type != MempoolAcceptResult::ResultType::VALID) {
            return HandleATMPError(result.m_state, err_string);
        }
    }
}
```

#### 实践实现
```cpp
#include <unordered_set>
#include <stdexcept>

// 交易验证器
class TransactionValidator {
public:
    // 验证交易基本结构
    bool validateBasicStructure(const SimpleTransaction& tx) {
        // 检查版本
        if (tx.version < 1) {
            return false;
        }

        // 检查输入输出数量
        if (tx.inputs.empty() || tx.outputs.empty()) {
            return false;
        }

        // 检查输出金额
        for (const auto& output : tx.outputs) {
            if (output.value <= 0) {
                return false;
            }
        }

        return true;
    }

    // 验证UTXO存在性
    bool validateUTXOs(const SimpleTransaction& tx,
                      const std::unordered_set<uint256>& availableUTXOs) {
        for (const auto& input : tx.inputs) {
            if (availableUTXOs.find(input.previousTxHash) == availableUTXOs.end()) {
                return false;
            }
        }
        return true;
    }

    // 验证交易费用
    bool validateFee(const SimpleTransaction& tx,
                    const std::vector<int64_t>& inputValues,
                    int64_t minFee, int64_t maxFee) {
        int64_t fee = tx.calculateFee(inputValues);
        return fee >= minFee && fee <= maxFee;
    }

    // 验证交易大小
    bool validateSize(const SimpleTransaction& tx, size_t maxSize) {
        return tx.getSize() <= maxSize;
    }

    // 综合验证
    bool validateTransaction(const SimpleTransaction& tx,
                           const std::unordered_set<uint256>& availableUTXOs,
                           const std::vector<int64_t>& inputValues,
                           int64_t minFee, int64_t maxFee,
                           size_t maxSize) {
        return validateBasicStructure(tx) &&
               validateUTXOs(tx, availableUTXOs) &&
               validateFee(tx, inputValues, minFee, maxFee) &&
               validateSize(tx, maxSize);
    }
};
```

## 2. 内存池管理

### 2.1 内存池概念

#### Bitcoin Core中的内存池
```cpp
// 来自 txmempool.h
class CTxMemPool {
public:
    // 添加交易到内存池
    bool addUnchecked(const CTxMemPoolEntry& entry);

    // 从内存池移除交易
    void removeRecursive(const CTransaction& tx, MemPoolRemovalReason reason);

    // 获取内存池中的交易
    CTransactionRef get(const Txid& txid) const;

    // 获取内存池大小
    size_t size() const;

    // 获取内存池费用
    CAmount GetTotalFee() const;
};
```

#### 实践实现
```cpp
#include <map>
#include <set>
#include <memory>
#include <mutex>

// 内存池条目
struct MempoolEntry {
    SimpleTransaction tx;
    int64_t fee;
    int64_t feeRate;  // 费用率（聪/字节）
    int64_t time;     // 进入时间

    MempoolEntry(const SimpleTransaction& transaction, int64_t transactionFee, int64_t currentTime)
        : tx(transaction), fee(transactionFee), time(currentTime) {
        feeRate = (fee * 1000) / tx.getSize(); // 聪/千字节
    }

    bool operator<(const MempoolEntry& other) const {
        // 按费用率排序，高费用率优先
        return feeRate > other.feeRate;
    }
};

// 简化的内存池实现
class SimpleMempool {
public:
    // 添加交易到内存池
    bool addTransaction(const SimpleTransaction& tx, int64_t fee) {
        std::lock_guard<std::mutex> lock(m_mutex);

        // 检查是否已存在
        uint256 txHash = tx.getHash();
        if (m_transactions.find(txHash) != m_transactions.end()) {
            return false;
        }

        // 创建内存池条目
        auto entry = std::make_shared<MempoolEntry>(tx, fee, getCurrentTime());

        // 添加到内存池
        m_transactions[txHash] = entry;
        m_feeRateIndex.insert(entry);

        return true;
    }

    // 从内存池移除交易
    bool removeTransaction(const uint256& txHash) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_transactions.find(txHash);
        if (it == m_transactions.end()) {
            return false;
        }

        m_feeRateIndex.erase(it->second);
        m_transactions.erase(it);

        return true;
    }

    // 获取内存池中的交易
    std::shared_ptr<MempoolEntry> getTransaction(const uint256& txHash) const {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_transactions.find(txHash);
        if (it != m_transactions.end()) {
            return it->second;
        }
        return nullptr;
    }

    // 获取内存池大小
    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_transactions.size();
    }

    // 获取总费用
    int64_t getTotalFee() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        int64_t total = 0;
        for (const auto& pair : m_transactions) {
            total += pair.second->fee;
        }
        return total;
    }

    // 获取按费用率排序的交易
    std::vector<std::shared_ptr<MempoolEntry>> getTransactionsByFeeRate() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return std::vector<std::shared_ptr<MempoolEntry>>(
            m_feeRateIndex.begin(), m_feeRateIndex.end());
    }

    // 清理过期交易
    void cleanupExpired(int64_t maxAge) {
        std::lock_guard<std::mutex> lock(m_mutex);
        int64_t currentTime = getCurrentTime();

        std::vector<uint256> toRemove;
        for (const auto& pair : m_transactions) {
            if (currentTime - pair.second->time > maxAge) {
                toRemove.push_back(pair.first);
            }
        }

        for (const auto& txHash : toRemove) {
            removeTransaction(txHash);
        }
    }

private:
    mutable std::mutex m_mutex;
    std::map<uint256, std::shared_ptr<MempoolEntry>> m_transactions;
    std::set<std::shared_ptr<MempoolEntry>> m_feeRateIndex; // 按费用率排序

    int64_t getCurrentTime() const {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};
```

### 2.2 费用计算

#### 实践实现
```cpp
#include <algorithm>

// 费用计算器
class FeeCalculator {
public:
    // 计算推荐费用率
    int64_t calculateRecommendedFeeRate(const SimpleMempool& mempool,
                                       size_t targetBlocks = 1) {
        auto transactions = mempool.getTransactionsByFeeRate();

        if (transactions.empty()) {
            return DEFAULT_FEE_RATE;
        }

        // 计算目标区块大小（假设1MB）
        size_t targetSize = targetBlocks * 1000000;
        size_t currentSize = 0;

        for (const auto& entry : transactions) {
            currentSize += entry->tx.getSize();
            if (currentSize >= targetSize) {
                return entry->feeRate;
            }
        }

        return transactions.back()->feeRate;
    }

    // 计算交易费用
    int64_t calculateFee(const SimpleTransaction& tx, int64_t feeRate) {
        size_t size = tx.getSize();
        return (feeRate * size) / 1000; // 转换为聪
    }

    // 验证费用是否足够
    bool isFeeSufficient(const SimpleTransaction& tx, int64_t fee, int64_t minFeeRate) {
        int64_t actualFeeRate = (fee * 1000) / tx.getSize();
        return actualFeeRate >= minFeeRate;
    }

    // 计算费用优先级
    double calculatePriority(const SimpleTransaction& tx,
                           const std::vector<int64_t>& inputValues,
                           int64_t fee) {
        int64_t inputTotal = 0;
        for (int64_t value : inputValues) {
            inputTotal += value;
        }

        // 优先级 = (输入金额 * 输入年龄) / 交易大小
        // 这里简化处理，假设所有输入的年龄都是1
        return static_cast<double>(inputTotal) / tx.getSize();
    }

private:
    static const int64_t DEFAULT_FEE_RATE = 1000; // 1聪/字节
};
```

## 3. PSBT (Partially Signed Bitcoin Transaction)

### 3.1 PSBT概念

#### Bitcoin Core中的PSBT
```cpp
// 来自 psbt.h
struct PSBTInputAnalysis {
    bool has_utxo;           // 是否有UTXO信息
    bool is_final;           // 是否已完成签名
    PSBTRole next;           // 下一个处理角色

    std::vector<CKeyID> missing_pubkeys;    // 缺少的公钥
    std::vector<CKeyID> missing_sigs;       // 缺少的签名
    uint160 missing_redeem_script;          // 缺少的赎回脚本
    uint256 missing_witness_script;         // 缺少的见证脚本
};

struct PSBTAnalysis {
    std::optional<size_t> estimated_vsize;      // 估计交易大小
    std::optional<CFeeRate> estimated_feerate;  // 估计费用率
    std::optional<CAmount> fee;                 // 费用金额
    std::vector<PSBTInputAnalysis> inputs;      // 输入分析
    PSBTRole next;                              // 下一个角色
    std::string error;                          // 错误信息
};
```

#### 实践实现
```cpp
#include <optional>
#include <variant>

// PSBT角色枚举
enum class PSBTRole {
    CREATOR,    // 创建者
    UPDATER,    // 更新者
    SIGNER,     // 签名者
    FINALIZER,  // 完成者
    EXTRACTOR   // 提取者
};

// PSBT输入
struct PSBTInput {
    std::optional<SimpleTransaction> non_witness_utxo;  // 非见证UTXO
    std::optional<TransactionOutput> witness_utxo;      // 见证UTXO
    std::vector<uint8_t> partial_sigs;                  // 部分签名
    std::vector<uint8_t> sighash_type;                  // 签名哈希类型
    std::vector<uint8_t> redeem_script;                 // 赎回脚本
    std::vector<uint8_t> witness_script;                // 见证脚本
    std::vector<uint8_t> final_script_sig;              // 最终脚本签名
    std::vector<uint8_t> final_script_witness;          // 最终脚本见证
};

// PSBT输出
struct PSBTOutput {
    std::vector<uint8_t> redeem_script;                 // 赎回脚本
    std::vector<uint8_t> witness_script;                // 见证脚本
    std::vector<uint8_t> bip32_derivation;              // BIP32派生路径
};

// PSBT结构
class PSBT {
public:
    SimpleTransaction tx;                    // 交易
    std::vector<PSBTInput> inputs;          // 输入
    std::vector<PSBTOutput> outputs;        // 输出
    std::vector<uint8_t> unknown;           // 未知字段

    PSBT() {
        inputs.resize(tx.inputs.size());
        outputs.resize(tx.outputs.size());
    }

    // 分析PSBT
    PSBTAnalysis analyze() const {
        PSBTAnalysis analysis;

        // 分析每个输入
        analysis.inputs.resize(inputs.size());
        for (size_t i = 0; i < inputs.size(); ++i) {
            analysis.inputs[i] = analyzeInput(i);
        }

        // 计算费用
        if (canCalculateFee()) {
            analysis.fee = calculateFee();
            analysis.estimated_vsize = estimateSize();
            if (analysis.estimated_vsize) {
                analysis.estimated_feerate = CFeeRate(*analysis.fee, *analysis.estimated_vsize);
            }
        }

        // 确定下一个角色
        analysis.next = determineNextRole(analysis.inputs);

        return analysis;
    }

private:
    PSBTInputAnalysis analyzeInput(size_t index) const {
        PSBTInputAnalysis input_analysis;

        // 检查是否有UTXO
        input_analysis.has_utxo = inputs[index].non_witness_utxo.has_value() ||
                                 inputs[index].witness_utxo.has_value();

        // 检查是否已完成
        input_analysis.is_final = !inputs[index].final_script_sig.empty() ||
                                 !inputs[index].final_script_witness.empty();

        // 确定下一个角色
        if (!input_analysis.has_utxo) {
            input_analysis.next = PSBTRole::UPDATER;
        } else if (!input_analysis.is_final) {
            if (inputs[index].partial_sigs.empty()) {
                input_analysis.next = PSBTRole::SIGNER;
            } else {
                input_analysis.next = PSBTRole::FINALIZER;
            }
        } else {
            input_analysis.next = PSBTRole::EXTRACTOR;
        }

        return input_analysis;
    }

    bool canCalculateFee() const {
        for (const auto& input : inputs) {
            if (!input.non_witness_utxo && !input.witness_utxo) {
                return false;
            }
        }
        return true;
    }

    int64_t calculateFee() const {
        int64_t inputTotal = 0;
        int64_t outputTotal = 0;

        // 计算输入总额
        for (size_t i = 0; i < inputs.size(); ++i) {
            if (inputs[i].non_witness_utxo) {
                inputTotal += inputs[i].non_witness_utxo->outputs[tx.inputs[i].outputIndex].value;
            } else if (inputs[i].witness_utxo) {
                inputTotal += inputs[i].witness_utxo->value;
            }
        }

        // 计算输出总额
        for (const auto& output : tx.outputs) {
            outputTotal += output.value;
        }

        return inputTotal - outputTotal;
    }

    std::optional<size_t> estimateSize() const {
        // 简化的大小估算
        size_t size = tx.getSize();

        // 添加见证数据大小
        for (const auto& input : inputs) {
            if (!input.final_script_witness.empty()) {
                size += input.final_script_witness.size();
            }
        }

        return size;
    }

    PSBTRole determineNextRole(const std::vector<PSBTInputAnalysis>& input_analyses) const {
        PSBTRole next = PSBTRole::EXTRACTOR;
        for (const auto& analysis : input_analyses) {
            if (static_cast<int>(analysis.next) < static_cast<int>(next)) {
                next = analysis.next;
            }
        }
        return next;
    }
};
```

### 3.2 PSBT处理流程

#### 实践实现
```cpp
// PSBT处理器
class PSBTProcessor {
public:
    // 创建PSBT
    PSBT createPSBT(const SimpleTransaction& tx) {
        PSBT psbt;
        psbt.tx = tx;
        return psbt;
    }

    // 更新PSBT（添加UTXO信息）
    bool updatePSBT(PSBT& psbt, size_t inputIndex, const TransactionOutput& utxo) {
        if (inputIndex >= psbt.inputs.size()) {
            return false;
        }

        psbt.inputs[inputIndex].witness_utxo = utxo;
        return true;
    }

    // 签名PSBT
    bool signPSBT(PSBT& psbt, size_t inputIndex, const std::vector<uint8_t>& signature) {
        if (inputIndex >= psbt.inputs.size()) {
            return false;
        }

        psbt.inputs[inputIndex].partial_sigs.insert(
            psbt.inputs[inputIndex].partial_sigs.end(),
            signature.begin(), signature.end());

        return true;
    }

    // 完成PSBT
    bool finalizePSBT(PSBT& psbt) {
        for (size_t i = 0; i < psbt.inputs.size(); ++i) {
            if (!finalizeInput(psbt, i)) {
                return false;
            }
        }
        return true;
    }

    // 提取最终交易
    std::optional<SimpleTransaction> extractTransaction(const PSBT& psbt) {
        if (!isComplete(psbt)) {
            return std::nullopt;
        }

        SimpleTransaction finalTx = psbt.tx;

        // 添加最终脚本
        for (size_t i = 0; i < finalTx.inputs.size(); ++i) {
            if (!psbt.inputs[i].final_script_sig.empty()) {
                finalTx.inputs[i].scriptSig = psbt.inputs[i].final_script_sig;
            }
        }

        return finalTx;
    }

private:
    bool finalizeInput(PSBT& psbt, size_t inputIndex) {
        auto& input = psbt.inputs[inputIndex];

        // 检查是否有足够的签名
        if (input.partial_sigs.empty()) {
            return false;
        }

        // 创建最终脚本签名
        input.final_script_sig = input.partial_sigs;

        return true;
    }

    bool isComplete(const PSBT& psbt) {
        for (const auto& input : psbt.inputs) {
            if (input.final_script_sig.empty() && input.final_script_witness.empty()) {
                return false;
            }
        }
        return true;
    }
};
```

## 4. 交易广播和网络传播

### 4.1 交易广播机制

#### Bitcoin Core中的广播逻辑
```cpp
// 来自 transaction.cpp
if (relay) {
    // 添加到未广播交易列表
    node.mempool->AddUnbroadcastTx(txid);

    // 中继到P2P网络
    node.peerman->RelayTransaction(txid, wtxid);
}
```

#### 实践实现
```cpp
#include <queue>
#include <unordered_set>

// 交易广播器
class TransactionBroadcaster {
public:
    // 添加交易到广播队列
    void addToBroadcastQueue(const uint256& txHash) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_broadcastQueue.push(txHash);
        m_pendingBroadcasts.insert(txHash);
    }

    // 广播交易
    void broadcastTransaction(const SimpleTransaction& tx) {
        uint256 txHash = tx.getHash();

        // 添加到广播队列
        addToBroadcastQueue(txHash);

        // 模拟网络传播
        std::thread([this, txHash]() {
            simulateNetworkPropagation(txHash);
        }).detach();
    }

    // 检查交易是否已广播
    bool isBroadcasted(const uint256& txHash) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_broadcastedTransactions.find(txHash) != m_broadcastedTransactions.end();
    }

    // 获取广播统计
    struct BroadcastStats {
        size_t pendingCount;
        size_t broadcastedCount;
        size_t failedCount;
    };

    BroadcastStats getStats() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return {
            m_pendingBroadcasts.size(),
            m_broadcastedTransactions.size(),
            m_failedBroadcasts.size()
        };
    }

private:
    mutable std::mutex m_mutex;
    std::queue<uint256> m_broadcastQueue;
    std::unordered_set<uint256> m_pendingBroadcasts;
    std::unordered_set<uint256> m_broadcastedTransactions;
    std::unordered_set<uint256> m_failedBroadcasts;

    void simulateNetworkPropagation(const uint256& txHash) {
        // 模拟网络延迟
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::lock_guard<std::mutex> lock(m_mutex);

        // 模拟90%的成功率
        if (rand() % 100 < 90) {
            m_broadcastedTransactions.insert(txHash);
        } else {
            m_failedBroadcasts.insert(txHash);
        }

        m_pendingBroadcasts.erase(txHash);
    }
};
```

### 4.2 交易确认监控

#### 实践实现
```cpp
#include <functional>
#include <map>

// 交易确认监控器
class TransactionConfirmationMonitor {
public:
    using ConfirmationCallback = std::function<void(const uint256&, int)>;

    // 注册确认回调
    void registerConfirmationCallback(const uint256& txHash,
                                    ConfirmationCallback callback) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_callbacks[txHash] = callback;
    }

    // 更新确认状态
    void updateConfirmationStatus(const uint256& txHash, int confirmations) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_callbacks.find(txHash);
        if (it != m_callbacks.end()) {
            // 调用回调函数
            it->second(txHash, confirmations);

            // 如果确认数足够，移除回调
            if (confirmations >= 6) {
                m_callbacks.erase(it);
            }
        }
    }

    // 获取待确认交易数量
    size_t getPendingCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_callbacks.size();
    }

private:
    mutable std::mutex m_mutex;
    std::map<uint256, ConfirmationCallback> m_callbacks;
};
```

## 5. 实践项目

### 项目1: 完整的交易处理系统

```cpp
#include <iostream>
#include <memory>

// 完整的交易处理系统
class TransactionProcessingSystem {
public:
    TransactionProcessingSystem()
        : mempool(std::make_unique<SimpleMempool>()),
          validator(std::make_unique<TransactionValidator>()),
          feeCalculator(std::make_unique<FeeCalculator>()),
          broadcaster(std::make_unique<TransactionBroadcaster>()),
          monitor(std::make_unique<TransactionConfirmationMonitor>()) {}

    // 处理新交易
    bool processTransaction(const SimpleTransaction& tx,
                          const std::vector<int64_t>& inputValues) {
        // 1. 验证交易
        if (!validator->validateTransaction(tx, m_availableUTXOs, inputValues,
                                          MIN_FEE, MAX_FEE, MAX_TX_SIZE)) {
            std::cout << "Transaction validation failed" << std::endl;
            return false;
        }

        // 2. 计算费用
        int64_t fee = tx.calculateFee(inputValues);
        int64_t recommendedFeeRate = feeCalculator->calculateRecommendedFeeRate(*mempool);

        if (!feeCalculator->isFeeSufficient(tx, fee, recommendedFeeRate)) {
            std::cout << "Insufficient fee" << std::endl;
            return false;
        }

        // 3. 添加到内存池
        if (!mempool->addTransaction(tx, fee)) {
            std::cout << "Failed to add transaction to mempool" << std::endl;
            return false;
        }

        // 4. 广播交易
        broadcaster->broadcastTransaction(tx);

        // 5. 注册确认监控
        uint256 txHash = tx.getHash();
        monitor->registerConfirmationCallback(txHash,
            [this](const uint256& hash, int confirmations) {
                onTransactionConfirmed(hash, confirmations);
            });

        std::cout << "Transaction processed successfully" << std::endl;
        return true;
    }

    // 获取内存池统计
    void printMempoolStats() const {
        auto stats = broadcaster->getStats();
        std::cout << "Mempool size: " << mempool->size() << std::endl;
        std::cout << "Total fee: " << mempool->getTotalFee() << " sats" << std::endl;
        std::cout << "Pending broadcasts: " << stats.pendingCount << std::endl;
        std::cout << "Broadcasted: " << stats.broadcastedCount << std::endl;
        std::cout << "Failed: " << stats.failedCount << std::endl;
    }

private:
    std::unique_ptr<SimpleMempool> mempool;
    std::unique_ptr<TransactionValidator> validator;
    std::unique_ptr<FeeCalculator> feeCalculator;
    std::unique_ptr<TransactionBroadcaster> broadcaster;
    std::unique_ptr<TransactionConfirmationMonitor> monitor;

    std::unordered_set<uint256> m_availableUTXOs;

    static const int64_t MIN_FEE = 1000;      // 最小费用（聪）
    static const int64_t MAX_FEE = 1000000;   // 最大费用（聪）
    static const size_t MAX_TX_SIZE = 1000000; // 最大交易大小（字节）

    void onTransactionConfirmed(const uint256& txHash, int confirmations) {
        std::cout << "Transaction " << txHash.ToString()
                  << " confirmed with " << confirmations << " confirmations" << std::endl;
    }
};

// 使用示例
void transaction_processing_demo() {
    TransactionProcessingSystem system;

    // 创建测试交易
    SimpleTransaction tx;
    tx.inputs.push_back({uint256::fromString("123"), 0, {}, 0xffffffff});
    tx.outputs.push_back({50000, {0x76, 0xa9, 0x14, 0x88, 0xac}}); // P2PKH输出

    std::vector<int64_t> inputValues = {60000}; // 输入60,000聪

    // 处理交易
    if (system.processTransaction(tx, inputValues)) {
        std::cout << "Transaction processing completed" << std::endl;
    }

    // 打印统计信息
    system.printMempoolStats();
}
```

### 项目2: PSBT工作流程演示

```cpp
void psbt_workflow_demo() {
    PSBTProcessor processor;

    // 1. 创建交易
    SimpleTransaction tx;
    tx.inputs.push_back({uint256::fromString("123"), 0, {}, 0xffffffff});
    tx.outputs.push_back({50000, {0x76, 0xa9, 0x14, 0x88, 0xac}});

    // 2. 创建PSBT
    PSBT psbt = processor.createPSBT(tx);

    // 3. 分析PSBT
    PSBTAnalysis analysis = psbt.analyze();
    std::cout << "PSBT analysis:" << std::endl;
    std::cout << "Next role: " << static_cast<int>(analysis.next) << std::endl;
    std::cout << "Inputs: " << analysis.inputs.size() << std::endl;

    // 4. 更新PSBT（添加UTXO信息）
    TransactionOutput utxo{60000, {0x76, 0xa9, 0x14, 0x88, 0xac}};
    processor.updatePSBT(psbt, 0, utxo);

    // 5. 重新分析
    analysis = psbt.analyze();
    std::cout << "After update - Next role: " << static_cast<int>(analysis.next) << std::endl;

    // 6. 签名PSBT
    std::vector<uint8_t> signature = {0x30, 0x45, 0x02, 0x21, 0x00}; // 示例签名
    processor.signPSBT(psbt, 0, signature);

    // 7. 完成PSBT
    if (processor.finalizePSBT(psbt)) {
        std::cout << "PSBT finalized successfully" << std::endl;

        // 8. 提取最终交易
        auto finalTx = processor.extractTransaction(psbt);
        if (finalTx) {
            std::cout << "Final transaction extracted" << std::endl;
        }
    }
}
```

## 总结

本教程涵盖了Bitcoin Core中交易处理的核心概念：

1. **交易结构**: 输入、输出、脚本、哈希计算
2. **交易验证**: 基本结构、UTXO验证、费用验证
3. **内存池管理**: 交易存储、费用排序、清理机制
4. **PSBT处理**: 部分签名交易、角色管理、工作流程
5. **网络传播**: 交易广播、确认监控、状态管理

这些技术在Bitcoin Core中得到了广泛应用，是理解区块链交易处理的重要基础。