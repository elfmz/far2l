#ifndef _COLORER_SHAREDINPUTSOURCE_H_
#define _COLORER_SHAREDINPUTSOURCE_H_

#include<colorer/Common.h>
#include<colorer/io/InputSource.h>

/**
 * InputSource class wrapper,
 * allows to manage class instances counter.
 * @ingroup common_io
 */
class SharedInputSource : colorer::InputSource
{

public:

  static SharedInputSource* getInputSource(const String* path, colorer::InputSource* base);

  /** Increments reference counter */
  int addref()
  {
    return ++ref_count;
  }

  /** Decrements reference counter */
  int delref()
  {
    if (ref_count == 0) {
      logger->error("[SharedInputSource] delref: already zeroed references");
    }
    ref_count--;
    if (ref_count <= 0) {
      delete this;
      return -1;
    }
    return ref_count;
  }

  /**
   * Returns currently opened stream.
   * Opens it, if not yet opened.
   */
  const byte* getStream()
  {
    if (stream == nullptr) {
      stream = openStream();
    }
    return stream;
  }

  const String* getLocation() const
  {
    return is->getLocation();
  }

  const byte* openStream()
  {
    return is->openStream();
  }

  void closeStream()
  {
    is->closeStream();
  }

  int length() const
  {
    return is->length();
  }

private:

  SharedInputSource(colorer::InputSource* source);
  ~SharedInputSource();

  static std::unordered_map<SString, SharedInputSource*>* isHash;

  colorer::InputSource* is;
  const byte* stream;
  int ref_count;
};

#endif

