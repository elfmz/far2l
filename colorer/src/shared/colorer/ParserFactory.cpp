#include<stdio.h>
#include<stdlib.h>
#include<sys/stat.h>
#ifdef __unix__
#include<dirent.h>
#endif
#ifdef _WIN32
#include<io.h>
#include<windows.h>
#endif

#include<common/Logging.h>

#include<colorer/ParserFactory.h>
#include<colorer/viewer/TextLinesStore.h>
#include<colorer/handlers/DefaultErrorHandler.h>
#include<colorer/handlers/FileErrorHandler.h>
#include<colorer/parsers/HRCParserImpl.h>
#include<colorer/parsers/TextParserImpl.h>

#ifndef __TIMESTAMP__
#define __TIMESTAMP__ "28 May 2006"
#endif


void ParserFactory::init()
{
  DocumentBuilder docbuilder;
  Document *catalog;

  hrcParser = null;
  fileErrorHandler = null;
  try{
    catalogFIS = InputSource::newInstance(catalogPath);
    catalog = docbuilder.parse(catalogFIS);
  }catch(Exception &e){
    throw ParserFactoryException(*e.getMessage());
  };

  Node *elem = catalog->getDocumentElement();

  if (elem == null || *elem->getNodeName() != "catalog"){
    throw ParserFactoryException(DString("bad catalog structure"));
  }

  elem = elem->getFirstChild();
  while(elem != null){
    // hrc locations
    if (elem->getNodeType() == Node::ELEMENT_NODE &&
        *elem->getNodeName() == "hrc-sets"){

      const String *logLocation = ((Element*)elem)->getAttribute(DString("log-location"));

      if ((logLocation != null) && (logLocation->length()!=0)){
        InputSource *dfis = InputSource::newInstance(logLocation, catalogFIS);
        try{
          fileErrorHandler = new FileErrorHandler(dfis->getLocation(), Encodings::ENC_UTF8, false);
          colorer_logger_set_target(dfis->getLocation()->getChars());
        }catch(Exception &){
          fileErrorHandler = null;
        };
        delete dfis;
      };

      if (fileErrorHandler == null){
        fileErrorHandler = new DefaultErrorHandler();
      };

      Node *loc = elem->getFirstChild();
      while(loc != null){
        if (loc->getNodeType() == Node::ELEMENT_NODE &&
            *loc->getNodeName() == "location"){
          hrcLocations.addElement(new SString(((Element*)loc)->getAttribute(DString("link"))));
        }
        loc = loc->getNextSibling();
      }
    }
    // hrd locations
    if (elem->getNodeType() == Node::ELEMENT_NODE && *elem->getNodeName() == "hrd-sets"){
      Node *hrd = elem->getFirstChild();
      while(hrd != null){
        if (hrd->getNodeType() == Node::ELEMENT_NODE && *hrd->getNodeName() == "hrd"){
          parseHRDSetsChild(hrd);
        };
        hrd = hrd->getNextSibling();
      };
    };
    elem = elem->getNextSibling();
  };
  docbuilder.free(catalog);
};

void ParserFactory::parseHRDSetsChild(Node *hrd)
{
  const String *hrd_class =((Element*)hrd)->getAttribute(DString("class"));
  const String *hrd_name =((Element*)hrd)->getAttribute(DString("name"));
  if (hrd_class == null || hrd_name == null){
    return;
  };

  Hashtable<Vector<const String*>*> *hrdClass = hrdLocations.get(hrd_class);
  if (hrdClass == null){
    hrdClass = new Hashtable<Vector<const String*>*>;
    hrdLocations.put(hrd_class, hrdClass);
  };
  if (hrdClass->get(hrd_name) != null){
    if (fileErrorHandler != null){
      fileErrorHandler->error(StringBuffer("Duplicate hrd class '")+hrd_name+"'");
    }
    return;
  };
  hrdClass->put(hrd_name, new Vector<const String*>);
  Vector<const String*> *hrdLocV = hrdClass->get(hrd_name);
  if (hrdLocV == null){
    hrdLocV = new Vector<const String*>;
    hrdClass->put(hrd_name, hrdLocV);
  };

  const String *hrd_descr = new SString(((Element*)hrd)->getAttribute(DString("description")));
  if (hrd_descr == null){
    hrd_descr = new SString(hrd_name);
  }
  hrdDescriptions.put(&(StringBuffer(hrd_class)+"-"+hrd_name), hrd_descr);

  Node *loc = hrd->getFirstChild();
  while(loc != null){
    if (loc->getNodeType() == Node::ELEMENT_NODE && *loc->getNodeName() == "location"){
      hrdLocV->addElement(new SString(((Element*)loc)->getAttribute(DString("link"))));
    };
    loc = loc->getNextSibling();
  };
};

