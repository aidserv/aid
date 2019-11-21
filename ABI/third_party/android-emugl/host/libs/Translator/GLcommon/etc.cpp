// Copyright 2009 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <GLcommon/etc.h>

#include <algorithm>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

typedef uint16_t etc1_uint16;

/* From http://www.khronos.org/registry/gles/extensions/OES/OES_compressed_ETC1_RGB8_texture.txt

 The number of bits that represent a 4x4 texel block is 64 bits if
 <internalformat> is given by ETC1_RGB8_OES.

 The data for a block is a number of bytes,

 {q0, q1, q2, q3, q4, q5, q6, q7}

 where byte q0 is located at the lowest memory address and q7 at
 the highest. The 64 bits specifying the block is then represented
 by the following 64 bit integer:

 int64bit = 256*(256*(256*(256*(256*(256*(256*q0+q1)+q2)+q3)+q4)+q5)+q6)+q7;

 ETC1_RGB8_OES:

 a) bit layout in bits 63 through 32 if diffbit = 0

 63 62 61 60 59 58 57 56 55 54 53 52 51 50 49 48
 -----------------------------------------------
 | base col1 | base col2 | base col1 | base col2 |
 | R1 (4bits)| R2 (4bits)| G1 (4bits)| G2 (4bits)|
 -----------------------------------------------

 47 46 45 44 43 42 41 40 39 38 37 36 35 34  33  32
 ---------------------------------------------------
 | base col1 | base col2 | table  | table  |diff|flip|
 | B1 (4bits)| B2 (4bits)| cw 1   | cw 2   |bit |bit |
 ---------------------------------------------------


 b) bit layout in bits 63 through 32 if diffbit = 1

 63 62 61 60 59 58 57 56 55 54 53 52 51 50 49 48
 -----------------------------------------------
 | base col1    | dcol 2 | base col1    | dcol 2 |
 | R1' (5 bits) | dR2    | G1' (5 bits) | dG2    |
 -----------------------------------------------

 47 46 45 44 43 42 41 40 39 38 37 36 35 34  33  32
 ---------------------------------------------------
 | base col 1   | dcol 2 | table  | table  |diff|flip|
 | B1' (5 bits) | dB2    | cw 1   | cw 2   |bit |bit |
 ---------------------------------------------------


 c) bit layout in bits 31 through 0 (in both cases)

 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16
 -----------------------------------------------
 |       most significant pixel index bits       |
 | p| o| n| m| l| k| j| i| h| g| f| e| d| c| b| a|
 -----------------------------------------------

 15 14 13 12 11 10  9  8  7  6  5  4  3   2   1  0
 --------------------------------------------------
 |         least significant pixel index bits       |
 | p| o| n| m| l| k| j| i| h| g| f| e| d| c | b | a |
 --------------------------------------------------


 Add table 3.17.2: Intensity modifier sets for ETC1 compressed textures:

 table codeword                modifier table
 ------------------        ----------------------
 0                     -8  -2  2   8
 1                    -17  -5  5  17
 2                    -29  -9  9  29
 3                    -42 -13 13  42
 4                    -60 -18 18  60
 5                    -80 -24 24  80
 6                   -106 -33 33 106
 7                   -183 -47 47 183


 Add table 3.17.3 Mapping from pixel index values to modifier values for
 ETC1 compressed textures:

 pixel index value
 ---------------
 msb     lsb           resulting modifier value
 -----   -----          -------------------------
 1       1            -b (large negative value)
 1       0            -a (small negative value)
 0       0             a (small positive value)
 0       1             b (large positive value)

 ETC2 codec:
     from https://www.khronos.org/registry/gles/specs/3.0/es_spec_3.0.4.pdf
     page 289
 */

static const int kRGBModifierTable[] = {
/* 0 */2, 8, -2, -8,
/* 1 */5, 17, -5, -17,
/* 2 */9, 29, -9, -29,
/* 3 */13, 42, -13, -42,
/* 4 */18, 60, -18, -60,
/* 5 */24, 80, -24, -80,
/* 6 */33, 106, -33, -106,
/* 7 */47, 183, -47, -183 };

static const int kRGBOpaqueModifierTable[] = {
/* 0 */0, 8, 0, -8,
/* 1 */0, 17, 0, -17,
/* 2 */0, 29, 0, -29,
/* 3 */0, 42, 0, -42,
/* 4 */0, 60, 0, -60,
/* 5 */0, 80, 0, -80,
/* 6 */0, 106, 0, -106,
/* 7 */0, 183, 0, -183 };

