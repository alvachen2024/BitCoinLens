# 挖矿和共识机制教程 - 基于Bitcoin Core实践

## 1. 挖矿基础概念

### 1.1 区块结构

#### Bitcoin Core中的区块定义
```cpp
// 来自 primitives/block.h
class CBlock {
public:
    // 区块头
    CBlockHeader header;

    // 交易列表
    std::vector<CTransactionRef> vtx;

    // 获取区块哈希
    uint256 GetHash() const;

    // 获取默克尔根
    uint256 ComputeMerkleRoot(bool* mutated = nullptr) const;
};
```

#### 实践实现
```cpp
#include <vector>
#include <cstdint>
#include <string>

// 区块头结构
struct BlockHeader {
    int32_t version;         // 版本号
    uint256 prevBlockHash;   // 前一个区块哈希
    uint256 merkleRoot;      // 默克尔根
    uint32_t timestamp;      // 时间戳
    uint32_t bits;           // 难度目标
    uint32_t nonce;          // 随机数

    // 计算区块头哈希
    uint256 getHash() const {
        // 简化的哈希计算（实际使用SHA256）
        std::string data = std::to_string(version) +
                          prevBlockHash.ToString() +
                          merkleRoot.ToString() +
                          std::to_string(timestamp) +
                          std::to_string(bits) +
                          std::to_string(nonce);
        return uint256::fromString(data);
    }
};

// 区块结构
class Block {
public:
    BlockHeader header;
    std::vector<SimpleTransaction> transactions;

    // 计算默克尔根
    uint256 computeMerkleRoot() const {
        if (transactions.empty()) {
            return uint256::zero();
        }

        std::vector<uint256> hashes;
        for (const auto& tx : transactions) {
            hashes.push_back(tx.getHash());
        }

        return computeMerkleRootFromHashes(hashes);
    }

    // 验证区块
    bool isValid() const {
        // 检查默克尔根
        if (header.merkleRoot != computeMerkleRoot()) {
            return false;
        }

        // 检查交易数量
        if (transactions.empty()) {
            return false;
        }

        // 检查第一个交易是否为coinbase
        if (!transactions[0].inputs.empty()) {
            return false;
        }

        return true;
    }

private:
    uint256 computeMerkleRootFromHashes(const std::vector<uint256>& hashes) const {
        if (hashes.empty()) {
            return uint256::zero();
        }

        if (hashes.size() == 1) {
            return hashes[0];
        }

        std::vector<uint256> newHashes;
        for (size_t i = 0; i < hashes.size(); i += 2) {
            uint256 left = hashes[i];
            uint256 right = (i + 1 < hashes.size()) ? hashes[i + 1] : left;

            // 连接两个哈希并计算新哈希
            std::string combined = left.ToString() + right.ToString();
            newHashes.push_back(uint256::fromString(combined));
        }

        return computeMerkleRootFromHashes(newHashes);
    }
};
```

### 1.2 工作量证明

