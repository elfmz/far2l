#include "colorer/xml/xercesc/XmlParserErrorHandler.h"
#include "colorer/Common.h"

void XmlParserErrorHandler::warning(const xercesc::SAXParseException& toCatch)
{
  COLORER_LOG_WARN("Warning at file %, line %, column %. Message: %",
               UStr::to_stdstr(toCatch.getSystemId()), toCatch.getLineNumber(),
               toCatch.getColumnNumber(), UStr::to_stdstr(toCatch.getMessage()));
}

void XmlParserErrorHandler::error(const xercesc::SAXParseException& toCatch)
{
  fSawErrors = true;
  COLORER_LOG_ERROR("Error at file %, line %, column %. Message: %",
                UStr::to_stdstr(toCatch.getSystemId()), toCatch.getLineNumber(),
                toCatch.getColumnNumber(), UStr::to_stdstr(toCatch.getMessage()));
}

void XmlParserErrorHandler::fatalError(const xercesc::SAXParseException& toCatch)
{
  fSawErrors = true;
  COLORER_LOG_ERROR("Fatal error at file %, line %, column %. Message: %",
                UStr::to_stdstr(toCatch.getSystemId()), toCatch.getLineNumber(),
                toCatch.getColumnNumber(), UStr::to_stdstr(toCatch.getMessage()));
}

bool XmlParserErrorHandler::getSawErrors() const
{
  return fSawErrors;
}

void XmlParserErrorHandler::resetErrors()
{
  fSawErrors = false;
}
