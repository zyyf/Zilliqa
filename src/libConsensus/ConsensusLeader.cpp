/**
* Copyright (c) 2018 Zilliqa 
* This is an alpha (internal) release and is not suitable for production.
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

#include "ConsensusLeader.h"
#include "common/Constants.h"
#include "common/Messages.h"
#include "libUtils/Logger.h"
#include "libUtils/DataConversion.h"
#include "libUtils/DetachedFunction.h"
#include "libNetwork/P2PComm.h"

using namespace std;

bool ConsensusLeader::CheckStateMain(Action action)
{
    bool result = true;

    switch(action)
    {
        case SEND_ANNOUNCEMENT:
            switch(m_state)
            {
                case INITIAL:
                    break;
                case ANNOUNCE_DONE:
                LOG_MESSAGE("Error: Doing announce but announce already done");
                    result = false;
                    break;
                case DONE:
                LOG_MESSAGE("Error: Doing announce but consensus already done");
                    result = false;
                    break;
                case ERROR:
                default:
                LOG_MESSAGE("Error: Unrecognized or error state");
                    result = false;
                    break;
            }
            break;
        case PROCESS_COMMIT:
            switch(m_state)
            {
                case INITIAL:
                LOG_MESSAGE("Error: Processing commit but announce not yet done");
                    result = false;
                    break;
                case ANNOUNCE_DONE:
                    break;
                case DONE:
                LOG_MESSAGE("Error: Processing commit but consensus already done");
                    result = false;
                    break;
                case ERROR:
                default:
                LOG_MESSAGE("Error: Unrecognized or error state");
                    result = false;
                    break;
            }
            break;
        default:
        LOG_MESSAGE("Error: Unrecognized action");
            result = false;
            break;
    }

    return result;
}

bool ConsensusLeader::CheckStateSubset(Action action, uint8_t subset_id)
{
    bool result = true;

    ConsensusSubset & subset = m_consensusSubsets.at(subset_id);

    switch(action)
    {
        case PROCESS_RESPONSE:
            switch(subset.m_state)
            {
                case INITIAL:
                LOG_MESSAGE("Error: Processing response but announce not yet done");
                    result = false;
                    break;
                case ANNOUNCE_DONE:
                LOG_MESSAGE("Error: Processing response but challenge not yet done");
                    result = false;
                    break;
                case CHALLENGE_DONE:
                    break;
                case COLLECTIVESIG_DONE:
                LOG_MESSAGE("Error: Processing response but collectivesig already done");
                    result = false;
                    break;
                case FINALCHALLENGE_DONE:
                LOG_MESSAGE("Error: Processing response but finalchallenge already done");
                    result = false;
                    break;
                case DONE:
                LOG_MESSAGE("Error: Processing response but consensus already done");
                    result = false;
                    break;
                case ERROR:
                default:
                LOG_MESSAGE("Error: Unrecognized or error state");
                    result = false;
                    break;
            }
            break;
        case PROCESS_FINALCOMMIT:
            switch(subset.m_state)
            {
                case INITIAL:
                LOG_MESSAGE("Error: Processing finalcommit but announce not yet done");
                    result = false;
                    break;
                case ANNOUNCE_DONE:
                LOG_MESSAGE("Error: Processing finalcommit but challenge not yet done");
                    result = false;
                    break;
                case CHALLENGE_DONE:
                LOG_MESSAGE("Error: Processing finalcommit but collectivesig not yet done");
                    result = false;
                    break;
                case COLLECTIVESIG_DONE:
                    break;
                case FINALCHALLENGE_DONE:
                LOG_MESSAGE("Error: Processing finalcommit but finalchallenge already done");
                    // LOG_MESSAGE("Processing redundant finalcommit messages");
                    result = false;
                    break;
                case DONE:
                LOG_MESSAGE("Error: Processing finalcommit but consensus already done");
                    result = false;
                    break;
                case ERROR:
                default:
                LOG_MESSAGE("Error: Unrecognized or error state");
                    result = false;
                    break;
            }
            break;
        case PROCESS_FINALRESPONSE:
            switch(subset.m_state)
            {
                case INITIAL:
                LOG_MESSAGE("Error: Processing finalresponse but announce not yet done");
                    result = false;
                    break;
                case ANNOUNCE_DONE:
                LOG_MESSAGE("Error: Processing finalresponse but challenge not yet done");
                    result = false;
                    break;
                case CHALLENGE_DONE:
                LOG_MESSAGE("Error: Processing finalresponse but collectivesig not yet done");
                    result = false;
                    break;
                case COLLECTIVESIG_DONE:
                LOG_MESSAGE("Error: Processing finalresponse but finalchallenge not yet done");
                    result = false;
                    break;
                case FINALCHALLENGE_DONE:
                    break;
                case DONE:
                LOG_MESSAGE("Error: Processing finalresponse but consensus already done");
                    result = false;
                    break;
                case ERROR:
                default:
                LOG_MESSAGE("Error: Unrecognized or error state");
                    result = false;
                    break;
            }
            break;
        default:
        LOG_MESSAGE("Error: Unrecognized action");
            result = false;
            break;
    }

    return result;
}

void ConsensusLeader::GenerateConsensusLists()
{
    {
        lock_guard<mutex> g(m_mutex);

        // Get the list of all the peers who committed, by peer index
        vector<unsigned int> peersWhoCommitted;
        for (unsigned int index = 0; index < m_commitPointMap.size(); index++)
        {
            if (m_commitPointMap.at(index).Initialized())
            {
                peersWhoCommitted.push_back(index);
            }
        }

        // Generate NUM_CONSENSUS_SETS lists = subsets of peersWhoCommitted
        for (unsigned int i = 0; i < NUM_CONSENSUS_SETS; i++)
        {
            m_consensusSubsets.push_back(ConsensusSubset());

            ConsensusSubset & subset = m_consensusSubsets.back();
            subset.m_commitMap.resize(m_pubKeys.size());
            subset.m_commitPointMap.resize(m_pubKeys.size());
            subset.m_commitPoints.clear();
            subset.m_responseData.clear();
            subset.m_state = ANNOUNCE_DONE;

            for (unsigned int j = 0; j < m_numForConsensus; j++)
            {
                unsigned int index = peersWhoCommitted.at(j);
                subset.m_commitMap.at(index) = true;
                subset.m_commitPointMap.at(index) = m_commitPointMap.at(index);
                subset.m_commitPoints.push_back(subset.m_commitPointMap.at(index));
            }

            random_shuffle(peersWhoCommitted.begin(), peersWhoCommitted.end());
        }

        // Clear out the original commit map, we don't need it anymore at this point
        m_commitPointMap.clear();
    }

    lock_guard<mutex> g(m_commitProcessingStateMutex);    
    m_commitProcessingState = COMMIT_LISTS_GENERATED;
}

void ConsensusLeader::InitiateConsensusListChallenges()
{
    for (unsigned int index = 0; index < m_consensusSubsets.size(); index++)
    {
        ConsensusSubset & subset = m_consensusSubsets.at(index);
        vector<unsigned char> challenge = { m_classByte, m_insByte, static_cast<unsigned char>(ConsensusMessageType::CHALLENGE) };
        bool result = GenerateChallengeMessage(challenge, MessageOffset::BODY + sizeof(unsigned char), (uint8_t)index, PROCESS_COMMIT);

        if (result == true)
        {
            // Multicast to all nodes in this subset who sent validated commits
            // ================================================================

            vector<Peer> commit_peers;
            deque<Peer>::const_iterator j = m_peerInfo.begin();
            for (unsigned int i = 0; i < subset.m_commitMap.size(); i++, j++)
            {
                if (subset.m_commitMap.at(i) == true)
                {
                    commit_peers.push_back(*j);
                }
            }
            P2PComm::GetInstance().SendMessage(commit_peers, challenge);

            // Update subset's internal state (Let's avoid holding a mutex here)
            // =================================================================

            subset.m_state = CHALLENGE_DONE;
        }
    }

    // Update overall internal state
    // =============================

    lock_guard<mutex> g(m_mutex);
    m_state = CHALLENGE_DONE;
}

bool ConsensusLeader::ProcessMessageCommitCore(const vector<unsigned char> & commit, unsigned int offset, Action action, ConsensusMessageType returnmsgtype, State nextstate)
{
    LOG_MARKER();

    // Initial checks
    // ==============

    if (!CheckStateMain(action))
    {
        return false;
    }

    // Extract and check commit message body
    // =====================================

    // Format Commit: [4-byte consensus id] [32-byte blockhash] [2-byte backup id] [33-byte commit] [64-byte signature]
    // Format FinalCommit: [4-byte consensus id] [32-byte blockhash] [2-byte backup id] [1-byte subset id] [33-byte commit] [64-byte signature]

    const unsigned int length_available = commit.size() - offset;
    const unsigned int length_needed = sizeof(uint32_t) + BLOCK_HASH_SIZE + sizeof(uint16_t) + (action == PROCESS_FINALCOMMIT ? sizeof(uint8_t) : 0) + COMMIT_POINT_SIZE + SIGNATURE_CHALLENGE_SIZE + SIGNATURE_RESPONSE_SIZE;

    if (length_needed > length_available)
    {
        LOG_MESSAGE("Error: Malformed message");
        return false;
    }

    unsigned int curr_offset = offset;

    // 4-byte consensus id
    uint32_t consensus_id = Serializable::GetNumber<uint32_t>(commit, curr_offset, sizeof(uint32_t));
    curr_offset += sizeof(uint32_t);

    // Check the consensus id
    if (consensus_id != m_consensusID)
    {
        LOG_MESSAGE("Error: Consensus ID in commitment (" << consensus_id << ") does not match instance consensus ID (" << m_consensusID << ")");
        return false;
    }

    // 32-byte blockhash

    // Check the block hash
    if (equal(m_blockHash.begin(), m_blockHash.end(), commit.begin() + curr_offset) == false)
    {
        LOG_MESSAGE("Error: Block hash in commitment does not match instance block hash");
        return false;
    }
    curr_offset += BLOCK_HASH_SIZE;

    // 2-byte backup id
    uint16_t backup_id = Serializable::GetNumber<uint16_t>(commit, curr_offset, sizeof(uint16_t));
    curr_offset += sizeof(uint16_t);

    uint8_t subset_id = 0;

    if (action == PROCESS_COMMIT)
    {
        // Check the backup id
        if (backup_id >= m_commitPointMap.size())
        {
            LOG_MESSAGE("Error: Backup ID beyond backup count");
            return false;
        }
        if (m_commitPointMap.at(backup_id).Initialized() == true)
        {
            LOG_MESSAGE("Error: Backup has already sent validated commit");
            return false;
        }
    }
    else
    {
        // 1-byte subset id
        subset_id = Serializable::GetNumber<uint8_t>(commit, curr_offset, sizeof(uint8_t));
        curr_offset += sizeof(uint8_t);

        // Check subset state
        if (!CheckStateSubset(action, subset_id))
        {
            return false;
        }

        ConsensusSubset & subset = m_consensusSubsets.at(subset_id);

        // Check the backup id
        if (backup_id >= subset.m_commitMap.size())
        {
            LOG_MESSAGE("Error: Backup ID beyond backup count");
            return false;
        }
        if (subset.m_commitMap.at(backup_id) == true)
        {
            LOG_MESSAGE("Error: Backup has already sent validated commit");
            return false;
        }

        // Check if this backup was part of the first round commit
        if (m_responseMapSubsets.at(subset_id).at(backup_id) == false)
        {
            LOG_MESSAGE("Error: Backup has not participated in the commit phase");
            return false;
        }
    }

    // 33-byte commit - skip for now, deserialize later below
    curr_offset += COMMIT_POINT_SIZE;

    // 64-byte signature
    Signature signature(commit, curr_offset);

    // Check the signature
    bool sig_valid = VerifyMessage(commit, offset, curr_offset - offset, signature, backup_id);
    if (sig_valid == false)
    {
        LOG_MESSAGE("Error: Invalid signature in commit message");
        return false;
    }

    {
        lock_guard<mutex> g(m_mutex);

        if (!CheckStateMain(action))
        {
            return false;
        }

        if (action == PROCESS_COMMIT)
        {
            // 33-byte commit
            m_commitPointMap.at(backup_id) = CommitPoint(commit, curr_offset - COMMIT_POINT_SIZE);
            m_commitCounter++;

            if (m_commitCounter % 10 == 0)
            {
                LOG_MESSAGE("Received " << m_commitCounter << " out of " << m_numForConsensus << ".");
            }
        }
        else
        {
            if (!CheckStateSubset(action, subset_id))
            {
                return false;
            }

            ConsensusSubset & subset = m_consensusSubsets.at(subset_id);

            // 33-byte commit
            subset.m_commitPointMap.at(backup_id) = CommitPoint(commit, curr_offset - COMMIT_POINT_SIZE);
            subset.m_commitPoints.push_back(CommitPoint(commit, curr_offset - COMMIT_POINT_SIZE));

            if (subset.m_commitPoints.size() % 10 == 0)
            {
                LOG_MESSAGE("Subset " << subset_id << " - Received " << subset.m_commitPoints.size() << " out of " << m_numForConsensus << ".");
            }

            if (subset.m_commitPoints.size() == subset.m_responseData.size())
            {
                vector<unsigned char> challenge = { m_classByte, m_insByte, static_cast<unsigned char>(ConsensusMessageType::FINALCHALLENGE) };
                bool result = GenerateChallengeMessage(challenge, MessageOffset::BODY + sizeof(unsigned char), subset_id, PROCESS_FINALCOMMIT);

                if (result == true)
                {
                    // Multicast to all nodes in this subset
                    // =====================================

                    vector<Peer> commit_peers;
                    deque<Peer>::const_iterator j = m_peerInfo.begin();
                    for (unsigned int i = 0; i < subset.m_commitMap.size(); i++, j++)
                    {
                        if (subset.m_commitMap.at(i) == true)
                        {
                            commit_peers.push_back(*j);
                        }
                    }
                    P2PComm::GetInstance().SendMessage(commit_peers, challenge);

                    // Update subset internal state
                    // ============================

                    subset.m_state = FINALCHALLENGE_DONE;

                    subset.m_responseData.clear();
                }
            }
        }
    }

    return true;
}

bool ConsensusLeader::ProcessMessageCommit(const vector<unsigned char> & commit, unsigned int offset)
{
    LOG_MARKER();

    bool processThisCommit = false;
    CommitProcessingState procState;

    {
        lock_guard<mutex> g(m_commitProcessingStateMutex);
        procState = m_commitProcessingState;
    }

    switch(procState)
    {
        case ACCEPTING_COMMITS:
            processThisCommit = true;
            break;
        case COMMIT_TIMER_EXPIRED:
            if (m_commitCounter < m_numForConsensus)
            {
                LOG_MESSAGE("Error: Insufficient commits. Required = " << m_numForConsensus << " Actual = " << m_commitCounter);
                m_state = ERROR;
            }
            else
            {
                LOG_MESSAGE("Sufficient commits obtained. Required = " << m_numForConsensus << " Actual = " << m_commitCounter);
                GenerateConsensusLists();
                InitiateConsensusListChallenges();
            }
            break;
        case COMMIT_LISTS_GENERATED:
        default:
            break;
    }

    if (processThisCommit)
    {
        processThisCommit = ProcessMessageCommitCore(commit, offset, PROCESS_COMMIT, CHALLENGE, CHALLENGE_DONE);
    }

    return processThisCommit;
}


bool ConsensusLeader::GenerateChallengeMessage(vector<unsigned char> & challenge, unsigned int offset, uint8_t subset_id, Action action)
{
    LOG_MARKER();

    // Generate challenge object
    // =========================

    ConsensusSubset & subset = m_consensusSubsets.at(subset_id);

    // Aggregate commits
    CommitPoint aggregated_commit = AggregateCommits(subset.m_commitPoints);
    if (aggregated_commit.Initialized() == false)
    {
        LOG_MESSAGE("Error: AggregateCommits failed");
        m_state = ERROR;
        return false;
    }

    // Aggregate keys
    PubKey aggregated_key = AggregateKeys(subset.m_commitMap);
    if (aggregated_key.Initialized() == false)
    {
        LOG_MESSAGE("Error: Aggregated key generation failed");
        m_state = ERROR;
        return false;
    }

    // Generate the challenge
    if (action == PROCESS_COMMIT)
    {
        subset.m_challenge = GetChallenge(m_message, 0, m_message.size(), aggregated_commit, aggregated_key);
    }
    else
    {
        subset.m_challenge = GetChallenge(subset.m_message, 0, subset.m_message.size(), aggregated_commit, aggregated_key);
    }

    if (subset.m_challenge.Initialized() == false)
    {
        LOG_MESSAGE("Error: Challenge generation failed");
        m_state = ERROR;
        return false;
    }

    // Assemble challenge message body
    // ===============================

    // Format: [4-byte consensus id] [32-byte blockhash] [2-byte leader id] [1-byte subset id] [33-byte aggregated commit] [33-byte aggregated key] [32-byte challenge] [64-byte signature]
    // Signature is over: [4-byte consensus id] [32-byte blockhash] [2-byte leader id] [1-byte subset id] [33-byte aggregated commit] [33-byte aggregated key] [32-byte challenge]

    unsigned int curr_offset = offset;

    // 4-byte consensus id
    Serializable::SetNumber<uint32_t>(challenge, curr_offset, m_consensusID, sizeof(uint32_t));
    curr_offset += sizeof(uint32_t);

    // 32-byte blockhash
    challenge.insert(challenge.begin() + curr_offset, m_blockHash.begin(), m_blockHash.end());
    curr_offset += m_blockHash.size();

    // 2-byte leader id
    Serializable::SetNumber<uint16_t>(challenge, curr_offset, m_myID, sizeof(uint16_t));
    curr_offset += sizeof(uint16_t);

    // 1-byte subset ID
    Serializable::SetNumber<uint8_t>(challenge, curr_offset, subset_id, sizeof(uint8_t));
    curr_offset += sizeof(uint8_t);

    // 33-byte aggregated commit
    aggregated_commit.Serialize(challenge, curr_offset);
    curr_offset += COMMIT_POINT_SIZE;

    // 33-byte aggregated key
    aggregated_key.Serialize(challenge, curr_offset);
    curr_offset += PUB_KEY_SIZE;

    // 32-byte challenge
    subset.m_challenge.Serialize(challenge, curr_offset);
    curr_offset += CHALLENGE_SIZE;

    // 64-byte signature
    Signature signature = SignMessage(challenge, offset, curr_offset - offset);
    if (signature.Initialized() == false)
    {
        LOG_MESSAGE("Error: Message signing failed");
        m_state = ERROR;
        return false;
    }
    signature.Serialize(challenge, curr_offset);

    return true;
}

bool ConsensusLeader::ProcessMessageResponseCore(const vector<unsigned char> & response, unsigned int offset, Action action, ConsensusMessageType returnmsgtype, State nextstate)
{
    LOG_MARKER();

    // Initial checks
    // ==============

    if (!CheckStateMain(action))
    {
        return false;
    }

    // Extract and check response message body
    // =======================================

    // Format: [4-byte consensus id] [32-byte blockhash] [2-byte backup id] [1-byte subset id] [32-byte response] [64-byte signature]

    const unsigned int length_available = response.size() - offset;
    const unsigned int length_needed = sizeof(uint32_t) + BLOCK_HASH_SIZE + sizeof(uint16_t) + sizeof(uint8_t) + RESPONSE_SIZE + SIGNATURE_CHALLENGE_SIZE + SIGNATURE_RESPONSE_SIZE;

    if (length_needed > length_available)
    {
        LOG_MESSAGE("Error: Malformed message");
        return false;
    }

    unsigned int curr_offset = offset;

    // 4-byte consensus id
    uint32_t consensus_id = Serializable::GetNumber<uint32_t>(response, curr_offset, sizeof(uint32_t));
    curr_offset += sizeof(uint32_t);

    // Check the consensus id
    if (consensus_id != m_consensusID)
    {
        LOG_MESSAGE("Error: Consensus ID in response (" << consensus_id << ") does not match instance consensus ID (" << m_consensusID << ")");
        return false;
    }

    // 32-byte blockhash

    // Check the block hash
    if (equal(m_blockHash.begin(), m_blockHash.end(), response.begin() + curr_offset) == false)
    {
        LOG_MESSAGE("Error: Block hash in response does not match instance block hash");
        return false;
    }
    curr_offset += BLOCK_HASH_SIZE;

    // 2-byte backup id
    uint32_t backup_id = Serializable::GetNumber<uint16_t>(response, curr_offset, sizeof(uint16_t));
    curr_offset += sizeof(uint16_t);

    // 1-byte subset id
    uint8_t subset_id = Serializable::GetNumber<uint8_t>(response, curr_offset, sizeof(uint8_t));
    curr_offset += sizeof(uint8_t);

    // Check the subset id
    if (subset_id >= NUM_CONSENSUS_SETS)
    {
        LOG_MESSAGE("Error: Subset ID >= NUM_CONSENSUS_SETS");
        return false;
    }

    // Check subset state
    if (!CheckStateSubset(action, subset_id))
    {
        return false;
    }

    ConsensusSubset & subset = m_consensusSubsets.at(subset_id);
    vector<bool> & subsetResponseMap = m_responseMapSubsets.at(subset_id);

    // Check the backup id
    if (backup_id >= subsetResponseMap.size())
    {
        LOG_MESSAGE("Error: Backup ID beyond backup count");
        return false;
    }
    if (subset.m_commitMap.at(backup_id) == false)
    {
        LOG_MESSAGE("Error: Backup has not participated in the commit phase");
        return false;
    }
    if (subsetResponseMap.at(backup_id) == true)
    {
        LOG_MESSAGE("Error: Backup has already sent validated response");
        return false;
    }

    // 32-byte response
    Response tmp_response = Response(response, curr_offset);
    curr_offset += RESPONSE_SIZE;

    if (MultiSig::VerifyResponse(tmp_response, subset.m_challenge, m_pubKeys.at(backup_id), subset.m_commitPointMap.at(backup_id)) == false)
    {
        LOG_MESSAGE("Error: Invalid response for this backup");
        return false;
    }

    // 64-byte signature
    Signature signature(response, curr_offset);

    // Check the signature
    bool sig_valid = VerifyMessage(response, offset, curr_offset - offset, signature, backup_id);
    if (sig_valid == false)
    {
        LOG_MESSAGE("Error: Invalid signature in response message");
        return false;
    }

    // Update internal state
    // =====================

    lock_guard<mutex> g(m_mutex);

    if (!CheckStateMain(action))
    {
        return false;
    }

    if (!CheckStateSubset(action, subset_id))
    {
        return false;
    }

    // 32-byte response
    subset.m_responseData.push_back(tmp_response);
    subsetResponseMap.at(backup_id) = true;

    // Generate collective sig if everyone has responded
    // =================================================

    bool result = true;

    if (subset.m_responseData.size() == subset.m_commitPoints.size())
    {
        LOG_MESSAGE("All responses obtained for subset " << subset_id);

        vector<unsigned char> collectivesig = { m_classByte, m_insByte, static_cast<unsigned char>(returnmsgtype) };
        result = GenerateCollectiveSigMessage(collectivesig, MessageOffset::BODY + sizeof(unsigned char), subset_id, action);

        if (result == true)
        {
            // Update subset's internal state
            // ==============================

            subset.m_state = nextstate;

            if (action == PROCESS_RESPONSE)
            {
                subset.m_commitPoints.clear();
                fill(subset.m_commitMap.begin(), subset.m_commitMap.end(), false);

                // First round: consensus over message (e.g., DS block)
                // Second round: consensus over collective sig
                m_subsetCollectiveSigs.at(subset_id).Serialize(subset.m_message, 0);

                // Multicast to all nodes in the subset
                // ====================================

                vector<Peer> subset_peers;
                deque<Peer>::const_iterator j = m_peerInfo.begin();
                for (unsigned int i = 0; i < subset.m_commitMap.size(); i++, j++)
                {
                    if (subset.m_commitMap.at(i) == true)
                    {
                        subset_peers.push_back(*j);
                    }
                }

                P2PComm::GetInstance().SendMessage(subset_peers, collectivesig);
            }
            else
            {
                // Update overall state
                // ====================

                m_state = nextstate;
                m_finalSubsetID = subset_id;

                // Multicast to all nodes in the committee
                // =======================================

                P2PComm::GetInstance().SendMessage(m_peerInfo, collectivesig);
            }
        }
    }

    return result;
}

bool ConsensusLeader::ProcessMessageResponse(const vector<unsigned char> & response, unsigned int offset)
{
    LOG_MARKER();
    return ProcessMessageResponseCore(response, offset, PROCESS_RESPONSE, COLLECTIVESIG, COLLECTIVESIG_DONE);
}

bool ConsensusLeader::GenerateCollectiveSigMessage(vector<unsigned char> & collectivesig, unsigned int offset, uint8_t subset_id, Action action)
{
    LOG_MARKER();

    // Generate collective signature object
    // ====================================

    ConsensusSubset & subset = m_consensusSubsets.at(subset_id);
    vector<bool> & subsetResponseMap = m_responseMapSubsets.at(subset_id);
    Signature & subsetCollectiveSig = m_subsetCollectiveSigs.at(subset_id);

    // Aggregate responses
    Response aggregated_response = AggregateResponses(subset.m_responseData);
    if (aggregated_response.Initialized() == false)
    {
        LOG_MESSAGE("Error: AggregateCommits failed");
        m_state = ERROR;
        return false;
    }

    // Aggregate keys
    PubKey aggregated_key = AggregateKeys(subsetResponseMap);
    if (aggregated_key.Initialized() == false)
    {
        LOG_MESSAGE("Error: Aggregated key generation failed");
        m_state = ERROR;
        return false;
    }

    // Generate the collective signature
    subsetCollectiveSig = AggregateSign(subset.m_challenge, aggregated_response);
    if (subsetCollectiveSig.Initialized() == false)
    {
        LOG_MESSAGE("Error: Collective sig generation failed");
        m_state = ERROR;
        return false;
    }

    // Verify the collective signature
    bool result = false;
    if (action == PROCESS_RESPONSE)
    {
        result = Schnorr::GetInstance().Verify(m_message, subsetCollectiveSig, aggregated_key);
    }
    else
    {
        result = Schnorr::GetInstance().Verify(subset.m_message, subsetCollectiveSig, aggregated_key);
    }

    if (result == false)
    {
        LOG_MESSAGE("Error: Collective sig verification failed");
        subset.m_state = ERROR;

        LOG_MESSAGE("num of pub keys: " << m_pubKeys.size())
        LOG_MESSAGE("num of peer_info keys: " << m_peerInfo.size())

        return false;
    }

    // Assemble collective signature message body
    // ==========================================

    // Format: [4-byte consensus id] [32-byte blockhash] [2-byte leader id] [1-byte subset id] [N-byte bitmap] [64-byte collective signature] [64-byte signature]
    // Signature is over: [4-byte consensus id] [32-byte blockhash] [2-byte leader id] [1-byte subset id] [N-byte bitmap] [64-byte collective signature]
    // Note on N-byte bitmap: N = number of bytes needed to represent all nodes (1 bit = 1 node) + 2 (length indicator)

    unsigned int curr_offset = offset;

    // 4-byte consensus id
    Serializable::SetNumber<uint32_t>(collectivesig, curr_offset, m_consensusID, sizeof(uint32_t));
    curr_offset += sizeof(uint32_t);

    // 32-byte blockhash
    collectivesig.insert(collectivesig.begin() + curr_offset, m_blockHash.begin(), m_blockHash.end());
    curr_offset += m_blockHash.size();

    // 2-byte leader id
    Serializable::SetNumber<uint16_t>(collectivesig, curr_offset, m_myID, sizeof(uint16_t));
    curr_offset += sizeof(uint16_t);

    // 1-byte subset id
    Serializable::SetNumber<uint8_t>(collectivesig, curr_offset, subset_id, sizeof(uint8_t));
    curr_offset += sizeof(uint8_t);

    // N-byte bitmap
    curr_offset += SetBitVector(collectivesig, curr_offset, subsetResponseMap);

    // 64-byte collective signature
    subsetCollectiveSig.Serialize(collectivesig, curr_offset);
    curr_offset += SIGNATURE_CHALLENGE_SIZE + SIGNATURE_RESPONSE_SIZE;

    // 64-byte signature
    Signature signature = SignMessage(collectivesig, offset, curr_offset - offset);
    if (signature.Initialized() == false)
    {
        LOG_MESSAGE("Error: Message signing failed");
        m_state = ERROR;
        return false;
    }
    signature.Serialize(collectivesig, curr_offset);

    return true;
}

bool ConsensusLeader::ProcessMessageFinalCommit(const vector<unsigned char> & finalcommit, unsigned int offset)
{
    LOG_MARKER();
    return ProcessMessageCommitCore(finalcommit, offset, PROCESS_FINALCOMMIT, FINALCHALLENGE, FINALCHALLENGE_DONE);
}

bool ConsensusLeader::ProcessMessageFinalResponse(const vector<unsigned char> & finalresponse, unsigned int offset)
{
    LOG_MARKER();
    return ProcessMessageResponseCore(finalresponse, offset, PROCESS_FINALRESPONSE, FINALCOLLECTIVESIG, DONE);
}

ConsensusLeader::ConsensusLeader
(
    uint32_t consensus_id,
    const vector<unsigned char> & block_hash,
    uint16_t node_id,
    const PrivKey & privkey,
    const deque<PubKey> & pubkeys,
    const deque<Peer> & peer_info,
    unsigned char class_byte,
    unsigned char ins_byte
) : ConsensusCommon(consensus_id, block_hash, node_id, privkey, pubkeys, peer_info, class_byte, ins_byte), m_commitPointMap(pubkeys.size(), CommitPoint())
{
    LOG_MARKER();

    const double TOLERANCE_FRACTION = (double) 0.667;

    m_state = INITIAL;
    m_numForConsensus = pubkeys.size() - (ceil(pubkeys.size() * (1 - TOLERANCE_FRACTION)) - 1) - 1;
    LOG_MESSAGE("TOLERANCE_FRACTION " << TOLERANCE_FRACTION << " pubkeys.size() " << pubkeys.size() << " m_numForConsensus " << m_numForConsensus);
}

ConsensusLeader::~ConsensusLeader()
{

}

bool ConsensusLeader::StartConsensus(const vector<unsigned char> & message)
{
    LOG_MARKER();

    // Initial checks
    // ==============

    if (message.size() == 0)
    {
        LOG_MESSAGE("Error: Empty message");
        return false;
    }

    if (!CheckStateMain(SEND_ANNOUNCEMENT))
    {
        return false;
    }

    // Assemble announcement message body
    // ==================================

    // Format: [CLA] [INS] [1-byte consensus message type] [4-byte consensus id] [32-byte blockhash] [2-byte leader id] [message] [64-byte signature]
    // Signature is over: [4-byte consensus id] [32-byte blockhash] [2-byte leader id] [message]

    LOG_MESSAGE("DEBUG: my ip is " << m_peerInfo.at(m_myID).GetPrintableIPAddress());
    LOG_MESSAGE("DEBUG: my pub is " << DataConversion::SerializableToHexStr(m_pubKeys.at(m_myID)) );

    vector<unsigned char> announcement = { m_classByte, m_insByte, static_cast<unsigned char>(ConsensusMessageType::ANNOUNCE) };
    unsigned int curr_offset = MessageOffset::BODY + sizeof(unsigned char);

    // 4-byte consensus id
    Serializable::SetNumber<uint32_t>(announcement, curr_offset, m_consensusID, sizeof(uint32_t));
    curr_offset += sizeof(uint32_t);
    LOG_MESSAGE("DEBUG: consensus id is " << m_consensusID);

    // 32-byte blockhash
    announcement.insert(announcement.begin() + curr_offset, m_blockHash.begin(), m_blockHash.end());
    curr_offset += m_blockHash.size();

    // 2-byte leader id
    Serializable::SetNumber<uint16_t>(announcement, curr_offset, m_myID, sizeof(uint16_t));
    curr_offset += sizeof(uint16_t);
    LOG_MESSAGE("DEBUG: consensus leader id is " << m_myID);

    // message
    announcement.insert(announcement.begin() + curr_offset, message.begin(), message.end());
    curr_offset += message.size();

    // 64-byte signature
    Signature signature = SignMessage(announcement, MessageOffset::BODY + sizeof(unsigned char), curr_offset - MessageOffset::BODY - sizeof(unsigned char));
    if (signature.Initialized() == false)
    {
        LOG_MESSAGE("Error: Message signing failed");
        m_state = ERROR;
        return false;
    }
    signature.Serialize(announcement, curr_offset);

    // Update internal state
    // =====================

    m_state = ANNOUNCE_DONE;
    m_commitCounter = 0;
    m_message = message;

    // Multicast to all nodes in the committee
    // =======================================

    P2PComm::GetInstance().SendMessage(m_peerInfo, announcement);

    // Start timer for accepting commits
    // =================================
    m_commitProcessingState = ACCEPTING_COMMITS;
    unsigned int wait_time = COMMIT_WINDOW_IN_SECONDS;
    auto func = [this, wait_time]() -> void
    {
        this_thread::sleep_for(chrono::seconds(wait_time));
        lock_guard<mutex> g(m_commitProcessingStateMutex);
        m_commitProcessingState = COMMIT_TIMER_EXPIRED;
    };
    DetachedFunction(1, func);

    return true;
}

bool ConsensusLeader::ProcessMessage(const vector<unsigned char> & message, unsigned int offset, 
                                     const Peer & from)
{
    LOG_MARKER();

    // Incoming message format (from offset): [1-byte consensus message type] [consensus message]

    bool result = false;

    switch(message.at(offset))
    {
        case ConsensusMessageType::COMMIT:
            result = ProcessMessageCommit(message, offset + 1);
            break;
        case ConsensusMessageType::RESPONSE:
            result = ProcessMessageResponse(message, offset + 1);
            break;
        case ConsensusMessageType::FINALCOMMIT:
            result = ProcessMessageFinalCommit(message, offset + 1);
            break;
        case ConsensusMessageType::FINALRESPONSE:
            result = ProcessMessageFinalResponse(message, offset + 1);
            break;
        default:
        LOG_MESSAGE("Error: Unknown consensus message received. No: "  << (unsigned int) message.at(offset));
            break;
    }

    return result;
}
