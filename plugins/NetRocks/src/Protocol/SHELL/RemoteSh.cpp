#include <errno.h>
#include <stdio.h>
#include <fstream>
#include <map>
#include <utils.h>
#include "../../Erroring.h"
#include "RemoteSh.h"

#define COMPACTIZE_REMOTE_SH

class CompactizedTokens
{
	std::map<std::string, size_t> _m;
	std::string _out;

public:
	CompactizedTokens(char heading) : _out(1, heading) { }

	const std::string &Compactize(const std::string &token)
	{
		auto ir = _m.emplace(token, _m.size());
		size_t n = ir.first->second;
		_out.resize(1);
		while (n) {
			const char c = 'a' + (n % (1 + 'z' - 'a'));
			n/= (1 + 'z' - 'a');
			_out+= c;
		}
		return _out;
	}

	size_t Count() const { return _m.size(); }
};

std::string LoadRemoteSh(const char *helper)
{
	std::ifstream helper_ifs;
	helper_ifs.open(helper);
	std::string out, tmp_str;
	if (!helper_ifs.is_open() ) {
		throw ProtocolError("can't open helper", helper, errno);
	}
	CompactizedTokens comp_funcs('F'), comp_vars('V');
	size_t orig_len = 0;
	while (std::getline(helper_ifs, tmp_str)) {
		orig_len+= tmp_str.size() + 1;
		// do some compactization
		StrTrim(tmp_str);
#ifdef COMPACTIZE_REMOTE_SH
		// skip no-code lines unless enabled logging (to keep line numbers)
		if (tmp_str.empty() || tmp_str.front() == '#') {
			if (g_netrocks_verbosity <= 0) {
				continue;
			}
			tmp_str.clear();
		}
		// rename tokens named for compactization
		for (;;) {
			CompactizedTokens *ct;
			size_t p = tmp_str.find("SHELLVAR_");
			if (p == std::string::npos) {
				p = tmp_str.find("SHELLFCN_");
				if (p == std::string::npos) {
					break;
				}
				ct = &comp_funcs;
			} else {
				ct = &comp_vars;
			}
			size_t e = p + 9;
			while (e < tmp_str.size() && (isalnum(tmp_str[e]) || tmp_str[e] == '_')) {
				++e;
			}
			tmp_str.replace(p, e - p, ct->Compactize(tmp_str.substr(p, e - p)));
		}
#endif
		out+= ' '; // prepend each line with space to avoid history pollution (as HISTCONTROL=ignorespace)
		out+= tmp_str;
		out+= '\n';
	}
	fprintf(stderr, "[SHELL] %s('%s'): %lu -> %lu bytes, renamed %lu funcs and %lu vars\n",
		__FUNCTION__, helper, (unsigned long)orig_len, (unsigned long)out.size(),
		(unsigned long)comp_funcs.Count(), (unsigned long)comp_vars.Count());
	if (g_netrocks_verbosity > 2) {
		fprintf(stderr, "---\n");
		fprintf(stderr, "%s\n", out.c_str());
		fprintf(stderr, "---\n");
	}
	return out;
}

