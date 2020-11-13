#ifndef NOWTECH_LOG_CONVERTER_CUSTOM_CHARACTER
#define NOWTECH_LOG_CONVERTER_CUSTOM_CHARACTER

#include <cstdint>

namespace nowtech::log {

// TODO for concept: unused parameters (with no name) don't get filled in a call

/// Independent of STL
class LogConverterCustomCharacter final {
private:
  static constexpr char csSpace = ' ';
  static constexpr char csNumericError            = '#';

  static constexpr char csEndOfMessage            = '\r';
  static constexpr char csEndOfLine               = '\n';
  static constexpr char csNumericFill             = '0';
  static constexpr char csNumericMarkBinary       = 'b';
  static constexpr char csNumericMarkHexadecimal  = 'x';
  static constexpr char csMinus                   = '-';
  static constexpr char csSpace                   = ' ';
  static constexpr char csSeparatorFailure        = '@';
  static constexpr char csFractionDot             = '.';
  static constexpr char csPlus                    = '+';
  static constexpr char csScientificE             = 'e';


  inline static constexpr char csNan[]               = "nan";
  inline static constexpr char csInf[]               = "inf";
  using Iterator = char*;

  Iterator&      mBegin;
  Iterator const mEnd;

public:
  LogConverterCustomCharacter(Iterator &aBegin, Iterator const aEnd) noexcept
  : mBegin(aBegin)
  , mEnd(aEnd) {
  }

// TODO these should emit as many digits as reasonable when fill is 0
  void convert(float const aValue, uint8_t const aBase, uint8_t const aFill) noexcept {
  }

  void convert(double const aValue, uint8_t const aBase, uint8_t const aFill) noexcept {
  }

  void convert(uint8_t const aValue, uint8_t const aBase, uint8_t const aFill) noexcept {
  }
  
  void convert(uint16_t const aValue, uint8_t const aBase, uint8_t const aFill) noexcept {
  }
  
  void convert(uint32_t const aValue, uint8_t const aBase, uint8_t const aFill) noexcept {
  }
  
  void convert(uint64_t const aValue, uint8_t const aBase, uint8_t const aFill) noexcept {
  }

  void convert(int8_t const aValue, uint8_t const aBase, uint8_t const aFill) noexcept {
  }
  
  void convert(int16_t const aValue, uint8_t const aBase, uint8_t const aFill) noexcept {
  }
  
  void convert(int32_t const aValue, uint8_t const aBase, uint8_t const aFill) noexcept {
  }
  
  void convert(int64_t const aValue, uint8_t const aBase, uint8_t const aFill) noexcept {
  }

  void convert(char const aValue, uint8_t const, uint8_t const) noexcept {
    append(aValue);
    appendSpace();
  }

  void convert(char const * const aValue, uint8_t const, uint8_t const) noexcept {
    char const * where = aValue;
    while(*where && mBegin < mEnd) {
      *mBegin = *where;
      mBegin++;
      where++;
    }
    appendSpace();
  }

  void terminateSequence() noexcept {
    append(csEnd);    // TODO handle endofline, endofmessage
  }

private:
  void appendSpace() noexcept {
    append(csSpace);
  }

  void append(char const aValue) {
    if(mBegin < mEnd) {
      *mBegin = csSpace;
      ++mBegin;
    }
    else { // nothing to do
    }
  }
};

}

#endif