#!/usr/bin/env python3
# Copyright (C) 2019 Zilliqa
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

import argparse
import datetime
import json
import os.path
import re
import subprocess
import sys
import time
from random import randint
from urllib import request, parse
from pprint import pprint

ERROR_MESSAGE = ''
DSGUARD_IPADDR_MAP = dict()

# TEST_MODE=True will introduce some random IP mismatches in the working variables after fetching info from the DS nodes
TEST_MODE = False
DEBUG_PRINT = False

def send_report(msg, url):
	post = {'text': '```' + msg + '```'}
	json_data = json.dumps(post)
	req = request.Request(url, data=json_data.encode('ascii'))
	resp = request.urlopen(req)

def is_valid_ip_address(input):
	pat = re.compile("\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}")
	test = pat.match(input)
	if test:
		return True
	return False

def extract_ip_address(input):
	pat = re.compile("\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}")
	test = pat.search(input)
	if test:
		return test.group()
	return ''

def trigger_test_change():
	return (TEST_MODE == True) and (randint(0,5) == 3)

def check_dsguards(args):
	global ERROR_MESSAGE
	global DSGUARD_IPADDR_MAP
	global TEST_MODE
	global DEBUG_PRINT

	DSGUARD_IPADDR_MAP.clear()

	# Collect IP addresses of all DS guards
	print("Getting IP addresses...")
	for pod_index in range(int(args['dscount'])):
		pod = args['testnet'] + '-dsguard-' + str(pod_index)
		cmd = ['kubectl', 'exec', pod, '--', 'bash', '-c', 'ps axf | grep -e "--address" | grep -v grep | grep -E -o "([0-9]{1,3}[\.]){3}[0-9]{1,3}"']
		ipaddr = subprocess.check_output(cmd).decode('utf-8').strip()
		if is_valid_ip_address(ipaddr) == False:
			raise Exception("["+ pod + "] Failed to get IP address")
		DSGUARD_IPADDR_MAP[pod_index] = ipaddr

	if DEBUG_PRINT == True:
		pprint(DSGUARD_IPADDR_MAP)

	# Check each DS guard's DS committee list for any IP mismatch
	print("Cross-checking each DS guard's IP list...")
	for pod_index in range(int(args['dscount'])):

		# First, generate the DS committee list
		pod = args['testnet'] + '-dsguard-' + str(pod_index)
		cmd = ['kubectl', 'exec', pod, '--', 'bash', '-c', 'python /zilliqa/scripts/miner_info.py ds']
		try:
			dslist = subprocess.check_output(cmd).decode('utf-8').strip()
		except Exception as e:
			msg = "[" + pod + "] " + str(e)
			ERROR_MESSAGE = ERROR_MESSAGE + "\n" + msg
			continue
		dslist_split = dslist.split()
		dslist_map = dict()
		index_counter = 0
		for entry in dslist_split:
			ipaddr = extract_ip_address(entry)
			if ipaddr != '':
				if trigger_test_change():
					print("[" + pod + "][TEST] Changing index=" + str(index_counter) + " IP to 12.345.678.90")
					dslist_map[index_counter] = '12.345.678.90'
				else:
					dslist_map[index_counter] = ipaddr
				index_counter = index_counter + 1
		if DEBUG_PRINT == True:
			pprint(dslist_map)

		# Then, check if the size of the list is correct
		if len(DSGUARD_IPADDR_MAP) > len(dslist_map):
			msg = "[" + pod + "] Wrong dslist size " + str(len(dslist_map))
			ERROR_MESSAGE = ERROR_MESSAGE + "\n" + msg
			continue

		# Finally, match one-by-one against DSGUARD_IPADDR_MAP (except for my own index which is 0.0.0.0)
		for index in DSGUARD_IPADDR_MAP.keys():
			if index == pod_index:
				if dslist_map[index] != '0.0.0.0':
					msg = "[" + pod + "] IP is not 0.0.0.0 at index " + str(index)
					ERROR_MESSAGE = ERROR_MESSAGE + "\n" + msg
			else:
				if dslist_map[index] != DSGUARD_IPADDR_MAP[index]:
					msg = "[" + pod + "] Index=" + str(index) + " Expected=" + DSGUARD_IPADDR_MAP[index] + " Actual=" + dslist_map[index]
					ERROR_MESSAGE = ERROR_MESSAGE + "\n" + msg

def parse_arguments():
	parser = argparse.ArgumentParser(description='Script to check for DS guard IP change')

	parser.add_argument("--dscount", help="Number of DS guards", required=True)
	parser.add_argument("--frequency", help="Checking frequency in minutes (default = 0 or run once)", required=False, default=0)
	parser.add_argument("--testnet", help="Testnet name (e.g., mainnet-beautyworld)", required=True)
	parser.add_argument("--webhook", help="Slack webhook URL", required=False, default='')

	args = vars(parser.parse_args())
	return args

def main():
	args = parse_arguments()
	pprint(args)

	frequency = int(args['frequency']) * 60

	while True:
		print("Check started at: " + str(datetime.datetime.now()))

		global ERROR_MESSAGE
		ERROR_MESSAGE = ''

		try:
			check_dsguards(args)
		except Exception as e:
			ERROR_MESSAGE = '[' + os.path.basename(__file__) + '] Error: ' + str(e)

		if (ERROR_MESSAGE != ''):
			print(ERROR_MESSAGE)
			if args['webhook'] != '':
				send_report(ERROR_MESSAGE, args['webhook'])

		sys.stdout.flush()

		if frequency == 0:
			break

		time.sleep(frequency)

if __name__ == '__main__':
	main()