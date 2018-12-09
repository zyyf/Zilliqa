/*
 * Lookup.h
 *
 *  Created on: Dec 7, 2018
 *      Author: jenda
 */

#ifndef TESTS_MEDIATOR_MOCKS_LIBLOOKUP_LOOKUP_H_
#define TESTS_MEDIATOR_MOCKS_LIBLOOKUP_LOOKUP_H_

#include <mutex>

#include "libCrypto/Schnorr.h"
#include "libNetwork/Peer.h"

using VectorOfLookupNode = std::vector<std::pair<PubKey, Peer>>;

class Lookup {
public:
  std::mutex m_mutexDSInfoUpdation;
  std::condition_variable cv_dsInfoUpdate;
};



#endif /* TESTS_MEDIATOR_MOCKS_LIBLOOKUP_LOOKUP_H_ */
