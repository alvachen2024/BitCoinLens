// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_CONNECTION_TYPES_H
#define BITCOIN_NODE_CONNECTION_TYPES_H

#include <cstdint>
#include <string>

/** Different types of connections to a peer. This enum encapsulates the
 * information we have available at the time of opening or accepting the
 * connection. Aside from INBOUND, all types are initiated by us.
 *
 * 与对等节点的不同连接类型。此枚举封装了我们在打开或接受连接时
 * 可用的信息。除了INBOUND之外，所有类型都由我们发起。
 *
 * If adding or removing types, please update CONNECTION_TYPE_DOC in
 * src/rpc/net.cpp and src/qt/rpcconsole.cpp, as well as the descriptions in
 * src/qt/guiutil.cpp and src/bitcoin-cli.cpp::NetinfoRequestHandler.
 *
 * 如果添加或删除类型，请更新src/rpc/net.cpp和src/qt/rpcconsole.cpp中的
 * CONNECTION_TYPE_DOC，以及src/qt/guiutil.cpp和src/bitcoin-cli.cpp::NetinfoRequestHandler
 * 中的描述。 */
enum class ConnectionType {
    /**
     * Inbound connections are those initiated by a peer. This is the only
     * property we know at the time of connection, until P2P messages are
     * exchanged.
     *
     * 入站连接是由对等节点发起的连接。这是我们在连接时知道的唯一属性，
     * 直到交换P2P消息为止。
     */
    INBOUND,

    /**
     * These are the default connections that we use to connect with the
     * network. There is no restriction on what is relayed; by default we relay
     * blocks, addresses & transactions. We automatically attempt to open
     * MAX_OUTBOUND_FULL_RELAY_CONNECTIONS using addresses from our AddrMan.
     *
     * 这些是我们用来连接网络的默认连接。对中继内容没有限制；默认情况下
     * 我们中继区块、地址和交易。我们自动尝试使用AddrMan中的地址打开
     * MAX_OUTBOUND_FULL_RELAY_CONNECTIONS个连接。
     */
    OUTBOUND_FULL_RELAY,

    /**
     * We open manual connections to addresses that users explicitly requested
     * via the addnode RPC or the -addnode/-connect configuration options. Even if a
     * manual connection is misbehaving, we do not automatically disconnect or
     * add it to our discouragement filter.
     *
     * 我们打开到用户通过addnode RPC或-addnode/-connect配置选项明确请求的
     * 地址的手动连接。即使手动连接行为不当，我们也不会自动断开连接或
     * 将其添加到我们的劝阻过滤器中。
     */
    MANUAL,

    /**
     * Feeler connections are short-lived connections made to check that a node
     * is alive. They can be useful for:
     * - test-before-evict: if one of the peers is considered for eviction from
     *   our AddrMan because another peer is mapped to the same slot in the tried table,
     *   evict only if this longer-known peer is offline.
     * - move node addresses from New to Tried table, so that we have more
     *   connectable addresses in our AddrMan.
     * Note that in the literature ("Eclipse Attacks on Bitcoin's Peer-to-Peer Network")
     * only the latter feature is referred to as "feeler connections",
     * although in our codebase feeler connections encompass test-before-evict as well.
     * We make these connections approximately every FEELER_INTERVAL:
     * first we resolve previously found collisions if they exist (test-before-evict),
     * otherwise we connect to a node from the new table.
     *
     * 探测连接是为了检查节点是否存活而建立的短期连接。它们可用于：
     * - 驱逐前测试：如果其中一个对等节点因为另一个对等节点映射到尝试表中的
     *   同一槽位而被考虑从我们的AddrMan中驱逐，只有当这个更早已知的对等节点
     *   离线时才驱逐。
     * - 将节点地址从New表移动到Tried表，这样我们在AddrMan中就有更多可连接的地址。
     * 注意，在文献中（"Eclipse Attacks on Bitcoin's Peer-to-Peer Network"）
     * 只有后者被称为"feeler connections"，尽管在我们的代码库中feeler connections
     * 也包含驱逐前测试。我们大约每FEELER_INTERVAL建立这些连接：
     * 首先，如果存在，我们解决之前发现的冲突（驱逐前测试），
     * 否则我们从新表连接到一个节点。
     */
    FEELER,

    /**
     * We use block-relay-only connections to help prevent against partition
     * attacks. By not relaying transactions or addresses, these connections
     * are harder to detect by a third party, thus helping obfuscate the
     * network topology. We automatically attempt to open
     * MAX_BLOCK_RELAY_ONLY_ANCHORS using addresses from our anchors.dat. Then
     * addresses from our AddrMan if MAX_BLOCK_RELAY_ONLY_CONNECTIONS
     * isn't reached yet.
     *
     * 我们使用仅区块中继连接来帮助防止分区攻击。通过不中继交易或地址，
     * 这些连接更难被第三方检测到，从而有助于混淆网络拓扑。我们自动尝试
     * 使用anchors.dat中的地址打开MAX_BLOCK_RELAY_ONLY_ANCHORS个连接。
     * 然后，如果还没有达到MAX_BLOCK_RELAY_ONLY_CONNECTIONS，则使用
     * 我们AddrMan中的地址。
     */
    BLOCK_RELAY,

    /**
     * AddrFetch connections are short lived connections used to solicit
     * addresses from peers. These are initiated to addresses submitted via the
     * -seednode command line argument, or under certain conditions when the
     * AddrMan is empty.
     *
     * AddrFetch连接是用于从对等节点获取地址的短期连接。这些连接到通过
     * -seednode命令行参数提交的地址，或在AddrMan为空时的某些条件下。
     */
    ADDR_FETCH,
};

/** Convert ConnectionType enum to a string value
 * 将ConnectionType枚举转换为字符串值 */
std::string ConnectionTypeAsString(ConnectionType conn_type);

/** Transport layer version
 * 传输层版本 */
enum class TransportProtocolType : uint8_t {
    DETECTING, //!< Peer could be v1 or v2 - 对等节点可能是v1或v2
    V1, //!< Unencrypted, plaintext protocol - 未加密的明文协议
    V2, //!< BIP324 protocol - BIP324协议
};

/** Convert TransportProtocolType enum to a string value
 * 将TransportProtocolType枚举转换为字符串值 */
std::string TransportTypeAsString(TransportProtocolType transport_type);

#endif // BITCOIN_NODE_CONNECTION_TYPES_H
