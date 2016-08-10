
#include<stdio.h>
#include<colorer/parsers/helpers/HRCParserHelpers.h>
#include<colorer/parsers/HRCParserImpl.h>

HRCParserImpl::HRCParserImpl()
 : fileTypeHash(200), fileTypeVector(150), schemeHash(4000),
 regionNamesVector(1000, 200), regionNamesHash(1000)
{
  parseType = null;
  versionName = null;
  errorHandler = null;
  curInputSource = null;
  updateStarted = false;
}

HRCParserImpl::~HRCParserImpl()
{
  int idx;
  for(FileTypeImpl *ft = fileTypeHash.enumerate(); ft != null; ft = fileTypeHash.next())
    delete ft;
  for(SchemeImpl *scheme = schemeHash.enumerate(); scheme != null; scheme = schemeHash.next()){
    delete scheme;
  };
  for(idx = 0; idx < regionNamesVector.size(); idx++)
    delete regionNamesVector.elementAt(idx);
  for(String *se = schemeEntitiesHash.enumerate(); se; se = schemeEntitiesHash.next()){
    delete se;
  };
  delete versionName;
}

void HRCParserImpl::setErrorHandler(ErrorHandler *eh){
  errorHandler = eh;
}

void HRCParserImpl::loadSource(InputSource *is){
  InputSource *istemp = curInputSource;
  curInputSource = is;
  if (is == null){
    if (errorHandler != null){
      errorHandler->error(StringBuffer("Can't open stream for type without location attribute"));
    }
    return;
  }
  try{
    parseHRC(is);
  }catch(Exception &e){
    curInputSource = istemp;
    throw e;
  }
  curInputSource = istemp;
}

void HRCParserImpl::unloadFileType(FileTypeImpl *filetype)
{
	bool loop=true;
	while (loop){
		loop=false;
		for(SchemeImpl *scheme = schemeHash.enumerate(); scheme != null; scheme = schemeHash.next()){
			if (scheme->fileType==filetype) {
				schemeHash.remove(scheme->getName());
				delete scheme;
				loop= true;
				break;
			}
		};
	}
	fileTypeVector.removeElement(filetype);
	fileTypeHash.remove(filetype->getName());
	delete filetype;
}

void HRCParserImpl::loadFileType(FileType *filetype)
{
  if (filetype == null) return;

  FileTypeImpl *thisType = (FileTypeImpl*)filetype;
  
  if (thisType->typeLoaded || thisType->inputSourceLoading || thisType->loadBroken){
    return;
  }
  thisType->inputSourceLoading = true;

  try{

    loadSource(thisType->inputSource);

  }catch(InputSourceException &e){
    if (errorHandler != null){
      errorHandler->fatalError(StringBuffer("Can't open source stream: ")+e.getMessage());
    }
    thisType->loadBroken = true;
  }catch(HRCParserException &e){
    if (errorHandler != null){
      errorHandler->fatalError(StringBuffer(e.getMessage())+" ["+thisType->inputSource->getLocation()+"]");
    }
    thisType->loadBroken = true;
  }catch(Exception &e){
    if (errorHandler != null){
      errorHandler->fatalError(StringBuffer(e.getMessage())+" ["+thisType->inputSource->getLocation()+"]");
    }
    thisType->loadBroken = true;
  }catch(...){
    if (errorHandler != null){
      errorHandler->fatalError(StringBuffer("Unknown exception while loading ")+thisType->inputSource->getLocation());
    }
    thisType->loadBroken = true;
  }
}

FileType *HRCParserImpl::chooseFileType(const String *fileName, const String *firstLine, int typeNo)
{
FileTypeImpl *best = null;
double max_prior = 0;
const double DELTA = 1e-6;
  for(int idx = 0; idx < fileTypeVector.size(); idx++){
    FileTypeImpl *ret = fileTypeVector.elementAt(idx);
    double prior = ret->getPriority(fileName, firstLine);

    if (typeNo > 0 && (prior-max_prior < DELTA)){
      best = ret;
      typeNo--;
    }
    if (prior-max_prior > DELTA || best == null){
      best = ret;
      max_prior = prior;
    }
  }
  if (typeNo > 0) return null;
  return best;
}

FileType *HRCParserImpl::getFileType(const String *name) {
  if (name == null) return null;
  return fileTypeHash.get(name);
}

