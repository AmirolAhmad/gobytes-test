// Copyright (c) 2014-2017 The Dash Core developers
// Copyright (c) 2017-2018 The GoByte Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dsnotificationinterface.h"
#include "instantx.h"
#include "governance.h"
#include "masternodeman.h"
#include "masternode-payments.h"
#include "masternode-sync.h"
#include "privatesend-client.h"
#include "txmempool.h"

void CDSNotificationInterface::InitializeCurrentBlockTip()
{
    LOCK(cs_main);
    UpdatedBlockTip(chainActive.Tip(), NULL, IsInitialBlockDownload());
}

void CDSNotificationInterface::AcceptedBlockHeader(const CBlockIndex *pindexNew)
{
    masternodeSync.AcceptedBlockHeader(pindexNew);
}

void CDSNotificationInterface::NotifyHeaderTip(const CBlockIndex *pindexNew, bool fInitialDownload)
{
    masternodeSync.NotifyHeaderTip(pindexNew, fInitialDownload, connman);
}

void CDSNotificationInterface::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload)
{
    if (pindexNew == pindexFork) // blocks were disconnected without any new ones
        return;

    masternodeSync.UpdatedBlockTip(pindexNew, fInitialDownload, connman);

    if (fInitialDownload) // In IBD
        return;

    mnodeman.UpdatedBlockTip(pindexNew);
    privateSendClient.UpdatedBlockTip(pindexNew);
    instantsend.UpdatedBlockTip(pindexNew);
    mnpayments.UpdatedBlockTip(pindexNew, connman);
    governance.UpdatedBlockTip(pindexNew, connman);

    // DIP0001 updates

    bool fDIP0001ActiveAtTipTmp = fDIP0001ActiveAtTip;
    // Update global flags
    fDIP0001LockedInAtTip = (VersionBitsState(pindexNew, Params().GetConsensus(), Consensus::DEPLOYMENT_DIP0001, versionbitscache) == THRESHOLD_LOCKED_IN);
    fDIP0001ActiveAtTip = (VersionBitsState(pindexNew, Params().GetConsensus(), Consensus::DEPLOYMENT_DIP0001, versionbitscache) == THRESHOLD_ACTIVE);

    // Update min fees only if activation changed and we are using default minRelayTxFee
    if (fDIP0001ActiveAtTipTmp != fDIP0001ActiveAtTip) {
        if (!mapArgs.count("-minrelaytxfee")) {
            ::minRelayTxFee = CFeeRate(fDIP0001ActiveAtTip ? DEFAULT_DIP0001_MIN_RELAY_TX_FEE : DEFAULT_LEGACY_MIN_RELAY_TX_FEE);
            mempool.UpdateMinFee(::minRelayTxFee);
        }
        if (!mapArgs.count("-mintxfee")) {
            CWallet::minTxFee = CFeeRate(fDIP0001ActiveAtTip ? DEFAULT_DIP0001_TRANSACTION_MINFEE : DEFAULT_LEGACY_TRANSACTION_MINFEE);
        }
    }
}

void CDSNotificationInterface::SyncTransaction(const CTransaction &tx, const CBlock *pblock)
{
    instantsend.SyncTransaction(tx, pblock);
    CPrivateSend::SyncTransaction(tx, pblock);
}