static const int kAlphaModifierTable[] = {
/* 0 */ -3, -6, -9, -15, 2, 5, 8, 14,
/* 1 */ -3, -7, -10, -13, 2, 6, 9, 12,
/* 2 */ -2, -5, -8, -13, 1, 4, 7, 12,
/* 3 */ -2, -4, -6, -13, 1, 3, 5, 12,
/* 4 */ -3, -6, -8, -12, 2, 5, 7, 11,
/* 5 */ -3, -7, -9, -11, 2, 6, 8, 10,
/* 6 */ -4, -7, -8, -11, 3, 6, 7, 10,
/* 7 */ -3, -5, -8, -11, 2, 4, 7, 10,
/* 8 */ -2, -6, -8, -10, 1, 5, 7, 9,
/* 9 */ -2, -5, -8, -10, 1, 4, 7, 9,
/* 10 */ -2, -4, -8, -10, 1, 3, 7, 9,
/* 11 */ -2, -5, -7, -10, 1, 4, 6, 9,
/* 12 */ -3, -4, -7, -10, 2, 3, 6, 9,
/* 13 */ -1, -2, -3, -10, 0, 1, 2, 9,
/* 14 */ -4, -6, -8, -9, 3, 5, 7, 8,
/* 15 */ -3, -5, -7, -9, 2, 4, 6, 8
};

static const int kLookup[8] = { 0, 1, 2, 3, -4, -3, -2, -1 };

static inline int clamp(int x) {
    return (x >= 0 ? (x < 255 ? x : 255) : 0);
}

static inline int clamp2047(int x) {
    return (x >= 0 ? (x < 2047 ? x : 2047) : 0);
}

static inline int clampSigned1023(int x) {
    return (x >= -1023 ? (x < 1023 ? x : 1023) : -1023);
}

static
inline int convert4To8(int b) {
    int c = b & 0xf;
    return (c << 4) | c;
}

static
inline int convert5To8(int b) {
    int c = b & 0x1f;
    return (c << 3) | (c >> 2);
}

static
inline int convert6To8(int b) {
    int c = b & 0x3f;
    return (c << 2) | (c >> 4);
}

static
inline int convert7To8(int b) {
    int c = b & 0x7f;
    return (c << 1) | (c >> 6);
}

static
inline int divideBy255(int d) {
    return (d + 128 + (d >> 8)) >> 8;
}

static
inline int convert8To4(int b) {
    int c = b & 0xff;
    return divideBy255(c * 15);
}

static
inline int convert8To5(int b) {
    int c = b & 0xff;
    return divideBy255(c * 31);
}

static
inline int convertDiff(int base, int diff) {
    return convert5To8((0x1f & base) + kLookup[0x7 & diff]);
}
static
int isOverflowed(int base, int diff) {
    int val = (0x1f & base) + kLookup[0x7 & diff];
    return val < 0 || val >= 32;
}

static
void decode_subblock(etc1_byte* pOut, int r, int g, int b, const int* table,
        etc1_uint32 low, bool second, bool flipped, bool isPunchthroughAlpha,
        bool opaque) {
    int baseX = 0;
    int baseY = 0;
    int channels = isPunchthroughAlpha ? 4 : 3;
    if (second) {
        if (flipped) {
            baseY = 2;
        } else {
            baseX = 2;
        }
    }
    for (int i = 0; i < 8; i++) {
        int x, y;
        if (flipped) {
            x = baseX + (i >> 1);
            y = baseY + (i & 1);
        } else {
            x = baseX + (i >> 2);
            y = baseY + (i & 3);
        }
        int k = y + (x * 4);
        int msb = ((low >> (k + 15)) & 2);
        int lsb = ((low >> k) & 1);
        etc1_byte* q = pOut + channels * (x + 4 * y);
        if (isPunchthroughAlpha && !opaque && msb && !lsb) {
            // rgba all 0
            memset(q, 0, 4);
            q += 4;
        } else {
            int offset = lsb | msb;
            int delta = table[offset];
            *q++ = clamp(r + delta);
            *q++ = clamp(g + delta);
            *q++ = clamp(b + delta);
            if (isPunchthroughAlpha) {
                *q++ = 255;
            }
        }
    }
}

static void etc2_T_H_index(const int* clrTable, etc1_uint32 low,
                           bool isPunchthroughAlpha, bool opaque,
                           etc1_byte* pOut) {
    etc1_byte* q = pOut;
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            int k = y + x * 4;
            int msb = (low >> (k + 15)) & 2;
            int lsb = (low >> k) & 1;
            if (isPunchthroughAlpha && !opaque && msb && !lsb) {
                // rgba all 0
                memset(q, 0, 4);
                q += 4;
            } else {
                int offset = lsb | msb;
                for (int c = 0; c < 3; c++) {
                    *q++ = clrTable[offset*3 + c];
                }
                if (isPunchthroughAlpha) {
                    *q++ = 255;
                }
            }
        }
    }
}

// ETC2 codec:
//     from https://www.khronos.org/registry/gles/specs/3.0/es_spec_3.0.4.pdf
//     page 289

