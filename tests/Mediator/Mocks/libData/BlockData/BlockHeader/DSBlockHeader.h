/*
 * DSBlockHeader.h
 *
 *  Created on: Dec 8, 2018
 *      Author: jenda
 */

#ifndef TESTS_MEDIATOR_MOCKS_LIBDATA_BLOCKDATA_BLOCKHEADER_DSBLOCKHEADER_H_
#define TESTS_MEDIATOR_MOCKS_LIBDATA_BLOCKDATA_BLOCKHEADER_DSBLOCKHEADER_H_

#include "libUtils/SWInfo.h"

class DSBlockHeader {
public:
  void Serialize(std::vector<unsigned char> vec, unsigned int offset) {
    (void)vec;
    (void)offset;
  }
  uint64_t GetBlockNum() {
    return 0;
  }
};

#endif /* TESTS_MEDIATOR_MOCKS_LIBDATA_BLOCKDATA_BLOCKHEADER_DSBLOCKHEADER_H_ */
