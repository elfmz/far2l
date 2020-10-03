#include <colorer/Common.h>
#include <colorer/xml/XStr.h>
#include <colorer/xml/XmlParserErrorHandler.h>


void XmlParserErrorHandler::warning(const xercesc::SAXParseException &toCatch)
{
  logger->warn("Warning at file {0}, line {1}, column {2}. Message: {3}",
               *XStr(toCatch.getSystemId()).get_stdstr(), toCatch.getLineNumber(), toCatch.getColumnNumber(), *XStr(toCatch.getMessage()).get_stdstr());
}

void XmlParserErrorHandler::error(const xercesc::SAXParseException &toCatch)
{
  fSawErrors = true;
  logger->error("Error at file {0}, line {1}, column {2}. Message: {3}",
               *XStr(toCatch.getSystemId()).get_stdstr(), toCatch.getLineNumber(), toCatch.getColumnNumber(), *XStr(toCatch.getMessage()).get_stdstr());
}

void XmlParserErrorHandler::fatalError(const xercesc::SAXParseException &toCatch)
{
  fSawErrors = true;
  logger->error("Fatal error at file {0}, line {1}, column {2}. Message: {3}",
                *XStr(toCatch.getSystemId()).get_stdstr(), toCatch.getLineNumber(), toCatch.getColumnNumber(), *XStr(toCatch.getMessage()).get_stdstr());
}