static void etc2_decode_block_T(etc1_uint32 high, etc1_uint32 low,
        bool isPunchthroughAlpha, bool opaque, etc1_byte* pOut) {
    const int LUT[] = {3, 6, 11, 16, 23, 32, 41, 64};
    int r1, r2, g1, g2, b1, b2;
    r1 = convert4To8((((high >> 27) & 3) << 2) | ((high >> 24) & 3));
    g1 = convert4To8(high >> 20);
    b1 = convert4To8(high >> 16);
    r2 = convert4To8(high >> 12);
    g2 = convert4To8(high >> 8);
    b2 = convert4To8(high >> 4);
    // 3 bits intense modifier
    int intenseIdx = (((high >> 2) & 3) << 1) | (high & 1);
    int intenseMod = LUT[intenseIdx];
    int clrTable[12];
    clrTable[0] = r1;
    clrTable[1] = g1;
    clrTable[2] = b1;
    clrTable[3] = clamp(r2 + intenseMod);
    clrTable[4] = clamp(g2 + intenseMod);
    clrTable[5] = clamp(b2 + intenseMod);
    clrTable[6] = r2;
    clrTable[7] = g2;
    clrTable[8] = b2;
    clrTable[9] = clamp(r2 - intenseMod);
    clrTable[10] = clamp(g2 - intenseMod);
    clrTable[11] = clamp(b2 - intenseMod);
    etc2_T_H_index(clrTable, low, isPunchthroughAlpha, opaque, pOut);
}

static void etc2_decode_block_H(etc1_uint32 high, etc1_uint32 low,
        bool isPunchthroughAlpha, bool opaque, etc1_byte* pOut) {
    const int LUT[] = {3, 6, 11, 16, 23, 32, 41, 64};
    int r1, r2, g1, g2, b1, b2;
    r1 = convert4To8(high >> 27);
    g1 = convert4To8((high >> 24) << 1 | ((high >> 20) & 1));
    b1 = convert4To8((high >> 19) << 3 | ((high >> 15) & 7));
    r2 = convert4To8(high >> 11);
    g2 = convert4To8(high >> 7);
    b2 = convert4To8(high >> 3);
    // 3 bits intense modifier
    int intenseIdx = high & 4;
    intenseIdx |= (high & 1) << 1;
    intenseIdx |= (((r1 << 16) | (g1 << 8) | b1) >= ((r2 << 16) | (g2 << 8) | b2));
    int intenseMod = LUT[intenseIdx];
    int clrTable[12];
    clrTable[0] = clamp(r1 + intenseMod);
    clrTable[1] = clamp(g1 + intenseMod);
    clrTable[2] = clamp(b1 + intenseMod);
    clrTable[3] = clamp(r1 - intenseMod);
    clrTable[4] = clamp(g1 - intenseMod);
    clrTable[5] = clamp(b1 - intenseMod);
    clrTable[6] = clamp(r2 + intenseMod);
    clrTable[7] = clamp(g2 + intenseMod);
    clrTable[8] = clamp(b2 + intenseMod);
    clrTable[9] = clamp(r2 - intenseMod);
    clrTable[10] = clamp(g2 - intenseMod);
    clrTable[11] = clamp(b2 - intenseMod);
    etc2_T_H_index(clrTable, low, isPunchthroughAlpha, opaque, pOut);
}

static void etc2_decode_block_P(etc1_uint32 high, etc1_uint32 low,
        bool isPunchthroughAlpha, etc1_byte* pOut) {
    int ro, go, bo, rh, gh, bh, rv, gv, bv;
    uint64_t data = high;
    data = data << 32 | low;
    ro = convert6To8(data >> 57);
    go = convert7To8((data >> 56 << 6) | ((data >> 49) & 63));
    bo = convert6To8((data >> 48 << 5)
            | (((data >> 43) & 3 ) << 3)
            | ((data >> 39) & 7));
    rh = convert6To8((data >> 34 << 1) | ((data >> 32) & 1));
    gh = convert7To8(data >> 25);
    bh = convert6To8(data >> 19);
    rv = convert6To8(data >> 13);
    gv = convert7To8(data >> 6);
    bv = convert6To8(data);
    etc1_byte* q = pOut;
    for (int i = 0; i < 16; i++) {
        int y = i >> 2;
        int x = i & 3;
        *q++ = clamp((x * (rh - ro) + y * (rv - ro) + 4 * ro + 2) >> 2);
        *q++ = clamp((x * (gh - go) + y * (gv - go) + 4 * go + 2) >> 2);
        *q++ = clamp((x * (bh - bo) + y * (bv - bo) + 4 * bo + 2) >> 2);
        if (isPunchthroughAlpha) *q++ = 255;
    }
}

// Input is an ETC1 / ETC2 compressed version of the data.
// Output is a 4 x 4 square of 3-byte pixels in form R, G, B
// ETC2 codec:
//     from https://www.khronos.org/registry/gles/specs/3.0/es_spec_3.0.4.pdf
//     page 289

