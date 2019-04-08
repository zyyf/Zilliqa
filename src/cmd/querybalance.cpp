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

#include <boost/filesystem.hpp>
#include <climits>
#include <fstream>
#include <string>
#include <vector>

#include <boost/program_options.hpp>

#include "jsonrpccpp/client.h"
#include "jsonrpccpp/client/connectors/httpclient.h"
#include "libCrypto/Schnorr.h"
#include "libCrypto/Sha2.h"
#include "libData/AccountData/Account.h"
#include "libData/AccountData/Address.h"
#include "libData/AccountData/Transaction.h"
#include "libMessage/Messenger.h"
#include "libUtils/Logger.h"

namespace po = boost::program_options;

#define SUCCESS 0
#define ERROR_IN_COMMAND_LINE -1
#define ERROR_UNHANDLED_EXCEPTION -2
#define ERROR_UNEXPECTED -3

void description() {
  std::cout << std::endl << "Description:\n";
  std::cout << "\tQuery balance of account by pubkey\n";
}

std::vector<std::string> getPrivKeyFromFile(const std::string& fileName) {
  std::vector<std::string> vecPrivKeys;

  std::ifstream fs(fileName, std::ifstream::in);
  if (!fs.is_open()) {
    std::cout << "Failed to open file " + fileName;
    return vecPrivKeys;
  }

  std::string strLine;
  size_t pos = std::string::npos;
  while (std::getline(fs, strLine, '\n')) {
    if ((pos = strLine.find(' ')) != std::string::npos) {
      auto strPrivKey = strLine.substr(pos + 1);
      vecPrivKeys.push_back(strPrivKey);
    }
  }
  fs.close();
  return vecPrivKeys;
}

const std::string ZILLIQA_API_URL = "https://api.zilliqa.com/";

int main(int argc, char** argv) {
  std::string keyFileName;

  po::options_description desc("Options");

  desc.add_options()("help,h", "Print help messages")(
      "keys, k", po::value<std::string>(&keyFileName),
      "File with keys of account");

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
      SWInfo::LogBrandBugReport();
      description();
      std::cout << desc << std::endl;
      return SUCCESS;
    }

    if (!vm.count("keys")) {
      description();
      return 1;
    }
  } catch (boost::program_options::required_option& e) {
    SWInfo::LogBrandBugReport();
    std::cout << "ERROR: " << e.what() << std::endl << std::endl;
    std::cout << desc;
    return ERROR_IN_COMMAND_LINE;
  } catch (boost::program_options::error& e) {
    SWInfo::LogBrandBugReport();
    std::cout << "ERROR: " << e.what() << std::endl << std::endl;
    return ERROR_IN_COMMAND_LINE;
  }

  jsonrpc::HttpClient httpClient(ZILLIQA_API_URL);
  jsonrpc::Client client(httpClient);
  std::ofstream outputFile("./privKey_Address.txt");

  auto vecPrivKeys = getPrivKeyFromFile(keyFileName);
  if (vecPrivKeys.empty()) {
    return ERROR_UNEXPECTED;
  }

  uint64_t totalBalance = 0;
  for (const auto& privKeyStr : vecPrivKeys) {
    PrivKey privKey = PrivKey::GetPrivKeyFromString(privKeyStr);
    PubKey pubkey(privKey);
    auto address = Account::GetAddressFromPublicKey(pubkey);

    auto addStr = address.hex();

    outputFile << "  <account>\n    <private_key>\n      " << privKeyStr
               << "\n    </private_key>\n    <wallet_address>" << addStr
               << "</wallet_address>\n  </account>" << std::endl;

    Json::Value jsonValue;
    jsonValue[0] = address.hex();

    std::cout << "Json value send out: " << jsonValue << std::endl;

    Json::Value ret = client.CallMethod("GetBalance", jsonValue);
    LOG_GENERAL(INFO, "GetBalance return: " << ret);

    std::cout << "Json value return: " << ret << std::endl;

    try {
      totalBalance += strtoul(ret["balance"].asString().c_str(), NULL, 10);
    } catch (const std::exception& e) {
      std::cout << "Failed to convert balance, exception: " << e.what()
                << std::endl;
    }
  }

  std::cout << "Total balance: " << totalBalance << std::endl;

  return SUCCESS;
}
