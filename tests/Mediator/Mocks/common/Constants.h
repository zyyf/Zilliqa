/*
 * Constants.h
 *
 *  Created on: Dec 7, 2018
 *      Author: jenda
 */

#ifndef TESTS_MEDIATOR_MOCKS_COMMON_CONSTANTS_H_
#define TESTS_MEDIATOR_MOCKS_COMMON_CONSTANTS_H_

const unsigned int UINT256_SIZE = 32;

const unsigned int POW_SIZE = 32;
const bool ARCHIVAL_NODE = true;

const unsigned int NUM_VACUOUS_EPOCHS = 1;
const unsigned int NUM_FINAL_BLOCK_PER_POW = 1;
const unsigned int COMM_SIZE = 0;
const std::string RAND1_GENESIS = "";
const std::string RAND2_GENESIS = "";
const bool LOOKUP_NODE_MODE = false;
const unsigned int NEW_NODE_SYNC_INTERVAL = 1;
const unsigned int POW_WINDOW_IN_SECONDS = 0;
const unsigned int POWPACKETSUBMISSION_WINDOW_IN_SECONDS = 0;
const unsigned int FINALBLOCK_DELAY_IN_MS = 0;
const unsigned int HEARTBEAT_INTERVAL_IN_SECONDS = 1;
const unsigned int TX_DISTRIBUTE_TIME_IN_MS = 1000;

#endif /* TESTS_MEDIATOR_MOCKS_COMMON_CONSTANTS_H_ */