void etc2_decode_rgb_block(const etc1_byte* pIn, bool isPunchthroughAlpha,
                           etc1_byte* pOut) {
    etc1_uint32 high = (pIn[0] << 24) | (pIn[1] << 16) | (pIn[2] << 8) | pIn[3];
    etc1_uint32 low = (pIn[4] << 24) | (pIn[5] << 16) | (pIn[6] << 8) | pIn[7];
    bool opaque = (high >> 1) & 1;
    int r1, r2, g1, g2, b1, b2;
    if (isPunchthroughAlpha || high & 2) {
        // differential
        int rBase = high >> 27;
        int gBase = high >> 19;
        int bBase = high >> 11;
        if (isOverflowed(rBase, high >> 24)) {
            etc2_decode_block_T(high, low, isPunchthroughAlpha, opaque, pOut);
            return;
        }
        if (isOverflowed(gBase, high >> 16)) {
            etc2_decode_block_H(high, low, isPunchthroughAlpha, opaque, pOut);
            return;
        }
        if (isOverflowed(bBase, high >> 8)) {
            etc2_decode_block_P(high, low, isPunchthroughAlpha, pOut);
            return;
        }
        r1 = convert5To8(rBase);
        r2 = convertDiff(rBase, high >> 24);
        g1 = convert5To8(gBase);
        g2 = convertDiff(gBase, high >> 16);
        b1 = convert5To8(bBase);
        b2 = convertDiff(bBase, high >> 8);
    } else {
        // not differential
        r1 = convert4To8(high >> 28);
        r2 = convert4To8(high >> 24);
        g1 = convert4To8(high >> 20);
        g2 = convert4To8(high >> 16);
        b1 = convert4To8(high >> 12);
        b2 = convert4To8(high >> 8);
    }
    int tableIndexA = 7 & (high >> 5);
    int tableIndexB = 7 & (high >> 2);
    const int* rgbModifierTable = opaque || !isPunchthroughAlpha ?
                                  kRGBModifierTable : kRGBOpaqueModifierTable;
    const int* tableA = rgbModifierTable + tableIndexA * 4;
    const int* tableB = rgbModifierTable + tableIndexB * 4;
    bool flipped = (high & 1) != 0;
    decode_subblock(pOut, r1, g1, b1, tableA, low, false, flipped,
                    isPunchthroughAlpha, opaque);
    decode_subblock(pOut, r2, g2, b2, tableB, low, true, flipped,
                    isPunchthroughAlpha, opaque);
}

void eac_decode_single_channel_block(const etc1_byte* pIn,
                                     int decodedElementBytes, bool isSigned,
                                     etc1_byte* pOut) {
    assert(decodedElementBytes == 1 || decodedElementBytes == 2 || decodedElementBytes == 4);
    int base_codeword = isSigned ? reinterpret_cast<const char*>(pIn)[0]
                                 : pIn[0];
    if (base_codeword == -128) base_codeword = -127;
    int multiplier = pIn[1] >> 4;
    int tblIdx = pIn[1] & 15;
    const int* table = kAlphaModifierTable + tblIdx * 8;
    const etc1_byte* p = pIn + 2;
    // position in a byte of the next 3-bit index:
    // | a a a | b b b | c c c | d d d ...
    // | byte               | byte...
    int bitOffset = 5;
    for (int i = 0; i < 16; i ++) {
        // flip x, y in output
        int outIdx = (i % 4) * 4 + i / 4;
        etc1_byte* q = pOut + outIdx * decodedElementBytes;

        int modifier = 0;
        if (bitOffset < 0) { // (Part of) the index is in the next byte.
            modifier += ((int*)p)[0] << (-bitOffset);
            p ++;
            bitOffset += 8;
        }
        modifier += p[0] >> bitOffset;
        modifier &= 7;
        bitOffset -= 3; // move to the next index
        if (bitOffset == -3) {
            bitOffset = 5;
            p++;
        }
        int modifierValue = table[modifier];
        int decoded = base_codeword + modifierValue * multiplier;
        if (decodedElementBytes == 1) {
            *q = clamp(decoded);
        } else { // decodedElementBytes == 4
            decoded *= 8;
            if (multiplier == 0) {
                decoded += modifierValue;
            }
            if (isSigned) {
                decoded = clampSigned1023(decoded);
                reinterpret_cast<float*>(q)[0] = (float)decoded / 1023.0;
            } else {
                decoded += 4;
                decoded = clamp2047(decoded);
                reinterpret_cast<float*>(q)[0] = (float)decoded / 2047.0;
            }
        }
    }
}

typedef struct {
    etc1_uint32 high;
    etc1_uint32 low;
    etc1_uint32 score; // Lower is more accurate
} etc_compressed;

static
inline void take_best(etc_compressed* a, const etc_compressed* b) {
    if (a->score > b->score) {
        *a = *b;
    }
}