String *ParserFactory::searchPath()
{
  Vector<String*> paths;
  TextLinesStore tls;

#ifdef _WIN32
  // image_path/  image_path/..  image_path/../..
  TCHAR cname[256];
  HMODULE hmod;
  hmod = GetModuleHandle(TEXT("colorer"));
#ifdef _WIN64
  if (hmod == null) hmod = GetModuleHandle(TEXT("colorer_x64"));
#endif
  if (hmod == null) hmod = GetModuleHandle(null);
  int len = GetModuleFileName(hmod, cname, 256) - 1;
  DString module(cname, 0, len);
  int pos[3];
  pos[0] = module.lastIndexOf('/');
  pos[1] = module.lastIndexOf('/', pos[0]);
  pos[2] = module.lastIndexOf('/', pos[1]);
  for(int idx = 0; idx < 3; idx++)
    if (pos[idx] >= 0)
      paths.addElement(&(new StringBuffer(DString(module, 0, pos[idx])))->append(DString("/catalog.xml")));
#endif

  // %COLORER5CATALOG%
  char *c = getenv("COLORER5CATALOG");
  if (c != null) paths.addElement(new SString(c));

#ifdef __unix__
  // %HOME%/.colorer5catalog or %HOMEPATH%
  c = getenv("HOME");
  if (c == null) c = getenv("HOMEPATH");
  if (c != null){
    try{
      tls.loadFile(&StringBuffer(c).append(DString("/.colorer5catalog")), null, false);
      if (tls.getLineCount() > 0) paths.addElement(new SString(tls.getLine(0)));
    }catch(InputSourceException &e){};
  };

   // /usr/share/colorer/catalog.xml
  paths.addElement(new SString("/usr/share/colorer/catalog.xml"));
  paths.addElement(new SString("/usr/local/share/colorer/catalog.xml"));
#endif

#ifdef _WIN32
    // %HOMEDRIVE%%HOMEPATH%\.colorer5catalog
  char *b = getenv("HOMEDRIVE");
  c = getenv("HOMEPATH");
  if ((c != null)&&(b != null)){
    try{
      StringBuffer *d=new StringBuffer(b);
      d->append(&StringBuffer(c).append(DString("/.colorer5catalog")));
      if (_access(d->getChars(),0) != -1){
        tls.loadFile(d, null, false);
        if (tls.getLineCount() > 0){
          paths.addElement(new SString(tls.getLine(0)));
        }
      }
      delete d;
    }catch(InputSourceException &){};
  };
  // %SYSTEMROOT%/.colorer5catalog
  c = getenv("SYSTEMROOT");
  if (c == null) c = getenv("WINDIR");
  if (c != null){ 
    try{
      StringBuffer *d=new StringBuffer(c);
      d->append(DString("/.colorer5catalog"));
      if (_access(d->getChars(),0) != -1){
        tls.loadFile(d, null, false);
        if (tls.getLineCount() > 0){
          paths.addElement(new SString(tls.getLine(0)));
        }
      }
    }catch(InputSourceException &){};
  };

#endif

  String *right_path = null;
  for(int i = 0; i < paths.size(); i++){
    String *path = paths.elementAt(i);
    if (right_path == null){
      InputSource *is = null;
      try{
        is = InputSource::newInstance(path);
        is->openStream();
        right_path = new SString(path);
        delete is;
      }catch(InputSourceException &){
        delete is;
      };
    };
    delete path;
  };
  if (right_path == null){
    if (fileErrorHandler != null)
      fileErrorHandler->fatalError(DString("Can't find suitable catalog.xml file. Check your program settings."));
    throw ParserFactoryException(DString("Can't find suitable catalog.xml file. Check your program settings."));
  };
  return right_path;
};

ParserFactory::ParserFactory(){
  RegExpStack = NULL;
  RegExpStack_Size = 0;
  fileErrorHandler = null;
  catalogPath = searchPath();
  init();
};