FileType *HRCParserImpl::enumerateFileTypes(int index) {
  if (index < fileTypeVector.size()) return fileTypeVector.elementAt(index);
  return null;
}

int HRCParserImpl::getFileTypesCount() {
  return fileTypeVector.size();
}

int HRCParserImpl::getRegionCount() {
  return regionNamesVector.size();
}

const Region *HRCParserImpl::getRegion(int id) {
  if (id < 0 || id >= regionNamesVector.size()){
    return null;
  }
  return regionNamesVector.elementAt(id);
}

const Region* HRCParserImpl::getRegion(const String *name) {
  if (name == null) return null;
  return getNCRegion(name, false); // regionNamesHash.get(name);
}

const String *HRCParserImpl::getVersion() {
  return versionName;
}


// protected methods


void HRCParserImpl::parseHRC(InputSource *is)
{
  Document *xmlDocument = docbuilder.parse(is);
  Element *types = xmlDocument->getDocumentElement();
  if (*types->getNodeName() != "hrc"){
    docbuilder.free(xmlDocument);
    throw HRCParserException(DString("main '<hrc>' block not found"));
  }else{
    if (versionName == null)
      versionName = new SString(types->getAttribute(DString("version")));
  };

  bool globalUpdateStarted = false;
  if (!updateStarted){
    globalUpdateStarted = true;
    updateStarted = true;
  };
  for (Node *elem = types->getFirstChild(); elem; elem = elem->getNextSibling()){
    if (*elem->getNodeName() == "prototype"){
      addPrototype((Element*)elem);
      continue;
    };
    if (*elem->getNodeName() == "package"){
      addPrototype((Element*)elem);
      continue;
    };
    if (*elem->getNodeName() == "type"){
      addType((Element*)elem);
      continue;
    };
  };
  docbuilder.free(xmlDocument);
  structureChanged = true;
  if (globalUpdateStarted){
    updateLinks();
    updateStarted = false;
  };
}


void HRCParserImpl::addPrototype(Element *elem)
{
  const String *typeName = elem->getAttribute(DString("name"));
  const String *typeGroup = elem->getAttribute(DString("group"));
  const String *typeDescription = elem->getAttribute(DString("description"));
  if (typeName == null){
    if (errorHandler != null) errorHandler->error(DString("unnamed prototype "));
    return;
  }
  if (typeDescription == null){
    typeDescription = typeName;
  }
	FileTypeImpl* f=fileTypeHash.get(typeName);
  if (f != null){
		unloadFileType(f);
    if (errorHandler != null){
      errorHandler->warning(StringBuffer("Duplicate prototype '")+typeName+"'");
    }
  //  return;
  };
  FileTypeImpl *type = new FileTypeImpl(this);
  type->name = new SString(typeName);
  type->description = new SString(typeDescription);
  if (typeGroup != null){
    type->group = new SString(typeGroup);
  }
  if (*elem->getNodeName() == "package"){
    type->isPackage = true;
  }

  for(Node *content = elem->getFirstChild(); content != null; content = content->getNextSibling()){
    if (*content->getNodeName() == "location"){
      const String *locationLink = ((Element*)content)->getAttribute(DString("link"));
      if (locationLink == null){
        if (errorHandler != null){
          errorHandler->error(StringBuffer("Bad 'location' link attribute in prototype '")+typeName+"'");
        }
        continue;
      };
      type->inputSource = InputSource::newInstance(locationLink, curInputSource);
    };
    if (*content->getNodeName() == "filename" || *content->getNodeName() == "firstline"){
      if (content->getFirstChild() == null || content->getFirstChild()->getNodeType() != Node::TEXT_NODE){
        if (errorHandler != null) errorHandler->warning(StringBuffer("Bad '")+content->getNodeName()+"' element in prototype '"+typeName+"'");
        continue;
      };
      const String *match = ((Text*)content->getFirstChild())->getData();
      CRegExp *matchRE = new CRegExp(match);
      matchRE->setPositionMoves(true);
      if (!matchRE->isOk()){
        if (errorHandler != null){
          errorHandler->warning(StringBuffer("Fault compiling chooser RE '")+match+"' in prototype '"+typeName+"'");
        }
        delete matchRE;
        continue;
      };
      int ctype = *content->getNodeName() == "filename" ? 0 : 1;
      //double prior = *content->getNodeName() == "filename" ? 2 : 1;
      double prior = ctype ? 1 : 2;
      UnicodeTools::getNumber(((Element*)content)->getAttribute(DString("weight")), &prior);
      FileTypeChooser *ftc = new FileTypeChooser(ctype, prior, matchRE);
      type->chooserVector.addElement(ftc);
    };
    if (*content->getNodeName() == "parameters"){
      for(Node *param = content->getFirstChild(); param != null; param = param->getNextSibling()){
        if (*param->getNodeName() == "param"){
          const String *name = ((Element*)param)->getAttribute(DString("name"));
          const String *value = ((Element*)param)->getAttribute(DString("value"));
          const String *descr = ((Element*)param)->getAttribute(DString("description"));
          if (name == null || value == null){
            if (errorHandler != null){
              errorHandler->warning(StringBuffer("Bad parameter in prototype '")+typeName+"'");
            }
            continue;
          };
          type->paramVector.addElement(new SString(name));
          if (descr != null){
            type->paramDescriptionHash.put(name, new SString(descr));
          }
          type->paramDefaultHash.put(name, new SString(value));
        };
      };
    };
  };

  type->protoLoaded = true;
  fileTypeHash.put(typeName, type);
  if (!type->isPackage){
    fileTypeVector.addElement(type);
  };
}

