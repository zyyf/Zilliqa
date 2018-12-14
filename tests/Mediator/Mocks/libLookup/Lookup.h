/*
 * Lookup.h
 *
 *  Created on: Dec 7, 2018
 *      Author: jenda
 */

#ifndef TESTS_MEDIATOR_MOCKS_LIBLOOKUP_LOOKUP_H_
#define TESTS_MEDIATOR_MOCKS_LIBLOOKUP_LOOKUP_H_

#include <mutex>
#include <chrono>
#include <condition_variable>

#include "libCrypto/Schnorr.h"
#include "libNetwork/Peer.h"

using VectorOfLookupNode = std::vector<std::pair<PubKey, Peer>>;


class Dummy_condition_variable {
public:
  Dummy_condition_variable (){
  }
  template<typename _Rep, typename _Period>
  std::cv_status wait_for(std::unique_lock<std::mutex>& __lock,
      const std::chrono::duration<_Rep, _Period>& __rtime){
    (void)__rtime;
    (void)__lock;
    return std::cv_status::no_timeout;
  }
  bool operator==(const std::cv_status& x) const {
    (void)x;
    return false;
  }
};

class Lookup {
public:
  Lookup(){
  }
  Lookup(const Lookup &l){
    (void)l;
  }
  std::mutex m_mutexDSInfoUpdation;
  Dummy_condition_variable cv_dsInfoUpdate;
};



#endif /* TESTS_MEDIATOR_MOCKS_LIBLOOKUP_LOOKUP_H_ */
