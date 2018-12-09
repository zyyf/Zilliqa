/*
 * Peer.h
 *
 *  Created on: Dec 7, 2018
 *      Author: jenda
 */

#ifndef TESTS_MEDIATOR_MOCKS_LIBNETWORK_PEER_H_
#define TESTS_MEDIATOR_MOCKS_LIBNETWORK_PEER_H_

class Peer{
public:
  bool operator==(const Peer& r) const {
    (void)r;
    return true;
  }
};



#endif /* TESTS_MEDIATOR_MOCKS_LIBNETWORK_PEER_H_ */
