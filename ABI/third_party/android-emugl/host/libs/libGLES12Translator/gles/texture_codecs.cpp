/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "gles/texture_codecs.h"

#include <GLES/gl.h>
#ifndef _WIN32
#include <netinet/in.h>  // for __BYTE_ORDER
#else
// Windows is little-endian only
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN    4321
#define __BYTE_ORDER __LITTLE_ENDIAN
#endif
#include "common/alog.h"
#include "gles/debug.h"

#if __APPLE__
#define __LITTLE_ENDIAN 1
#define __BYTE_ORDER __LITTLE_ENDIAN
#endif

namespace {

// Static (compile time) count of one bits in an integer constant.
// StaticPopulationCount<0>::kValue == 0
// StaticPopulationCount<1>::kValue == 1
// StaticPopulationCount<2>::kValue == 1
// StaticPopulationCount<3>::kValue == 2
// StaticPopulationCount<4>::kValue == 1
// StaticPopulationCount<5>::kValue == 2
// StaticPopulationCount<6>::kValue == 2
// StaticPopulationCount<7>::kValue == 3
// (etc)
template <size_t N> struct StaticPopulationCount {
  static const size_t kValue = 1 + StaticPopulationCount<(N & (N - 1))>::kValue;
};
template <> struct StaticPopulationCount<0> {
  static const size_t kValue = 0;
};

// Static (compile time) leading zero bit count of an integer constant.
// StaticLeadingZeroCount<0>::kValue == 32
// StaticLeadingZeroCount<1>::kValue == 0
// StaticLeadingZeroCount<2>::kValue == 1
// StaticLeadingZeroCount<3>::kValue == 0
// StaticLeadingZeroCount<4>::kValue == 2
// StaticLeadingZeroCount<5>::kValue == 0
// StaticLeadingZeroCount<6>::kValue == 1
// StaticLeadingZeroCount<7>::kValue == 0
// (etc)
template <uint32_t N> struct StaticLeadingZeroCount {
  static const uint32_t _0 = N;
  static const uint32_t _1 = _0 | (_0 << 1);
  static const uint32_t _2 = _1 | (_1 << 2);
  static const uint32_t _3 = _2 | (_2 << 4);
  static const uint32_t _4 = _3 | (_3 << 8);
  static const uint32_t _5 = _4 | (_4 << 16);
  static const size_t kValue = StaticPopulationCount<~_5>::kValue;
};

// Broadcast(component, bits) fills in missing bits in the lower bits of an
// eight bit value, with the goal of expanding a value to the full range of an
// eight bit integer.
//
// The input 'component' should be the value to fully expand to eight bits, and
// should have its most significant bits set correctly, and the remaining bits
// should be zero. 'bits' indicates how many bits at the top are set.
//
// For example, for a one bit value, the topmost bit is just used to fill the
// entire byte:
//
//     Broadcast(0x00, 1) -> 0x00
//     Broadcast(0x80, 1) -> 0xff
//
// For a four bit value, there are sixteen expected inputs, with the lower four
// bits just set identically to the upper four bits on output:
//
//     Broadcast(0x00, 4) -> 0x00
//     Broadcast(0x10, 4) -> 0x11
//     Broadcast(0x20, 4) -> 0x22
//     . . .
//     Broadcast(0xf0, 4) -> 0xff
uint8_t Broadcast(uint8_t component, int bits) {
  if (bits < 8)
    component |= component >> (bits * 1);
  if (bits < 4)
    component |= component >> (bits * 2);
  if (bits < 2)
    component |= component >> (bits * 4);
  return component;
}

// Bitwise shift helper function
//
// In C/C++, the right hand argument to the ">>" operator is expected to be
// positive, otherwise the behavior is undefined.
//
// This function chooses between left and right shifts
uint32_t ShiftRight(uint32_t n, int cnt) {
  return (cnt >= 0) ? (n >> cnt) : (n << -cnt);
}

// Represents a single bit-packed component.
//
// Defines functionality to extract/unpacked the component from the packed value
// as a uint8_t value, or to take a uint8_t value and form the packed value
// using it.
template <uint32_t kMask, uint8_t kDefault> struct PackedComponent {
  static const int kMaskShift = StaticLeadingZeroCount<kMask>::kValue;
  static const int kMaskWidth = StaticPopulationCount<kMask>::kValue;
  // Compute the (possibly negative) right shift amount such that the unpacked
  // component will be in the most significant bits of a byte.
  // This shift amount is also used to pack up the component again.
  static const int kUnpackShift = kMaskWidth + kMaskShift - 8;
  static const int kPackShift = -kUnpackShift;

  static uint8_t Unpack(uint32_t pv) {
    uint8_t masked = static_cast<uint8_t>(ShiftRight(pv & kMask, kUnpackShift));
    return Broadcast(masked, kMaskWidth);
  }

  static uint32_t Pack(uint8_t v) {
    uint32_t shifted = ShiftRight(v, kPackShift);
    uint32_t packed = shifted & kMask;
    return packed;
  }
};
// This specialization handles missing color and alpha components, by using the
// value kDefault when unpacking. When packing a value, nothing is written.
// See es_full_spec1.1.12.pdf section 3.6 "Final Expansion to RGBA"
template <uint8_t kDefault> struct PackedComponent<0, kDefault> {
  static uint8_t Unpack(uint32_t pv) {
    return kDefault;
  }

  static uint32_t Pack(uint8_t v) {
    return 0;
  }
};

// Defines the order of bytes in an N byte integer for a Little Endian (LE), Big
// Endian (BE), the current (Native) architecture.
//
// Also defines SwapBE to ensure an integer value is in Big Endian order.
struct Memory2ByteFormatLE {
  typedef PackedComponent<0x0000ff, 0> C0;
  typedef PackedComponent<0x00ff00, 0> C1;
};

struct Memory2ByteFormatBE {
  typedef PackedComponent<0x00ff00, 0> C0;
  typedef PackedComponent<0x0000ff, 0> C1;
};

uint32_t SwapEndian(uint32_t v) {
  uint32_t result = 0;
  result |= (v >> 24) & 0x000000ff;
  result |= (v >>  8) & 0x0000ff00;
  result |= (v <<  8) & 0x00ff0000;
  result |= (v << 24) & 0xff000000;
  return result;
}

#ifdef __BYTE_ORDER
    # if __BYTE_ORDER == __LITTLE_ENDIAN
    typedef Memory2ByteFormatLE Memory2ByteFormatNative;

