/*
 * Sha2.h
 *
 *  Created on: Dec 7, 2018
 *      Author: jenda
 */

#ifndef TESTS_MEDIATOR_MOCKS_LIBCRYPTO_SHA2_H_
#define TESTS_MEDIATOR_MOCKS_LIBCRYPTO_SHA2_H_

#include <vector>

class HASH_TYPE {
 public:
  static const unsigned int HASH_VARIANT_256 = 256;
  static const unsigned int HASH_VARIANT_512 = 512;
};

template <unsigned int SIZE>
class SHA2 {
public:
  void Update(const std::vector<unsigned char>& input) {
    (void)input;
  };
  std::vector<unsigned char> Finalize() {
    return std::vector<unsigned char>();
  }
};



#endif /* TESTS_MEDIATOR_MOCKS_LIBCRYPTO_SHA2_H_ */
