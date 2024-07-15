#ifndef COLORER_XMLCHLITERAL_H
#define COLORER_XMLCHLITERAL_H

// crutch for a case when Xerces has XMLCh be alias for uint16_t instead of char16_t

static_assert(sizeof(char16_t) == sizeof(XMLCh), "XMLCh must be 16-bit type");

struct XMLChLiteral
{
	const char16_t *value;
	inline operator const XMLCh *() const { return (const XMLCh *)value; }
};

# define XMLCH_LITERAL(name, value) constexpr const XMLChLiteral name{value};
# define XMLCH_ARRAY(name, value) XMLCh name[sizeof(value) / sizeof(char16_t)]; memcpy(name, value, sizeof(name));
#endif // COLORER_XMLCHLITERAL_H
