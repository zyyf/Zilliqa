/*
 * Synchronizer.h
 *
 *  Created on: Dec 9, 2018
 *      Author: jenda
 */

#ifndef TESTS_MEDIATOR_MOCKS_LIBLOOKUP_SYNCHRONIZER_H_
#define TESTS_MEDIATOR_MOCKS_LIBLOOKUP_SYNCHRONIZER_H_

#include "libLookup/Lookup.h"

class Synchronizer {
public:
  bool FetchDSInfo(Lookup* lookup) {
    (void) lookup;
    return true;
  }
};



#endif /* TESTS_MEDIATOR_MOCKS_LIBLOOKUP_SYNCHRONIZER_H_ */
