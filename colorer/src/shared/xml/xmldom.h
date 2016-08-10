#ifndef _COLORER_XMLDOM_H_
#define _COLORER_XMLDOM_H_

#include<common/Vector.h>
#include<common/Hashtable.h>
#include<common/io/InputSource.h>
#include<common/io/Writer.h>

/**
 * If true, traces line/column of an errors in the document
 */
#ifndef XMLDOM_FEATURE_POSITION_TRACE
  #define XMLDOM_FEATURE_POSITION_TRACE 0
#endif

/**
 * @addtogroup xml XMLDOM Parser
 * Simple DOM-based XML Parser.
 * Please refer to the w3c DOM API specification
 * for API review.
 */

class Node;
class Document;
class Element;
class ProcessingInstruction;
class CharacterData;
class Comment;
class Text;

/**
 * Basic XML Parser exception class
 * Contains information about exception and position of the
 * error in the text.
 * @ingroup xml
 */
class ParseException : public Exception
{
public:
  ParseException(const String &msg)
  {
    message->append(msg);
  }

  ParseException(const String &msg, int line, int pos)
  {
    message->append(DString("ParseException: ")) + msg;
    if (line > -1)
    {
      message->append(DString(" at line: ")) + SString(line);
    }
    if (pos > -1)
    {
      message->append(DString(", pos: ")) + SString(pos);
    }
  };
protected:
};

/**
 * Entity resolver, used to resolve addresses of the external entities
 * @ingroup xml
 */
class EntityResolver
{
public:
  virtual InputSource *resolveEntity(const String *publicId, const String *systemId) = 0;
};

/**
 * Default entity resolver class, uses InputSource object rules
 * to resolve relative addresses of the entities.
 * @ingroup xml
 */
class DefaultEntityResolver : public EntityResolver
{
public:
  DefaultEntityResolver(InputSource *_is) : is(_is){};
  InputSource *resolveEntity(const String *publicId, const String *systemId){
    return is->createRelative(systemId);
  }
private:
  InputSource *is;
};

/**
 * Document factory, used to build xml document tree from input stream.
 * Contains parser settings, can be used to generate multiple DOM trees.
 * Should not be used simultaneously from several threads.
 * @ingroup xml
 */
class DocumentBuilder
{
public:
  DocumentBuilder() : ignoreComments(true), ignoreWhitespace(true),
           er(null), inputSource(null) {}

  /**
   * Setups this builder to ignore and not to include in DOM tree
   * XML comments
   */
  void setIgnoringComments(bool _ignoreComments){
    ignoreComments = _ignoreComments;
  }
  /**
   * Returns current Ignoring Comments status.
   */
  inline bool isIgnoringComments(){
    return ignoreComments;
  }
  /**
   * Ignores empty element's text content (content with only
   * spaces, tabs, CR/LF).
   */
  void setIgnoringElementContentWhitespace(bool _ignoreWhitespace)
  {
    ignoreWhitespace = _ignoreWhitespace;
  }
  /**
   * Retrieves whitespace ignore state.
   */
  inline bool isIgnoringElementContentWhitespace()
  {
    return ignoreWhitespace;
  }

  /**
   * Changes entity resolver, used while parsing external entity references.
   */
  void setEntityResolver(EntityResolver *_er){
    er = _er;
  }

  /**
   * Allocates new document object.
   */
  Document *newDocument();

  /**
   * Parses input stream and creates DOM tree.
   */
  Document *parse(InputSource *is, const char *codepage = 0);

  /**
   * Parses input bytes in specified encoding and creates DOM tree.
   */
  Document *parse(const byte *bytes, int length, const char *codepage = 0);

  /**
   * Deletes all DOM tree structure.
   */
  void free(Document *doc);

protected:
  bool ignoreComments;
  bool ignoreWhitespace;
  Hashtable<const String*> entitiesHash;
  Hashtable<const String*> extEntitiesHash;
private:
  int ppos, opos;
  DString src;
  int src_length;
  String *src_overflow;
  Document *doc;
  EntityResolver *er;
  InputSource *inputSource;

