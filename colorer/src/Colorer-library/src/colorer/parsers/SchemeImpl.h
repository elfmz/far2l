#ifndef _COLORER_HRCPARSERPELPERS_H_
#define _COLORER_HRCPARSERPELPERS_H_

#include <vector>
#include <colorer/cregexp/cregexp.h>
#include <colorer/Scheme.h>
#include <colorer/parsers/SchemeNode.h>


class FileTypeImpl;


/** Scheme storage implementation.
    Manages the vector of SchemeNode's.
    @ingroup colorer_parsers
*/
class SchemeImpl : public Scheme
{
  friend class HRCParserImpl;
  friend class TextParserImpl;
public:
  const String* getName() const
  {
    return schemeName.get();
  }

  FileType* getFileType() const
  {
    return (FileType*)fileType;
  }


protected:
  UString schemeName;
  std::vector<SchemeNode*> nodes;
  FileTypeImpl* fileType;

  SchemeImpl(const String* sn)
  {
    schemeName.reset(new SString(sn));
    fileType = nullptr;
  }

  ~SchemeImpl()
  {
    for (auto it : nodes) {
      delete it;
    }
  }
};

#endif


