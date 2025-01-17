/*
 * Copyright (C) 2004, 2006, 2007, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov <ap@nypop.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TextCodecICU_h
#define TextCodecICU_h

#include "base/gtest_prod_util.h"
#include "platform/wtf/text/TextCodec.h"
#include "platform/wtf/text/TextEncoding.h"
#include <memory>
#include <unicode/utypes.h>

typedef struct UConverter UConverter;

namespace WTF {

class TextCodecInput;

class TextCodecICU final : public TextCodec {
 public:
  static void RegisterEncodingNames(EncodingNameRegistrar);
  static void RegisterCodecs(TextCodecRegistrar);

  ~TextCodecICU() override;

 private:
  TextCodecICU(const TextEncoding&);
  WTF_EXPORT static std::unique_ptr<TextCodec> Create(const TextEncoding&,
                                                      const void*);

  String Decode(const char*,
                size_t length,
                FlushBehavior,
                bool stop_on_error,
                bool& saw_error) override;
  CString Encode(const UChar*, size_t length, UnencodableHandling) override;
  CString Encode(const LChar*, size_t length, UnencodableHandling) override;

  template <typename CharType>
  CString EncodeCommon(const CharType*, size_t length, UnencodableHandling);
  CString EncodeInternal(const TextCodecInput&, UnencodableHandling);

  void CreateICUConverter() const;
  void ReleaseICUConverter() const;
#if defined(USING_SYSTEM_ICU)
  bool needsGBKFallbacks() const { return m_needsGBKFallbacks; }
  void setNeedsGBKFallbacks(bool needsFallbacks) {
    m_needsGBKFallbacks = needsFallbacks;
  }
#endif

  int DecodeToBuffer(UChar* buffer,
                     UChar* buffer_limit,
                     const char*& source,
                     const char* source_limit,
                     int32_t* offsets,
                     bool flush,
                     UErrorCode&);

  TextEncoding encoding_;
  mutable UConverter* converter_icu_;
#if defined(USING_SYSTEM_ICU)
  mutable bool m_needsGBKFallbacks;
#endif

  FRIEND_TEST_ALL_PREFIXES(TextCodecICUTest, IgnorableCodePoint);
  FRIEND_TEST_ALL_PREFIXES(TextCodecICUTest, UTF32AndQuestionMarks);
  FRIEND_TEST_ALL_PREFIXES(TextCodecICUTest, UTF32Aliases);
};

struct ICUConverterWrapper {
  WTF_MAKE_NONCOPYABLE(ICUConverterWrapper);
  USING_FAST_MALLOC(ICUConverterWrapper);

 public:
  ICUConverterWrapper() : converter(0) {}
  ~ICUConverterWrapper();

  UConverter* converter;
};

}  // namespace WTF

#endif  // TextCodecICU_h
