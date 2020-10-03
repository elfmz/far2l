#include <colorer/xml/XStr.h>


XStr::XStr(const XMLCh* const toTranscode)
{
  _xmlch.reset(xercesc::XMLString::replicate(toTranscode));
  std::unique_ptr<char> tmp_str(xercesc::XMLString::transcode(toTranscode));
  _string = std::string(tmp_str.get());

}

XStr::XStr(const std::string &toTranscode)
{
  if (toTranscode.empty()) {
    _xmlch = nullptr;
  } else {
    _string = toTranscode;
    _xmlch.reset(xercesc::XMLString::transcode(toTranscode.c_str()));
  }
}

XStr::XStr(const std::string* toTranscode)
{
  if (toTranscode == nullptr || toTranscode->empty()) {
    _xmlch = nullptr;
  } else {
    _string = *toTranscode;
    _xmlch.reset(xercesc::XMLString::transcode(toTranscode->c_str()));
  }
}

XStr::~XStr()
{
}

std::ostream &operator<<(std::ostream &stream, const XStr &x)
{
  if (x._string.empty()) {
    stream << "\\0";
  } else {
    stream << x._string;
  }
  return stream;
}

const char* XStr::get_char() const
{
  if (_string.empty()) {
    return nullptr;
  } else {
    return _string.c_str();
  }
}

const std::string* XStr::get_stdstr() const
{
  return &_string;
}

const XMLCh* XStr::get_xmlchar() const
{
  return _xmlch.get();
}

