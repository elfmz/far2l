import glob
import json

class Main:
	def __init__(self):
		self.project = "edsort"
		self.headers = {
			'en': '.Language=English,English',
			'pl': '.Language=Polish,Polish (Polski)',
			'ru': '.Language=Russian,Russian (Русский)',
		}
		self.translations = {}
		fnames = glob.glob('i18n/*.json')
		for fname in fnames:
			with open(fname, 'rt') as fp:
				data = json.loads(fp.read())
				del data['_']
				self.translations[fname.split('/')[1].split('.')[0]] = data

	def save(self):
		for lang in self.translations.keys():
			with open(f'configs/plug/{self.project}_{lang}.lng', 'wt') as fp:
				fp.write(f'{self.headers[lang]}\n')
				data = self.translations[lang]
				for key in sorted(data.keys()):
					fp.write(f'"{data[key]}"\n')
		with open('src/i18nindex.h', 'wt') as fp:
			fp.write('#define I18N(x) _PSI.GetMsg(_PSI.ModuleNumber, static_cast<int>(x))\n\n')
			fp.write('enum i18n_index {\n')
			data = self.translations[list(self.translations.keys())[0]]
			for key in sorted(data.keys()):
				fp.write(f'{key},\n')
			fp.write('};\n')

def main():
	cls = Main()
	cls.save()

main()
	