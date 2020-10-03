#include <xercesc/dom/DOM.hpp>
#include <colorer/common/Colorer.h>

Colorer::Colorer()
{
  initColorer();
}

Colorer::~Colorer()
{
  // закрываем xerces, подчищая память
  xercesc::XMLPlatformUtils::Terminate();
}

void Colorer::initColorer()
{
  // инициализация xerces, иначе будут ошибки работы со строками
  xercesc::XMLPlatformUtils::Initialize();
}


