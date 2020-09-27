#ifndef _COLORER_XML_PARSER_ERROR_HANDLER_H_
#define _COLORER_XML_PARSER_ERROR_HANDLER_H_

#include <xercesc/sax/SAXParseException.hpp>
#include <xercesc/sax/ErrorHandler.hpp>

/* XmlParserErrorHandler - class to catch errors and warnings from the XML Parser*/
class XmlParserErrorHandler : public xercesc::ErrorHandler
{
public:

  XmlParserErrorHandler(): fSawErrors(false)
  {
  }

  ~XmlParserErrorHandler()
  {
  }

  void warning(const xercesc::SAXParseException &toCatch) override;
  void error(const xercesc::SAXParseException &toCatch) override;
  void fatalError(const xercesc::SAXParseException &toCatch) override;
  void resetErrors() override;
  bool getSawErrors() const;

private:
  /* fSawErrors
  This is set if we get any errors, and is queryable via a getter
  method. Its used by the main code to suppress output if there are
  errors. */
  bool fSawErrors;
};

inline bool XmlParserErrorHandler::getSawErrors() const
{
  return fSawErrors;
}

inline void XmlParserErrorHandler::resetErrors()
{
  fSawErrors = false;
}

#endif  // _COLORER_XML_PARSER_ERROR_HANDLER_H_

