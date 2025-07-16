// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_UTXO_SNAPSHOT_H
#define BITCOIN_NODE_UTXO_SNAPSHOT_H

#include <chainparams.h>
#include <kernel/chainparams.h>
#include <kernel/cs_main.h>
#include <serialize.h>
#include <sync.h>
#include <uint256.h>
#include <util/chaintype.h>
#include <util/check.h>
#include <util/fs.h>

#include <cstdint>
#include <optional>
#include <string_view>

// UTXO set snapshot magic bytes
// UTXO集合快照魔数字节
static constexpr std::array<uint8_t, 5> SNAPSHOT_MAGIC_BYTES = {'u', 't', 'x', 'o', 0xff};

class Chainstate;

namespace node {

//! Metadata describing a serialized version of a UTXO set from which an
//! assumeutxo Chainstate can be constructed.
//! All metadata fields come from an untrusted file, so must be validated
//! before being used. Thus, new fields should be added only if needed.
//!
//! 描述UTXO集合序列化版本的元数据，从中可以构造assumeutxo链状态。
//! 所有元数据字段都来自不受信任的文件，因此在使用前必须验证。
//! 因此，只有在需要时才应添加新字段。
class SnapshotMetadata
{
    inline static const uint16_t VERSION{2}; // 快照元数据版本
    const std::set<uint16_t> m_supported_versions{VERSION}; // 支持的版本集合
    const MessageStartChars m_network_magic; // 网络魔数

public:
    //! The hash of the block that reflects the tip of the chain for the
    //! UTXO set contained in this snapshot.
    //!
    //! 反映此快照中包含的UTXO集合链尖端的区块哈希。
    uint256 m_base_blockhash;

    //! The number of coins in the UTXO set contained in this snapshot. Used
    //! during snapshot load to estimate progress of UTXO set reconstruction.
    //!
    //! 此快照中包含的UTXO集合中的硬币数量。在快照加载期间用于估计UTXO集合重建的进度。
    uint64_t m_coins_count = 0;

    /**
     * 构造函数（仅网络魔数）
     * @param network_magic 网络魔数
     */
    SnapshotMetadata(
        const MessageStartChars network_magic) :
            m_network_magic(network_magic) { }

    /**
     * 构造函数（完整参数）
     * @param network_magic 网络魔数
     * @param base_blockhash 基础区块哈希
     * @param coins_count 硬币数量
     */
    SnapshotMetadata(
        const MessageStartChars network_magic,
        const uint256& base_blockhash,
        uint64_t coins_count) :
            m_network_magic(network_magic),
            m_base_blockhash(base_blockhash),
            m_coins_count(coins_count) { }

    /**
     * 序列化快照元数据
     * @param s 输出流
     */
    template <typename Stream>
    inline void Serialize(Stream& s) const {
        s << SNAPSHOT_MAGIC_BYTES;  // 写入魔数
        s << VERSION;               // 写入版本
        s << m_network_magic;       // 写入网络魔数
        s << m_base_blockhash;      // 写入基础区块哈希
        s << m_coins_count;         // 写入硬币数量
    }

    /**
     * 反序列化快照元数据
     * @param s 输入流
     */
    template <typename Stream>
    inline void Unserialize(Stream& s) {
        // Read the snapshot magic bytes
        // 读取快照魔数字节
        std::array<uint8_t, SNAPSHOT_MAGIC_BYTES.size()> snapshot_magic;
        s >> snapshot_magic;
        if (snapshot_magic != SNAPSHOT_MAGIC_BYTES) {
            throw std::ios_base::failure("Invalid UTXO set snapshot magic bytes. Please check if this is indeed a snapshot file or if you are using an outdated snapshot format.");
        }

        // Read the version
        // 读取版本
        uint16_t version;
        s >> version;
        if (m_supported_versions.find(version) == m_supported_versions.end()) {
            throw std::ios_base::failure(strprintf("Version of snapshot %s does not match any of the supported versions.", version));
        }

        // Read the network magic (pchMessageStart)
        // 读取网络魔数（pchMessageStart）
        MessageStartChars message;
        s >> message;
        if (!std::equal(message.begin(), message.end(), m_network_magic.data())) {
            auto metadata_network{GetNetworkForMagic(message)};
            if (metadata_network) {
                std::string network_string{ChainTypeToString(metadata_network.value())};
                auto node_network{GetNetworkForMagic(m_network_magic)};
                std::string node_network_string{ChainTypeToString(node_network.value())};
                throw std::ios_base::failure(strprintf("The network of the snapshot (%s) does not match the network of this node (%s).", network_string, node_network_string));
            } else {
                throw std::ios_base::failure("This snapshot has been created for an unrecognized network. This could be a custom signet, a new testnet or possibly caused by data corruption.");
            }
        }

        s >> m_base_blockhash;  // 读取基础区块哈希
        s >> m_coins_count;     // 读取硬币数量
    }
};

//! The file in the snapshot chainstate dir which stores the base blockhash. This is
//! needed to reconstruct snapshot chainstates on init.
//!
//! Because we only allow loading a single snapshot at a time, there will only be one
//! chainstate directory with this filename present within it.
//!
//! 快照链状态目录中存储基础区块哈希的文件。这在初始化时重建快照链状态时需要。
//!
//! 因为我们一次只允许加载一个快照，所以只会有一个包含此文件名的链状态目录。
const fs::path SNAPSHOT_BLOCKHASH_FILENAME{"base_blockhash"};

//! Write out the blockhash of the snapshot base block that was used to construct
//! this chainstate. This value is read in during subsequent initializations and
//! used to reconstruct snapshot-based chainstates.
//!
//! 写出用于构造此链状态的快照基础区块的区块哈希。此值在后续初始化期间读取，
//! 并用于重建基于快照的链状态。
bool WriteSnapshotBaseBlockhash(Chainstate& snapshot_chainstate)
    EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

//! Read the blockhash of the snapshot base block that was used to construct the
//! chainstate.
//!
//! 读取用于构造链状态的快照基础区块的区块哈希。
std::optional<uint256> ReadSnapshotBaseBlockhash(fs::path chaindir)
    EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

//! Suffix appended to the chainstate (leveldb) dir when created based upon
//! a snapshot.
//!
//! 基于快照创建时附加到链状态（leveldb）目录的后缀。
constexpr std::string_view SNAPSHOT_CHAINSTATE_SUFFIX = "_snapshot";

//! Return a path to the snapshot-based chainstate dir, if one exists.
//!
//! 如果存在，返回基于快照的链状态目录的路径。
std::optional<fs::path> FindSnapshotChainstateDir(const fs::path& data_dir);

} // namespace node

#endif // BITCOIN_NODE_UTXO_SNAPSHOT_H