    uint32_t SwapBE(uint32_t v) {
        return SwapEndian(v);
    }

    # elif __BYTE_ORDER == __BIG_ENDIAN
    typedef Memory2ByteFormatBE Memory2ByteFormatNative;

    uint32_t SwapBE(uint32_t v) {
        return v;
    }
    # else
    #  error Unknown endian
    # endif
    #else
    #  error Unknown endian
#endif /* __BYTE_ORDER */

template <typename StorageInt, uint32_t kRedMask, uint32_t kGreenMask,
         uint32_t kBlueMask, uint32_t kAlphaMask>
struct PackedFormat {
  typedef StorageInt StorageType;
  typedef PackedComponent<kRedMask, 0> Red;
  typedef PackedComponent<kGreenMask, 0> Green;
  typedef PackedComponent<kBlueMask, 0> Blue;
  typedef PackedComponent<kAlphaMask, 255> Alpha;
};

bool IsAligned(const void* ptr, int align) {
  if (align == 1)
    return true;
  if (align == 3)
    return false;

  return ((reinterpret_cast<intptr_t>(ptr) & (align - 1)) == 0);
};

// Defines the intermediate format used when encoding and decoding all other
// formats.
//
// This format allow the data conversion to any format by only supporting N
// conversions to/from this intermediate format rather than the N*N conversions
// directly between every possible format.
//
// All memory access using this format are expected to be aligned.

typedef PackedFormat<uint32_t, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff>
    IntermediateFormat;

template <typename SrcF, typename DstF>
struct Convert {
  static typename DstF::StorageType Apply(typename SrcF::StorageType v) {
    typename DstF::StorageType result = 0;
    result |= DstF::Red::Pack(SrcF::Red::Unpack(v));
    result |= DstF::Green::Pack(SrcF::Green::Unpack(v));
    result |= DstF::Blue::Pack(SrcF::Blue::Unpack(v));
    result |= DstF::Alpha::Pack(SrcF::Alpha::Unpack(v));
    return result;
  }
};

template <typename P, GLenum F, GLenum T>
struct PackedShortCodec {
  static const size_t kSize = 2;
  static const GLenum kFormat = F;
  static const GLenum kType = T;

