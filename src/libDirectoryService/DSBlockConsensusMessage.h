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

#ifndef __DSBLOCKCONSENSUSMESSAGE_H__
#define __DSBLOCKCONSENSUSMESSAGE_H__

#include "libCrypto/Schnorr.h"
#include "libNetwork/Peer.h"
#include <deque>
#include <map>
#include <utility>
#include <vector>

class DSBlockConsensusMessage
{
public:
    // Sharding structure message format:
    // [4-byte num of committees]
    // [4-byte committee size]
    //   [33-byte public key] [16-byte ip] [4-byte port]
    //   [33-byte public key] [16-byte ip] [4-byte port]
    //   ...
    // [4-byte committee size]
    //   [33-byte public key] [16-byte ip] [4-byte port]
    //   [33-byte public key] [16-byte ip] [4-byte port]
    //   ...

    static unsigned int SerializeShardingStructure(
        const std::vector<std::map<PubKey, Peer>>& shards,
        std::vector<unsigned char>& output, unsigned int curr_offset)
    {
        LOG_MARKER();

        uint32_t numOfComms = shards.size();

        // 4-byte num of committees
        Serializable::SetNumber<uint32_t>(output, curr_offset, numOfComms,
                                          sizeof(uint32_t));
        curr_offset += sizeof(uint32_t);

        LOG_GENERAL(INFO, "Number of committees = " << numOfComms);

        for (unsigned int i = 0; i < numOfComms; i++)
        {
            const std::map<PubKey, Peer>& shard = shards.at(i);

            // 4-byte committee size
            Serializable::SetNumber<uint32_t>(output, curr_offset, shard.size(),
                                              sizeof(uint32_t));
            curr_offset += sizeof(uint32_t);

            LOG_GENERAL(INFO,
                        "Committee size = " << shard.size() << "\n"
                                            << "Members:");

            for (auto& kv : shard)
            {
                // 33-byte public key
                kv.first.Serialize(output, curr_offset);
                curr_offset += PUB_KEY_SIZE;

                // 16-byte ip + 4-byte port
                kv.second.Serialize(output, curr_offset);
                curr_offset += IP_SIZE + PORT_SIZE;

                LOG_GENERAL(
                    INFO,
                    " PubKey = "
                        << DataConversion::SerializableToHexStr(kv.first)
                        << " at " << kv.second.GetPrintableIPAddress()
                        << " Port: " << kv.second.m_listenPortHost);
            }
        }

        return curr_offset;
    }

    static bool DeserializeAndValidateShardingStructure(
        const std::vector<unsigned char>& input, unsigned int& curr_offset,
        const std::map<PubKey, Peer>& allPoWConns,
        std::vector<std::map<PubKey, Peer>>& shards,
        std::map<PubKey, uint32_t>& publicKeyToShardIdMap)
    {
        LOG_MARKER();

        shards.clear();
        publicKeyToShardIdMap.clear();

        // 4-byte num of committees
        uint32_t numOfComms = Serializable::GetNumber<uint32_t>(
            input, curr_offset, sizeof(uint32_t));
        curr_offset += sizeof(uint32_t);

        LOG_GENERAL(INFO, "Number of committees = " << numOfComms);

        for (unsigned int i = 0; i < numOfComms; i++)
        {
            shards.push_back(std::map<PubKey, Peer>());

            // 4-byte committee size
            uint32_t shard_size = Serializable::GetNumber<uint32_t>(
                input, curr_offset, sizeof(uint32_t));
            curr_offset += sizeof(uint32_t);

            LOG_GENERAL(INFO, "Committee size = " << shard_size);
            LOG_GENERAL(INFO, "Members:");

            for (unsigned int j = 0; j < shard_size; j++)
            {
                PubKey memberPubkey(input, curr_offset);
                curr_offset += PUB_KEY_SIZE;

                auto expectedMemberPeer = allPoWConns.find(memberPubkey);
                if (expectedMemberPeer == allPoWConns.end())
                {
                    LOG_GENERAL(
                        WARNING,
                        "Shard node not inside allPoWConns. "
                            << expectedMemberPeer->second
                                   .GetPrintableIPAddress()
                            << " Port: "
                            << expectedMemberPeer->second.m_listenPortHost);
                    return false;
                }

                Peer deserializedMemberPeer(input, curr_offset);
                curr_offset += IP_SIZE + PORT_SIZE;

                if (expectedMemberPeer->second != deserializedMemberPeer)
                {
                    LOG_GENERAL(
                        WARNING,
                        "Deserialized node IP info different from node IP info "
                        "in allPoWConns. Expected: "
                            << expectedMemberPeer->second
                                   .GetPrintableIPAddress()
                            << " Port "
                            << expectedMemberPeer->second.m_listenPortHost
                            << ". Actual: "
                            << deserializedMemberPeer.GetPrintableIPAddress()
                            << " Port "
                            << deserializedMemberPeer.m_listenPortHost);
                    return false;
                }

                // TODO: Should we check for a public key that's been assigned to more than 1 shard?
                shards.back().insert(
                    std::make_pair(memberPubkey, expectedMemberPeer->second));
                publicKeyToShardIdMap.insert(std::make_pair(memberPubkey, i));

                LOG_GENERAL(
                    INFO,
                    " PubKey = "
                        << DataConversion::SerializableToHexStr(memberPubkey)
                        << " at "
                        << expectedMemberPeer->second.GetPrintableIPAddress()
                        << " Port: "
                        << expectedMemberPeer->second.m_listenPortHost);
            }
        }

        return true;
    }