static
void etc_average_colors_subblock(const etc1_byte* pIn, etc1_uint32 inMask,
        etc1_byte* pColors, bool flipped, bool second) {
    int r = 0;
    int g = 0;
    int b = 0;

    if (flipped) {
        int by = 0;
        if (second) {
            by = 2;
        }
        for (int y = 0; y < 2; y++) {
            int yy = by + y;
            for (int x = 0; x < 4; x++) {
                int i = x + 4 * yy;
                if (inMask & (1 << i)) {
                    const etc1_byte* p = pIn + i * 3;
                    r += *(p++);
                    g += *(p++);
                    b += *(p++);
                }
            }
        }
    } else {
        int bx = 0;
        if (second) {
            bx = 2;
        }
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 2; x++) {
                int xx = bx + x;
                int i = xx + 4 * y;
                if (inMask & (1 << i)) {
                    const etc1_byte* p = pIn + i * 3;
                    r += *(p++);
                    g += *(p++);
                    b += *(p++);
                }
            }
        }
    }
    pColors[0] = (etc1_byte)((r + 4) >> 3);
    pColors[1] = (etc1_byte)((g + 4) >> 3);
    pColors[2] = (etc1_byte)((b + 4) >> 3);
}

static
inline int square(int x) {
    return x * x;
}

static etc1_uint32 chooseModifier(const etc1_byte* pBaseColors,
        const etc1_byte* pIn, etc1_uint32 *pLow, int bitIndex,
        const int* pModifierTable) {
    etc1_uint32 bestScore = ~0;
    int bestIndex = 0;
    int pixelR = pIn[0];
    int pixelG = pIn[1];
    int pixelB = pIn[2];
    int r = pBaseColors[0];
    int g = pBaseColors[1];
    int b = pBaseColors[2];
    for (int i = 0; i < 4; i++) {
        int modifier = pModifierTable[i];
        int decodedG = clamp(g + modifier);
        etc1_uint32 score = (etc1_uint32) (6 * square(decodedG - pixelG));
        if (score >= bestScore) {
            continue;
        }
        int decodedR = clamp(r + modifier);
        score += (etc1_uint32) (3 * square(decodedR - pixelR));
        if (score >= bestScore) {
            continue;
        }
        int decodedB = clamp(b + modifier);
        score += (etc1_uint32) square(decodedB - pixelB);
        if (score < bestScore) {
            bestScore = score;
            bestIndex = i;
        }
    }
    etc1_uint32 lowMask = (((bestIndex >> 1) << 16) | (bestIndex & 1))
            << bitIndex;
    *pLow |= lowMask;
    return bestScore;
}

static
void etc_encode_subblock_helper(const etc1_byte* pIn, etc1_uint32 inMask,
        etc_compressed* pCompressed, bool flipped, bool second,
        const etc1_byte* pBaseColors, const int* pModifierTable) {
    int score = pCompressed->score;
    if (flipped) {
        int by = 0;
        if (second) {
            by = 2;
        }
        for (int y = 0; y < 2; y++) {
            int yy = by + y;
            for (int x = 0; x < 4; x++) {
                int i = x + 4 * yy;
                if (inMask & (1 << i)) {
                    score += chooseModifier(pBaseColors, pIn + i * 3,
                            &pCompressed->low, yy + x * 4, pModifierTable);
                }
            }
        }
    } else {
        int bx = 0;
        if (second) {
            bx = 2;
        }
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 2; x++) {
                int xx = bx + x;
                int i = xx + 4 * y;
                if (inMask & (1 << i)) {
                    score += chooseModifier(pBaseColors, pIn + i * 3,
                            &pCompressed->low, y + xx * 4, pModifierTable);
                }
            }
        }
    }
    pCompressed->score = score;
}

static bool inRange4bitSigned(int color) {
    return color >= -4 && color <= 3;
}

static void etc_encodeBaseColors(etc1_byte* pBaseColors,
        const etc1_byte* pColors, etc_compressed* pCompressed) {
    int r1, g1, b1, r2, g2, b2; // 8 bit base colors for sub-blocks
    bool differential;
    {
        int r51 = convert8To5(pColors[0]);
        int g51 = convert8To5(pColors[1]);
        int b51 = convert8To5(pColors[2]);
        int r52 = convert8To5(pColors[3]);
        int g52 = convert8To5(pColors[4]);
        int b52 = convert8To5(pColors[5]);

        r1 = convert5To8(r51);
        g1 = convert5To8(g51);
        b1 = convert5To8(b51);

        int dr = r52 - r51;
        int dg = g52 - g51;
        int db = b52 - b51;

        differential = inRange4bitSigned(dr) && inRange4bitSigned(dg)
                && inRange4bitSigned(db);
        if (differential) {
            r2 = convert5To8(r51 + dr);
            g2 = convert5To8(g51 + dg);
            b2 = convert5To8(b51 + db);
            pCompressed->high |= (r51 << 27) | ((7 & dr) << 24) | (g51 << 19)
                    | ((7 & dg) << 16) | (b51 << 11) | ((7 & db) << 8) | 2;
        }
    }

    if (!differential) {
        int r41 = convert8To4(pColors[0]);
        int g41 = convert8To4(pColors[1]);
        int b41 = convert8To4(pColors[2]);
        int r42 = convert8To4(pColors[3]);
        int g42 = convert8To4(pColors[4]);
        int b42 = convert8To4(pColors[5]);
        r1 = convert4To8(r41);
        g1 = convert4To8(g41);
        b1 = convert4To8(b41);
        r2 = convert4To8(r42);
        g2 = convert4To8(g42);
        b2 = convert4To8(b42);
        pCompressed->high |= (r41 << 28) | (r42 << 24) | (g41 << 20) | (g42
                << 16) | (b41 << 12) | (b42 << 8);
    }
    pBaseColors[0] = r1;
    pBaseColors[1] = g1;
    pBaseColors[2] = b1;
    pBaseColors[3] = r2;
    pBaseColors[4] = g2;
    pBaseColors[5] = b2;
}