#### 实践实现
```cpp
#include <chrono>
#include <thread>

// 工作量证明验证器
class ProofOfWorkValidator {
public:
    // 验证区块头哈希是否满足难度要求
    bool validateProofOfWork(const BlockHeader& header, uint32_t targetBits) {
        uint256 hash = header.getHash();
        uint256 target = getTargetFromBits(targetBits);

        return hash < target;
    }

    // 从难度位计算目标值
    uint256 getTargetFromBits(uint32_t bits) {
        uint8_t exponent = (bits >> 24) & 0xFF;
        uint32_t mantissa = bits & 0xFFFFFF;

        uint256 target = mantissa;
        target = target << (8 * (exponent - 3));

        return target;
    }

    // 计算难度
    double calculateDifficulty(uint32_t targetBits) {
        uint256 target = getTargetFromBits(targetBits);
        uint256 maxTarget = uint256::max();

        return static_cast<double>(maxTarget.GetLow64()) / target.GetLow64();
    }

    // 验证区块
    bool validateBlock(const Block& block, uint32_t targetBits) {
        // 验证工作量证明
        if (!validateProofOfWork(block.header, targetBits)) {
            return false;
        }

        // 验证区块结构
        if (!block.isValid()) {
            return false;
        }

        return true;
    }
};

// 简单挖矿器
class SimpleMiner {
public:
    SimpleMiner(uint32_t targetBits) : m_targetBits(targetBits) {}

    // 挖矿
    std::optional<Block> mineBlock(const std::vector<SimpleTransaction>& transactions,
                                  const uint256& prevBlockHash) {
        Block block;
        block.header.version = 1;
        block.header.prevBlockHash = prevBlockHash;
        block.header.timestamp = getCurrentTimestamp();
        block.header.bits = m_targetBits;
        block.transactions = transactions;

        // 更新默克尔根
        block.header.merkleRoot = block.computeMerkleRoot();

        // 尝试不同的nonce值
        for (uint32_t nonce = 0; nonce < UINT32_MAX; ++nonce) {
            block.header.nonce = nonce;

            if (ProofOfWorkValidator().validateProofOfWork(block.header, m_targetBits)) {
                std::cout << "Block mined! Nonce: " << nonce << std::endl;
                return block;
            }

            // 每1000次尝试更新一次时间戳
            if (nonce % 1000 == 0) {
                block.header.timestamp = getCurrentTimestamp();
                block.header.merkleRoot = block.computeMerkleRoot();
            }
        }

        return std::nullopt;
    }

    // 多线程挖矿
    std::optional<Block> mineBlockParallel(const std::vector<SimpleTransaction>& transactions,
                                          const uint256& prevBlockHash,
                                          int numThreads = 4) {
        std::vector<std::thread> threads;
        std::atomic<bool> found(false);
        std::optional<Block> result;
        std::mutex resultMutex;

        for (int i = 0; i < numThreads; ++i) {
            threads.emplace_back([this, &transactions, &prevBlockHash, &found, &result, &resultMutex, i, numThreads]() {
                Block block;
                block.header.version = 1;
                block.header.prevBlockHash = prevBlockHash;
                block.header.timestamp = getCurrentTimestamp();
                block.header.bits = m_targetBits;
                block.transactions = transactions;
                block.header.merkleRoot = block.computeMerkleRoot();

                // 每个线程处理不同的nonce范围
                uint32_t startNonce = i * (UINT32_MAX / numThreads);
                uint32_t endNonce = (i + 1) * (UINT32_MAX / numThreads);

                for (uint32_t nonce = startNonce; nonce < endNonce && !found; ++nonce) {
                    block.header.nonce = nonce;

                    if (ProofOfWorkValidator().validateProofOfWork(block.header, m_targetBits)) {
                        std::lock_guard<std::mutex> lock(resultMutex);
                        if (!found) {
                            found = true;
                            result = block;
                            std::cout << "Block mined by thread " << i << "! Nonce: " << nonce << std::endl;
                        }
                        break;
                    }
                }
            });
        }

        // 等待所有线程完成
        for (auto& thread : threads) {
            thread.join();
        }

        return result;
    }

private:
    uint32_t m_targetBits;

    uint32_t getCurrentTimestamp() {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};
```

## 2. 难度调整

### 2.1 难度计算

#### Bitcoin Core中的难度调整
```cpp
// 来自 pow.cpp
unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast,
                                     const Consensus::Params& params) {
    // 计算目标难度
    int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexLast->pprev->GetBlockTime();

    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;

    // 重新计算目标
    const arith_uint256 bnOld = pindexLast->nBits;
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetTimespan;

    return bnNew.GetCompact();
}
```

