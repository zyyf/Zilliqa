/*
 * ShardSizeCalculator.h
 *
 *  Created on: Dec 7, 2018
 *      Author: jenda
 */

#ifndef TESTS_MEDIATOR_MOCKS_LIBUTILS_SHARDSIZECALCULATOR_H_
#define TESTS_MEDIATOR_MOCKS_LIBUTILS_SHARDSIZECALCULATOR_H_

#include <cstdint>

class ShardSizeCalculator {
public:
  static uint32_t CalculateShardSize(const uint32_t numberOfNodes) {
    (void)numberOfNodes;
    return 0;
  }
};



#endif /* TESTS_MEDIATOR_MOCKS_LIBUTILS_SHARDSIZECALCULATOR_H_ */