void HRCParserImpl::addType(Element *elem)
{
  const String *typeName = elem->getAttribute(DString("name"));

  if (typeName == null){
    if (errorHandler != null){
      errorHandler->error(DString("Unnamed type found"));
    }
    return;
  };
  FileTypeImpl *type_ref = fileTypeHash.get(typeName);
  if (type_ref == null){
    if (errorHandler != null){
      errorHandler->error(StringBuffer("type '")+typeName+"' without prototype");
    }
    return;
  };
  FileTypeImpl *type = type_ref;
  if (type->typeLoaded){
    if (errorHandler != null) errorHandler->warning(StringBuffer("type '")+typeName+"' is already loaded");
    return;
  };
  type->typeLoaded = true;

  FileTypeImpl *o_parseType = parseType;
  parseType = type;

  for(Node *xmlpar = elem->getFirstChild(); xmlpar; xmlpar = xmlpar->getNextSibling()){
    if (*xmlpar->getNodeName() == "region"){
      const String *regionName = ((Element*)xmlpar)->getAttribute(DString("name"));
      const String *regionParent = ((Element*)xmlpar)->getAttribute(DString("parent"));
      const String *regionDescr = ((Element*)xmlpar)->getAttribute(DString("description"));
      if (regionName == null){
        if (errorHandler != null) errorHandler->error(DString("No 'name' attribute in <region> element"));
        continue;
      };
      String *qname1 = qualifyOwnName(regionName);
      if (qname1 == null) continue;
      String *qname2 = qualifyForeignName(regionParent, QNT_DEFINE, true);
      if (regionNamesHash.get(qname1) != null){
        if (errorHandler != null){
          errorHandler->warning(StringBuffer("Duplicate region '") + qname1 + "' definition in type '"+parseType->getName()+"'");
        }
        delete qname1;
        delete qname2;
        continue;
      };

      const Region *region = new Region(qname1, regionDescr, getRegion(qname2), regionNamesVector.size());
      regionNamesVector.addElement(region);
      regionNamesHash.put(qname1, region);

      delete qname1;
      delete qname2;
    };
    if (*xmlpar->getNodeName() == "entity"){
      const String *entityName  = ((Element*)xmlpar)->getAttribute(DString("name"));
      const String *entityValue = ((Element*)xmlpar)->getAttribute(DString("value"));
      if (entityName == null || entityValue == null){
        if (errorHandler != null){
          errorHandler->error(DString("Bad entity attributes"));
        }
        continue;
      };
      String *qname1 = qualifyOwnName(entityName);
      String *qname2 = useEntities(entityValue);
      if (qname1 != null && qname2 != null){
        schemeEntitiesHash.put(qname1, qname2);
        delete qname1;
      };
    };
    if (*xmlpar->getNodeName() == "import"){
      const String *typeParam = ((Element*)xmlpar)->getAttribute(DString("type"));
      if (typeParam == null || fileTypeHash.get(typeParam) == null){
        if (errorHandler != null){
          errorHandler->error(StringBuffer("Import with bad '")+typeParam+"' attribute in type '"+typeName+"'");
        }
        continue;
      };
      type->importVector.addElement(new SString(typeParam));
    };
    if (*xmlpar->getNodeName() == "scheme"){
      addScheme((Element*)xmlpar);
      continue;
    };
  };
  String *baseSchemeName = qualifyOwnName(type->name);
  if (baseSchemeName != null){
    type->baseScheme = schemeHash.get(baseSchemeName);
  }
  delete baseSchemeName;
  if (type->baseScheme == null && !type->isPackage){
    if (errorHandler != null){
      errorHandler->warning(StringBuffer("type '")+typeName+"' has no default scheme");
    }
  }
  type->loadDone = true;
  parseType = o_parseType;
}

