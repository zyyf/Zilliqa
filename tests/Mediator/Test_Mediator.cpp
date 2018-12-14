/*
 * Copyright (c) 2018 Zilliqa
 * This source code is being disclosed to you solely for the purpose of your
 * participation in testing Zilliqa. You may view, compile and run the code for
 * that purpose and pursuant to the protocols and algorithms that are programmed
 * into, and intended by, the code. You may not do anything else with the code
 * without express permission from Zilliqa Research Pte. Ltd., including
 * modifying or publishing the code (or any part of it), and developing or
 * forming another public or private blockchain network. This source code is
 * provided 'as is' and no warranties are given as to title or non-infringement,
 * merchantability or fitness for purpose and, to the extent permitted by law,
 * all liability for your use of the code is disclaimed. Some programs in this
 * code are governed by the GNU General Public License v3.0 (available at
 * https://www.gnu.org/licenses/gpl-3.0.en.html) ('GPLv3'). The programs that
 * are governed by GPLv3.0 are those programs that are located in the folders
 * src/depends and tests/depends and which include a reference to GPLv3 in their
 * program files.
 */

#include <limits>
#include <random>
#include <utility>
#include <memory>
#include "libArchival/Archival.h"
#include "libArchival/ArchiveDB.h"
#include "Mediator.h"
#include "libCrypto/Schnorr.h"
#include "libTestUtils/TestUtils.h"
#include "libNode/Node.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <boost/multiprecision/cpp_int.hpp>

using namespace std;
using namespace boost::multiprecision;
using ::testing::AtLeast;

Mediator* m;

TEST(Mediator, RegisterColleagues_HeartBeatLaunch) {
  std::pair<PrivKey, PubKey> ppk_p = std::make_pair(PrivKey(), PubKey());
  Peer p = Peer();
  m = new Mediator(ppk_p, p);

  std::deque<std::pair<PubKey, Peer>> ds_c;
  ds_c.push_front(std::make_pair(PubKey(), Peer()));
  m->m_DSCommittee = std::make_shared<std::deque<pair<PubKey, Peer>>>(ds_c);

  DirectoryService ds(*m);
  ds.m_mode = DirectoryService::Mode::BACKUP_DS;
  MockNode mn = MockNode();
  Lookup lookup = Lookup();
  ValidatorBase validator = ValidatorBase();
  BaseDB archDB = BaseDB();
  Archival arch = Archival();

  EXPECT_CALL(mn, RejoinAsNormal()).Times(AtLeast(1));
  m->RegisterColleagues(&ds, &mn, &lookup, &validator, &archDB, &arch);
  m->HeartBeatLaunch();
}