static
void etc_encode_block_helper(const etc1_byte* pIn, etc1_uint32 inMask,
        const etc1_byte* pColors, etc_compressed* pCompressed, bool flipped) {
    pCompressed->score = ~0;
    pCompressed->high = (flipped ? 1 : 0);
    pCompressed->low = 0;

    etc1_byte pBaseColors[6];

    etc_encodeBaseColors(pBaseColors, pColors, pCompressed);

    int originalHigh = pCompressed->high;

    const int* pModifierTable = kRGBModifierTable;
    for (int i = 0; i < 8; i++, pModifierTable += 4) {
        etc_compressed temp;
        temp.score = 0;
        temp.high = originalHigh | (i << 5);
        temp.low = 0;
        etc_encode_subblock_helper(pIn, inMask, &temp, flipped, false,
                pBaseColors, pModifierTable);
        take_best(pCompressed, &temp);
    }
    pModifierTable = kRGBModifierTable;
    etc_compressed firstHalf = *pCompressed;
    for (int i = 0; i < 8; i++, pModifierTable += 4) {
        etc_compressed temp;
        temp.score = firstHalf.score;
        temp.high = firstHalf.high | (i << 2);
        temp.low = firstHalf.low;
        etc_encode_subblock_helper(pIn, inMask, &temp, flipped, true,
                pBaseColors + 3, pModifierTable);
        if (i == 0) {
            *pCompressed = temp;
        } else {
            take_best(pCompressed, &temp);
        }
    }
}

static void writeBigEndian(etc1_byte* pOut, etc1_uint32 d) {
    pOut[0] = (etc1_byte)(d >> 24);
    pOut[1] = (etc1_byte)(d >> 16);
    pOut[2] = (etc1_byte)(d >> 8);
    pOut[3] = (etc1_byte) d;
}

// Input is a 4 x 4 square of 3-byte pixels in form R, G, B
// inmask is a 16-bit mask where bit (1 << (x + y * 4)) tells whether the corresponding (x,y)
// pixel is valid or not. Invalid pixel color values are ignored when compressing.
// Output is an ETC1 compressed version of the data.

void etc1_encode_block(const etc1_byte* pIn, etc1_uint32 inMask,
        etc1_byte* pOut) {
    etc1_byte colors[6];
    etc1_byte flippedColors[6];
    etc_average_colors_subblock(pIn, inMask, colors, false, false);
    etc_average_colors_subblock(pIn, inMask, colors + 3, false, true);
    etc_average_colors_subblock(pIn, inMask, flippedColors, true, false);
    etc_average_colors_subblock(pIn, inMask, flippedColors + 3, true, true);

    etc_compressed a, b;
    etc_encode_block_helper(pIn, inMask, colors, &a, false);
    etc_encode_block_helper(pIn, inMask, flippedColors, &b, true);
    take_best(&a, &b);
    writeBigEndian(pOut, a.high);
    writeBigEndian(pOut + 4, a.low);
}

// Return the size of the encoded image data (does not include size of PKM header).

etc1_uint32 etc1_get_encoded_data_size(etc1_uint32 width, etc1_uint32 height) {
    return (((width + 3) & ~3) * ((height + 3) & ~3)) >> 1;
}

etc1_uint32 etc_get_encoded_data_size(ETC2ImageFormat format, etc1_uint32 width,
                                      etc1_uint32 height) {
    etc1_uint32 size = ((width + 3) & ~3) * ((height + 3) & ~3);
    switch (format) {
        case EtcRGB8:
        case EtcRGB8A1:
        case EtcR11:
        case EtcSignedR11:
            return size >> 1;
        case EtcRG11:
        case EtcSignedRG11:
        case EtcRGBA8:
            return size;
        default:
            assert(0);
            return 0;
    }
}

etc1_uint32 etc_get_decoded_pixel_size(ETC2ImageFormat format) {
    switch (format) {
        case EtcRGB8:
            return 3;
        case EtcRGBA8:
            return 4;
        case EtcRGB8A1:
        case EtcR11:
        case EtcSignedR11:
            return 4;
        case EtcRG11:
        case EtcSignedRG11:
            return 8;
        default:
            assert(0);
            return 0;
    }
}

// Encode an entire image.
// pIn - pointer to the image data. Formatted such that the Red component of
//       pixel (x,y) is at pIn + pixelSize * x + stride * y + redOffset;
// pOut - pointer to encoded data. Must be large enough to store entire encoded image.

