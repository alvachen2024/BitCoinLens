// Copyright (c) 2012-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_PROTOCOL_VERSION_H
#define BITCOIN_NODE_PROTOCOL_VERSION_H

/**
 * network protocol versioning
 *
 * 网络协议版本控制
 */

/** 当前协议版本号
 * 这是Bitcoin Core当前支持的最高协议版本
 */
static const int PROTOCOL_VERSION = 70016;

//! initial proto version, to be increased after version/verack negotiation
//! 初始协议版本，在version/verack协商后增加
static const int INIT_PROTO_VERSION = 209;

//! disconnect from peers older than this proto version
//! 断开比此协议版本更旧的对等节点连接
static const int MIN_PEER_PROTO_VERSION = 31800;

//! BIP 0031, pong message, is enabled for all versions AFTER this one
//! BIP 0031，pong消息，在此版本之后的所有版本中启用
static const int BIP0031_VERSION = 60000;

//! "sendheaders" command and announcing blocks with headers starts with this version
//! "sendheaders"命令和通过头部宣布区块从此版本开始
static const int SENDHEADERS_VERSION = 70012;

//! "feefilter" tells peers to filter invs to you by fee starts with this version
//! "feefilter"告诉对等节点按费用过滤发送给你的inv从此版本开始
static const int FEEFILTER_VERSION = 70013;

//! short-id-based block download starts with this version
//! 基于短ID的区块下载从此版本开始
static const int SHORT_IDS_BLOCKS_VERSION = 70014;

//! not banning for invalid compact blocks starts with this version
//! 不因无效紧凑区块而封禁从此版本开始
static const int INVALID_CB_NO_BAN_VERSION = 70015;

//! "wtxidrelay" command for wtxid-based relay starts with this version
//! 基于wtxid的中继的"wtxidrelay"命令从此版本开始
static const int WTXID_RELAY_VERSION = 70016;

#endif // BITCOIN_NODE_PROTOCOL_VERSION_H
