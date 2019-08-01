/*
 * Copyright (C) 2019 Zilliqa
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <execinfo.h>  // for backtrace
#include <signal.h>

#include <arpa/inet.h>
#include <algorithm>
#include <iostream>

#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>

#include "depends/NAT/nat.h"
#include "libNetwork/P2PComm.h"
#include "libUtils/DataConversion.h"
#include "libUtils/IPConverter.h"
#include "libUtils/Logger.h"
#include "libZilliqa/Zilliqa.h"

using namespace std;
using namespace boost::multiprecision;

#define SUCCESS 0
#define ERROR_IN_COMMAND_LINE -1
#define ERROR_UNHANDLED_EXCEPTION -2

namespace po = boost::program_options;

int main(int argc, const char* argv[]) {
  try {
    Peer my_network_info;
    string privK;
    string pubK;
    PrivKey privkey;
    PubKey pubkey;
    string address;
    int port = -1;
    unique_ptr<NAT> nt;
    uint128_t ip;
    unsigned int synctype = 0;
    const char* synctype_descr =
        "0(default) for no, 1 for new, 2 for normal, 3 for ds, 4 for lookup, 5 "
        "for node recovery, 6 for new lookup , 7 for ds guard node sync and 8 "
        "for offline validation of DB";
    po::options_description desc("Options");

    desc.add_options()("help,h", "Print help messages")(
        "privk,i", po::value<string>(&privK)->required(),
        "32-byte private key")("pubk,u", po::value<string>(&pubK)->required(),
                               "33-byte public key")(
        "address,a", po::value<string>(&address)->required(),
        "Listen IPv4/6 address formated as \"dotted decimal\" or optionally "
        "\"dotted decimal:portnumber\" format, otherwise \"NAT\"")(
        "port,p", po::value<int>(&port),
        "Specifies port to bind to, if not specified in address")(
        "loadconfig,l", "Loads configuration if set (deprecated)")(
        "synctype,s", po::value<unsigned int>(&synctype), synctype_descr)(
        "recovery,r", "Runs in recovery mode if set");

    po::variables_map vm;
    try {
      po::store(po::parse_command_line(argc, argv, desc), vm);

      /** --help option
       */
      if (vm.count("help")) {
        SWInfo::LogBrandBugReport();
        cout << desc << endl;
        return SUCCESS;
      }
      po::notify(vm);

      try {
        privkey = PrivKey::GetPrivKeyFromString(privK);
      } catch (std::invalid_argument& e) {
        std::cerr << e.what() << endl;
        return ERROR_IN_COMMAND_LINE;
      }

      try {
        pubkey = PubKey::GetPubKeyFromString(pubK);
      } catch (std::invalid_argument& e) {
        std::cerr << e.what() << endl;
        return ERROR_IN_COMMAND_LINE;
      }

      if (synctype > 8) {
        SWInfo::LogBrandBugReport();
        std::cerr << "Invalid synctype '" << synctype
                  << "', please select: " << synctype_descr << "." << endl;
      }

      if (address != "NAT") {
        if (!IPConverter::ToNumericalIPFromStr(address, ip)) {
          return ERROR_IN_COMMAND_LINE;
        }

        string address_;
        if (IPConverter::GetIPPortFromSocket(address, address_, port)) {
          address = address_;
        }
      }

      if ((port < 0) || (port > 65535)) {
        SWInfo::LogBrandBugReport();
        std::cerr << "Invalid or missing port number" << endl;
        return ERROR_IN_COMMAND_LINE;
      }
    } catch (boost::program_options::required_option& e) {
      SWInfo::LogBrandBugReport();
      std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
      std::cout << desc;
      return ERROR_IN_COMMAND_LINE;
    } catch (boost::program_options::error& e) {
      SWInfo::LogBrandBugReport();
      std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
      return ERROR_IN_COMMAND_LINE;
    }

    INIT_FILE_LOGGER("zilliqa");
    INIT_STATE_LOGGER("state");
    INIT_EPOCHINFO_LOGGER("epochinfo");

    LOG_GENERAL(INFO, ZILLIQA_BRAND);

    if (SyncType::NEW_SYNC == synctype && CHAIN_ID == MAINNET_CHAIN_ID) {
      SWInfo::IsLatestVersion();
    }

    if (address == "NAT") {
      nt = make_unique<NAT>();
      nt->init();

      int mappedPort = nt->addRedirect(port);

      if (mappedPort <= 0) {
        SWInfo::LogBrandBugReport();
        LOG_GENERAL(WARNING, "NAT ERROR");
        return -1;
      } else {
        LOG_GENERAL(INFO, "My external IP is " << nt->externalIP().c_str()
                                               << " and my mapped port is "
                                               << mappedPort);
      }

      if (!IPConverter::ToNumericalIPFromStr(nt->externalIP().c_str(), ip)) {
        return ERROR_IN_COMMAND_LINE;
      }
      my_network_info = Peer(ip, mappedPort);
    } else {
      my_network_info = Peer(ip, port);
    }

    if (vm.count("loadconfig")) {
      std::cout << "WARNING: loadconfig deprecated" << std::endl;
    }

    Zilliqa zilliqa(make_pair(privkey, pubkey), my_network_info,
                    (SyncType)synctype, vm.count("recovery"));
    auto dispatcher = [&zilliqa](pair<bytes, Peer>* message) mutable -> void {
      zilliqa.Dispatch(message);
    };

    P2PComm::GetInstance().StartMessagePump(my_network_info.m_listenPortHost,
                                            dispatcher);

  } catch (std::exception& e) {
    std::cerr << "Unhandled Exception reached the top of main: " << e.what()
              << ", application will now exit" << std::endl;
    return ERROR_UNHANDLED_EXCEPTION;
  }

  return SUCCESS;
}
