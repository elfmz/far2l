#include "colorer/Exception.h"
#include "colorer/handlers/TextRegion.h"

TextRegion::TextRegion(std::shared_ptr<const UnicodeString>& _start_text, std::shared_ptr<const UnicodeString>& _end_text,
                       std::shared_ptr<const UnicodeString>& _start_back, std::shared_ptr<const UnicodeString>& _end_back)
{
  start_text = std::move(_start_text);
  end_text = std::move(_end_text);
  start_back = std::move(_start_back);
  end_back = std::move(_end_back);
  type = RegionDefine::RegionDefineType::TEXT_REGION;
}

TextRegion::TextRegion()
{
  type = RegionDefine::RegionDefineType::TEXT_REGION;
}

TextRegion::TextRegion(const TextRegion& rd) : RegionDefine()
{
  operator=(rd);
}

TextRegion& TextRegion::operator=(const TextRegion& rd)
{
  if (this == &rd)
    return *this;
  setValues(&rd);
  return *this;
}

const TextRegion* TextRegion::cast(const RegionDefine* rd)
{
  if (rd == nullptr)
    return nullptr;
  if (rd->type != RegionDefine::RegionDefineType::TEXT_REGION) {
    throw Exception("Bad type cast exception into TextRegion");
  }
  const auto* tr = (const TextRegion*) (rd);
  return tr;
}

void TextRegion::assignParent(const RegionDefine* _parent)
{
  const TextRegion* parent = TextRegion::cast(_parent);
  if (parent == nullptr)
    return;
  if (start_text == nullptr || end_text == nullptr) {
    start_text = parent->start_text;
    end_text = parent->end_text;
  }
  if (start_back == nullptr || end_back == nullptr) {
    start_back = parent->start_back;
    end_back = parent->end_back;
  }
}

void TextRegion::setValues(const RegionDefine* _rd)
{
  const TextRegion* rd = TextRegion::cast(_rd);
  if (rd) {
    start_text = rd->start_text;
    end_text = rd->end_text;
    start_back = rd->start_back;
    end_back = rd->end_back;
    type = rd->type;
  }
}

RegionDefine* TextRegion::clone() const
{
  RegionDefine* rd = new TextRegion(*this);
  return rd;
}