  static uint32_t ReadAligned(const uint8_t* p) {
    return Convert<P, IntermediateFormat>::Apply(
        *reinterpret_cast<const uint16_t*>(p));
  }

  static void WriteAligned(uint8_t* p, uint32_t v) {
    *reinterpret_cast<uint16_t*>(p) = Convert<IntermediateFormat, P>::Apply(v);
  }

  static uint32_t ReadUnaligned(const uint8_t* p) {
    uint16_t pv = Memory2ByteFormatNative::C0::Pack(*p++);
    pv |= Memory2ByteFormatNative::C1::Pack(*p++);
    return Convert<P, IntermediateFormat>::Apply(pv);
  }

  static void WriteUnaligned(uint8_t* p, uint32_t v) {
    uint16_t pv = Convert<IntermediateFormat, P>::Apply(v);
    *p++ = Memory2ByteFormatNative::C0::Unpack(pv);
    *p++ = Memory2ByteFormatNative::C1::Unpack(pv);
  }
};

// In addition to the packed pixel types, OpenGL has a notion of storing the
// pixels as a simple sequence of bytes in memory. To read it, you just read the
// bytes in sequence out of memory. The first byte read is the first component,
// and so on.
//
// The number and meaning of each component is defined by format parameter. For
// example, if the format is RGBA, then there are for components, and the first
// component is R (Red), and the last is A.
//
// Note there are no padding/alignment bytes. For the RGB format for example,
// the data for the R component of the next pixel follows immediately after the
// B component for the previous pixel.

template <bool kHasRed, bool kHasGreen, bool kHasBlue, bool kHasLuminance,
         bool kHasAlpha, GLenum F>
struct BytestreamCodec {
  static const size_t kSize =
    kHasRed + kHasGreen + kHasBlue + kHasLuminance + kHasAlpha;
  static const GLenum kFormat = F;
  static const GLenum kType = GL_UNSIGNED_BYTE;

  static uint32_t ReadUnaligned(const uint8_t* p) {
    // Byte stream is always aligned.
    return ReadAligned(p);
  }

  static void WriteUnaligned(uint8_t* p, uint32_t v) {
    // Byte stream is always aligned.
    WriteAligned(p, v);
  }

  static uint32_t ReadAligned(const uint8_t* p) {
    uint32_t v = 0;
    if (kHasRed)
      v |= IntermediateFormat::Red::Pack(*p++);
    if (kHasGreen)
      v |= IntermediateFormat::Green::Pack(*p++);
    if (kHasBlue)
      v |= IntermediateFormat::Blue::Pack(*p++);
    if (kHasLuminance) {
      // Note: See es_full_spec_1.1.12.pdf section 3.6, "Conversion to
      // RGB".
      uint8_t luminance = *p++;
      v |= IntermediateFormat::Red::Pack(luminance);
      v |= IntermediateFormat::Blue::Pack(luminance);
      v |= IntermediateFormat::Green::Pack(luminance);
    }
    if (kHasAlpha)
      v |= IntermediateFormat::Alpha::Pack(*p++);
    else
      v |= IntermediateFormat::Alpha::Pack(255);
    return v;
  }


  static void WriteAligned(uint8_t* p, uint32_t v) {
    if (kHasRed)
      *p++ = IntermediateFormat::Red::Unpack(v);
    if (kHasGreen)
      *p++ = IntermediateFormat::Green::Unpack(v);
    if (kHasBlue)
      *p++ = IntermediateFormat::Blue::Unpack(v);
    if (kHasLuminance)
      // Note: es_full_spec_1.1.12.pdf section 3.7.1 table 3.8. gles uses red
      // as luminance.
      *p++ = IntermediateFormat::Red::Unpack(v);
    if (kHasAlpha)
      *p++ = IntermediateFormat::Alpha::Unpack(v);
  }
};

// As a special optimization for RGBA byte streams, allow them to be
// read and written directly out of memory as a stream of uint32_t instead.
// However this may mean that the bytes need to be swapped to match, and it also
// requires the memory to be aligned.
//
// Note: This code assumes that the IntermediateFormat is equivalent to this
// format after correcting for any endian differences.

struct CodecRgba {
  static const size_t kSize = 4;
  static const GLenum kFormat = GL_RGBA;
  static const GLenum kType = GL_UNSIGNED_BYTE;

