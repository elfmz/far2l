#include "LookupDebugSymbol.h"

#if defined(__linux__) || defined(__FreeBSD__)
# include <elf.h>
# include <unistd.h>
# include <stdlib.h>
# include <fcntl.h>
# include <vector>
# include "ScopeHelpers.h"

# if defined(__LP64__) || defined(_LP64)
	typedef Elf64_Ehdr Elf_Ehdr;
	typedef Elf64_Phdr Elf_Phdr;
	typedef Elf64_Shdr Elf_Shdr;
	typedef Elf64_Sym  Elf_Sym;
#  define ELF_ST_TYPE(INFO) ELF64_ST_TYPE(INFO)
# else
	typedef Elf32_Ehdr Elf_Ehdr;
	typedef Elf32_Phdr Elf_Phdr;
	typedef Elf32_Shdr Elf_Shdr;
	typedef Elf32_Sym  Elf_Sym;
#  define ELF_ST_TYPE(INFO) ELF32_ST_TYPE(INFO)
# endif

static FDScope s_initial_cwdfd(open(".", O_PATH));

struct FileData : std::vector<char>
{
	FileData(const char *file, size_t offset, size_t size) : std::vector<char>(size)
	{
		FDScope fd(openat(s_initial_cwdfd.Valid() ? (int)s_initial_cwdfd : AT_FDCWD, file, O_RDONLY));
		if (fd.Valid()) {
			while (pread(fd, data(), size, offset) == -1 && errno == EINTR) {
			}
		}
	}
};


static unsigned long GetElfLoadAddr(const Elf_Ehdr *eh)
{
	for (int i = 0; i < (int)eh->e_phnum; ++i) {
		const Elf_Phdr *ph = (const Elf_Phdr *)
			(((const char *)eh) + eh->e_phoff + i * eh->e_phentsize);
		if (ph->p_type == PT_LOAD) {
			return ph->p_vaddr;
		}
	}

	return 0;
}

LookupDebugSymbol::LookupDebugSymbol(const char *module_file, const void *module_base, const void *lookup_addr)
{
	const Elf_Ehdr *eh = (const Elf_Ehdr *)module_base;
	const unsigned long lookup_offset = (const char *)lookup_addr - (const char *)module_base;
	const unsigned long load_addr = GetElfLoadAddr(eh);

	FileData shtab(module_file, eh->e_shoff, eh->e_shnum * eh->e_shentsize);
	for (int i = 0; i < (int)eh->e_shnum; ++i) {
		const Elf_Shdr *sh = (const Elf_Shdr *)&shtab[i * eh->e_shentsize];
		if (sh->sh_type == SHT_SYMTAB && sh->sh_link < eh->e_shnum) {
			FileData syms(module_file, sh->sh_offset, sh->sh_size);
			size_t syms_count = sh->sh_size / sh->sh_entsize;
			for (size_t s = 0; s < syms_count; ++s) {
				const Elf_Sym *sym = (const Elf_Sym *)&syms[s * sh->sh_entsize];
				const auto sym_type = ELF_ST_TYPE(sym->st_info);
				if (lookup_offset >= sym->st_value - load_addr && lookup_offset < sym->st_value + sym->st_size - load_addr
						&& (sym_type == STT_FUNC || sym_type == STT_OBJECT || sym_type == STT_TLS)) {
					const Elf_Shdr *strtab_sh = (const Elf_Shdr *)&shtab[sh->sh_link * eh->e_shentsize];
					FileData strtab(module_file, strtab_sh->sh_offset, strtab_sh->sh_size);
					strtab.emplace_back(0); //ensure 0-terminated
					if (sym->st_name < strtab.size() && strtab[sym->st_name] != '$') {
						name = &strtab[sym->st_name];
						offset = lookup_offset - (sym->st_value - load_addr);
						break;
					}
				}
			}
		}
	}
}

#else // TODO: MacOS

LookupDebugSymbol::LookupDebugSymbol(const char *module_file, const void *module_base, const void *lookup_addr)
{
}

#endif

