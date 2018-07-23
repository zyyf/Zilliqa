/**
* Copyright (c) 2018 Zilliqa 
* This source code is being disclosed to you solely for the purpose of your participation in 
* testing Zilliqa. You may view, compile and run the code for that purpose and pursuant to 
* the protocols and algorithms that are programmed into, and intended by, the code. You may 
* not do anything else with the code without express permission from Zilliqa Research Pte. Ltd., 
* including modifying or publishing the code (or any part of it), and developing or forming 
* another public or private blockchain network. This source code is provided ‘as is’ and no 
* warranties are given as to title or non-infringement, merchantability or fitness for purpose 
* and, to the extent permitted by law, all liability for your use of the code is disclaimed. 
* Some programs in this code are governed by the GNU General Public License v3.0 (available at 
* https://www.gnu.org/licenses/gpl-3.0.en.html) (‘GPLv3’). The programs that are governed by 
* GPLv3.0 are those programs that are located in the folders src/depends and tests/depends 
* and which include a reference to GPLv3 in their program files.
**/

#include <algorithm>
#include <chrono>
#include <thread>

#include "DSBlockConsensusMessage.h"
#include "DirectoryService.h"
#include "common/Constants.h"
#include "common/Messages.h"
#include "common/Serializable.h"
#include "depends/common/RLP.h"
#include "depends/libDatabase/MemoryDB.h"
#include "depends/libTrie/TrieDB.h"
#include "depends/libTrie/TrieHash.h"
#include "libCrypto/Sha2.h"
#include "libMediator/Mediator.h"
#include "libNetwork/P2PComm.h"
#include "libUtils/DataConversion.h"
#include "libUtils/DetachedFunction.h"
#include "libUtils/Logger.h"
#include "libUtils/SanityChecks.h"

using namespace std;
using namespace boost::multiprecision;

#ifndef IS_LOOKUP_NODE

void DirectoryService::ComposeDSBlock()
{
    LOG_MARKER();

    // Compute hash of previous DS block header
    BlockHash prevHash;
    if (m_mediator.m_dsBlockChain.GetBlockCount() > 0)
    {
        DSBlock lastBlock = m_mediator.m_dsBlockChain.GetLastBlock();
        SHA2<HASH_TYPE::HASH_VARIANT_256> sha2;
        vector<unsigned char> vec;
        const DSBlockHeader& lastHeader = lastBlock.GetHeader();
        lastHeader.Serialize(vec, 0);
        sha2.Update(vec);
        const vector<unsigned char>& resVec = sha2.Finalize();
        copy(resVec.begin(), resVec.end(), prevHash.asArray().begin());
    }

    // Assemble DS block header

    lock_guard<mutex> g(m_mutexAllPOW1);
    const PubKey& winnerKey = m_allPoW1s.front().first;
    const uint256_t& winnerNonce = m_allPoW1s.front().second;

    uint256_t blockNum = 0;
    uint8_t difficulty = POW2_DIFFICULTY;
    if (m_mediator.m_dsBlockChain.GetBlockCount() > 0)
    {
        DSBlock lastBlock = m_mediator.m_dsBlockChain.GetLastBlock();
        blockNum = lastBlock.GetHeader().GetBlockNum() + 1;
        difficulty = lastBlock.GetHeader().GetDifficulty();
    }

    // Assemble DS block
    {
        lock_guard<mutex> g(m_mutexPendingDSBlock);
        // TODO: Handle exceptions.
        m_pendingDSBlock.reset(
            new DSBlock(DSBlockHeader(difficulty, prevHash, winnerNonce,
                                      winnerKey, m_mediator.m_selfKey.second,
                                      blockNum, get_time_as_int()),
                        CoSignatures()));
    }

    LOG_EPOCH(INFO, to_string(m_mediator.m_currentEpochNum).c_str(),
              "New DSBlock created with chosen nonce = 0x" << hex
                                                           << winnerNonce);
}