void HRCParserImpl::addScheme(Element *elem)
{
  const String *schemeName = elem->getAttribute(DString("name"));
  String *qSchemeName = qualifyOwnName(schemeName);
  if (qSchemeName == null){
    if (errorHandler != null) errorHandler->error(StringBuffer("bad scheme name in type '")+parseType->getName()+"'");
    return;
  }
  if (schemeHash.get(qSchemeName) != null ||
      disabledSchemes.get(qSchemeName) != 0){
    if (errorHandler != null) errorHandler->error(StringBuffer("duplicate scheme name '")+qSchemeName+"'");
    delete qSchemeName;
    return;
  }

  SchemeImpl *scheme = new SchemeImpl(qSchemeName);
  delete qSchemeName;
  scheme->fileType = parseType;

  schemeHash.put(scheme->schemeName, scheme);

  const String *condIf = elem->getAttribute(DString("if"));
  const String *condUnless = elem->getAttribute(DString("unless"));
  if ((condIf != null && !DString("true").equals(parseType->getParamValue(*condIf))) ||
      (condUnless != null && DString("true").equals(parseType->getParamValue(*condUnless)))){
    //disabledSchemes.put(scheme->schemeName, 1);
    return;
  }

  addSchemeNodes(scheme, elem->getFirstChild());
}

