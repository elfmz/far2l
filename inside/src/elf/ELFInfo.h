#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <elf.h>
#include <fcntl.h> // for O_RDONLY

#include "../../../WinPort/sudo.h" // for sdc_open

struct ELFInfo
{
	struct Region
	{
		std::string info;
		uint64_t begin = 0;
		uint64_t length = 0;
	};

	struct Regions : std::vector<Region> {} phdrs, sections;

	uint64_t elf_length = 0;
	uint64_t total_length = 0;
	uint16_t machine = 0;
};


struct ELF_EndianessSame
{
	template <class T>
		static T C(T v) { return v; }
};

struct ELF_EndianessReverse
{
	template <class T>
		static T C(T v)
	{
		for (size_t i = 0; i< sizeof(v) / 2; ++i) {
			std::swap( ((char *)&v)[i], ((char *)&v)[sizeof(v) - 1 - i]);
		}
		return v; 
	}
};

template <class E, class Ehdr, class Phdr, class Shdr>
	static void FillELFInfo(ELFInfo &info, const char *filename)
{
	int fd = sdc_open(filename, O_RDONLY);
	if (fd == -1)
		return;

	struct stat s = {};
	sdc_fstat(fd, &s);
	info.total_length = s.st_size;
	info.elf_length = sizeof(Ehdr);
	Ehdr eh = {};
	if (sdc_read(fd, &eh, sizeof(eh)) == sizeof(eh) && eh.e_phnum) {
		
		info.machine = E::C(((uint16_t *)&eh.e_ident)[9]);
		uint64_t tmp = E::C(eh.e_phoff) + uint64_t(E::C(eh.e_phnum)) * E::C(eh.e_phentsize);
		if (info.elf_length < tmp) info.elf_length = tmp;
		for (size_t i = 0; i < E::C(eh.e_phnum); ++i) {
			Phdr ph = {};
			if (sdc_lseek(fd, E::C(eh.e_phoff) + i * E::C(eh.e_phentsize), SEEK_SET) != (off_t)-1
			&& sdc_read(fd, &ph, sizeof(ph)) == sizeof(ph) && ph.p_filesz
			&& E::C(ph.p_type) == PT_LOAD) {
				info.phdrs.emplace_back();
				auto &r = info.phdrs.back();
				r.info+= '[';
				if ( E::C(ph.p_flags) & PF_R) r.info+= 'R';
				if ( E::C(ph.p_flags) & PF_W) r.info+= 'W';
				if ( E::C(ph.p_flags) & PF_X) r.info+= 'X';
				r.info+= ']';
				r.begin = E::C(ph.p_offset);
				r.length = E::C(ph.p_filesz);
				tmp =  r.begin + r.length;
				if (info.elf_length < tmp) info.elf_length = tmp;
			}
		}
		if (eh.e_shnum) {
			Shdr sh = {};
			std::vector<char> strings;
			E::C(eh.e_shstrndx);
			if (sdc_lseek(fd, E::C(eh.e_shoff) + off_t(E::C(eh.e_shstrndx)) * E::C(eh.e_shentsize), SEEK_SET) != (off_t)-1
			&& sdc_read(fd, &sh, sizeof(sh)) == sizeof(sh)) {
				strings.resize(E::C(sh.sh_size) + 1);
				if (sdc_lseek(fd, E::C(sh.sh_offset), SEEK_SET) != -1) {
					sdc_read(fd, &strings[0], strings.size() - 1);
				}
			}

			tmp = E::C(eh.e_shoff) + uint64_t(E::C(eh.e_shnum)) * E::C(eh.e_shentsize);
			if (info.elf_length < tmp) info.elf_length = tmp;
			for (size_t i = 0; i < E::C(eh.e_shnum); ++i) {
				if (sdc_lseek(fd, E::C(eh.e_shoff) + i * E::C(eh.e_shentsize), SEEK_SET) != (off_t)-1
				&& sdc_read(fd, &sh, sizeof(sh)) == sizeof(sh) && sh.sh_size
				&& E::C(sh.sh_type) != SHT_NOBITS) {
					info.sections.emplace_back();
					auto &r = info.sections.back();
					r.info+= '[';
					if ( E::C(sh.sh_flags) & SHF_WRITE) r.info+= 'W';
					if ( E::C(sh.sh_flags) & SHF_ALLOC) r.info+= 'A';
					if ( E::C(sh.sh_flags) & SHF_EXECINSTR) r.info+= 'X';
					if ( E::C(sh.sh_flags) & SHF_MERGE) r.info+= 'M';
					if ( E::C(sh.sh_flags) & SHF_STRINGS) r.info+= 'S';
					if ( E::C(sh.sh_flags) & SHF_INFO_LINK) r.info+= "I";
					r.info+= ']';
					if (strings.size() > E::C(sh.sh_name)) {
						r.info+= &strings[E::C(sh.sh_name)];
					}
					r.begin = E::C(sh.sh_offset);
					r.length = E::C(sh.sh_size);
					tmp =  r.begin + r.length;
					if (info.elf_length < tmp) info.elf_length = tmp;
				}
			}
		}
	}
	
	close(fd);
}