void DirectoryService::ComputeSharding()
{
    // Aggregate validated PoW2 submissions into m_allPoWs and m_allPoWConns

    LOG_MARKER();

    m_shards.clear();
    m_publicKeyToShardIdMap.clear();

    uint32_t numOfComms = m_allPoW2s.size() / COMM_SIZE;

    if (numOfComms == 0)
    {
        LOG_GENERAL(WARNING,
                    "Zero Pow2 collected, numOfComms is temporarlly set to 1");
        numOfComms = 1;
    }

    for (unsigned int i = 0; i < numOfComms; i++)
    {
        m_shards.push_back(map<PubKey, Peer>());
    }

    for (auto& kv : m_allPoW2s)
    {
        PubKey key = kv.first;
        uint256_t nonce = kv.second;
        // sort all PoW2 submissions according to H(nonce, pubkey)
        SHA2<HASH_TYPE::HASH_VARIANT_256> sha2;
        vector<unsigned char> hashVec;
        hashVec.resize(POW_SIZE + PUB_KEY_SIZE);
        Serializable::SetNumber<uint256_t>(hashVec, 0, nonce, UINT256_SIZE);
        key.Serialize(hashVec, POW_SIZE);
        sha2.Update(hashVec);
        const vector<unsigned char>& sortHashVec = sha2.Finalize();
        array<unsigned char, BLOCK_HASH_SIZE> sortHash;
        copy(sortHashVec.begin(), sortHashVec.end(), sortHash.begin());
        m_sortedPoW2s.insert(make_pair(sortHash, key));
    }

    lock_guard<mutex> g(m_mutexAllPoWConns, adopt_lock);
    unsigned int i = 0;
    for (auto& kv : m_sortedPoW2s)
    {
        PubKey key = kv.second;
        map<PubKey, Peer>& shard = m_shards.at(i % numOfComms);
        shard.insert(make_pair(key, m_allPoWConns.at(key)));
        m_publicKeyToShardIdMap.insert(make_pair(key, i % numOfComms));
        i++;
    }
}

bool DirectoryService::RunConsensusOnDSBlockWhenDSPrimary()
{
    LOG_MARKER();

    LOG_EPOCH(
        INFO, to_string(m_mediator.m_currentEpochNum).c_str(),
        "I am the leader DS node. Creating DS block and sharding structure.");

    // DS leader will announce: DSBlock + Sharding structure + Txn sharing assignments
    ComposeDSBlock();

    // Populate the list of nodes to include in the shards
    {
        lock_guard<mutex> g(m_mutexAllPOW1);

        if (m_mode != IDLE)
        {
            // Copy POW1 to POW2
            lock(m_mutexAllPOW2, m_mutexAllPoWConns);
            lock_guard<mutex> g2(m_mutexAllPOW2, adopt_lock);
            lock_guard<mutex> g3(m_mutexAllPoWConns, adopt_lock);
            m_allPoW2s.clear();

            Peer winnerpeer = m_allPoWConns.at(
                m_pendingDSBlock->GetHeader().GetMinerPubKey());

            for (auto const& i : m_allPoW1s)
            {
                // Winner will become DS (leader), thus we should not put in POW2
                if (m_allPoWConns[i.first] == winnerpeer)
                {
                    continue;
                }

                m_allPoW2s.emplace(i.first, i.second);
            }

            // Add the previous DS committee's oldest member, because it will be back to being a normal node and should be collected here
            lock_guard<mutex> g4(m_mediator.m_mutexDSCommittee);
            m_allPoW2s.emplace(m_mediator.m_DSCommittee.back().first,
                               (boost::multiprecision::uint256_t){1});
            m_allPoWConns.emplace(m_mediator.m_DSCommittee.back().first,
                                  m_mediator.m_DSCommittee.back().second);
        }

        m_allPoW1s.clear();
    }

    ComputeSharding();

    // Create new consensus object
    // Dummy values for now
    uint32_t consensusID = 0;
    m_consensusBlockHash.resize(BLOCK_HASH_SIZE);
    fill(m_consensusBlockHash.begin(), m_consensusBlockHash.end(), 0x77);

    // kill first ds leader (used for view change testing)
    /**
    if (m_consensusMyID == 0 && m_viewChangeCounter < 1)
    {
        LOG_GENERAL(INFO, "I am killing myself to test view change");
        throw exception();
    }
    **/

    m_consensusObject.reset(new ConsensusLeader(
        consensusID, m_consensusBlockHash, m_consensusMyID,
        m_mediator.m_selfKey.first, m_mediator.m_DSCommittee,
        static_cast<unsigned char>(DIRECTORY),
        static_cast<unsigned char>(DSBLOCKCONSENSUS),
        std::function<bool(const vector<unsigned char>&, unsigned int,
                           const Peer&)>(),
        std::function<bool(map<unsigned int, vector<unsigned char>>)>()));

    if (m_consensusObject == nullptr)
    {
        LOG_EPOCH(WARNING, to_string(m_mediator.m_currentEpochNum).c_str(),
                  "WARNING: Unable to create consensus object");
        return false;
    }

    ConsensusLeader* cl
        = dynamic_cast<ConsensusLeader*>(m_consensusObject.get());

    m_DSBlockConsensusRawMessage.clear();

    {
        lock_guard<mutex> g(m_mutexPendingDSBlock);

        unsigned int curr_offset = 0;

        // DS Block
        m_pendingDSBlock->Serialize(m_DSBlockConsensusRawMessage, curr_offset);
        curr_offset += m_DSBlockConsensusRawMessage.size();

        // Sharding structure
        curr_offset += DSBlockConsensusMessage::SerializeShardingStructure(
            m_shards, m_DSBlockConsensusRawMessage, curr_offset);

        // Txn sharing assignments
        DSBlockConsensusMessage::ComputeAndSerializeTxnSharingAssignments(
            m_mediator.m_DSCommittee, m_consensusMyID, m_mediator.m_selfPeer,
            m_shards, m_sharingAssignment, m_DSBlockConsensusRawMessage,
            curr_offset);
    }

    LOG_STATE("[DSCON][" << std::setw(15) << std::left
                         << m_mediator.m_selfPeer.GetPrintableIPAddress()
                         << "][" << m_mediator.m_txBlockChain.GetBlockCount()
                         << "] BGIN");

    cl->StartConsensus(m_DSBlockConsensusRawMessage, DSBlockHeader::SIZE);

    return true;
}