  // This allows us to use the BytestreamCodec functionality for the unaligned
  // case, while specializing for the aligned case.
  typedef BytestreamCodec<true, true, true, false, true, GL_RGBA> Unaligned;

  static uint32_t ReadUnaligned(const uint8_t* p) {
    return Unaligned::ReadUnaligned(p);
  }

  static void WriteUnaligned(uint8_t* p, uint32_t v) {
    Unaligned::WriteUnaligned(p, v);
  }

  static uint32_t ReadAligned(const uint8_t* p) {
    uint32_t v = SwapBE(*reinterpret_cast<const uint32_t*>(p));
    return v;
  }

  static void WriteAligned(uint8_t* p, uint32_t v) {
    *reinterpret_cast<uint32_t*>(p) = SwapBE(v);
  }
};

// Core GLES1 only defines four simple packed formats.
// See es_full_spec1.1.12.pdf section 3.6 table 3.4 where "Type" is
// UNSIGNED_BYTE.

typedef BytestreamCodec<true, true, true, false, false, GL_RGB>
    CodecRgb;
typedef BytestreamCodec<false, false, false, true, true, GL_LUMINANCE_ALPHA>
    CodecLuminanceAlpha;
typedef BytestreamCodec<false, false, false, true, false, GL_LUMINANCE>
    CodecLuminance;
typedef BytestreamCodec<false, false, false, false, true, GL_ALPHA>
    CodecAlpha;

// For the packed pixel types, the components are always ordered from the most
// significant bits to the least significant bits.
//
// For example, if the type is UNSIGNED_SHORT_4_4_4_4, then the first component
// will be in the top four bits, and the last will be in the bottom four bits.
//
// The meaning of each component is defined by format parameter. For example, if
// the format is RGBA, then the first component is R (Red), and the last is A
// (Alpha).
//
// Core GLES1 only defines three simple packed formats.
//
// See es_full_spec1.1.12.pdf section 3.6 table 3.6

typedef PackedFormat<uint16_t, 0xf000, 0x0f00, 0x00f0, 0x000f>
    PackedShort4444Rgba;
typedef PackedFormat<uint16_t, 0xf800, 0x07c0, 0x003e, 0x0001>
    PackedShort5551Rgba;
typedef PackedFormat<uint16_t, 0xf800, 0x07e0, 0x001f, 0>
    PackedShort565Rgb;

typedef PackedShortCodec<PackedShort4444Rgba, GL_RGBA,
        GL_UNSIGNED_SHORT_4_4_4_4> CodecRgba4444;
typedef PackedShortCodec<PackedShort5551Rgba, GL_RGBA,
        GL_UNSIGNED_SHORT_5_5_5_1> CodecRgba5551;
typedef PackedShortCodec<PackedShort565Rgb, GL_RGB,
        GL_UNSIGNED_SHORT_5_6_5> CodecRgb565;

}  // end anonymous namespace

class ConverterBase {
 public:
  ConverterBase(size_t src_bpp, size_t dst_bpp)
    : src_bpp_(src_bpp),
      dst_bpp_(dst_bpp) {
  }

  virtual ~ConverterBase() {
  }

