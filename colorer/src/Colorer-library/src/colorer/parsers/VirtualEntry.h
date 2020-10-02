#ifndef _COLORER_VIRTUALENTRY_H_
#define _COLORER_VIRTUALENTRY_H_

class SchemeImpl;

/** One entry of 'inherit' element virtualization content.
    @ingroup colorer_parsers
*/
class VirtualEntry
{
public:
  SchemeImpl* virtScheme;
  SchemeImpl* substScheme;
  UString virtSchemeName;
  UString substSchemeName;

  VirtualEntry(const String* scheme, const String* subst)
  {
    virtScheme = substScheme = nullptr;
    virtSchemeName.reset(new SString(scheme));
    substSchemeName.reset(new SString(subst));
  }

  ~VirtualEntry() {}


};

#endif // _COLORER_VIRTUALENTRY_H_