void HRCParserImpl::addSchemeNodes(SchemeImpl *scheme, Node *elem)
{
  SchemeNode *next = null;
  for(Node *tmpel = elem; tmpel; tmpel = tmpel->getNextSibling()){
    if (!tmpel->getNodeName()) continue;

    if (next == null){
      next = new SchemeNode();
    }

    if (*tmpel->getNodeName() == "inherit"){
      const String *nqSchemeName = ((Element*)tmpel)->getAttribute(DString("scheme"));
      if (nqSchemeName == null || nqSchemeName->length() == 0){
        if (errorHandler != null){
          errorHandler->error(StringBuffer("empty scheme name in inheritance operator in scheme '")+scheme->schemeName+"'");
        }
        continue;
      };
      next->type = SNT_INHERIT;
      next->schemeName = new SString(nqSchemeName);
      String *schemeName = qualifyForeignName(nqSchemeName, QNT_SCHEME, false);
      if (schemeName == null){
//        if (errorHandler != null) errorHandler->warning(StringBuffer("forward inheritance of '")+nqSchemeName+"'. possible inherit loop with '"+scheme->schemeName+"'");
//        delete next;
//        continue;
      }else
        next->scheme = schemeHash.get(schemeName);
      if (schemeName != null){
        delete next->schemeName;
        next->schemeName = schemeName;
      };

      if (tmpel->getFirstChild() != null){
        for(Node *vel = tmpel->getFirstChild(); vel; vel = vel->getNextSibling()){
          if (*vel->getNodeName() != "virtual"){
            continue;
          }
          const String *schemeName = ((Element*)vel)->getAttribute(DString("scheme"));
          const String *substName = ((Element*)vel)->getAttribute(DString("subst-scheme"));
          if (schemeName == null || substName == null){
            if (errorHandler != null){
              errorHandler->error(StringBuffer("bad virtualize attributes in scheme '")+scheme->schemeName+"'");
            }
            continue;
          };
          next->virtualEntryVector.addElement(new VirtualEntry(schemeName, substName));
        };
      };
      scheme->nodes.addElement(next);
      next = null;
      continue;
    };

    if (*tmpel->getNodeName() == "regexp"){
      const String *matchParam = ((Element*)tmpel)->getAttribute(DString("match"));
      if (matchParam == null && tmpel->getFirstChild() && tmpel->getFirstChild()->getNodeType() == Node::TEXT_NODE){
        matchParam = ((Text*)tmpel->getFirstChild())->getData();
      }
      if (matchParam == null){
        if (errorHandler != null){
          errorHandler->error(StringBuffer("no 'match' in regexp in scheme ")+scheme->schemeName);
        }
        delete next;
        continue;
      };
      String *entMatchParam = useEntities(matchParam);
      next->lowPriority = DString("low").equals(((Element*)tmpel)->getAttribute(DString("priority")));
      next->type = SNT_RE;
      next->start = new CRegExp(entMatchParam);
      next->start->setPositionMoves(false);
      if (!next->start || !next->start->isOk())
        if (errorHandler != null) errorHandler->error(StringBuffer("fault compiling regexp '")+entMatchParam+"' in scheme '"+scheme->schemeName+"'");
      delete entMatchParam;
      next->end = 0;
      
      loadRegions(next, (Element*)tmpel, true);
      if(next->region){
        next->regions[0] = next->region;
      }

      scheme->nodes.addElement(next);
      next = null;
      continue;
    }

    
    if (*tmpel->getNodeName() == "block"){
    
      const String *sParam = ((Element*)tmpel)->getAttribute(DString("start"));
      const String *eParam = ((Element*)tmpel)->getAttribute(DString("end"));
      
      Element *eStart = NULL, *eEnd = NULL;
          
      for(Node *blkn = tmpel->getFirstChild(); blkn && !(eParam && sParam); blkn = blkn->getNextSibling())
      {
      	Element *blkel;
      	if(blkn->getNodeType() == Node::ELEMENT_NODE) blkel = (Element*)blkn;
      	else continue;
      	
      	const String *p = (blkel->getFirstChild() && blkel->getFirstChild()->getNodeType() == Node::TEXT_NODE)
      	                ? ((Text*)blkel->getFirstChild())->getData()
      	                : blkel->getAttribute(DString("match"));
      	
      	if(*blkel->getNodeName() == "start") 
      	{
      		sParam = p;
      		eStart = blkel;
      	}
      	if(*blkel->getNodeName() == "end")   
      	{
      		eParam = p;
      		eEnd = blkel;
      	}
      }

      String *startParam;
      String *endParam;
      if (!(startParam = useEntities(sParam))){
        if (errorHandler != null){
          errorHandler->error(StringBuffer("'start' block attribute not found in scheme '")+scheme->schemeName+"'");
        }
        delete startParam;
        delete next;
        continue;
      }
      if (!(endParam = useEntities(eParam))){
        if (errorHandler != null){
          errorHandler->error(StringBuffer("'end' block attribute not found in scheme '")+scheme->schemeName+"'");
        }
        delete startParam;
        delete endParam;
        delete next;
        continue;
      }
      const String *schemeName = ((Element*)tmpel)->getAttribute(DString("scheme"));
      if (schemeName == null || schemeName->length() == 0){
        if (errorHandler != null){
          errorHandler->error(StringBuffer("block with bad scheme attribute in scheme '")+scheme->getName()+"'");
        }
        delete startParam;
        delete endParam;
        continue;
      }
      next->schemeName = new SString(schemeName);
      next->lowPriority = DString("low").equals(((Element*)tmpel)->getAttribute(DString("priority")));
      next->lowContentPriority = DString("low").equals(((Element*)tmpel)->getAttribute(DString("content-priority")));
      next->innerRegion = DString("yes").equals(((Element*)tmpel)->getAttribute(DString("inner-region")));
      next->type = SNT_SCHEME;
      next->start = new CRegExp(startParam);
      next->start->setPositionMoves(false);
      if (!next->start->isOk()){
        if (errorHandler != null){
          errorHandler->error(StringBuffer("fault compiling regexp '")+startParam+"' in scheme '"+scheme->schemeName+"'");
        }
      }
      next->end = new CRegExp();
      next->end->setPositionMoves(true);
      next->end->setBackRE(next->start);
      next->end->setRE(endParam);
      if (!next->end->isOk()){
        if (errorHandler != null){
          errorHandler->error(StringBuffer("fault compiling regexp '")+endParam+"' in scheme '"+scheme->schemeName+"'");
        }
      }
      delete startParam;
      delete endParam;

      // !! EE
      loadBlockRegions(next, (Element*)tmpel);
      loadRegions(next, eStart, true);
      loadRegions(next, eEnd, false);
      scheme->nodes.addElement(next);
      next = null;
      continue;
    }

    if (*tmpel->getNodeName() == "keywords"){
      bool isCase = !DString("yes").equals(((Element*)tmpel)->getAttribute(DString("ignorecase")));
      next->lowPriority = !DString("normal").equals(((Element*)tmpel)->getAttribute(DString("priority")));
      const Region *brgn = getNCRegion((Element*)tmpel, DString("region"));
      if (brgn == null){
        continue;
      }
      const String *worddiv = ((Element*)tmpel)->getAttribute(DString("worddiv"));

      next->worddiv = null;
      if (worddiv){
        String *entWordDiv = useEntities(worddiv);
        next->worddiv = CharacterClass::createCharClass(*entWordDiv, 0, null);
        if(next->worddiv == null){
          if (errorHandler != null) errorHandler->warning(StringBuffer("fault compiling worddiv regexp '")+entWordDiv+"' in scheme '"+scheme->schemeName+"'");
        }
        delete entWordDiv;
      };

      next->kwList = new KeywordList;
      for(Node *keywrd_count = tmpel->getFirstChild(); keywrd_count; keywrd_count = keywrd_count->getNextSibling()){
        if (*keywrd_count->getNodeName() == "word" ||
            *keywrd_count->getNodeName() == "symb")
        {
          next->kwList->num++;
        }
      }

      next->kwList->kwList = new KeywordInfo[next->kwList->num];
      memset(next->kwList->kwList ,0,sizeof(KeywordInfo)*next->kwList->num);
      next->kwList->num = 0;
      KeywordInfo *pIDs = next->kwList->kwList;
      next->kwList->matchCase = isCase;
      next->kwList->kwList = pIDs;
      next->type = SNT_KEYWORDS;

      for(Node *keywrd = tmpel->getFirstChild(); keywrd; keywrd = keywrd->getNextSibling()){
        int type = 0;
        if (*keywrd->getNodeName() == "word") type = 1;
        if (*keywrd->getNodeName() == "symb") type = 2;
        if (!type){
          continue;
        }
        const String *param;
        if (!(param = ((Element*)keywrd)->getAttribute(DString("name"))) || !param->length()){
          continue;
        }

        const Region *rgn = brgn;
        if (((Element*)keywrd)->getAttribute(DString("region")))
          rgn = getNCRegion((Element*)keywrd, DString("region"));

        int pos = next->kwList->num;
        pIDs[pos].keyword = new SString(param);
        pIDs[pos].region = rgn;
        pIDs[pos].isSymbol = (type == 2);
        pIDs[pos].ssShorter = -1;
        next->kwList->firstChar->addChar((*param)[0]);
        if (!isCase){
          next->kwList->firstChar->addChar(Character::toLowerCase((*param)[0]));
          next->kwList->firstChar->addChar(Character::toUpperCase((*param)[0]));
          next->kwList->firstChar->addChar(Character::toTitleCase((*param)[0]));
        };
        next->kwList->num++;
        if (next->kwList->minKeywordLength > pIDs[pos].keyword->length())
          next->kwList->minKeywordLength = pIDs[pos].keyword->length();
      };
      next->kwList->sortList();
      next->kwList->substrIndex();
      scheme->nodes.addElement(next);
      next = null;
      continue;
    };
  };
  // drop last unused node
  if (next != null) delete next;
};