    // Transaction body sharing setup
    // Everyone (DS and non-DS) needs to remember their sharing assignments for this particular DS epoch

    // Transaction body sharing assignments:
    // PART 1. Select X random nodes from DS committee for receiving Tx bodies and broadcasting to other DS nodes
    // PART 2. Select X random nodes per shard for receiving Tx bodies and broadcasting to other nodes in the shard
    // PART 3. Select X random nodes per shard for sending Tx bodies to the receiving nodes in other committees (DS and shards)

    // Message format:
    // [4-byte num of DS nodes]
    //   [16-byte IP] [4-byte port]
    //   [16-byte IP] [4-byte port]
    //   ...
    // [4-byte num of committees]
    // [4-byte num of committee receiving nodes]
    //   [16-byte IP] [4-byte port]
    //   [16-byte IP] [4-byte port]
    //   ...
    // [4-byte num of committee sending nodes]
    //   [16-byte IP] [4-byte port]
    //   [16-byte IP] [4-byte port]
    //   ...
    // [4-byte num of committee receiving nodes]
    //   [16-byte IP] [4-byte port]
    //   [16-byte IP] [4-byte port]
    //   ...
    // [4-byte num of committee sending nodes]
    //   [16-byte IP] [4-byte port]
    //   [16-byte IP] [4-byte port]
    //   ...
    // ...

    static unsigned int ComputeAndSerializeTxnSharingAssignments(
        const std::deque<std::pair<PubKey, Peer>>& DSCommittee,
        uint16_t consensusMyID, const Peer& selfPeer,
        const std::vector<std::map<PubKey, Peer>>& shards,
        std::vector<Peer> sharingAssignment, std::vector<unsigned char>& output,
        unsigned int curr_offset)
    {
        LOG_MARKER();

        // PART 1
        // First version: We just take the first X nodes in DS committee
        LOG_GENERAL(INFO,
                    "DS committee size = " << DSCommittee.size()
                                           << " TX sharing cluster size = "
                                           << TX_SHARING_CLUSTER_SIZE);

        // 4-byte num of DS nodes
        uint32_t num_ds_nodes = (DSCommittee.size() < TX_SHARING_CLUSTER_SIZE)
            ? DSCommittee.size()
            : TX_SHARING_CLUSTER_SIZE;
        Serializable::SetNumber<uint32_t>(output, curr_offset, num_ds_nodes,
                                          sizeof(uint32_t));
        curr_offset += sizeof(uint32_t);
        LOG_GENERAL(INFO,
                    "Forwarders inside the DS committee (" << num_ds_nodes
                                                           << "):");

        // 16-byte IP and 4-byte port
        for (unsigned int i = 0; i < consensusMyID; i++)
        {
            DSCommittee.at(i).second.Serialize(output, curr_offset);
            LOG_GENERAL(INFO, DSCommittee.at(i).second);
            curr_offset += IP_SIZE + PORT_SIZE;
        }

        // when i == consensusMyID use m_mediator.m_selfPeer since IP/port in DSCommitteeNetworkInfo.at(consensusMyID) is zeroed out
        selfPeer.Serialize(output, curr_offset);
        LOG_GENERAL(INFO, selfPeer);
        curr_offset += IP_SIZE + PORT_SIZE;

        for (unsigned int i = consensusMyID + 1; i < num_ds_nodes; i++)
        {
            DSCommittee.at(i).second.Serialize(output, curr_offset);
            LOG_GENERAL(INFO, DSCommittee.at(i).second);
            curr_offset += IP_SIZE + PORT_SIZE;
        }

        // 4-byte num of committees
        LOG_GENERAL(INFO, "Number of shards: " << shards.size());

        Serializable::SetNumber<uint32_t>(
            output, curr_offset, (uint32_t)shards.size(), sizeof(uint32_t));
        curr_offset += sizeof(uint32_t);

        // PART 2 and 3
        // First version: We just take the first X nodes for receiving and next X nodes for sending
        for (unsigned int i = 0; i < shards.size(); i++)
        {
            const std::map<PubKey, Peer>& shard = shards.at(i);

            LOG_GENERAL(INFO, "Shard " << i << " forwarders:");

            // PART 2
            uint32_t nodes_recv_lo = 0;
            uint32_t nodes_recv_hi
                = nodes_recv_lo + TX_SHARING_CLUSTER_SIZE - 1;
            if (nodes_recv_hi >= shard.size())
            {
                nodes_recv_hi = shard.size() - 1;
            }

            unsigned int num_nodes = nodes_recv_hi - nodes_recv_lo + 1;

            // 4-byte num of committee receiving nodes
            Serializable::SetNumber<uint32_t>(output, curr_offset, num_nodes,
                                              sizeof(uint32_t));
            curr_offset += sizeof(uint32_t);

            std::map<PubKey, Peer>::const_iterator node_peer = shard.begin();
            for (unsigned int j = 0; j < num_nodes; j++)
            {
                // 16-byte IP and 4-byte port
                node_peer->second.Serialize(output, curr_offset);
                curr_offset += IP_SIZE + PORT_SIZE;

                LOG_GENERAL(INFO, node_peer->second);

                node_peer++;
            }

            LOG_GENERAL(INFO, "Shard " << i << " senders:");

            // PART 3
            uint32_t nodes_send_lo = 0;
            uint32_t nodes_send_hi = 0;

            if (shard.size() <= TX_SHARING_CLUSTER_SIZE)
            {
                nodes_send_lo = nodes_recv_lo;
                nodes_send_hi = nodes_recv_hi;
            }
            else if (shard.size() < (2 * TX_SHARING_CLUSTER_SIZE))
            {
                nodes_send_lo = shard.size() - TX_SHARING_CLUSTER_SIZE;
                nodes_send_hi = nodes_send_lo + TX_SHARING_CLUSTER_SIZE - 1;
            }
            else
            {
                nodes_send_lo = TX_SHARING_CLUSTER_SIZE;
                nodes_send_hi = nodes_send_lo + TX_SHARING_CLUSTER_SIZE - 1;
            }

            num_nodes = nodes_send_hi - nodes_send_lo + 1;

            LOG_GENERAL(INFO, "DEBUG lo " << nodes_send_lo);
            LOG_GENERAL(INFO, "DEBUG hi " << nodes_send_hi);
            LOG_GENERAL(INFO, "DEBUG num_nodes " << num_nodes);

            // 4-byte num of committee sending nodes
            Serializable::SetNumber<uint32_t>(output, curr_offset, num_nodes,
                                              sizeof(uint32_t));
            curr_offset += sizeof(uint32_t);

            node_peer = shard.begin();
            advance(node_peer, nodes_send_lo);

            for (unsigned int j = 0; j < num_nodes; j++)
            {
                // 16-byte IP and 4-byte port
                node_peer->second.Serialize(output, curr_offset);
                curr_offset += IP_SIZE + PORT_SIZE;

                LOG_GENERAL(INFO, node_peer->second);

                node_peer++;
            }
        }

        // For this version, DS leader is part of the X nodes to receive and share Tx bodies
        sharingAssignment.clear();

        for (unsigned int i = num_ds_nodes; i < DSCommittee.size(); i++)
        {
            sharingAssignment.push_back(DSCommittee.at(i).second);
        }

        return curr_offset;
    }

