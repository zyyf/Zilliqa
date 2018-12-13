/*
 * DirectoryService.h
 *
 *  Created on: Dec 7, 2018
 *      Author: jenda
 */

#ifndef TESTS_MEDIATOR_MOCKS_LIBDIRECTORYSERVICE_DIRECTORYSERVICE_H_
#define TESTS_MEDIATOR_MOCKS_LIBDIRECTORYSERVICE_DIRECTORYSERVICE_H_

#include <atomic>
#include <vector>
#include <queue>

#include "libLookup/Synchronizer.h"
#include "libCrypto/Schnorr.h"

using Shard = std::vector<std::tuple<PubKey, Peer, uint16_t>>;
using DequeOfShard = std::deque<Shard>;

class DirectoryService {
public:
  DirectoryService(){};
  DirectoryService(const DirectoryService &ds)
  {(void)ds;}
  enum Mode : unsigned char { IDLE = 0x00, PRIMARY_DS, BACKUP_DS };
  std::atomic<Mode> m_mode;
  void RejoinAsDS(){
  }
  Synchronizer m_synchronizer;
  DequeOfShard m_shards;
  int64_t GetAllPoWSize() const{
    return 0;
  }
};



#endif /* TESTS_MEDIATOR_MOCKS_LIBDIRECTORYSERVICE_DIRECTORYSERVICE_H_ */
