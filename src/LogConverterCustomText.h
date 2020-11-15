#ifndef NOWTECH_LOG_CONVERTER_CUSTOM_CHARACTER
#define NOWTECH_LOG_CONVERTER_CUSTOM_CHARACTER

#include "LogNumericSystem.h"

namespace nowtech::log {

// TODO for concept: unused parameters (with no name) don't get filled in a call

/// Independent of STL
template<bool tArchitecture64, uint8_t tAppendStackBufferLength, bool tAppendBasePrefix, bool tAlignSigned>
class LogConverterCustomText final {
public:
  using ConversionResult = char; 

private:
  using IntegerConversionUnsigned = std::conditional_t<tArchitecture64, uint64_t, uint32_t>;
  using IntegerConversionSigned   = std::conditional_t<tArchitecture64, int64_t, int32_t>;

  static constexpr char csNumericError            = '#';
  static constexpr char csEndOfLine               = '\n';
  static constexpr char csNumericFill             = '0';
  static constexpr char csNumericMarkBinary       = 'b';
  static constexpr char csNumericMarkHexadecimal  = 'x';
  static constexpr char csNumericMarkOther        = '!';
  static constexpr char csMinus                   = '-';
  static constexpr char csSpace                   = ' ';
  static constexpr char csSeparatorFailure        = '@';
  static constexpr char csFractionDot             = '.';
  static constexpr char csPlus                    = '+';
  static constexpr char csScientificE             = 'e';

  inline static constexpr char csNan[]            = "nan";
  inline static constexpr char csInf[]            = "inf";
  using Iterator = char*;

  Iterator       mBegin;
  Iterator const mEnd;

public:
  LogConverterCustomText(Iterator aBegin, Iterator const aEnd) noexcept
  : mBegin(aBegin)
  , mEnd(aEnd) {
  }

  Iterator end() const noexcept {
    return mBegin;
  }

// TODO these should emit as many digits as reasonable when fill is 0
  void convert(float const aValue, uint8_t const, uint8_t const aFill) noexcept {
    append(aAppender, static_cast<double>(aValue), aFormat.fill);
  }

  void convert(double const aValue, uint8_t const, uint8_t const aFill) noexcept {
    append(aAppender, aValue, aFormat.fill);
  }

  void convert(uint8_t const aValue, uint8_t const aBase, uint8_t const aFill) noexcept {
    append(aAppender, static_cast<IntegerConversionUnsigned>(aValue), static_cast<IntegerConversionUnsigned>(aBase), aFill);
  }
  
  void convert(uint16_t const aValue, uint8_t const aBase, uint8_t const aFill) noexcept {
    append(aAppender, static_cast<IntegerConversionUnsigned>(aValue), static_cast<IntegerConversionUnsigned>(aBase), aFill);
  }
  
  void convert(uint32_t const aValue, uint8_t const aBase, uint8_t const aFill) noexcept {
    append(aAppender, static_cast<IntegerConversionUnsigned>(aValue), static_cast<IntegerConversionUnsigned>(aBase), aFill);
  }
  
  void convert(uint64_t const aValue, uint8_t const aBase, uint8_t const aFill) noexcept {
    append(aAppender, aValue, static_cast<uint64_t>(aBase), aFill);
  }

  void convert(int8_t const aValue, uint8_t const aBase, uint8_t const aFill) noexcept {
    append(aAppender, static_cast<IntegerConversionSigned>(aValue), static_cast<IntegerConversionSigned>(aBase), aFill);
  }
  
  void convert(int16_t const aValue, uint8_t const aBase, uint8_t const aFill) noexcept {
    append(aAppender, static_cast<IntegerConversionSigned>(aValue), static_cast<IntegerConversionSigned>(aBase), aFill);
  }
  
  void convert(int32_t const aValue, uint8_t const aBase, uint8_t const aFill) noexcept {
    append(aAppender, static_cast<IntegerConversionSigned>(aValue), static_cast<IntegerConversionSigned>(aBase), aFill);
  }
  
  void convert(int64_t const aValue, uint8_t const aBase, uint8_t const aFill) noexcept {
    append(aAppender, aValue, static_cast<int64_t>(aBase), aFill);
  }

