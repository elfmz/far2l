#include "colorer/handlers/TextRegion.h"
#include "colorer/Exception.h"

TextRegion::TextRegion(const UnicodeString& _start_text, const UnicodeString& _end_text,
                       const UnicodeString& _start_back, const UnicodeString& _end_back)
{
  type = RegionDefineType::TEXT_REGION;

  if (!_start_text.isEmpty()) {
    start_text = std::make_shared<UnicodeString>(_start_text);
  }
  if (!_end_text.isEmpty()) {
    end_text = std::make_shared<UnicodeString>(_end_text);
  }
  if (!_start_back.isEmpty()) {
    start_back = std::make_shared<UnicodeString>(_start_back);
  }
  if (!_end_back.isEmpty()) {
    end_back = std::make_shared<UnicodeString>(_end_back);
  }
}

TextRegion::TextRegion()
{
  type = RegionDefineType::TEXT_REGION;
}

TextRegion::TextRegion(const TextRegion& rd) : RegionDefine()
{
  operator=(rd);
}

TextRegion& TextRegion::operator=(const TextRegion& rd)
{
  if (this == &rd) {
    return *this;
  }

  setValues(&rd);
  return *this;
}

const TextRegion* TextRegion::cast(const RegionDefine* rd)
{
  if (rd == nullptr) {
    return nullptr;
  }
  if (rd->type != RegionDefineType::TEXT_REGION) {
    throw Exception("Bad type cast exception into TextRegion");
  }
  const auto* tr = dynamic_cast<const TextRegion*>(rd);
  return tr;
}

void TextRegion::assignParent(const RegionDefine* _parent)
{
  const TextRegion* parent = cast(_parent);
  if (parent == nullptr) {
    return;
  }
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
  const TextRegion* rd = cast(_rd);
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
