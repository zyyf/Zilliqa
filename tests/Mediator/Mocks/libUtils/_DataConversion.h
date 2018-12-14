/*
 * DataConversion.h
 *
 *  Created on: Dec 7, 2018
 *      Author: jenda
 */

#ifndef TESTS_MEDIATOR_MOCKS_LIBUTILS_DATACONVERSION_H_
#define TESTS_MEDIATOR_MOCKS_LIBUTILS_DATACONVERSION_H_

#include <vector>

class DataConversion {
  static const std::array<unsigned char, 32> HexStrToStdArray(
      const std::string& hex_input) {
    std::string in(hex_input);
    std::array<unsigned char, 32> d;
    std::vector<unsigned char> v = HexStrToUint8Vec(hex_input);
    std::copy(std::begin(v), std::end(v),
              std::begin(d));  // this is the recommended way
    return d;
  }
};


#endif /* TESTS_MEDIATOR_MOCKS_LIBUTILS_DATACONVERSION_H_ */
