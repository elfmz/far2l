#ifndef _COLORER_XSTR_H_
#define _COLORER_XSTR_H_

#include <ostream>
#include <memory>
#include <string>
#include <xercesc/util/XMLString.hpp>

/**
 * This is a simple class that lets us do easy trancoding of
 * XMLCh data to char* data.
 *
 */
class XStr
{
public:
  XStr(const XMLCh* const toTranscode);
  XStr(const std::string &toTranscode);
  XStr(const std::string* toTranscode);
  ~XStr();

  const char* get_char() const;
  const std::string* get_stdstr() const;
  const XMLCh* get_xmlchar() const;

private:
  XStr(XStr const &) = delete;
  XStr &operator=(XStr const &) = delete;
  XStr(XStr &&) = delete;
  XStr &operator=(XStr &&) = delete;

  friend std::ostream &operator<<(std::ostream &stream, const XStr &x);

  std::unique_ptr<XMLCh> _xmlch;
  std::string _string;
};

#endif //_COLORER_XSTR_H_

