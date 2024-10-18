#ifndef SHAREDXMLINPUTSOURCE_H
#define SHAREDXMLINPUTSOURCE_H

#include <unordered_map>
#include "colorer/Common.h"

class SharedXmlInputSource
{
 public:
  static SharedXmlInputSource* getSharedInputSource(const UnicodeString& path);

  /** Increments reference counter */
  int addref();
  /** Decrements reference counter */
  int delref();

  [[nodiscard]]
  int getSize() const;
  [[nodiscard]]
  byte* getSrc() const;

  void open();

  SharedXmlInputSource(SharedXmlInputSource const&) = delete;
  SharedXmlInputSource& operator=(SharedXmlInputSource const&) = delete;
  SharedXmlInputSource(SharedXmlInputSource&&) = delete;
  SharedXmlInputSource& operator=(SharedXmlInputSource&&) = delete;

 private:
  explicit SharedXmlInputSource(const UnicodeString& path);
  ~SharedXmlInputSource();

  static std::unordered_map<UnicodeString, SharedXmlInputSource*>* isHash;

  int ref_count {1};
  bool is_open {false};
  UnicodeString source_path;
  std::unique_ptr<byte[]> mSrc;
  int mSize {0};
};

#endif  // SHAREDXMLINPUTSOURCE_H