void HRCParserImpl::loadRegions(SchemeNode *node, Element *el, bool st)
{
	static char rg_tmpl[8] = "region\0";
	
	if(el)
	{
        if (node->region == null){
          node->region = getNCRegion(el, DString("region"));
        }

		for(int i = 0; i < REGIONS_NUM; i++)
		{
			rg_tmpl[6] = (i < 0xA ? i : i+7+32) + '0';
			
			if(st){
			  node->regions[i] = getNCRegion(el, DString(rg_tmpl));
			}
			else{
			  node->regione[i] = getNCRegion(el, DString(rg_tmpl));
		    }
		}
	}
	
	for (int i = 0; i < NAMED_REGIONS_NUM; i++)
	{
		if(st) node->regionsn[i] = getNCRegion(node->start->getBracketName(i), false);
		else   node->regionen[i] = getNCRegion(node->end->getBracketName(i), false);
	}
}


void HRCParserImpl::loadBlockRegions(SchemeNode *node, Element *el)
{
int i;
static char rg_tmpl[9] = "region\0\0";

  node->region = getNCRegion(el, DString("region"));
  for (i = 0; i < REGIONS_NUM; i++){
      rg_tmpl[6] = '0'; rg_tmpl[7] = (i<0xA?i:i+7+32)+'0';
      node->regions[i] = getNCRegion(el, DString(rg_tmpl));
      rg_tmpl[6] = '1';
      node->regione[i] = getNCRegion(el, DString(rg_tmpl));
  }
}
  

