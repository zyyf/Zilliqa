/*
 * BlockChain.h
 *
 *  Created on: Dec 5, 2018
 *      Author: jenda
 */

#ifndef TESTS_MEDIATOR_MOCKS_LIBDATA_BLOCKCHAINDATA_BLOCKCHAIN_H_
#define TESTS_MEDIATOR_MOCKS_LIBDATA_BLOCKCHAINDATA_BLOCKCHAIN_H_

#include "gmock/gmock.h"

#include "libData/BlockData/BlockHeader/DSBlockHeader.h"


class TxBlockHeader {
  public:
  void Serialize(std::vector<unsigned char> vec, unsigned int offset) {
    (void)vec;
    (void)offset;
  }
};

class DSBlock {
public:
  DSBlockHeader GetHeader() {
    return DSBlockHeader();
  }

};

class TxBlock {
public:
  TxBlockHeader GetHeader() {
    return TxBlockHeader();
  }
};

template <class T>
class BlockChain {
public:
  T GetLastBlock() {
    return T();
  }
};

class DSBlockChain : public BlockChain<DSBlock> {
 public:
};

class TxBlockChain : public BlockChain<TxBlock> {
public:
  TxBlock GetLastBlock() {
    return TxBlock();
  }
};



#endif /* TESTS_MEDIATOR_MOCKS_LIBDATA_BLOCKCHAINDATA_BLOCKCHAIN_H_ */
