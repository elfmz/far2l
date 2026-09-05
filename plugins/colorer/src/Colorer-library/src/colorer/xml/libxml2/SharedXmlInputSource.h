#ifndef SHAREDXMLINPUTSOURCE_H
#define SHAREDXMLINPUTSOURCE_H

#include <memory>
#include <unordered_map>
#include "colorer/Common.h"

class SharedXmlInputSource;

class XmlJarCache
{
 public:
  class Current
  {
   public:
    explicit Current(XmlJarCache& cache);
    ~Current();
    Current(const Current&) = delete;
    Current& operator=(const Current&) = delete;

   private:
    XmlJarCache* previous;
  };

  SharedXmlInputSource* get(const UnicodeString& path);

  static XmlJarCache& active();
  static XmlJarCache* bound();

 private:
  std::unordered_map<UnicodeString, std::unique_ptr<SharedXmlInputSource>> entries;
  static thread_local XmlJarCache* current_;
  static XmlJarCache& fallback();
};

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
  ~SharedXmlInputSource() = default;

 private:
  friend class XmlJarCache;
  explicit SharedXmlInputSource(const UnicodeString& path);

  int ref_count {1};
  bool is_open {false};
  UnicodeString source_path;
  std::unique_ptr<byte[]> mSrc;
  int mSize {0};
};

#endif  // SHAREDXMLINPUTSOURCE_H