ParserFactory::ParserFactory(const String *catalogPath){
  RegExpStack = NULL;
  RegExpStack_Size = 0;
  fileErrorHandler = null;
  if (catalogPath == null){
    this->catalogPath = searchPath();
  }else{
    this->catalogPath = new SString(catalogPath);
  }
  init();
};

ParserFactory::~ParserFactory(){
  for(Hashtable<Vector<const String*>*> *hrdClass = hrdLocations.enumerate();
      hrdClass;
      hrdClass = hrdLocations.next())
  {
    for(Vector<const String*> *hrd_name = hrdClass->enumerate();hrd_name ; hrd_name = hrdClass->next()){
      for (int i=0;i<hrd_name->size();i++) delete hrd_name->elementAt(i);
      delete hrd_name;
    };

    delete hrdClass;
  };
  for (int i=0;i<hrcLocations.size();i++) delete hrcLocations.elementAt(i);
  for (const String* hrdD=hrdDescriptions.enumerate(); hrdD; hrdD=hrdDescriptions.next())
  {    
    delete hrdD;
  }

  delete hrcParser;
  delete catalogPath;
  delete catalogFIS;
  delete fileErrorHandler;
  delete[] RegExpStack;
};

const char *ParserFactory::getVersion(){
  return "Colorer-take5 Library be5 " __TIMESTAMP__;
};

int ParserFactory::countHRD(const String &classID)
{
  Hashtable<Vector<const String*>*> *hash = hrdLocations.get(&classID);
  if (hash == null) return 0;
  return hash->size();
}

const String* ParserFactory::enumerateHRDClasses(int idx){
  return hrdLocations.key(idx);
};
const String* ParserFactory::enumerateHRDInstances(const String &classID, int idx){
  Hashtable<Vector<const String*>*> *hash = hrdLocations.get(&classID);
  if (hash == null) return null;
  return hash->key(idx);
};
const String* ParserFactory::getHRDescription(const String &classID, const String &nameID){
  return hrdDescriptions.get(&(StringBuffer(classID)+"-"+nameID));
};

HRCParser* ParserFactory::getHRCParser(){
  if (hrcParser != null) return hrcParser;
  hrcParser = new HRCParserImpl();
  hrcParser->setErrorHandler(fileErrorHandler);
  for(int idx = 0; idx < hrcLocations.size(); idx++){
    if (hrcLocations.elementAt(idx) != null){
      const String *relPath = hrcLocations.elementAt(idx);
      const String * path = null;
      if (InputSource::isRelative(relPath)){
        path = InputSource::getAbsolutePath(catalogPath, relPath);
        const String *path2del = path;
        if (path->startsWith(DString("file://"))) path = new SString(path, 7, -1);
        if (path->startsWith(DString("file:/"))) path = new SString(path, 6, -1);
        if (path->startsWith(DString("file:"))) path = new SString(path, 5, -1);
        if (path != path2del) delete path2del;
      }else
        path = new SString(relPath);

      struct stat st;
      int ret = stat(path->getChars(), &st);

      if (ret != -1 && (st.st_mode & S_IFDIR)){
#ifdef _WIN32
        WIN32_FIND_DATA ffd;
        HANDLE dir = FindFirstFile((StringBuffer(path)+"\\*.*").getTChars(), &ffd);
        if (dir != INVALID_HANDLE_VALUE){
          while(true){
            if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
              InputSource *dfis = InputSource::newInstance(&(StringBuffer(relPath)+"\\"+SString(ffd.cFileName)), catalogFIS);
              try{
                hrcParser->loadSource(dfis);
                delete dfis;
              }catch(Exception &e){
                if (fileErrorHandler != null){
                  fileErrorHandler->fatalError(StringBuffer("Can't load hrc: ") + dfis->getLocation());
                  fileErrorHandler->fatalError(*e.getMessage());
                };
                delete dfis;
              };
            };
            if (FindNextFile(dir, &ffd) == FALSE) break;
          };
          FindClose(dir);
        };
#endif
#ifdef __unix__
        DIR *dir = opendir(path->getChars());
        dirent *dire;
        if (dir != null){
          while((dire = readdir(dir)) != null){
            stat((StringBuffer(path)+"/"+dire->d_name).getChars(), &st);
            if (!(st.st_mode & S_IFDIR)){
              InputSource *dfis = InputSource::newInstance(&(StringBuffer(relPath)+"/"+dire->d_name), catalogFIS);
              try{
                hrcParser->loadSource(dfis);
                delete dfis;
              }catch(Exception &e){
                if (fileErrorHandler != null){
                  fileErrorHandler->fatalError(StringBuffer("Can't load hrc: ") + dfis->getLocation());
                  fileErrorHandler->fatalError(*e.getMessage());
                }
                delete dfis;
              }
            }
          }
        }
#endif
      }else{
        InputSource *dfis;
        try{
          dfis = InputSource::newInstance(hrcLocations.elementAt(idx), catalogFIS);
          hrcParser->loadSource(dfis);
          delete dfis;
        }catch(Exception &e){
          if (fileErrorHandler != null){
            fileErrorHandler->fatalError(DString("Can't load hrc: "));
            fileErrorHandler->fatalError(*e.getMessage());
          };
          delete dfis;
        };
      };
      delete path;
    };
  };
  return hrcParser;
};

