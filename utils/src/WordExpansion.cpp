#include <wordexp.h>
#include "WordExpansion.h"

WordExpansion::WordExpansion(const char *expression)
{
	if (expression) {
		Expand(expression);
	}
}

WordExpansion::WordExpansion(const std::string &expression)
{
	Expand(expression.c_str());
}

void WordExpansion::Expand(const char *expression)
{
	wordexp_t p = {};
	int r = wordexp( expression, &p, 0 );
	if (r == 0 || r == WRDE_NOSPACE) {
		for (size_t i = 0; i < p.we_wordc; ++i ) {
			if (p.we_wordv[i]) {
				insert(p.we_wordv[i]);
			}
		}
		wordfree( &p );
	}
}