  static bool getXMLNumber(const String &str, int *res);

  void consumeDocument();
  void consumeXmlDecl(Node *root);
  void consumeDTD();
  bool isElement();
  void consumeElement(Node *root);
  void consumeContent(Node *root);

  void appendToLastTextNode(Node *root, String *stext);
  bool isCDataSection();
  void consumeCDataSection(Node *root);
  void consumeText(Node *root);
  bool isCharRef();
  wchar consumeCharRef();
  bool isEntityRef();
  String *consumeEntityRef(bool useExtEnt);

  void consumeSpaces(int mins = 0);
  String *consumeQoutedValue();
  String *consumeAttributeValue();
  String *consumeNCName();
  String *consumeName();
  bool isComment();
  void consumeComment(Node *root);
  bool isPI();
  void consumePI(Node *root);
  void consumeMisc(Node *root);
  void consume(String &s);
  void consume(char *s, int len = -1);
  void incDocumentLine();
  void setDocumentPos(int pos);
  void incDocumentPos();
  void clearEntitiesHash();

  inline int peek(int offset = 0){
    if (src_overflow){
      if (opos+offset < src_overflow->length()){
        return (*src_overflow)[opos+offset];
      }else{
        offset -= (src_overflow->length() - opos);
      }
    }
    if (ppos+offset >= src_length) return -1;
    return src[ppos+offset];
  }

  inline wchar get(){
    if (src_overflow){
      if (opos == src_overflow->length()){
        delete src_overflow;
        src_overflow = null;
        opos = 0;
      }else{
        return (*src_overflow)[opos++];
      }
    }
    if (ppos >= src_length){
      throw ParseException(DString("End of stream is reached"));
    }
#if XMLDOM_FEATURE_POSITION_TRACE
    if (src[ppos] == '\n'){
      incDocumentLine();
      setDocumentPos(0);
    }
    incDocumentPos();
#endif
    return src[ppos++];
  }

  Node *next;
};

/**
 * Abstract DOM tree node.
 * @ingroup xml
 */
class Node
{
public:
  enum NodeType {
        ELEMENT_NODE                = 1,
        TEXT_NODE                   = 3,
        PROCESSING_INSTRUCTION_NODE = 7,
        COMMENT_NODE                = 8,
        DOCUMENT_NODE               = 9,
    };

  bool hasChildNodes()
  {
    return firstChild != null;
  }

  Node *getFirstChild()
  {
    return firstChild;
  }

  Node *getLastChild()
  {
    return lastChild;
  }

  int getChildrenCount()
  {
    Node *tmp=firstChild;
    int c=0;
    while (tmp){
      c++;
      tmp = tmp->getNextSibling();
    }
    return c;    
  }

  Node *getParent()
  {
    return parent;
  }

  Node *getNextSibling()
  {
    if (parent == null) return null;
    return next;
  }

  Node *getPrevSibling()
  {
    if (parent == null) return null;
    return prev;
  }

  const String *getNodeName()
  {
    return name;
  }

  virtual const Vector<const String*> *getAttributes()
  {
    return null;
  };

  NodeType getNodeType()
  {
    return type;
  }

  Document *getOwnerDocument()
  {
    return ownerDocument;
  }

  virtual Node *appendChild(Node *newChild);
  virtual Node *insertBefore(Node *newChild, Node *refChild);
  virtual Node *removeChild(Node *oldChild);

  /*  write the contents of the class in a 'Writer'
   *  @level - level of child compared with root
   *  @spacesInLevel - size of the indentation level
   */
  virtual void serialize(Writer *writer, short level, short spacesInLevel) {};

  //virtual Node *cloneNode(bool deep) = 0;

  virtual ~Node()
  {
    delete name;
  };
protected:
  NodeType type;
  Node *next, *prev;
  Node *parent, *firstChild, *lastChild;
  const String *name;
  Document *ownerDocument;
  Node(NodeType _type, const String *_name): type(_type), next(null),
    prev(null), parent(null), firstChild(null), lastChild(null), name(_name) {};
};


/**
 * Document node.
 * @ingroup xml
 */