TextParser *ParserFactory::createTextParser(){
  return new TextParserImpl();
};

StyledHRDMapper *ParserFactory::createStyledMapper(const String *classID, const String *nameID)
{
  Hashtable<Vector<const String*>*> *hrdClass = null;
  if (classID == null) {
	DString ds("rgb");
    hrdClass = hrdLocations.get(&ds);
  } else
    hrdClass = hrdLocations.get(classID);

  if (hrdClass == null)
    throw ParserFactoryException(StringBuffer("can't find hrdClass '")+classID+"'");

  Vector<const String*> *hrdLocV = null;
  if (nameID == null)
  {
    char *hrd = getenv("COLORER5HRD");
	DString dsd("default");
	DString dsh(hrd);
    hrdLocV = (hrd) ? hrdClass->get(&dsh) : hrdClass->get(&dsd);
    if(hrdLocV == null)
    {
      hrdLocV = hrdClass->enumerate();
    }
  }
  else
    hrdLocV = hrdClass->get(nameID);
  if (hrdLocV == null)
    throw ParserFactoryException(StringBuffer("can't find hrdName '")+nameID+"'");

  StyledHRDMapper *mapper = new StyledHRDMapper();
  for(int idx = 0; idx < hrdLocV->size(); idx++)
    if (hrdLocV->elementAt(idx) != null){
      InputSource *dfis;
      try{
			fprintf(stderr, "zz1\n");
        dfis = InputSource::newInstance(hrdLocV->elementAt(idx), catalogFIS);
			fprintf(stderr, "zz2\n");
        mapper->loadRegionMappings(dfis);
			fprintf(stderr, "zz3\n");
        delete dfis;
      }catch(Exception &e){
        if (fileErrorHandler != null){
          fileErrorHandler->error(DString("Can't load hrd: "));
          fileErrorHandler->error(*e.getMessage());
          delete dfis;
          throw ParserFactoryException(DString("Error load hrd"));
        };
      };
    };
  return mapper;
};

TextHRDMapper *ParserFactory::createTextMapper(const String *nameID){
  // fixed class 'text'
  DString dst("text");
  Hashtable<Vector<const String*>*> *hrdClass = hrdLocations.get(&dst);
  if (hrdClass == null) throw ParserFactoryException(StringBuffer("can't find hrdClass 'text'"));

  Vector<const String*> *hrdLocV = null;
  if (nameID == null) {
	DString dsd("default");
    hrdLocV = hrdClass->get(&dsd);
  } else
    hrdLocV = hrdClass->get(nameID);
  if (hrdLocV == null)
    throw ParserFactoryException(StringBuffer("can't find hrdName '")+nameID+"'");

  TextHRDMapper *mapper = new TextHRDMapper();
  for(int idx = 0; idx < hrdLocV->size(); idx++)
    if (hrdLocV->elementAt(idx) != null){
      try{
        InputSource *dfis = InputSource::newInstance(hrdLocV->elementAt(idx), catalogFIS);
        mapper->loadRegionMappings(dfis);
        delete dfis;
      }catch(Exception &e){
        if (fileErrorHandler != null){
          fileErrorHandler->error(DString("Can't load hrd: "));
          fileErrorHandler->error(*e.getMessage());
        };
      };
    };
  return mapper;
};

/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Colorer Library.
 *
 * The Initial Developer of the Original Code is
 * Cail Lomecb <cail@nm.ru>.
 * Portions created by the Initial Developer are Copyright (C) 1999-2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