int etc1_encode_image(const etc1_byte* pIn, etc1_uint32 width, etc1_uint32 height,
        etc1_uint32 pixelSize, etc1_uint32 stride, etc1_byte* pOut) {
    if (pixelSize < 2 || pixelSize > 3) {
        return -1;
    }
    static const unsigned short kYMask[] = { 0x0, 0xf, 0xff, 0xfff, 0xffff };
    static const unsigned short kXMask[] = { 0x0, 0x1111, 0x3333, 0x7777,
            0xffff };
    etc1_byte block[ETC1_DECODED_BLOCK_SIZE];
    etc1_byte encoded[ETC1_ENCODED_BLOCK_SIZE];

    etc1_uint32 encodedWidth = (width + 3) & ~3;
    etc1_uint32 encodedHeight = (height + 3) & ~3;

    for (etc1_uint32 y = 0; y < encodedHeight; y += 4) {
        etc1_uint32 yEnd = height - y;
        if (yEnd > 4) {
            yEnd = 4;
        }
        int ymask = kYMask[yEnd];
        for (etc1_uint32 x = 0; x < encodedWidth; x += 4) {
            etc1_uint32 xEnd = width - x;
            if (xEnd > 4) {
                xEnd = 4;
            }
            int mask = ymask & kXMask[xEnd];
            for (etc1_uint32 cy = 0; cy < yEnd; cy++) {
                etc1_byte* q = block + (cy * 4) * 3;
                const etc1_byte* p = pIn + pixelSize * x + stride * (y + cy);
                if (pixelSize == 3) {
                    memcpy(q, p, xEnd * 3);
                } else {
                    for (etc1_uint32 cx = 0; cx < xEnd; cx++) {
                        int pixel = (p[1] << 8) | p[0];
                        *q++ = convert5To8(pixel >> 11);
                        *q++ = convert6To8(pixel >> 5);
                        *q++ = convert5To8(pixel);
                        p += pixelSize;
                    }
                }
            }
            etc1_encode_block(block, mask, encoded);
            memcpy(pOut, encoded, sizeof(encoded));
            pOut += sizeof(encoded);
        }
    }
    return 0;
}

// Decode an entire image.
// pIn - pointer to encoded data.
// pOut - pointer to the image data. Will be written such that the Red component of
//       pixel (x,y) is at pIn + pixelSize * x + stride * y + redOffset. Must be
//        large enough to store entire image.


int etc2_decode_image(const etc1_byte* pIn, ETC2ImageFormat format,
        etc1_byte* pOut,
        etc1_uint32 width, etc1_uint32 height,
        etc1_uint32 stride) {
    etc1_byte block[std::max({ETC1_DECODED_BLOCK_SIZE,
                              ETC2_DECODED_RGB8A1_BLOCK_SIZE,
                              EAC_DECODED_R11_BLOCK_SIZE,
                              EAC_DECODED_RG11_BLOCK_SIZE})];
    etc1_byte alphaBlock[EAC_DECODED_ALPHA_BLOCK_SIZE];

    etc1_uint32 encodedWidth = (width + 3) & ~3;
    etc1_uint32 encodedHeight = (height + 3) & ~3;

    int pixelSize = etc_get_decoded_pixel_size(format);
    bool isSigned = (format == EtcSignedR11 || format == EtcSignedRG11);

    for (etc1_uint32 y = 0; y < encodedHeight; y += 4) {
        etc1_uint32 yEnd = height - y;
        if (yEnd > 4) {
            yEnd = 4;
        }
        for (etc1_uint32 x = 0; x < encodedWidth; x += 4) {
            etc1_uint32 xEnd = width - x;
            if (xEnd > 4) {
                xEnd = 4;
            }
            switch (format) {
                case EtcRGBA8:
                    eac_decode_single_channel_block(pIn, 1, false, alphaBlock);
                    pIn += EAC_ENCODE_ALPHA_BLOCK_SIZE;
                    // Do not break
                    // Fall through to EtcRGB8 to decode the RGB part
                case EtcRGB8:
                    etc2_decode_rgb_block(pIn, false, block);
                    pIn += ETC1_ENCODED_BLOCK_SIZE;
                    break;
                case EtcRGB8A1:
                    etc2_decode_rgb_block(pIn, true, block);
                    pIn += ETC1_ENCODED_BLOCK_SIZE;
                    break;
                case EtcR11:
                case EtcSignedR11:
                    eac_decode_single_channel_block(pIn, 4, isSigned, block);
                    pIn += EAC_ENCODE_R11_BLOCK_SIZE;
                    break;
                case EtcRG11:
                case EtcSignedRG11:
                    // r channel
                    eac_decode_single_channel_block(pIn, 4, isSigned, block);
                    pIn += EAC_ENCODE_R11_BLOCK_SIZE;
                    // g channel
                    eac_decode_single_channel_block(pIn, 4, isSigned,
                            block + EAC_DECODED_R11_BLOCK_SIZE);
                    pIn += EAC_ENCODE_R11_BLOCK_SIZE;
                    break;
                default:
                    assert(0);
            }
            for (etc1_uint32 cy = 0; cy < yEnd; cy++) {
                etc1_byte* p = pOut + pixelSize * x + stride * (y + cy);
                switch (format) {
                    case EtcRGB8:
                    case EtcRGB8A1:
                    case EtcR11:
                    case EtcSignedR11: {
                            const etc1_byte* q = block + (cy * 4) * pixelSize;
                            memcpy(p, q, xEnd * pixelSize);
                        }
                        break;
                    case EtcRG11:
                    case EtcSignedRG11: {
                            const etc1_byte* r = block + cy * EAC_DECODED_R11_BLOCK_SIZE / 4;
                            const etc1_byte* g = block + cy * EAC_DECODED_R11_BLOCK_SIZE / 4 + EAC_DECODED_R11_BLOCK_SIZE;
                            int channelSize = pixelSize / 2;
                            for (etc1_uint32 cx = 0; cx < xEnd; cx++) {
                                memcpy(p, r, channelSize);
                                p += channelSize;
                                r += channelSize;
                                memcpy(p, g, channelSize);
                                p += channelSize;
                                g += channelSize;
                            }
                        }
                        break;
                    case EtcRGBA8: {
                            const etc1_byte* q = block + (cy * 4) * 3;
                            const etc1_byte* qa = alphaBlock + cy * 4;
                            for (etc1_uint32 cx = 0; cx < xEnd; cx++) {
                                // copy rgb data
                                memcpy(p, q, 3);
                                p += 3;
                                q += 3;
                                *p++ = *qa++;
                            }
                        }
                        break;
                    default:
                        assert(0);
                }
            }
        }
    }
    return 0;
}

