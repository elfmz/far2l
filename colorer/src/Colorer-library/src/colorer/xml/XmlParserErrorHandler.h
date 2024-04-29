#ifndef COLORER_XML_PARSER_ERROR_HANDLER_H
#define COLORER_XML_PARSER_ERROR_HANDLER_H

#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>

/* XmlParserErrorHandler - class to catch errors and warnings from the XML Parser*/
class XmlParserErrorHandler : public xercesc::ErrorHandler
{
 public:
  XmlParserErrorHandler() : fSawErrors {false} {}

  void warning(const xercesc::SAXParseException& toCatch) override;
  void error(const xercesc::SAXParseException& toCatch) override;
  void fatalError(const xercesc::SAXParseException& toCatch) override;
  void resetErrors() override;
  [[nodiscard]] bool getSawErrors() const;

 private:
  /* fSawErrors
  This is set if we get any errors, and is queryable via a getter
  method. Its used by the main code to suppress output if there are
  errors. */
  bool fSawErrors;
};

#endif  // COLORER_XML_PARSER_ERROR_HANDLER_H