void HRCParserImpl::updateLinks()
{
  while(structureChanged){
    structureChanged = false;
    for(SchemeImpl *scheme = schemeHash.enumerate(); scheme != null; scheme = schemeHash.next()){
      if (!scheme->fileType->loadDone) continue;
      FileTypeImpl *old_parseType = parseType;
      parseType = scheme->fileType;
      for (int sni = 0; sni < scheme->nodes.size(); sni++){
        SchemeNode *snode = scheme->nodes.elementAt(sni);
        if (snode->schemeName != null && (snode->type == SNT_SCHEME || snode->type == SNT_INHERIT) && snode->scheme == null){
          String *schemeName = qualifyForeignName(snode->schemeName, QNT_SCHEME, true);
          if (schemeName != null){
            snode->scheme = schemeHash.get(schemeName);
          }else{
            if (errorHandler != null) errorHandler->error(StringBuffer("cannot resolve scheme name '")+snode->schemeName+"' in scheme '"+scheme->schemeName+"'");
          };
          delete schemeName;
          delete snode->schemeName;
          snode->schemeName = null;
        };
        if (snode->type == SNT_INHERIT){
          for(int vti = 0; vti < snode->virtualEntryVector.size(); vti++){
            VirtualEntry *vt = snode->virtualEntryVector.elementAt(vti);
            if (vt->virtScheme == null && vt->virtSchemeName != null){
              String *vsn = qualifyForeignName(vt->virtSchemeName, QNT_SCHEME, true);
              if (vsn) vt->virtScheme = schemeHash.get(vsn);
              if (!vsn) if (errorHandler != null) errorHandler->error(StringBuffer("cannot virtualize scheme '")+vt->virtSchemeName+"' in scheme '"+scheme->schemeName+"'");
              delete vsn;
              delete vt->virtSchemeName;
              vt->virtSchemeName = null;
            };
            if (vt->substScheme == null && vt->substSchemeName != null){
              String *vsn = qualifyForeignName(vt->substSchemeName, QNT_SCHEME, true);
              if (vsn) vt->substScheme = schemeHash.get(vsn);
              else if (errorHandler != null) errorHandler->error(StringBuffer("cannot virtualize using subst-scheme scheme '")+vt->substSchemeName+"' in scheme '"+scheme->schemeName+"'");
              delete vsn;
              delete vt->substSchemeName;
              vt->substSchemeName = null;
            };
          };
        };
      };
      parseType = old_parseType;
      if (structureChanged) break;
    };
  };
};



