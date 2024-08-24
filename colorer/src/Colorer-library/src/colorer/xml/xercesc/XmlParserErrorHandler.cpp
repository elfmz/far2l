#include "colorer/xml/xercesc/XmlParserErrorHandler.h"
#include "colorer/Common.h"

void XmlParserErrorHandler::warning(const xercesc::SAXParseException& toCatch)
{
  logger->warn("Warning at file {0}, line {1}, column {2}. Message: {3}",
               UStr::to_stdstr(toCatch.getSystemId()), toCatch.getLineNumber(),
               toCatch.getColumnNumber(), UStr::to_stdstr(toCatch.getMessage()));
}

void XmlParserErrorHandler::error(const xercesc::SAXParseException& toCatch)
{
  fSawErrors = true;
  logger->error("Error at file {0}, line {1}, column {2}. Message: {3}",
                UStr::to_stdstr(toCatch.getSystemId()), toCatch.getLineNumber(),
                toCatch.getColumnNumber(), UStr::to_stdstr(toCatch.getMessage()));
}

void XmlParserErrorHandler::fatalError(const xercesc::SAXParseException& toCatch)
{
  fSawErrors = true;
  logger->error("Fatal error at file {0}, line {1}, column {2}. Message: {3}",
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