    static bool DeserializeAndValidateTxnSharingAssignments(
        const std::vector<unsigned char>& input, unsigned int& curr_offset,
        const std::deque<std::pair<PubKey, Peer>>& DSCommittee,
        const Peer& selfPeer, std::vector<Peer> sharingAssignment)
    {
        // TODO: Put in the logic here for checking the sharing configuration

        // 4-byte num of DS nodes
        uint32_t num_ds_nodes = Serializable::GetNumber<uint32_t>(
            input, curr_offset, sizeof(uint32_t));
        curr_offset += sizeof(uint32_t);

        LOG_GENERAL(INFO,
                    "Forwarders inside the DS committee (" << num_ds_nodes
                                                           << "):");

        std::vector<Peer> ds_receivers;

        bool i_am_forwarder = false;
        for (uint32_t i = 0; i < num_ds_nodes; i++)
        {
            // TODO: Handle exceptions
            ds_receivers.push_back(Peer(input, curr_offset));
            curr_offset += IP_SIZE + PORT_SIZE;

            LOG_GENERAL(INFO,
                        "  IP: " << ds_receivers.back().GetPrintableIPAddress()
                                 << " Port: "
                                 << ds_receivers.back().m_listenPortHost);

            if (ds_receivers.back() == selfPeer)
            {
                i_am_forwarder = true;
            }
        }

        sharingAssignment.clear();

        if ((i_am_forwarder == true) && (DSCommittee.size() > num_ds_nodes))
        {
            for (unsigned int i = 0; i < DSCommittee.size(); i++)
            {
                bool is_a_receiver = false;

                if (num_ds_nodes > 0)
                {
                    for (unsigned int j = 0; j < ds_receivers.size(); j++)
                    {
                        if (DSCommittee.at(i).second == ds_receivers.at(j))
                        {
                            is_a_receiver = true;
                            break;
                        }
                    }
                    num_ds_nodes--;
                }

                if (is_a_receiver == false)
                {
                    sharingAssignment.push_back(DSCommittee.at(i).second);
                }
            }
        }

        return true;
    }
};

#endif // __DSBLOCKCONSENSUSMESSAGE_H__