#### 实践实现
```cpp
#include <deque>

// 难度调整器
class DifficultyAdjuster {
public:
    DifficultyAdjuster(int targetTimespan = 2016, int targetSpacing = 600)
        : m_targetTimespan(targetTimespan), m_targetSpacing(targetSpacing) {}

    // 计算下一个难度目标
    uint32_t calculateNextDifficulty(const std::vector<BlockHeader>& recentBlocks) {
        if (recentBlocks.size() < 2) {
            return DEFAULT_BITS;
        }

        // 计算实际时间跨度
        int64_t actualTimespan = recentBlocks.back().timestamp -
                                recentBlocks[recentBlocks.size() - 2].timestamp;

        // 限制时间跨度范围
        int64_t minTimespan = m_targetTimespan / 4;
        int64_t maxTimespan = m_targetTimespan * 4;

        if (actualTimespan < minTimespan) {
            actualTimespan = minTimespan;
        }
        if (actualTimespan > maxTimespan) {
            actualTimespan = maxTimespan;
        }

        // 计算新的难度目标
        uint32_t currentBits = recentBlocks.back().bits;
        uint256 currentTarget = getTargetFromBits(currentBits);

        // 调整目标
        uint256 newTarget = currentTarget * actualTimespan / m_targetTimespan;

        return getBitsFromTarget(newTarget);
    }

    // 验证难度调整
    bool validateDifficultyAdjustment(const std::vector<BlockHeader>& blocks) {
        if (blocks.size() < m_targetTimespan) {
            return true; // 不足2016个区块，不调整难度
        }

        // 检查是否在正确的区块调整难度
        size_t adjustmentBlock = blocks.size() - 1;
        if (adjustmentBlock % m_targetTimespan != 0) {
            return true; // 不是调整区块
        }

        // 计算期望的难度
        std::vector<BlockHeader> recentBlocks(blocks.end() - m_targetTimespan, blocks.end());
        uint32_t expectedBits = calculateNextDifficulty(recentBlocks);

        return blocks.back().bits == expectedBits;
    }

    // 计算平均出块时间
    double calculateAverageBlockTime(const std::vector<BlockHeader>& blocks) {
        if (blocks.size() < 2) {
            return m_targetSpacing;
        }

        int64_t totalTime = blocks.back().timestamp - blocks.front().timestamp;
        return static_cast<double>(totalTime) / (blocks.size() - 1);
    }

private:
    int m_targetTimespan;  // 目标时间跨度（2016个区块）
    int m_targetSpacing;   // 目标区块间隔（600秒）

    static const uint32_t DEFAULT_BITS = 0x1d00ffff;

    uint256 getTargetFromBits(uint32_t bits) {
        uint8_t exponent = (bits >> 24) & 0xFF;
        uint32_t mantissa = bits & 0xFFFFFF;

        uint256 target = mantissa;
        target = target << (8 * (exponent - 3));

        return target;
    }

    uint32_t getBitsFromTarget(const uint256& target) {
        // 简化的目标到位的转换
        uint64_t targetValue = target.GetLow64();

        if (targetValue == 0) {
            return 0x1d00ffff;
        }

        // 计算指数和尾数
        int exponent = 0;
        uint64_t mantissa = targetValue;

        while (mantissa > 0xFFFFFF) {
            mantissa >>= 8;
            exponent++;
        }

        return (exponent << 24) | mantissa;
    }
};
```

### 2.2 难度历史跟踪

