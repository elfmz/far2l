import json

class Main:
	def __init__(self, fname, lng):
		with open("src/string_rc.h", 'rt') as fp:
			data = fp.read()
		data = data[data.find('{')+1:]
		data = data[:data.find('}')]
		data = data.split(',')
		index = []
		for line in data:
			line = line.strip()
			if line:
				index.append(line)

		self.index = index
		self.data = {}
		self.lng = lng
		with open(fname, 'rt') as fp:
			self.lines = fp.readlines()

	def convert(self):
		i = 0
		for line in self.lines:
			line = line.strip()
			if line and line[0] == '"':
				self.data[self.index[i]] = line[1:-1]
				i += 1
		assert len(self.index) == i

	def save(self):
		with open(f'i18n/{self.lng}.json', 'wt') as fp:
			fp.write('{\n')
			for key in sorted(self.index):
				fp.write(f'    "{key}": "{self.data[key]}",\n')
			fp.write(f'    "_": "_"\n')
			fp.write('}\n')

def main():
	import sys
	cls = Main(sys.argv[1], sys.argv[2])
	cls.convert()
	cls.save()

main()
	