  virtual void* Convert(size_t width, size_t height, size_t alignment,
      const void* __restrict__ src, void* __restrict__ dst) const {
    ALOG_ASSERT(alignment == 1 || alignment == 2 ||
                alignment == 4 || alignment == 8);
    ALOG_ASSERT(src);
    ALOG_ASSERT(dst);
    ALOG_ASSERT(width != 0);
    ALOG_ASSERT(height != 0);

    size_t src_stride =
      (width * src_bpp_ + alignment - 1) & ~(alignment - 1);
    // Make output buffer 4 bytes aligned.
    size_t dst_stride = (width * dst_bpp_ + 3) & ~3;

    if (alignment >= src_bpp_ || IsAligned(src, src_bpp_)) {
      ConvertAligned(width, height,
          src_stride, static_cast<const uint8_t*>(src),
          dst_stride, static_cast<uint8_t*>(dst));
    } else {
      ConvertUnaligned(width, height,
          src_stride, static_cast<const uint8_t*>(src),
          dst_stride, static_cast<uint8_t*>(dst));
    }
    return dst;
  }

 protected:
  virtual void ConvertAligned(size_t width, size_t height,
      size_t src_stride, const uint8_t* __restrict__ src,
      size_t dst_stride, uint8_t* __restrict__ dst) const = 0;

  virtual void ConvertUnaligned(size_t width, size_t height,
      size_t src_stride, const uint8_t* __restrict__ src,
      size_t dst_stride, uint8_t* __restrict__ dst) const = 0;

  size_t src_bpp_;
  size_t dst_bpp_;
};

template <typename SrcF, typename DstF>
class OptimizedConverter : public ConverterBase {
 public:
  OptimizedConverter() : ConverterBase(SrcF::kSize, DstF::kSize) {}

 protected:
  virtual void ConvertAligned(size_t width, size_t height,
      size_t src_stride, const uint8_t* __restrict__ src,
      size_t dst_stride, uint8_t* __restrict__ dst) const {
    for (size_t y = 0; y < height; y++) {
      const uint8_t* src_next = src + src_stride;
      uint8_t* dst_next = dst + dst_stride;
      for (size_t x = 0; x < width; x++) {
        uint32_t v = SrcF::ReadAligned(src);
        DstF::WriteAligned(dst, v);
        src += SrcF::kSize;
        dst += DstF::kSize;
      }
      src = src_next;
      dst = dst_next;
    }
  }

  virtual void ConvertUnaligned(size_t width, size_t height,
      size_t src_stride, const uint8_t* __restrict__ src,
      size_t dst_stride, uint8_t* __restrict__ dst) const {
    for (size_t y = 0; y < height; y++) {
      const uint8_t* src_next = src + src_stride;
      uint8_t* dst_next = dst + dst_stride;
      for (size_t x = 0; x < width; x++) {
        uint32_t v = SrcF::ReadUnaligned(src);
        DstF::WriteAligned(dst, v);
        src += SrcF::kSize;
        dst += DstF::kSize;
      }
      src = src_next;
      dst = dst_next;
    }
  }
};

class GeneralConverter : public ConverterBase {
 public:
  typedef uint32_t (*ReadFunc)(const uint8_t *);
  typedef void (*WriteFunc)(uint8_t*, uint32_t);

  GeneralConverter(
      size_t src_bpp, ReadFunc read_aligned_cb, ReadFunc read_unaligned_cb,
      size_t dst_bpp, WriteFunc write_aligned_cb)
    : ConverterBase(src_bpp, dst_bpp),
      read_aligned_cb_(read_aligned_cb),
      read_unaligned_cb_(read_unaligned_cb),
      write_aligned_cb_(write_aligned_cb) {
    ALOG_ASSERT(read_aligned_cb_);
    ALOG_ASSERT(read_unaligned_cb_);
    ALOG_ASSERT(write_aligned_cb_);
  }

 protected:
  virtual void ConvertAligned(size_t width, size_t height,
      size_t src_stride, const uint8_t* __restrict__ src,
      size_t dst_stride, uint8_t* __restrict__ dst) const {
    for (size_t y = 0; y < height; y++) {
      const uint8_t* src_next = src + src_stride;
      uint8_t* dst_next = dst + dst_stride;
      for (size_t x = 0; x < width; x++) {
        uint32_t v = read_aligned_cb_(src);
        write_aligned_cb_(dst, v);
        src += src_bpp_;
        dst += dst_bpp_;
      }
      src = src_next;
      dst = dst_next;
    }
  }

