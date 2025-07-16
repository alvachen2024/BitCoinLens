// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_PSBT_H
#define BITCOIN_NODE_PSBT_H

#include <psbt.h>

#include <optional>

namespace node {

/**
 * Holds an analysis of one input from a PSBT
 *
 * 保存PSBT中一个输入的分析结果
 */
struct PSBTInputAnalysis {
    bool has_utxo; //!< Whether we have UTXO information for this input - 我们是否有此输入的UTXO信息
    bool is_final; //!< Whether the input has all required information including signatures - 输入是否包含所有必需信息，包括签名
    PSBTRole next; //!< Which of the BIP 174 roles needs to handle this input next - BIP 174中哪个角色需要下一步处理此输入

    std::vector<CKeyID> missing_pubkeys; //!< Pubkeys whose BIP32 derivation path is missing - 缺少BIP32派生路径的公钥
    std::vector<CKeyID> missing_sigs;    //!< Pubkeys whose signatures are missing - 缺少签名的公钥
    uint160 missing_redeem_script;       //!< Hash160 of redeem script, if missing - 如果缺少，则为赎回脚本的Hash160
    uint256 missing_witness_script;      //!< SHA256 of witness script, if missing - 如果缺少，则为见证脚本的SHA256
};

/**
 * Holds the results of AnalyzePSBT (miscellaneous information about a PSBT)
 *
 * 保存AnalyzePSBT的结果（关于PSBT的各种信息）
 */
struct PSBTAnalysis {
    std::optional<size_t> estimated_vsize;      //!< Estimated weight of the transaction - 交易的估计权重
    std::optional<CFeeRate> estimated_feerate;  //!< Estimated feerate (fee / weight) of the transaction - 交易的估计费用率（费用/权重）
    std::optional<CAmount> fee;                 //!< Amount of fee being paid by the transaction - 交易支付的费用金额
    std::vector<PSBTInputAnalysis> inputs;      //!< More information about the individual inputs of the transaction - 关于交易各个输入的更多信息
    PSBTRole next;                              //!< Which of the BIP 174 roles needs to handle the transaction next - BIP 174中哪个角色需要下一步处理交易
    std::string error;                          //!< Error message - 错误消息

    /**
     * 设置分析结果为无效状态
     * 清除所有估计值并设置错误消息
     *
     * @param err_msg 错误消息
     */
    void SetInvalid(std::string err_msg)
    {
        estimated_vsize = std::nullopt;    // 清除估计权重
        estimated_feerate = std::nullopt;  // 清除估计费用率
        fee = std::nullopt;                // 清除费用
        inputs.clear();                    // 清除输入分析
        next = PSBTRole::CREATOR;          // 重置为创建者角色
        error = err_msg;                   // 设置错误消息
    }
};

/**
 * Provides helpful miscellaneous information about where a PSBT is in the signing workflow.
 *
 * 提供关于PSBT在签名工作流程中位置的帮助性信息。
 *
 * @param[in] psbtx the PSBT to analyze - 要分析的PSBT
 * @return A PSBTAnalysis with information about the provided PSBT - 包含所提供PSBT信息的PSBTAnalysis
 */
PSBTAnalysis AnalyzePSBT(PartiallySignedTransaction psbtx);

} // namespace node

#endif // BITCOIN_NODE_PSBT_H
