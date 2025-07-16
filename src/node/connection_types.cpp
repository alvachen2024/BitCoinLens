// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/connection_types.h>
#include <cassert>

/**
 * 将连接类型枚举转换为字符串表示
 * 用于调试、日志记录和RPC接口
 *
 * @param conn_type 连接类型枚举值
 * @return 对应的字符串表示
 */
std::string ConnectionTypeAsString(ConnectionType conn_type)
{
    switch (conn_type) {
    case ConnectionType::INBOUND:
        return "inbound";           // 入站连接
    case ConnectionType::MANUAL:
        return "manual";            // 手动连接
    case ConnectionType::FEELER:
        return "feeler";            // 探测连接
    case ConnectionType::OUTBOUND_FULL_RELAY:
        return "outbound-full-relay"; // 出站全中继连接
    case ConnectionType::BLOCK_RELAY:
        return "block-relay-only";  // 仅区块中继连接
    case ConnectionType::ADDR_FETCH:
        return "addr-fetch";        // 地址获取连接
    } // no default case, so the compiler can warn about missing cases
    // 没有默认情况，这样编译器可以警告缺失的情况

    assert(false); // 如果到达这里，说明有未处理的枚举值
}

/**
 * 将传输协议类型枚举转换为字符串表示
 * 用于调试、日志记录和网络协议协商
 *
 * @param transport_type 传输协议类型枚举值
 * @return 对应的字符串表示
 */
std::string TransportTypeAsString(TransportProtocolType transport_type)
{
    switch (transport_type) {
    case TransportProtocolType::DETECTING:
        return "detecting";         // 检测中（可能是v1或v2）
    case TransportProtocolType::V1:
        return "v1";               // 版本1（未加密明文协议）
    case TransportProtocolType::V2:
        return "v2";               // 版本2（BIP324协议）
    } // no default case, so the compiler can warn about missing cases
    // 没有默认情况，这样编译器可以警告缺失的情况

    assert(false); // 如果到达这里，说明有未处理的枚举值
}