#### 实践实现
```cpp
// 难度历史跟踪器
class DifficultyTracker {
public:
    // 添加新区块
    void addBlock(const BlockHeader& header) {
        m_blocks.push_back(header);

        // 保持最近2016个区块
        if (m_blocks.size() > 2016) {
            m_blocks.erase(m_blocks.begin());
        }
    }

    // 获取当前难度
    double getCurrentDifficulty() const {
        if (m_blocks.empty()) {
            return 1.0;
        }

        uint32_t currentBits = m_blocks.back().bits;
        return calculateDifficulty(currentBits);
    }

    // 获取难度历史
    std::vector<double> getDifficultyHistory() const {
        std::vector<double> history;
        for (const auto& block : m_blocks) {
            history.push_back(calculateDifficulty(block.bits));
        }
        return history;
    }

    // 预测下一个难度
    double predictNextDifficulty() const {
        if (m_blocks.size() < 2016) {
            return getCurrentDifficulty();
        }

        std::vector<BlockHeader> recentBlocks(m_blocks.end() - 2016, m_blocks.end());
        DifficultyAdjuster adjuster;
        uint32_t nextBits = adjuster.calculateNextDifficulty(recentBlocks);

        return calculateDifficulty(nextBits);
    }

    // 获取挖矿统计
    struct MiningStats {
        double currentDifficulty;
        double averageBlockTime;
        double hashRate;  // 估计的哈希率
        int blocksInPeriod;
    };

    MiningStats getMiningStats() const {
        MiningStats stats;
        stats.currentDifficulty = getCurrentDifficulty();
        stats.blocksInPeriod = m_blocks.size();

        if (m_blocks.size() >= 2) {
            int64_t totalTime = m_blocks.back().timestamp - m_blocks.front().timestamp;
            stats.averageBlockTime = static_cast<double>(totalTime) / (m_blocks.size() - 1);

            // 估算哈希率（简化计算）
            uint256 target = getTargetFromBits(m_blocks.back().bits);
            stats.hashRate = static_cast<double>(target.GetLow64()) / stats.averageBlockTime;
        }

        return stats;
    }

private:
    std::deque<BlockHeader> m_blocks;

    double calculateDifficulty(uint32_t bits) const {
        uint256 target = getTargetFromBits(bits);
        uint256 maxTarget = uint256::max();

        return static_cast<double>(maxTarget.GetLow64()) / target.GetLow64();
    }

    uint256 getTargetFromBits(uint32_t bits) const {
        uint8_t exponent = (bits >> 24) & 0xFF;
        uint32_t mantissa = bits & 0xFFFFFF;

        uint256 target = mantissa;
        target = target << (8 * (exponent - 3));

        return target;
    }
};
```

## 3. 共识规则

### 3.1 区块验证规则

#### Bitcoin Core中的共识验证
```cpp
// 来自 validation.cpp
bool CheckBlock(const CBlock& block, BlockValidationState& state,
                const Consensus::Params& consensusParams,
                bool fCheckPOW, bool fCheckMerkleRoot) {
    // 检查区块大小
    if (block.vtx.empty() || block.vtx.size() * WITNESS_SCALE_FACTOR > MAX_BLOCK_WEIGHT ||
        ::GetSerializeSize(block, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) * WITNESS_SCALE_FACTOR > MAX_BLOCK_WEIGHT) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-blk-length", "size limits failed");
    }

    // 检查工作量证明
    if (fCheckPOW && fCheckMerkleRoot) {
        if (!CheckProofOfWork(block.GetHash(), block.nBits, consensusParams)) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "high-hash", "proof of work failed");
        }
    }

    // 检查默克尔根
    if (fCheckMerkleRoot) {
        bool mutated;
        uint256 hashMerkleRoot2 = BlockMerkleRoot(block, &mutated);
        if (block.hashMerkleRoot != hashMerkleRoot2) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-txnmrklroot", "hashMerkleRoot mismatch");
        }
    }

    return true;
}
```