  void convert(char const aValue, uint8_t const, uint8_t const) noexcept {
    append(aValue);
    appendSpace();
  }

  void convert(char const * const aValue, uint8_t const, uint8_t const) noexcept {
    char const * where = aValue;
    while(*where && mBegin < mEnd) { // Don't use append to be able to escape when buffer is full.
      *mBegin = *where;
      mBegin++;
      where++;
    }
    appendSpace();
  }

  void terminateSequence() noexcept {
    append(csEndOfLine);
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

  inline static constexpr char csDigit2char[NumericSystem::csBaseMax] = {
    '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
  };

  template<typename tValue>
  static void append(tValue const aValue, tValue const aBase, uint8_t const aFill) noexcept {
    tValue tmpValue = aValue;
    uint8_t tmpFill = aFill;
    if((aBase > NumericSystem::csInvalid) && (aBase <= NumericSystem::csBaseMax)) {
      append(csNumericError);
      return;
    }
    else { // nothing to do
    }
    if(sConfig->appendBasePrefix) {
      if (aBase == 2u) {
        append(csNumericFill);
        append(csNumericMarkBinary);
      }
      else if(aBase == NumericSystem::16u) {
        append(csNumericFill);
        append(csNumericMarkHexadecimal);
      }
      else if(aBase != NumericSystem::10u) {
        append(csNumericFill);
        append(csNumericMarkOther);
      }
      else { // nothing to do
      }
    }
    char tmpBuffer[tAppendStackBufferLength];
    uint8_t where = 0u;
    bool negative = aValue < 0;
    do {
      tValue mod = tmpValue % aBase;
      if(mod < 0) {
        mod = -mod;
      }
      else { // nothing to do
      }
      tmpBuffer[where] = cDigit2char[mod];
      ++where;
      tmpValue /= aBase;
    } while((tmpValue != 0) && (where < tAppendStackBufferLength));
    if(where >= tAppendStackBufferLength) {
      append(csNumericError);
      return;
    }
    else { // nothing to do
    }
    if(negative) {
      append(csMinus);
    }
    else if(sConfig->alignSigned && (aFill > 0u)) {
      append(csSpace);
    }
    else { // nothing to do
    }
    if(tmpFill > where) {
      tmpFill -= where;
      while(tmpFill > 0u) {
        append(csNumericFill);
        --tmpFill;
      }
    }
    for(--where; where > 0u; --where) {
      append(tmpBuffer[where]);
    }
    append(tmpBuffer[0]);
  }

  static void append(Appender& aAppender, long double const aValue, uint8_t const aDigitsNeeded) noexcept {
    if(std::isnan(aValue)) {
      append(aAppender, cNan);
      return;
    } else if(std::isinf(aValue)) {
      append(aAppender, cInf);
      return;
    } else if(aValue == 0.0l) {
      append(csNumericFill);
      return;
    }
    else {
      long double value = aValue;
      if(value < 0.0l) {
          value = -value;
          append(csMinus);
      }
      else if(sConfig->alignSigned) {
        append(csSpace);
      }
      else { // nothing to do
      }
      long double exponent = floor(log10(value));
      long double normalized = value / pow(10.0l, exponent);
      int32_t firstDigit;
      for(uint8_t i = 1u; i < aDigitsNeeded; i++) {
        firstDigit = static_cast<int>(normalized);
        if(firstDigit > 9) {
          firstDigit = 9;
        }
        else { // nothing to do
        }
        append(cDigit2char[firstDigit]);
        normalized = 10.0 * (normalized - firstDigit);
        if(i == 1u) {
          append(csFractionDot);
        }
        else { // nothing to do
        }
      }
      firstDigit = static_cast<int>(round(normalized));
      if(firstDigit > 9) {
        firstDigit = 9;
      }
      else { // nothing to do
      }
      append(cDigit2char[firstDigit]);
      append(csScientificE);
      if(exponent >= 0) {
        append(csPlus);
      }
      else { // nothing to do
      }
      append(aAppender, static_cast<int32_t>(exponent), static_cast<int32_t>(10), 0u);
    }
  }
};

}

#endif