String *HRCParserImpl::qualifyOwnName(const String *name) {
  if (name == null) return null;
  int colon = name->indexOf(':');
  if (colon != -1){
    if (parseType && DString(name, 0, colon) != *parseType->name){
      if (errorHandler != null) errorHandler->error(StringBuffer("type name qualifer in '")+name+"' doesn't match type '"+parseType->name+"'");
      return null;
    }else return new SString(name);
  }else{
    if (parseType == null) return null;
    StringBuffer *sbuf = new StringBuffer(parseType->name);
    sbuf->append(DString(":")).append(name);
    return sbuf;
  };
};
bool HRCParserImpl::checkNameExist(const String *name, FileTypeImpl *parseType, QualifyNameType qntype, bool logErrors) {
  if (qntype == QNT_DEFINE && regionNamesHash.get(name) == null){
    if (logErrors)
      if (errorHandler != null) errorHandler->error(StringBuffer("region '")+name+"', referenced in type '"+parseType->name+"', is not defined");
    return false;
  }else if (qntype == QNT_ENTITY && schemeEntitiesHash.get(name) == null){
    if (logErrors)
      if (errorHandler != null) errorHandler->error(StringBuffer("entity '")+name+"', referenced in type '"+parseType->name+"', is not defined");
    return false;
  }else if (qntype == QNT_SCHEME && schemeHash.get(name) == null){
    if (logErrors)
      if (errorHandler != null) errorHandler->error(StringBuffer("scheme '")+name+"', referenced in type '"+parseType->name+"', is not defined");
    return false;
  };
  return true;
};
String *HRCParserImpl::qualifyForeignName(const String *name, QualifyNameType qntype, bool logErrors){
  if (name == null) return null;
  int colon = name->indexOf(':');
  if (colon != -1){ // qualified name
    DString prefix(name, 0, colon);
    FileTypeImpl *prefType = fileTypeHash.get(&prefix);

    if (prefType == null){
      if (logErrors && errorHandler != null) errorHandler->error(StringBuffer("type name qualifer in '")+name+"' doesn't match any type");
      return null;
    }else
      if (!prefType->typeLoaded) loadFileType(prefType);
    if (prefType == parseType || prefType->typeLoaded)
      return checkNameExist(name, prefType, qntype, logErrors)?(new SString(name)):null;
  }else{ // unqualified name
    for(int idx = -1; parseType != null && idx < parseType->importVector.size(); idx++){
      const String *tname = parseType->name;
      if (idx > -1) tname = parseType->importVector.elementAt(idx);
      FileTypeImpl *importer = fileTypeHash.get(tname);
      if (!importer->typeLoaded) loadFileType(importer);

      StringBuffer *qname = new StringBuffer(tname);
      qname->append(DString(":")).append(name);
      if (checkNameExist(qname, importer, qntype, false)) return qname;
      delete qname;
    };
    if (logErrors && errorHandler != null){
      errorHandler->error(StringBuffer("unqualified name '")+name+"' doesn't belong to any imported type ["+curInputSource->getLocation()+"]");
    }
  };
  return null;
};


String *HRCParserImpl::useEntities(const String *name)
{
  int copypos = 0;
  int epos = 0;

  if (!name) return null;
  StringBuffer *newname = new StringBuffer();

  while(true){
    epos = name->indexOf('%', epos);
    if (epos == -1){
      epos = name->length();
      break;
    };
    if (epos && (*name)[epos-1] == '\\'){
      epos++;
      continue;
    };
    int elpos = name->indexOf(';', epos);
    if (elpos == -1){
      epos = name->length();
      break;
    };
    DString enname(name, epos+1, elpos-epos-1);

    String *qEnName = qualifyForeignName(&enname, QNT_ENTITY, true);
    const String *enval = null;
    if (qEnName != null){
      enval = schemeEntitiesHash.get(qEnName);
      delete qEnName;
    };
    if (enval == null){
      epos++;
      continue;
    };
    newname->append(DString(name, copypos, epos-copypos));
    newname->append(enval);
    epos = elpos+1;
    copypos = epos;
  };
  if (epos > copypos) newname->append(DString(name, copypos, epos-copypos));
  return newname;
};

const Region* HRCParserImpl::getNCRegion(const String *name, bool logErrors){
  if (name == null) return null;
  const Region *reg = null;
  String *qname = qualifyForeignName(name, QNT_DEFINE, logErrors);
  if (qname == null) return null;
  reg = regionNamesHash.get(qname);
  delete qname;
  /** Check for 'default' region request.
      Regions with this name are always transparent
  */
  if (reg != null){
    const String *name = reg->getName();
    int idx = name->indexOf(DString(":default"));
    if (idx != -1  && idx+8 == name->length()) return null;
  };
  return reg;
};

const Region* HRCParserImpl::getNCRegion(Element *el, const String &tag){
  const String *par = el->getAttribute(tag);
  if (par == null) return null;
  return getNCRegion(par, true);
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
 *  Eugene Efremov <4mirror@mail.ru>
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