#### 实践实现
```cpp
#include <set>

// 共识验证器
class ConsensusValidator {
public:
    // 验证区块
    bool validateBlock(const Block& block, const BlockHeader& prevBlock) {
        // 1. 检查区块大小
        if (!validateBlockSize(block)) {
            return false;
        }

        // 2. 检查工作量证明
        if (!validateProofOfWork(block.header)) {
            return false;
        }

        // 3. 检查默克尔根
        if (!validateMerkleRoot(block)) {
            return false;
        }

        // 4. 检查时间戳
        if (!validateTimestamp(block.header, prevBlock)) {
            return false;
        }

        // 5. 检查交易
        if (!validateTransactions(block)) {
            return false;
        }

        return true;
    }

    // 验证区块大小
    bool validateBlockSize(const Block& block) {
        size_t totalSize = 0;

        // 计算区块头大小
        totalSize += 80; // 区块头固定80字节

        // 计算交易大小
        for (const auto& tx : block.transactions) {
            totalSize += tx.getSize();
        }

        return totalSize <= MAX_BLOCK_SIZE;
    }

    // 验证工作量证明
    bool validateProofOfWork(const BlockHeader& header) {
        uint256 hash = header.getHash();
        uint256 target = getTargetFromBits(header.bits);

        return hash < target;
    }

    // 验证默克尔根
    bool validateMerkleRoot(const Block& block) {
        uint256 computedRoot = block.computeMerkleRoot();
        return block.header.merkleRoot == computedRoot;
    }

    // 验证时间戳
    bool validateTimestamp(const BlockHeader& header, const BlockHeader& prevBlock) {
        // 时间戳不能太旧
        uint32_t currentTime = getCurrentTimestamp();
        if (header.timestamp > currentTime + MAX_FUTURE_BLOCK_TIME) {
            return false;
        }

        // 时间戳不能比前一个区块早太多
        if (header.timestamp < prevBlock.timestamp - MAX_PAST_BLOCK_TIME) {
            return false;
        }

        return true;
    }

    // 验证交易
    bool validateTransactions(const Block& block) {
        if (block.transactions.empty()) {
            return false;
        }

        // 检查第一个交易是否为coinbase
        if (!block.transactions[0].inputs.empty()) {
            return false;
        }

        // 检查交易重复
        std::set<uint256> txHashes;
        for (const auto& tx : block.transactions) {
            uint256 txHash = tx.getHash();
            if (txHashes.find(txHash) != txHashes.end()) {
                return false; // 重复交易
            }
            txHashes.insert(txHash);
        }

        return true;
    }

private:
    static const size_t MAX_BLOCK_SIZE = 1000000;        // 1MB
    static const uint32_t MAX_FUTURE_BLOCK_TIME = 7200;  // 2小时
    static const uint32_t MAX_PAST_BLOCK_TIME = 7200;    // 2小时

    uint256 getTargetFromBits(uint32_t bits) {
        uint8_t exponent = (bits >> 24) & 0xFF;
        uint32_t mantissa = bits & 0xFFFFFF;

        uint256 target = mantissa;
        target = target << (8 * (exponent - 3));

        return target;
    }

    uint32_t getCurrentTimestamp() {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};
```

### 3.2 分叉处理

