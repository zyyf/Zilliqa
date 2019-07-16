/*
 * Copyright (C) 2019 Zilliqa
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <array>
#include <string>

#include "libCrypto/Sha2.h"
#include "libData/BlockData/Block.h"
#include "libTestUtils/TestUtils.h"
#include "libUtils/DataConversion.h"
#include "libUtils/Logger.h"

#define BOOST_TEST_MODULE blocktest
#include <boost/test/included/unit_test.hpp>

using namespace std;
using namespace boost::multiprecision;

DSBlockHeader dsHeader;

BOOST_AUTO_TEST_SUITE(blocktest)

BOOST_AUTO_TEST_CASE(DSBlock_test) {
  INIT_STDOUT_LOGGER();

  LOG_MARKER();

  std::array<unsigned char, BLOCK_HASH_SIZE> prevHash1;

  for (unsigned int i = 0; i < prevHash1.size(); i++) {
    prevHash1.at(i) = i + 1;
  }

  std::array<unsigned char, PUB_KEY_SIZE> pubKey1;

  for (unsigned int i = 0; i < pubKey1.size(); i++) {
    pubKey1.at(i) = i + 4;
  }

  std::array<unsigned char, BLOCK_SIG_SIZE> signature1;

  for (unsigned int i = 0; i < signature1.size(); i++) {
    signature1.at(i) = i + 8;
  }

  auto leaderPubKey = TestUtils::GenerateRandomPubKey();
  std::map<PubKey, Peer> powDSWinners;
  for (int i = 0; i < 5; ++i) {
    auto pubKey = TestUtils::GenerateRandomPubKey();
    auto peer = TestUtils::GenerateRandomPeer();
    powDSWinners.insert(std::make_pair(pubKey, peer));
  }

  // FIXME: Handle exceptions.
  DSBlockHeader header1(20, 10, leaderPubKey, 123, 12345, 0, SWInfo(),
                        powDSWinners, DSBlockHashSet());

  DSBlock block1(header1, TestUtils::GenerateRandomCoSignatures());

  bytes message1;
  block1.Serialize(message1, 0);
  LOG_PAYLOAD(INFO, "Block1 serialized", message1,
              Logger::MAX_BYTES_TO_DISPLAY);

  DSBlock block2(message1, 0);

  bytes message2;
  block2.Serialize(message2, 0);
  dsHeader = block2.GetHeader();
  LOG_PAYLOAD(INFO, "Block2 serialized", message2,
              Logger::MAX_BYTES_TO_DISPLAY);
  DSBlockHeader header2 = block2.GetHeader();
  uint8_t diff2 = header2.GetDifficulty();

  uint128_t blockNum2 = header2.GetBlockNum();

  LOG_GENERAL(INFO, "Block 2 difficulty: " << to_string(diff2));
  BOOST_CHECK_MESSAGE(
      diff2 == header1.GetDifficulty(),
      "expected: " << header1.GetDifficulty() << " actual: " << diff2 << "\n");

  LOG_GENERAL(INFO, "Block 2 blockNum: " << blockNum2);
  BOOST_CHECK_MESSAGE(blockNum2 == header1.GetBlockNum(),
                      "expected: " << header1.GetBlockNum()
                                   << " actual: " << blockNum2 << "\n");
}

BOOST_AUTO_TEST_CASE(TxBlock_test) {
  INIT_STDOUT_LOGGER();

  LOG_MARKER();

  Transaction tx1 = TestUtils::GenerateRandomTransaction(
      0, 1, Transaction::ContractType::NON_CONTRACT);
  Transaction tx2 = TestUtils::GenerateRandomTransaction(
      0, 2, Transaction::ContractType::NON_CONTRACT);

  // compute tx root hash
  bytes vec;
  tx1.Serialize(vec, 0);
  SHA2<HashType::HASH_VARIANT_256> sha2;
  sha2.Update(vec);
  bytes tx1HashVec = sha2.Finalize();
  std::array<unsigned char, TRAN_HASH_SIZE> tx1Hash;
  copy(tx1HashVec.begin(), tx1HashVec.end(), tx1Hash.begin());

  vec.clear();
  sha2.Reset();
  tx2.Serialize(vec, 0);
  sha2.Update(vec);
  bytes tx2HashVec = sha2.Finalize();
  std::array<unsigned char, TRAN_HASH_SIZE> tx2Hash;
  copy(tx2HashVec.begin(), tx2HashVec.end(), tx2Hash.begin());

  auto minerPubkey = TestUtils::GenerateRandomPubKey();
  TxBlockHeader header0(10000, 8000, 100000, 50, TxBlockHashSet(), 2,
                        minerPubkey, 5);

  std::vector<MicroBlockInfo> mbInfos;
  for (int i = 0; i < 4; ++i) {
    MicroBlockInfo mbInfo;
    mbInfo.m_shardId = i;
    mbInfos.push_back(mbInfo);
  }

  TxBlock block0(header0, mbInfos, TestUtils::GenerateRandomCoSignatures());

  LOG_GENERAL(INFO, "block0 size " << sizeof(block0));

  // compute header hash
  vec.clear();
  header0.Serialize(vec, 0);
  sha2.Reset();
  sha2.Update(vec);
  bytes prevHash1Vec = sha2.Finalize();
  std::array<unsigned char, BLOCK_HASH_SIZE> prevHash1;
  copy(prevHash1Vec.begin(), prevHash1Vec.end(), prevHash1.begin());

  vec.clear();
  copy(tx1Hash.begin(), tx1Hash.end(), back_inserter(vec));
  sha2.Reset();
  sha2.Update(vec);
  vec.clear();
  copy(tx2Hash.begin(), tx2Hash.end(), back_inserter(vec));
  sha2.Update(vec);
  bytes txRootHash1Vec = sha2.Finalize();
  std::array<unsigned char, TRAN_HASH_SIZE> txRootHash1;
  copy(txRootHash1Vec.begin(), txRootHash1Vec.end(), txRootHash1.begin());

  sha2.Reset();
  bytes headerSerialized;
  dsHeader.Serialize(headerSerialized, 0);
  sha2.Update(headerSerialized);
  bytes headerHashVec = sha2.Finalize();
  std::array<unsigned char, BLOCK_HASH_SIZE> headerHash;
  copy(headerHashVec.begin(), headerHashVec.end(), headerHash.begin());
  TxBlockHeader header1(10000, 8000, 100000, 500, TxBlockHashSet(), 2,
                        minerPubkey, 5);
  TxBlock block1(header1, mbInfos, TestUtils::GenerateRandomCoSignatures());

  LOG_GENERAL(INFO, "block1 size " << sizeof(block1));

  bytes message1;
  block1.Serialize(message1, 0);

  LOG_PAYLOAD(INFO, "Block1 serialized", message1,
              Logger::MAX_BYTES_TO_DISPLAY);

  TxBlock block2(message1, 0);

  bytes message2;
  block2.Serialize(message2, 0);

  LOG_PAYLOAD(INFO, "Block2 serialized", message2,
              Logger::MAX_BYTES_TO_DISPLAY);

  for (unsigned int i = 0; i < message1.size(); i++) {
    if (message1.at(i) != message2.at(i)) {
      LOG_GENERAL(INFO, "message1[" << i << "]=" << std::hex << message1.at(i)
                                    << ", message2[" << i << "]=" << std::hex
                                    << message2.at(i));
    }
  }

  BOOST_CHECK_MESSAGE(message1 == message2,
                      "Block1 serialized != Block2 serialized!");

  TxBlockHeader header2 = block2.GetHeader();
  uint32_t version2 = header2.GetVersion();
  uint128_t gasLimit2 = header2.GetGasLimit();
  uint128_t gasUsed2 = header2.GetGasUsed();

  uint128_t blockNum2 = header2.GetBlockNum();
  uint32_t numTxs2 = header2.GetNumTxs();
  uint128_t dsBlockNum2 = header2.GetDSBlockNum();

  LOG_GENERAL(INFO, "Block 2 version: " << version2);
  BOOST_CHECK_MESSAGE(version2 == header1.GetVersion(),
                      "expected: " << 1 << " actual: " << version2 << "\n");

  LOG_GENERAL(INFO, "Block 2 gasLimit: " << gasLimit2);
  BOOST_CHECK_MESSAGE(gasLimit2 == header1.GetGasLimit(),
                      "expected: " << header1.GetGasLimit()
                                   << " actual: " << gasLimit2 << "\n");

  LOG_GENERAL(INFO, "Block 2 gasUsed: " << gasUsed2);
  BOOST_CHECK_MESSAGE(
      gasUsed2 == header1.GetGasUsed(),
      "expected: " << header1.GetGasUsed() << " actual: " << gasUsed2 << "\n");

  BOOST_CHECK_MESSAGE(
      numTxs2 == header1.GetNumTxs(),
      "expected: " << header1.GetNumTxs() << " actual: " << gasUsed2 << "\n");

  BOOST_CHECK_MESSAGE(dsBlockNum2 == header1.GetDSBlockNum(),
                      "expected: " << header1.GetDSBlockNum()
                                   << " actual: " << gasUsed2 << "\n");

  BOOST_CHECK_MESSAGE(
      blockNum2 == header1.GetBlockNum(),
      "expected: " << header1.GetBlockNum() << " actual: " << gasUsed2 << "\n");
}

BOOST_AUTO_TEST_SUITE_END()