bool DirectoryService::DSBlockValidator(const vector<unsigned char>& msg,
                                        std::vector<unsigned char>& errorMsg)
{
    LOG_MARKER();

    // TODO: Put in the logic here for checking the proposed DS block

    lock(m_mutexPendingDSBlock, m_mutexAllPoWConns);
    lock_guard<mutex> g(m_mutexPendingDSBlock, adopt_lock);
    lock_guard<mutex> g2(m_mutexAllPoWConns, adopt_lock);

    unsigned int curr_offset = 0;
    bool result = false;

    // DS Block
    m_pendingDSBlock.reset(new DSBlock(msg, 0));
    curr_offset = m_pendingDSBlock->GetSerializedSize();

    // Populate the list of nodes we expect to be included in the shards
    {
        // Copy POW1 to POW2
        lock(m_mutexAllPOW1, m_mutexAllPOW1);
        lock_guard<mutex> g3(m_mutexAllPOW1, adopt_lock);
        lock_guard<mutex> g4(m_mutexAllPOW1, adopt_lock);

        m_allPoW2s.clear();
        Peer winnerpeer
            = m_allPoWConns.at(m_pendingDSBlock->GetHeader().GetMinerPubKey());

        for (auto const& i : m_allPoW1s)
        {
            // Winner will become DS (leader), thus we should not put in POW2
            if (m_allPoWConns[i.first] == winnerpeer)
            {
                continue;
            }

            m_allPoW2s.emplace(i.first, i.second);
        }

        // Add the previous DS committee's oldest member, because it will be back to being a normal node and should be collected here
        lock_guard<mutex> g5(m_mediator.m_mutexDSCommittee);
        m_allPoW2s.emplace(m_mediator.m_DSCommittee.back().first,
                           (boost::multiprecision::uint256_t){1});
        m_allPoWConns.emplace(m_mediator.m_DSCommittee.back().first,
                              m_mediator.m_DSCommittee.back().second);

        m_allPoW1s.clear();
    }

    // Sharding structure
    result = DSBlockConsensusMessage::DeserializeAndValidateShardingStructure(
        msg, curr_offset, m_allPoWConns, m_shards, m_publicKeyToShardIdMap);
    if (result == false)
    {
        LOG_EPOCH(
            WARNING, to_string(m_mediator.m_currentEpochNum).c_str(),
            "DeserializeAndValidateShardingStructure failed! What to do?");
    }

    // Txn sharing assignments
    result
        = DSBlockConsensusMessage::DeserializeAndValidateTxnSharingAssignments(
            msg, curr_offset, m_mediator.m_DSCommittee, m_mediator.m_selfPeer,
            m_sharingAssignment);
    if (result == false)
    {
        LOG_EPOCH(
            WARNING, to_string(m_mediator.m_currentEpochNum).c_str(),
            "DeserializeAndValidateTxnSharingAssignments failed! What to do?");
    }

    if (m_allPoWConns.find(m_pendingDSBlock->GetHeader().GetMinerPubKey())
        == m_allPoWConns.end())
    {
        LOG_EPOCH(WARNING, to_string(m_mediator.m_currentEpochNum).c_str(),
                  "Winning node of PoW1 not inside m_allPoWConns! What to do?");
        result = false;
    }

    if (result == false)
    {
        m_DSBlockConsensusRawMessage.clear();
    }
    else
    {
        m_DSBlockConsensusRawMessage = msg;
    }

    return result;
}

