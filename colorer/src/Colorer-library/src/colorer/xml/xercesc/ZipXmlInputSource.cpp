#include "colorer/xml/xercesc/ZipXmlInputSource.h"
#include "colorer/Exception.h"
#include "colorer/xml/xercesc/MemoryFile.h"

ZipXmlInputSource::ZipXmlInputSource(const XMLCh* path, const XMLCh* base)
{
  create(path, base);
}

void ZipXmlInputSource::create(const XMLCh* path, const XMLCh* base)
{
  const auto kJar_len = xercesc::XMLString::stringLen(kJar);
  if (xercesc::XMLString::startsWith(path, kJar)) {
    int path_idx = xercesc::XMLString::lastIndexOf(path, '!');
    if (path_idx == -1) {
      throw InputSourceException("Bad jar uri format: " + UnicodeString(path));
    }

    auto bpath = std::make_unique<XMLCh[]>(path_idx - kJar_len + 1);
    xercesc::XMLString::subString(bpath.get(), path, kJar_len, path_idx);
    jar_input_source = SharedXmlInputSource::getSharedInputSource(bpath.get(), base);

    in_jar_location = std::make_unique<UnicodeString>(UnicodeString(path), path_idx + 1);

  } else if (base != nullptr && xercesc::XMLString::startsWith(base, kJar)) {
    int base_idx = xercesc::XMLString::lastIndexOf(base, '!');
    if (base_idx == -1) {
      throw InputSourceException("Bad jar uri format: " + UnicodeString(path));
    }

    auto bpath = std::make_unique<XMLCh[]>(base_idx - kJar_len + 1);
    xercesc::XMLString::subString(bpath.get(), base, kJar_len, base_idx);
    jar_input_source = SharedXmlInputSource::getSharedInputSource(bpath.get(), nullptr);

    auto in_base = std::make_unique<UnicodeString>(UnicodeString(base), base_idx + 1);
    UnicodeString d_path = UnicodeString(path);
    in_jar_location = getAbsolutePath(in_base.get(), &d_path);

  } else {
    throw InputSourceException("Can't create jar source");
  }

  UnicodeString str("jar:");
  str.append(UnicodeString(jar_input_source->getInputSource()->getSystemId()));
  str.append("!");
  str.append(*in_jar_location);
  setSystemId(UStr::to_xmlch(&str).get());
  source_path = std::make_unique<UnicodeString>(str);
}

ZipXmlInputSource::~ZipXmlInputSource()
{
  jar_input_source->delref();
}

xercesc::InputSource* ZipXmlInputSource::getInputSource() const
{
  return (xercesc::InputSource*) this;
}

xercesc::BinInputStream* ZipXmlInputSource::makeStream() const
{
  return new UnZip(jar_input_source->getSrc(), jar_input_source->getSize(), in_jar_location.get());
}

uUnicodeString ZipXmlInputSource::getAbsolutePath(const UnicodeString* basePath, const UnicodeString* relPath)
{
  auto root_pos = basePath->lastIndexOf('/');
  auto root_pos2 = basePath->lastIndexOf('\\');
  if (root_pos2 > root_pos) {
    root_pos = root_pos2;
  }
  if (root_pos == -1) {
    root_pos = 0;
  } else {
    root_pos++;
  }
  auto newPath = std::make_unique<UnicodeString>();
  newPath->append(UnicodeString(*basePath, 0, root_pos)).append(*relPath);
  return newPath;
}

UnZip::UnZip(const XMLByte* src, XMLSize_t size, const UnicodeString* path) : mPos(0), mBoundary(0), stream(nullptr), len(0)
{
  MemoryFile mf;
  mf.stream = src;
  mf.length = (int) size;
  zlib_filefunc_def zlib_ff;
  fill_mem_filefunc(&zlib_ff, &mf);

  unzFile fid = unzOpen2(nullptr, &zlib_ff);

  if (!fid) {
    unzClose(fid);
    throw InputSourceException("Can't locate file in JAR content: '" + *path + "'");
  }
  int ret = unzLocateFile(fid, UStr::to_stdstr(path).c_str(), 0);
  if (ret != UNZ_OK) {
    unzClose(fid);
    throw InputSourceException("Can't locate file in JAR content: '" + *path + "'");
  }
  unz_file_info file_info;
  ret = unzGetCurrentFileInfo(fid, &file_info, nullptr, 0, nullptr, 0, nullptr, 0);
  if (ret != UNZ_OK) {
    unzClose(fid);
    throw InputSourceException("Can't retrieve current file in JAR content: '" + *path + "'");
  }

  len = file_info.uncompressed_size;
  stream.reset(new byte[len]);
  ret = unzOpenCurrentFile(fid);
  if (ret != UNZ_OK) {
    unzClose(fid);
    throw InputSourceException("Can't open current file in JAR content: '" + *path + "'");
  }
  ret = unzReadCurrentFile(fid, stream.get(), len);
  if (ret <= 0) {
    unzClose(fid);
    throw InputSourceException("Can't read current file in JAR content: '" + *path + "' (" + ret + ")");
  }
  ret = unzCloseCurrentFile(fid);
  if (ret == UNZ_CRCERROR) {
    unzClose(fid);
    throw InputSourceException("Bad JAR file CRC");
  }
  unzClose(fid);
}

XMLFilePos UnZip::curPos() const
{
  return mPos;
}

XMLSize_t UnZip::readBytes(XMLByte* const toFill, const XMLSize_t maxToRead)
{
  mBoundary = len;
  XMLSize_t remain = mBoundary - mPos;
  XMLSize_t toRead = (maxToRead < remain) ? maxToRead : remain;
  memcpy(toFill, stream.get() + mPos, toRead);
  mPos += toRead;
  return toRead;
}

const XMLCh* UnZip::getContentType() const
{
  return nullptr;
}

UnZip::~UnZip() = default;