#### 实践实现
```cpp
#include <map>

// 区块链分叉处理器
class ForkProcessor {
public:
    // 添加新区块
    bool addBlock(const Block& block) {
        uint256 blockHash = block.header.getHash();

        // 检查是否已存在
        if (m_blocks.find(blockHash) != m_blocks.end()) {
            return false;
        }

        // 添加到区块映射
        m_blocks[blockHash] = block;

        // 更新链结构
        updateChainStructure(block);

        return true;
    }

    // 获取最长链
    std::vector<Block> getLongestChain() const {
        std::vector<Block> longestChain;
        uint256 currentHash = m_bestChainTip;

        while (!currentHash.IsNull()) {
            auto it = m_blocks.find(currentHash);
            if (it == m_blocks.end()) {
                break;
            }

            longestChain.insert(longestChain.begin(), it->second);
            currentHash = it->second.header.prevBlockHash;
        }

        return longestChain;
    }

    // 处理分叉
    void handleFork(const Block& newBlock) {
        uint256 newBlockHash = newBlock.header.getHash();

        // 检查是否创建了更长的链
        std::vector<Block> newChain = getChainFromBlock(newBlockHash);
        std::vector<Block> currentChain = getLongestChain();

        if (newChain.size() > currentChain.size()) {
            // 发生重组
            std::cout << "Chain reorganization detected!" << std::endl;
            std::cout << "Old chain length: " << currentChain.size() << std::endl;
            std::cout << "New chain length: " << newChain.size() << std::endl;

            // 更新最佳链
            m_bestChainTip = newBlockHash;

            // 处理孤立的区块
            handleOrphanedBlocks(currentChain, newChain);
        }
    }

    // 获取分叉统计
    struct ForkStats {
        int totalBlocks;
        int orphanedBlocks;
        int activeForks;
        int chainLength;
    };

    ForkStats getForkStats() const {
        ForkStats stats;
        stats.totalBlocks = m_blocks.size();
        stats.chainLength = getLongestChain().size();

        // 计算孤立区块
        std::set<uint256> reachableBlocks;
        uint256 current = m_bestChainTip;
        while (!current.IsNull()) {
            reachableBlocks.insert(current);
            auto it = m_blocks.find(current);
            if (it != m_blocks.end()) {
                current = it->second.header.prevBlockHash;
            } else {
                break;
            }
        }

        stats.orphanedBlocks = m_blocks.size() - reachableBlocks.size();

        // 计算活跃分叉
        stats.activeForks = countActiveForks();

        return stats;
    }

private:
    std::map<uint256, Block> m_blocks;
    std::map<uint256, std::vector<uint256>> m_children;  // 父区块到子区块的映射
    uint256 m_bestChainTip;

    void updateChainStructure(const Block& block) {
        uint256 blockHash = block.header.getHash();
        uint256 parentHash = block.header.prevBlockHash;

        // 添加子区块关系
        m_children[parentHash].push_back(blockHash);

        // 更新最佳链
        if (m_bestChainTip.IsNull() ||
            getChainLength(blockHash) > getChainLength(m_bestChainTip)) {
            m_bestChainTip = blockHash;
        }
    }

    int getChainLength(const uint256& blockHash) const {
        int length = 0;
        uint256 current = blockHash;

        while (!current.IsNull()) {
            length++;
            auto it = m_blocks.find(current);
            if (it != m_blocks.end()) {
                current = it->second.header.prevBlockHash;
            } else {
                break;
            }
        }

        return length;
    }

    std::vector<Block> getChainFromBlock(const uint256& blockHash) const {
        std::vector<Block> chain;
        uint256 current = blockHash;

        while (!current.IsNull()) {
            auto it = m_blocks.find(current);
            if (it != m_blocks.end()) {
                chain.insert(chain.begin(), it->second);
                current = it->second.header.prevBlockHash;
            } else {
                break;
            }
        }

        return chain;
    }

    void handleOrphanedBlocks(const std::vector<Block>& oldChain,
                             const std::vector<Block>& newChain) {
        std::set<uint256> newChainHashes;
        for (const auto& block : newChain) {
            newChainHashes.insert(block.header.getHash());
        }

        std::vector<uint256> orphaned;
        for (const auto& block : oldChain) {
            if (newChainHashes.find(block.header.getHash()) == newChainHashes.end()) {
                orphaned.push_back(block.header.getHash());
            }
        }

        std::cout << "Orphaned blocks: " << orphaned.size() << std::endl;
    }

    int countActiveForks() const {
        int forks = 0;
        for (const auto& pair : m_children) {
            if (pair.second.size() > 1) {
                forks += pair.second.size() - 1;
            }
        }
        return forks;
    }
};
```

## 4. 实践项目

### 项目1: 完整的挖矿系统

