#pragma once
namespace Codepages
{
	#define IsLocaleMatches(current, wanted_literal) \
		( strncmp((current), wanted_literal, sizeof(wanted_literal) - 1) == 0 && \
		( (current)[sizeof(wanted_literal) - 1] == 0 || (current)[sizeof(wanted_literal) - 1] == '.') )

	unsigned int DetectOemCP();
}