  virtual void ConvertUnaligned(size_t width, size_t height,
      size_t src_stride, const uint8_t* __restrict__ src,
      size_t dst_stride, uint8_t* __restrict__ dst) const {
    for (size_t y = 0; y < height; y++) {
      const uint8_t* src_next = src + src_stride;
      uint8_t* dst_next = dst + dst_stride;
      for (size_t x = 0; x < width; x++) {
        uint32_t v = read_unaligned_cb_(src);
        write_aligned_cb_(dst, v);
        src += src_bpp_;
        dst += dst_bpp_;
      }
      src = src_next;
      dst = dst_next;
    }
  }

 private:
  ReadFunc read_aligned_cb_;
  ReadFunc read_unaligned_cb_;
  WriteFunc write_aligned_cb_;
};

struct Codec {
  GLenum format;
  GLenum type;
  size_t bpp;
  GeneralConverter::ReadFunc read_aligned_cb;
  GeneralConverter::ReadFunc read_unaligned_cb;
  GeneralConverter::WriteFunc write_aligned_cb;
};

#define CODEC_ENTRY(C) \
  { \
    C::kFormat, \
    C::kType, \
    C::kSize, \
    C::ReadAligned, \
    C::ReadUnaligned, \
    C::WriteAligned, \
  }

const Codec codecs[] = {
  CODEC_ENTRY(CodecRgb),
  CODEC_ENTRY(CodecRgba),
  CODEC_ENTRY(CodecRgba4444),
  CODEC_ENTRY(CodecRgba5551),
  CODEC_ENTRY(CodecRgb565),
  CODEC_ENTRY(CodecLuminance),
  CODEC_ENTRY(CodecLuminanceAlpha),
  CODEC_ENTRY(CodecAlpha),
};

TextureConverter::TextureConverter(GLenum src_format, GLenum src_type,
                                   GLenum dst_format, GLenum dst_type)
  : converter_(NULL),
    src_format_(src_format),
    src_type_(src_type),
    dst_format_(dst_format),
    dst_type_(dst_type) {
  InitializeConverter();
}

TextureConverter::~TextureConverter() {
  delete converter_;
}

bool TextureConverter::IsValid() const {
  return converter_ != NULL;
}

void TextureConverter::InitializeConverter() {
  // Generate optimized converters for frequently used conversions.
#define CONVERTER_ENTRY(S, D) \
  if (src_format_ == S::kFormat && src_type_ == S::kType && \
      dst_format_ == D::kFormat && dst_type_ == D::kType) { \
    converter_ = new OptimizedConverter<S, D>();          \
    return;                                               \
  }
  CONVERTER_ENTRY(CodecRgb565, CodecRgb);
  CONVERTER_ENTRY(CodecRgba4444, CodecRgba);
  CONVERTER_ENTRY(CodecRgba5551, CodecRgba);

  CONVERTER_ENTRY(CodecRgba, CodecRgb);
  CONVERTER_ENTRY(CodecRgba4444, CodecRgb);
  CONVERTER_ENTRY(CodecRgba5551, CodecRgb);
#undef CONVERTER_ENTRY

  const Codec* src = NULL;
  const Codec* dst = NULL;
  for (size_t i = 0; i < sizeof(codecs) / sizeof(codecs[0]); i++) {
    if (!src && codecs[i].format == src_format_ && codecs[i].type == src_type_)
      src = &codecs[i];
    if (!dst && codecs[i].format == dst_format_ && codecs[i].type == dst_type_)
      dst = &codecs[i];
    if (src && dst)
      break;
  }
  if (src && dst) {
    ALOGV("(%s, %s) to (%s, %s) conversion is not optimized.",
        GetEnumString(src_format_), GetEnumString(src_type_),
        GetEnumString(dst_format_), GetEnumString(dst_type_));
    converter_ = new GeneralConverter(src->bpp, src->read_aligned_cb,
                                      src->read_unaligned_cb, dst->bpp,
                                      dst->write_aligned_cb);
  }
}

void* TextureConverter::Convert(size_t width, size_t height, size_t alignment,
                                const void* __restrict__ src,
                                void* __restrict__ dst) const {
  return converter_->Convert(width, height, alignment, src, dst);
}