```cpp
#include <iostream>
#include <memory>
#include <thread>

// 完整的挖矿系统
class MiningSystem {
public:
    MiningSystem(uint32_t initialDifficulty = 0x1d00ffff)
        : miner(std::make_unique<SimpleMiner>(initialDifficulty)),
          difficultyTracker(std::make_unique<DifficultyTracker>()),
          forkProcessor(std::make_unique<ForkProcessor>()),
          consensusValidator(std::make_unique<ConsensusValidator>()) {}

    // 开始挖矿
    void startMining(const std::vector<SimpleTransaction>& pendingTransactions) {
        std::cout << "Starting mining with " << pendingTransactions.size() << " transactions" << std::endl;

        while (m_isMining) {
            // 创建新区块
            Block newBlock = createNewBlock(pendingTransactions);

            // 尝试挖矿
            auto minedBlock = miner->mineBlockParallel(newBlock.transactions,
                                                      newBlock.header.prevBlockHash);

            if (minedBlock) {
                std::cout << "Block mined successfully!" << std::endl;

                // 验证区块
                if (consensusValidator->validateBlock(*minedBlock, getLastBlockHeader())) {
                    // 添加到区块链
                    forkProcessor->addBlock(*minedBlock);
                    difficultyTracker->addBlock(minedBlock->header);

                    // 处理分叉
                    forkProcessor->handleFork(*minedBlock);

                    // 打印统计信息
                    printMiningStats();
                } else {
                    std::cout << "Block validation failed!" << std::endl;
                }
            }

            // 调整难度
            adjustDifficulty();

            // 短暂休息
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    // 停止挖矿
    void stopMining() {
        m_isMining = false;
    }

    // 获取挖矿统计
    void printMiningStats() const {
        auto chain = forkProcessor->getLongestChain();
        auto stats = difficultyTracker->getMiningStats();
        auto forkStats = forkProcessor->getForkStats();

        std::cout << "\n=== Mining Statistics ===" << std::endl;
        std::cout << "Chain length: " << chain.size() << std::endl;
        std::cout << "Current difficulty: " << stats.currentDifficulty << std::endl;
        std::cout << "Average block time: " << stats.averageBlockTime << " seconds" << std::endl;
        std::cout << "Estimated hash rate: " << stats.hashRate << " H/s" << std::endl;
        std::cout << "Total blocks: " << forkStats.totalBlocks << std::endl;
        std::cout << "Orphaned blocks: " << forkStats.orphanedBlocks << std::endl;
        std::cout << "Active forks: " << forkStats.activeForks << std::endl;
        std::cout << "========================\n" << std::endl;
    }

private:
    std::unique_ptr<SimpleMiner> miner;
    std::unique_ptr<DifficultyTracker> difficultyTracker;
    std::unique_ptr<ForkProcessor> forkProcessor;
    std::unique_ptr<ConsensusValidator> consensusValidator;

    std::atomic<bool> m_isMining{false};

    Block createNewBlock(const std::vector<SimpleTransaction>& transactions) {
        Block block;

        // 设置区块头
        block.header.version = 1;
        block.header.timestamp = getCurrentTimestamp();
        block.header.bits = miner->getTargetBits();

        // 设置前一个区块哈希
        auto chain = forkProcessor->getLongestChain();
        if (chain.empty()) {
            block.header.prevBlockHash = uint256::zero(); // 创世区块
        } else {
            block.header.prevBlockHash = chain.back().header.getHash();
        }

        // 添加coinbase交易
        SimpleTransaction coinbase;
        coinbase.outputs.push_back({5000000000, {0x76, 0xa9, 0x14, 0x88, 0xac}}); // 50 BTC
        block.transactions.push_back(coinbase);

        // 添加其他交易
        for (const auto& tx : transactions) {
            block.transactions.push_back(tx);
        }

        // 计算默克尔根
        block.header.merkleRoot = block.computeMerkleRoot();

        return block;
    }

    BlockHeader getLastBlockHeader() const {
        auto chain = forkProcessor->getLongestChain();
        if (chain.empty()) {
            return BlockHeader{}; // 空区块头
        }
        return chain.back().header;
    }

    void adjustDifficulty() {
        // 每2016个区块调整一次难度
        auto chain = forkProcessor->getLongestChain();
        if (chain.size() % 2016 == 0 && chain.size() > 0) {
            std::cout << "Adjusting difficulty..." << std::endl;
            double newDifficulty = difficultyTracker->predictNextDifficulty();
            std::cout << "New difficulty: " << newDifficulty << std::endl;
        }
    }

    uint32_t getCurrentTimestamp() {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

// 使用示例
void mining_system_demo() {
    MiningSystem miningSystem;

    // 创建一些测试交易
    std::vector<SimpleTransaction> transactions;
    for (int i = 0; i < 5; ++i) {
        SimpleTransaction tx;
        tx.inputs.push_back({uint256::fromString(std::to_string(i)), 0, {}, 0xffffffff});
        tx.outputs.push_back({1000000, {0x76, 0xa9, 0x14, 0x88, 0xac}});
        transactions.push_back(tx);
    }

    // 开始挖矿
    std::thread miningThread([&miningSystem, &transactions]() {
        miningSystem.startMining(transactions);
    });

    // 运行一段时间后停止
    std::this_thread::sleep_for(std::chrono::seconds(30));
    miningSystem.stopMining();

    if (miningThread.joinable()) {
        miningThread.join();
    }

    std::cout << "Mining completed!" << std::endl;
}
```

