/*
 * DetachedFunction.h
 *
 *  Created on: Dec 7, 2018
 *      Author: jenda
 */

#ifndef TESTS_MEDIATOR_MOCKS_LIBUTILS_DETACHEDFUNCTION_H_
#define TESTS_MEDIATOR_MOCKS_LIBUTILS_DETACHEDFUNCTION_H_

class DetachedFunction {
 public:
  /// Template constructor.
  template <class callable, class... arguments>
  DetachedFunction(int num_threads, callable&& f, arguments&&... args) {
    (void)num_threads;
    (void)f;
  }
};


#endif /* TESTS_MEDIATOR_MOCKS_LIBUTILS_DETACHEDFUNCTION_H_ */
