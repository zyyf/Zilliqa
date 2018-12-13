/*
 * Archival.h
 *
 *  Created on: Dec 5, 2018
 *      Author: jenda
 */

#ifndef TESTS_MEDIATOR_MOCKS_LIBARCHIVAL_ARCHIVAL_H_
#define TESTS_MEDIATOR_MOCKS_LIBARCHIVAL_ARCHIVAL_H_
#include <mutex>
#include "libUtils/Logger.h"

class Archival {
public:
  Archival(){
  }
  Archival(const Archival &a){
    (void)a;
  }
};


#endif /* TESTS_MEDIATOR_MOCKS_LIBARCHIVAL_ARCHIVAL_H_ */
