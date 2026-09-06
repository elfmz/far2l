#ifndef COLORER_VIRTUALENTRY_H
#define COLORER_VIRTUALENTRY_H

class SchemeImpl;

/** One entry of 'inherit' element virtualization content.
    @ingroup colorer_parsers
*/
class VirtualEntry
{
 public:
  SchemeImpl* virtScheme = nullptr;
  SchemeImpl* substScheme = nullptr;
  uUnicodeString virtSchemeName;
  uUnicodeString substSchemeName;

  VirtualEntry(const UnicodeString* scheme, const UnicodeString* subst)
  {
    virtSchemeName = std::make_unique<UnicodeString>(*scheme);
    substSchemeName = std::make_unique<UnicodeString>(*subst);
  }

  ~VirtualEntry() = default;
};

#endif  // COLORER_VIRTUALENTRY_H
