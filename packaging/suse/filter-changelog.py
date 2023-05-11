#!python3
import sys

def filter_by_word (line):
	bad_words = [
		'cosmetic', 'minor fix', 'refactor', 'Merge pull request', 
		'cleanup', 'comment', 'Merge branch', 'formatting', 'minor', 
		'commit missing', 'debugging', 'logging', 'naming']
	for x in bad_words:
		if x in line:
			return True
	return False

dates_list = []
dates_map = {}
date = ''

while True:
	try:
		line = input()
		line = line.strip()
		if line.startswith('*'):
			items = line.split();
			date = items[1] + ' ' + items[2] + ' ' + items[3] + ' ' + items[4]
			if '2022' in items[4]:
				dates_list.append(date)
				dates_map[date] = [line, '- much more development from 2015 - 2022 years']
				break
			if not date in dates_map:
				dates_list.append(date)
				dates_map[date] = [line]
		elif line.startswith('-'):
			if not filter_by_word(line):
				dates_map[date].append(line)

	except EOFError:
		break

for date in dates_list:
	lines = dates_map[date]
	if len(lines) > 1:
		for y in lines:
			print(y)
		print ()