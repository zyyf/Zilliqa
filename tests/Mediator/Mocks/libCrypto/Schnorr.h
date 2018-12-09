/*
 * Schnorr.h
 *
 *  Created on: Dec 7, 2018
 *      Author: jenda
 */

#ifndef TESTS_MEDIATOR_MOCKS_LIBCRYPTO_SCHNORR_H_
#define TESTS_MEDIATOR_MOCKS_LIBCRYPTO_SCHNORR_H_

#include "common/Constants.h"
#include <vector>

struct PrivKey {
  PrivKey(const PrivKey& src){
    (void)src;
  }
  PrivKey(const std::vector<unsigned char>& src, unsigned int offset){
    (void)src;
    (void)offset;
  }
  PrivKey(){
  }
  PrivKey& operator=(const PrivKey& src) {
    (void)src;
    return *this;
  }

  bool operator==(const PrivKey& r) const {
      (void)r;
      return true;
    }

};

struct PubKey {
  PubKey(const PubKey& src){
    (void)src;
  }
  PubKey(const std::vector<unsigned char>& src, unsigned int offset){
    (void)src;
    (void)offset;
  }
  PubKey(){
  }
  PubKey(const PrivKey& privkey){
    (void)privkey;
  }

  PubKey& operator=(const PubKey& src) {
    (void)src;
    return *this;
  }
  bool operator==(const PubKey& r) const {
    (void)r;
    return true;
  }
};



#endif /* TESTS_MEDIATOR_MOCKS_LIBCRYPTO_SCHNORR_H_ */
