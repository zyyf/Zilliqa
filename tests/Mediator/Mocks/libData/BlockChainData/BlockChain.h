/*
 * BlockChain.h
 *
 *  Created on: Dec 5, 2018
 *      Author: jenda
 */

#ifndef TESTS_MEDIATOR_MOCKS_LIBDATA_BLOCKCHAINDATA_BLOCKCHAIN_H_
#define TESTS_MEDIATOR_MOCKS_LIBDATA_BLOCKCHAINDATA_BLOCKCHAIN_H_

#include "gmock/gmock.h"

class DSBlock {
  DSBlock() {};
  ~DSBlock() {};
};

class TxBlock {
  TxBlock() {};
  ~TxBlock() {};
};

template <class T>
class BlockChain {

 protected:

 public:
  /// Destructor.
  BlockChain() {};
  ~BlockChain() {};
};

class DSBlockChain : public BlockChain<DSBlock> {
 public:
  DSBlock GetBlockFromPersistentStorage(const uint64_t& blockNum) {
    std::shared_ptr<DSBlock> block;
    BlockStorage::GetBlockStorage().GetDSBlock(blockNum, block);
    return *block;
  }
};

class TxBlockChain : public BlockChain<TxBlock> {
 public:
  TxBlock GetBlockFromPersistentStorage(const uint64_t& blockNum) {
    TxBlockSharedPtr block;
    BlockStorage::GetBlockStorage().GetTxBlock(blockNum, block);
    return *block;
  }
};



#endif /* TESTS_MEDIATOR_MOCKS_LIBDATA_BLOCKCHAINDATA_BLOCKCHAIN_H_ */