bool DirectoryService::RunConsensusOnDSBlockWhenDSBackup()
{
    LOG_MARKER();

    LOG_EPOCH(INFO, to_string(m_mediator.m_currentEpochNum).c_str(),
              "I am a backup DS node. Waiting for DS block and sharding "
              "structure announcement.");

    // Dummy values for now
    uint32_t consensusID = 0x0;
    m_consensusBlockHash.resize(BLOCK_HASH_SIZE);
    fill(m_consensusBlockHash.begin(), m_consensusBlockHash.end(), 0x77);

    auto func = [this](const vector<unsigned char>& message,
                       vector<unsigned char>& errorMsg) mutable -> bool {
        return DSBlockValidator(message, errorMsg);
    };

    m_consensusObject.reset(new ConsensusBackup(
        consensusID, m_consensusBlockHash, m_consensusMyID, m_consensusLeaderID,
        m_mediator.m_selfKey.first, m_mediator.m_DSCommittee,
        static_cast<unsigned char>(DIRECTORY),
        static_cast<unsigned char>(DSBLOCKCONSENSUS), func));

    if (m_consensusObject == nullptr)
    {
        LOG_EPOCH(WARNING, to_string(m_mediator.m_currentEpochNum).c_str(),
                  "Unable to create consensus object");
        return false;
    }

    return true;
}

void DirectoryService::RunConsensusOnDSBlock(bool isRejoin)
{
    LOG_MARKER();

    SetState(DSBLOCK_CONSENSUS_PREP);

    {
        lock_guard<mutex> g(m_mutexAllPOW1);
        LOG_EPOCH(INFO, to_string(m_mediator.m_currentEpochNum).c_str(),
                  "Num of PoW1 sub rec: " << m_allPoW1s.size());
        LOG_STATE("[POW1R][" << std::setw(15) << std::left
                             << m_mediator.m_selfPeer.GetPrintableIPAddress()
                             << "][" << m_allPoW1s.size() << "] ");

        if (m_allPoW1s.size() == 0)
        {
            LOG_EPOCH(WARNING, to_string(m_mediator.m_currentEpochNum).c_str(),
                      "To-do: Code up the logic for if we didn't get any "
                      "submissions at all");
            // throw exception();
            if (!isRejoin)
            {
                return;
            }
        }
    }

    if (m_mode == PRIMARY_DS)
    {
        if (!RunConsensusOnDSBlockWhenDSPrimary())
        {
            LOG_GENERAL(WARNING,
                        "Error after RunConsensusOnDSBlockWhenDSPrimary");
            return;
        }
    }
    else
    {
        if (!RunConsensusOnDSBlockWhenDSBackup())
        {
            LOG_GENERAL(WARNING,
                        "Error after RunConsensusOnDSBlockWhenDSBackup");
            return;
        }
    }

    SetState(DSBLOCK_CONSENSUS);

    if (m_mode != PRIMARY_DS)
    {
        std::unique_lock<std::mutex> cv_lk(m_MutexCVViewChangeDSBlock);
        if (cv_viewChangeDSBlock.wait_for(cv_lk,
                                          std::chrono::seconds(VIEWCHANGE_TIME))
            == std::cv_status::timeout)
        {
            //View change.
            LOG_EPOCH(INFO, to_string(m_mediator.m_currentEpochNum).c_str(),
                      "Initiated DS block view change. ");
            auto func = [this]() -> void { RunConsensusOnViewChange(); };
            DetachedFunction(1, func);
        }
    }
}

#endif // IS_LOOKUP_NODE