static const char kMagic[] = { 'P', 'K', 'M', ' ', '1', '0' };

static const etc1_uint32 ETC1_PKM_FORMAT_OFFSET = 6;
static const etc1_uint32 ETC1_PKM_ENCODED_WIDTH_OFFSET = 8;
static const etc1_uint32 ETC1_PKM_ENCODED_HEIGHT_OFFSET = 10;
static const etc1_uint32 ETC1_PKM_WIDTH_OFFSET = 12;
static const etc1_uint32 ETC1_PKM_HEIGHT_OFFSET = 14;

static const etc1_uint32 ETC1_RGB_NO_MIPMAPS = 0;

static void writeBEUint16(etc1_byte* pOut, etc1_uint32 data) {
    pOut[0] = (etc1_byte) (data >> 8);
    pOut[1] = (etc1_byte) data;
}

static etc1_uint32 readBEUint16(const etc1_byte* pIn) {
    return (pIn[0] << 8) | pIn[1];
}

// Format a PKM header

void etc1_pkm_format_header(etc1_byte* pHeader, etc1_uint32 width, etc1_uint32 height) {
    memcpy(pHeader, kMagic, sizeof(kMagic));
    etc1_uint32 encodedWidth = (width + 3) & ~3;
    etc1_uint32 encodedHeight = (height + 3) & ~3;
    writeBEUint16(pHeader + ETC1_PKM_FORMAT_OFFSET, ETC1_RGB_NO_MIPMAPS);
    writeBEUint16(pHeader + ETC1_PKM_ENCODED_WIDTH_OFFSET, encodedWidth);
    writeBEUint16(pHeader + ETC1_PKM_ENCODED_HEIGHT_OFFSET, encodedHeight);
    writeBEUint16(pHeader + ETC1_PKM_WIDTH_OFFSET, width);
    writeBEUint16(pHeader + ETC1_PKM_HEIGHT_OFFSET, height);
}

// Check if a PKM header is correctly formatted.

etc1_bool etc1_pkm_is_valid(const etc1_byte* pHeader) {
    if (memcmp(pHeader, kMagic, sizeof(kMagic))) {
        return false;
    }
    etc1_uint32 format = readBEUint16(pHeader + ETC1_PKM_FORMAT_OFFSET);
    etc1_uint32 encodedWidth = readBEUint16(pHeader + ETC1_PKM_ENCODED_WIDTH_OFFSET);
    etc1_uint32 encodedHeight = readBEUint16(pHeader + ETC1_PKM_ENCODED_HEIGHT_OFFSET);
    etc1_uint32 width = readBEUint16(pHeader + ETC1_PKM_WIDTH_OFFSET);
    etc1_uint32 height = readBEUint16(pHeader + ETC1_PKM_HEIGHT_OFFSET);
    return format == ETC1_RGB_NO_MIPMAPS &&
            encodedWidth >= width && encodedWidth - width < 4 &&
            encodedHeight >= height && encodedHeight - height < 4;
}

// Read the image width from a PKM header

etc1_uint32 etc1_pkm_get_width(const etc1_byte* pHeader) {
    return readBEUint16(pHeader + ETC1_PKM_WIDTH_OFFSET);
}

// Read the image height from a PKM header

etc1_uint32 etc1_pkm_get_height(const etc1_byte* pHeader){
    return readBEUint16(pHeader + ETC1_PKM_HEIGHT_OFFSET);
}
