#include <colorer/xml/ZipXmlInputSource.h>
#include <minizip/unzip.h>
#include <xercesc/util/XMLString.hpp>
#include <colorer/io/MemoryFile.h>

ZipXmlInputSource::ZipXmlInputSource(const XMLCh* path, const XMLCh* base)
{
  create(path, base);
}

ZipXmlInputSource::ZipXmlInputSource(const XMLCh* path, XmlInputSource* base)
{
  const XMLCh* base_path = nullptr;
  if (base) {
    base_path = base->getInputSource()->getSystemId();
  }
  create(path, base_path);
}

void ZipXmlInputSource::create(const XMLCh* path, const XMLCh* base)
{
  if (!path || *path == '\0') {
    throw InputSourceException(CString("Can't create jar source"));
  }
  if (xercesc::XMLString::startsWith(path, kJar)) {
    int path_idx = xercesc::XMLString::lastIndexOf(path, '!');
    if (path_idx == -1) {
      throw InputSourceException(SString("Bad jar uri format: ") + CString(path));
    }

    std::unique_ptr<XMLCh[]> bpath( new XMLCh[path_idx - 4 + 1]);
    xercesc::XMLString::subString(bpath.get(), path, 4, path_idx);
    jar_input_source = SharedXmlInputSource::getSharedInputSource(bpath.get(), base);

    in_jar_location.reset(new SString(CString(path), path_idx + 1, -1));

  } else if (base != nullptr && xercesc::XMLString::startsWith(base, kJar)) {

    int base_idx = xercesc::XMLString::lastIndexOf(base, '!');
    if (base_idx == -1) {
      throw InputSourceException(SString("Bad jar uri format: ") + CString(path));
    }

    std::unique_ptr<XMLCh[]> bpath(new XMLCh[base_idx - 4 + 1]);
    xercesc::XMLString::subString(bpath.get(), base, 4, base_idx);
    jar_input_source = SharedXmlInputSource::getSharedInputSource(bpath.get(), nullptr);

    UString in_base(new SString(CString(base), base_idx + 1, -1));
    CString d_path = CString(path);
    in_jar_location = XmlInputSource::getAbsolutePath(in_base.get(), &d_path);

  } else {
    throw InputSourceException(CString("Can't create jar source"));
  }

  SString str("jar:");
  str.append(CString(jar_input_source->getInputSource()->getSystemId()));
  str.append(CString("!"));
  str.append(in_jar_location.get());
  setSystemId(str.getW2Chars());
}

ZipXmlInputSource::~ZipXmlInputSource()
{
  jar_input_source->delref();
}

uXmlInputSource ZipXmlInputSource::createRelative(const XMLCh* relPath) const
{
  return std::unique_ptr<ZipXmlInputSource>(new ZipXmlInputSource(relPath, this->getSystemId()));
}

xercesc::InputSource* ZipXmlInputSource::getInputSource()
{
  return this;
}

xercesc::BinInputStream* ZipXmlInputSource::makeStream() const
{
  return new UnZip(jar_input_source->getSrc(), jar_input_source->getSize(), in_jar_location.get());
}


UnZip::UnZip(const XMLByte* src, XMLSize_t size, const String* path)
  : mPos(0), mBoundary(0), stream(nullptr), len(0)
{
  MemoryFile* mf = new MemoryFile;
  mf->stream = src;
  mf->length = size;
  zlib_filefunc_def zlib_ff;
  fill_mem_filefunc(&zlib_ff, mf);

  unzFile fid = unzOpen2(nullptr, &zlib_ff);

  if (!fid) {
    delete mf;
    unzClose(fid);
    throw InputSourceException(SString("Can't locate file in JAR content: '") + path + "'");
  }
  int ret = unzLocateFile(fid, path->getChars(), 0);
  if (ret != UNZ_OK)  {
    delete mf;
    unzClose(fid);
    throw InputSourceException(SString("Can't locate file in JAR content: '") + path + "'");
  }
  unz_file_info file_info;
  ret = unzGetCurrentFileInfo(fid, &file_info, nullptr, 0, nullptr, 0, nullptr, 0);
  if (ret != UNZ_OK)  {
    delete mf;
    unzClose(fid);
    throw InputSourceException(SString("Can't retrieve current file in JAR content: '") + path + "'");
  }

  len = file_info.uncompressed_size;
  stream.reset(new byte[len]);
  ret = unzOpenCurrentFile(fid);
  if (ret != UNZ_OK)  {
    delete mf;
    unzClose(fid);
    throw InputSourceException(SString("Can't open current file in JAR content: '") + path + "'");
  }
  ret = unzReadCurrentFile(fid, stream.get(), len);
  if (ret <= 0) {
    delete mf;
    unzClose(fid);
    throw InputSourceException(SString("Can't read current file in JAR content: '") + path + "' (" + SString(ret) + ")");
  }
  ret = unzCloseCurrentFile(fid);
  if (ret == UNZ_CRCERROR) {
    delete mf;
    unzClose(fid);
    throw InputSourceException(SString("Bad JAR file CRC"));
  }
  ret = unzClose(fid);
  delete mf;
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

UnZip::~UnZip()
{
}


