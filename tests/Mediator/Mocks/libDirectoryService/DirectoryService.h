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

class Mediator;

class DirectoryService {
public:
  enum Mode : unsigned char { IDLE = 0x00, PRIMARY_DS, BACKUP_DS };
  std::atomic<Mode> m_mode;
  Synchronizer m_synchronizer;
  DequeOfShard m_shards;
  Mediator& m;

  DirectoryService(Mediator&);

  int64_t GetAllPoWSize() const;
  void RejoinAsDS();
};



#endif /* TESTS_MEDIATOR_MOCKS_LIBDIRECTORYSERVICE_DIRECTORYSERVICE_H_ */