### 项目2: 共识规则测试

```cpp
void consensus_rules_test() {
    ConsensusValidator validator;

    // 测试1: 有效区块
    Block validBlock;
    validBlock.header.version = 1;
    validBlock.header.prevBlockHash = uint256::fromString("123");
    validBlock.header.timestamp = getCurrentTimestamp();
    validBlock.header.bits = 0x1d00ffff;

    // 添加coinbase交易
    SimpleTransaction coinbase;
    validBlock.transactions.push_back(coinbase);

    // 添加普通交易
    SimpleTransaction tx;
    tx.inputs.push_back({uint256::fromString("456"), 0, {0x01}, 0xffffffff});
    tx.outputs.push_back({50000, {0x76, 0xa9, 0x14, 0x88, 0xac}});
    validBlock.transactions.push_back(tx);

    validBlock.header.merkleRoot = validBlock.computeMerkleRoot();

    BlockHeader prevBlock;
    prevBlock.timestamp = getCurrentTimestamp() - 600;

    std::cout << "Testing valid block..." << std::endl;
    bool isValid = validator.validateBlock(validBlock, prevBlock);
    std::cout << "Valid block result: " << (isValid ? "PASS" : "FAIL") << std::endl;

    // 测试2: 无效工作量证明
    Block invalidPowBlock = validBlock;
    invalidPowBlock.header.bits = 0x1d00ffff; // 设置高难度

    std::cout << "Testing invalid PoW..." << std::endl;
    bool isInvalidPow = validator.validateBlock(invalidPowBlock, prevBlock);
    std::cout << "Invalid PoW result: " << (isInvalidPow ? "FAIL" : "PASS") << std::endl;

    // 测试3: 无效默克尔根
    Block invalidMerkleBlock = validBlock;
    invalidMerkleBlock.header.merkleRoot = uint256::fromString("999");

    std::cout << "Testing invalid merkle root..." << std::endl;
    bool isInvalidMerkle = validator.validateBlock(invalidMerkleBlock, prevBlock);
    std::cout << "Invalid merkle root result: " << (isInvalidMerkle ? "FAIL" : "PASS") << std::endl;

    // 测试4: 无效时间戳
    Block invalidTimestampBlock = validBlock;
    invalidTimestampBlock.header.timestamp = getCurrentTimestamp() + 10000; // 未来时间

    std::cout << "Testing invalid timestamp..." << std::endl;
    bool isInvalidTimestamp = validator.validateBlock(invalidTimestampBlock, prevBlock);
    std::cout << "Invalid timestamp result: " << (isInvalidTimestamp ? "FAIL" : "PASS") << std::endl;
}
```

## 总结

本教程涵盖了Bitcoin Core中挖矿和共识机制的核心概念：

1. **区块结构**: 区块头、交易、默克尔树
2. **工作量证明**: 哈希计算、难度调整、挖矿算法
3. **共识规则**: 区块验证、分叉处理、链重组
4. **难度调整**: 时间跨度计算、目标调整、历史跟踪
5. **实践项目**: 完整挖矿系统、共识规则测试

这些技术是比特币网络安全性和去中心化的核心基础。