class Document : public Node
{
public:
  Element *getDocumentElement()
  {
    return documentElement;
  }

  String *getXmlVersion()
  {
    return xmlVersion;
  }

  String *getXmlEncoding()
  {
    return xmlEncoding;
  }

  bool getUseBOM()
  {
    return useBOM;
  }

  void setXmlEncoding(String *_xmlEncoding)
  {
    xmlEncoding = _xmlEncoding;
  }

  Node *appendChild(Node *newChild){
    if (newChild->getNodeType() == Node::ELEMENT_NODE)
    {
      if (documentElement != null)
      {
        throw ParseException(DString("Invalid document root content"), line, pos);
      }
      documentElement = (Element*)newChild;
    };
    Node::appendChild(newChild);
    return newChild;
  }

  void serialize(Writer *writer, short level, short spacesInLevel);

  Element *createElement(const String *tagName);
  Text *createTextNode(const String *data);
  Comment *createComment(const String *data);
  ProcessingInstruction *createProcessingInstruction(const String *target, const String *data);

protected:
  int line, pos;
  Element *documentElement;
  bool useBOM;
  String *xmlVersion;
  String *xmlEncoding;
  Document() : Node(Node::DOCUMENT_NODE, new DString("#document")), documentElement(null), useBOM(false) ,xmlEncoding(NULL),xmlVersion(NULL) {};
  friend class DocumentBuilder;
};


/**
 * Element node.
 * @ingroup xml
 */
class Element : public Node
{
public:

  const String *getAttribute(const String &name)
  {
    return attributesHash.get(&name);
  }
  const String *getAttribute(const String *name)
  {
    return attributesHash.get(name);
  }

  const Vector<const String*> *getAttributes()
  {
    return &attributes;
  };

  void setAttribute(const String *name, const String *value);
  void removeAttribute(const String *name);
  void serialize(Writer *writer, short level, short spacesInLevel);

protected:
  // TODO: static tagName index
  Vector<const String*> attributes;
  Hashtable<const String*> attributesHash;

  Element(const String *_tagName): Node(Node::ELEMENT_NODE, _tagName){};

  ~Element()
  {
    for(int idx = 0; idx < attributes.size(); idx++)
    {
      delete attributes.elementAt(idx);
    }
    for (const String* st = attributesHash.enumerate(); st != null; st = attributesHash.next())
    {
      delete st;
    }
  }

  friend class Document;
};

/**
 * Processing Instruction node.
 * @ingroup xml
 */
class ProcessingInstruction : public Node
{
public:
  const String *getData()
  {
    return data;
  }

  const String *getTarget()
  {
    return target;
  }

  void serialize(Writer *writer, short level, short spacesInLevel);
protected:

  const String *data;
  const String *target;

  ProcessingInstruction(const String *_target, const String *_data):
        Node(Node::PROCESSING_INSTRUCTION_NODE, new DString("#pi")),
        data(_data), target(_target) {}

  ~ProcessingInstruction()
  {
    delete data;
    delete target;
  };

  friend class Document;
};

/**
 * Abstract Text Data section node.
 * @ingroup xml
 */
class CharacterData : public Node
{
public:
  const String *getData()
  {
    return data;
  }

  int getLength()
  {
    return data->length();
  }

protected:

  const String *data;

  CharacterData(NodeType type, const String *_data): Node(type, new DString("#cdata")), data(_data) {};
  ~CharacterData(){ delete data; };
  friend class Document;
};

/**
 * XML Comment node.
 * @ingroup xml
 */
class Comment : public CharacterData
{
public:
  void serialize(Writer *writer, short level, short spacesInLevel);
protected:
  Comment(const String *data): CharacterData(Node::COMMENT_NODE, data){};
  friend class Document;
};

/**
 * XML Text / CDATA node.
 * @ingroup xml
 */
class Text : public CharacterData
{
public:
  void serialize(Writer *writer, short level, short spacesInLevel);
protected:
  Text(const String *data): CharacterData(Node::TEXT_NODE, data){};
  friend class Document;
};

#endif
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
