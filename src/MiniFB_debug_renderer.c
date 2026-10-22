#if defined(MINIFB_USE_DEBUG_RENDERER) && MINIFB_USE_DEBUG_RENDERER

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <MiniFB_debug_renderer.h>
#include <MiniFB_macros.h>

//-------------------------------------
// Helpers
//-------------------------------------

//-------------------------------------
#define UINT16_T_BITS(b15, b14, b13, b12, b11, b10, b9, b8, b7, b6, b5, b4, b3, b2, b1, b0) \
    ((((uint16_t)(b15)) & 1u) << 15) | ((((uint16_t)(b14)) & 1u) << 14) | ((((uint16_t)(b13)) & 1u) << 13) | ((((uint16_t)(b12)) & 1u) << 12) | \
    ((((uint16_t)(b11)) & 1u) << 11) | ((((uint16_t)(b10)) & 1u) << 10) | ((((uint16_t)( b9)) & 1u) <<  9) | ((((uint16_t)( b8)) & 1u) <<  8) | \
    ((((uint16_t)( b7)) & 1u) <<  7) | ((((uint16_t)( b6)) & 1u) <<  6) | ((((uint16_t)( b5)) & 1u) <<  5) | ((((uint16_t)( b4)) & 1u) <<  4) | \
    ((((uint16_t)( b3)) & 1u) <<  3) | ((((uint16_t)( b2)) & 1u) <<  2) | ((((uint16_t)( b1)) & 1u) <<  1) | ((((uint16_t)( b0)) & 1u) <<  0)

//-------------------------------------
// Returns true, if a valid UTF-8 codepoint was decoded and the `codepoint` argument, if not NULL, is set accordingly;
// otherwise, returns false and the `codepoint` argument is left unmodified.
// The `bytes_read` argument, if not NULL, will always be set to the number of bytes consumed, even if decoding fails.
// Also decodes the null-terminator ('\0'), if encountered.
static inline bool
try_decode_next_utf8_codepoint(const char *text, uint32_t *codepoint, size_t *bytes_read) {
    if (text == NULL) {
        if (bytes_read != NULL) {
            *bytes_read = 0;
        }
        return false;
    }
    
    uint32_t result = 0;
    unsigned char ch = (unsigned char)*text;
    if (ch < 0x80u) {
        // single-byte sequence (ASCII)
        result = ch;

        if (bytes_read != NULL) {
            *bytes_read = 1;
        }
    } else if (ch < 0xC0u) {
        // continuation byte: we're in the middle of a multi-byte sequence or the sequence is invalid, either way, fail
        if (bytes_read != NULL) {
            *bytes_read = 1;
        }
        return false;
    } else if (ch < 0xE0u) {
        // 2-byte sequence
        result = (ch & 0x1Fu) << 6;

        ch = (unsigned char)*(text + 1);
        if ((ch & 0xC0u) != 0x80u) {
            // missing continuation byte: the sequence is invalid, fail
            if (bytes_read != NULL) {
                *bytes_read = 1; // only account for the invalid part of sequence (subsequent bytes could still form a valid sequence in the next call; this also prevents a null-terminator from being consumed prematurely)
            }
            return false;
        }
        result |= (ch & 0x3Fu);

        if (bytes_read != NULL) {
            *bytes_read = 2;
        }

        if (result < 0x80u) {
            // overlong encoding: the sequence is invalid, fail
            return false;
        }
    } else if (ch < 0xF0u) {
        // 3-byte sequence
        result = (ch & 0x0Fu) << 12;

        ch = (unsigned char)*(text + 1);
        if ((ch & 0xC0u) != 0x80u) {
            // missing continuation byte: the sequence is invalid, fail
            if (bytes_read != NULL) {
                *bytes_read = 1; // only account for the invalid part of sequence
            }
            return false;
        }
        result |= (ch & 0x3Fu) << 6;

        ch = (unsigned char)*(text + 2);
        if ((ch & 0xC0u) != 0x80u) {
            // missing continuation byte: the sequence is invalid, fail
            if (bytes_read != NULL) {
                *bytes_read = 2; // only account for the invalid part of sequence
            }
            return false;
        }
        result |= (ch & 0x3Fu);

        if (bytes_read != NULL) {
            *bytes_read = 3;
        }

        if (result < 0x800u || (result >= 0xD800 && result <= 0xDFFF)) {
            // overlong encoding or codepoint out of range: either way, the sequence is invalid, fail
            return false;
        }
    } else if (ch < 0xF8u) {
        // 4-byte sequence
        result = (ch & 0x07u) << 18;

        ch = (unsigned char)*(text + 1);
        if ((ch & 0xC0u) != 0x80u) {
            // missing continuation byte: the sequence is invalid, fail
            if (bytes_read != NULL) {
                *bytes_read = 1; // only account for the invalid part of sequence 
            }
            return false;
        }
        result |= (ch & 0x3Fu) << 12;

        ch = (unsigned char)*(text + 2);
        if ((ch & 0xC0u) != 0x80u) {
            // missing continuation byte: the sequence is invalid, fail
            if (bytes_read != NULL) {
                *bytes_read = 2; // only account for the invalid part of sequence
            }
            return false;
        }
        result |= (ch & 0x3Fu) << 6;

        ch = (unsigned char)*(text + 3);
        if ((ch & 0xC0u) != 0x80u) {
            // missing continuation byte: the sequence is invalid, fail
            if (bytes_read != NULL) {
                *bytes_read = 3; // only account for the invalid part of sequence
            }
            return false;
        }
        result |= (ch & 0x3Fu);

        if (bytes_read != NULL) {
            *bytes_read = 4;
        }

        if (result < 0x10000u || result > 0x10FFFFu) {
            // overlong encoding or codepoint out of range: either way, the sequence is invalid, fail
            return false;
        }
    } else {
        // invalid first UTF-8 byte: the sequence is invalid, fail
        if (bytes_read != NULL) {
            *bytes_read = 1;
        }
        return false;
    }

    if (codepoint != NULL) {
        *codepoint = result;
    }
    return true;
}

//-------------------------------------
typedef struct {
    union {
        struct {
            uint16_t weighted_alpha;
            uint8_t  inverse_alpha;
        };
        uint32_t     _union_alpha;
    };
    uint32_t         weighted_red;
    uint32_t         weighted_green;
    uint32_t         weighted_blue;
} precalculated_color;

//-------------------------------------
// Note: This function does NOT validate its inputs. Make sure they're sensible before calling it.
static inline void
precalculate_color(const uint32_t color, precalculated_color* result) {
    const uint32_t a = MFB_GET_A(color);
    if (a == 0) {
        *result = (precalculated_color){ {{ 0, 0xFFu }}, 0, 0, 0 };
        return;
    }
    const uint16_t weighted_alpha = (uint16_t)(a * 0xFFu);
    *result = (precalculated_color){
        {{ weighted_alpha, (uint8_t)(0xFFu - a) }},
        (uint32_t)((uint32_t)weighted_alpha * MFB_GET_R(color)),
        (uint32_t)((uint32_t)weighted_alpha * MFB_GET_G(color)),
        (uint32_t)((uint32_t)weighted_alpha * MFB_GET_B(color))
    };
}

//-------------------------------------
// Note: This function does NOT validate its inputs. Make sure they're sensible before calling it.
static inline uint32_t
blend_precalculated_color_over(const precalculated_color *src, const uint32_t dst) {
    const uint32_t dst_weighted_alpha = (uint32_t)(src->inverse_alpha) * MFB_GET_A(dst);
    const uint32_t total_weights      = (uint32_t)(src->weighted_alpha) + dst_weighted_alpha;
    if (total_weights == 0) {
        // Both, source and destination, are fully transparent.
        // Since this operation is `src` over `dst`, we can just return `dst` in that case.
        return dst;
    }
    return MFB_ARGB(
        total_weights / 0xFFu,
        (src->weighted_red + dst_weighted_alpha * MFB_GET_R(dst)) / total_weights,
        (src->weighted_green + dst_weighted_alpha * MFB_GET_G(dst)) / total_weights,
        (src->weighted_blue + dst_weighted_alpha * MFB_GET_B(dst)) / total_weights
    );
}

//-------------------------------------
// Bit glyph representation
//-------------------------------------

//-------------------------------------
// Note that `bit_glyph` is deliberately NOT defined as a union or struct with explicit bit fields.
// Instead, the way we construct a `bit_glyph` by using the `MAKE_BIT_GLYPH` macro ensures that
// the individual bits are consistently packed across various platforms and compilers.
// (C standard does not guarantee the layout of bit fields, nor the size and alignment of types containing them.)
typedef struct { uint16_t raw[3]; } bit_glyph;

//-------------------------------------
#define MAKE_BIT_GLYPH(p00, p01, p02, p03, p04, p05, p10, p11, p12, p13, p14, p15, p20, p21, p22, p23, p24, p25, p30, p31, p32, p33, p34, p35, p40, p41, p42, p43, p44, p45, p50, p51, p52, p53, p54, p55, p60, p61, p62, p63, p64, p65, p70, p71, p72, p73, p74, p75) {{ \
    UINT16_T_BITS(p23, p22, p21, p20, p15, p14, p13, p12, p11, p10, p05, p04, p03, p02, p01, p00), \
    UINT16_T_BITS(p51, p50, p45, p44, p43, p42, p41, p40, p35, p34, p33, p32, p31, p30, p25, p24), \
    UINT16_T_BITS(p75, p74, p73, p72, p71, p70, p65, p64, p63, p62, p61, p60, p55, p54, p53, p52) \
}}

//-------------------------------------
// Containing, predefined bit glyphs definition
//-------------------------------------

// Macros for defining pixel states
//-------------------------------------
// Since they replace very simple characters and could interfere with parsing,
// they're just defined for defining bit glyphs and will be undefined afterwards.
#define _ 0
#define M 1

// Global bit glyphs array
//-------------------------------------
// Legend:
//   _ is used to represent an empty pixel
//   M is used to represent a filled pixel
//
// _ and M are predefined macros representing 0 and 1, respectively.
// They're chosen to preview the visual representation of the glyphs in the code.
// Hence, you should always use the macros _ and M define pixels states only.
//
// Remember that row 8 and column 6 are reserved for spacing and should almost always be set to _.
// However, some glyphs may use filled pixels in these reserved areas for specific design purposes.
//
// Glyphs currently defined in this array:
// - One glyph at the start of the array representing a replacement for undefined characters
// - ASCII characters for Space (decimal 32) through ~ (decimal 126) in ascending order
//   (i.e., printable ASCII characters)
// - Selected box drawing characters (single and double lines and mixed)
//
// If the contents of the array are changed, make sure to update the `get_bit_glyph_from_codepoint` function accordingly!
static const bit_glyph g_bit_glyphs[] = {  
    // Replacement glyph for undefined characters
    MAKE_BIT_GLYPH(
        M,M,M,M,M,_,
        M,_,M,_,M,_,
        M,_,M,_,M,_,
        M,M,M,M,M,_,
        M,_,M,_,M,_,
        M,_,M,_,M,_,
        M,M,M,M,M,_,
        _,_,_,_,_,_
    ),
    // Space
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // !
    MAKE_BIT_GLYPH(
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,_,_,_,_,
        _,_,M,_,_,_,
        _,_,_,_,_,_
    ),
    // "
    MAKE_BIT_GLYPH(
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // #
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,M,_,M,_,_,
        M,M,M,M,M,_,
        _,M,_,M,_,_,
        M,M,M,M,M,_,
        _,M,_,M,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // $
    MAKE_BIT_GLYPH(
        _,_,M,_,_,_,
        _,M,M,M,_,_,
        M,_,M,_,_,_,
        _,M,M,M,_,_,
        _,_,M,_,M,_,
        _,M,M,M,_,_,
        _,_,M,_,_,_,
        _,_,_,_,_,_
    ),
    // %
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        M,M,_,_,M,_,
        M,M,_,M,_,_,
        _,_,M,_,_,_,
        _,M,_,M,M,_,
        M,_,_,M,M,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // &
    MAKE_BIT_GLYPH(
        _,M,_,_,_,_,
        M,_,M,_,_,_,
        M,_,M,_,_,_,
        _,M,_,_,_,_,
        M,_,M,_,M,_,
        M,_,_,M,_,_,
        _,M,M,_,M,_,
        _,_,_,_,_,_
    ),
    // '
    MAKE_BIT_GLYPH(
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // (
    MAKE_BIT_GLYPH(
        _,_,_,M,_,_,
        _,_,M,_,_,_,
        _,M,_,_,_,_,
        _,M,_,_,_,_,
        _,M,_,_,_,_,
        _,_,M,_,_,_,
        _,_,_,M,_,_,
        _,_,_,_,_,_
    ),
    // )
    MAKE_BIT_GLYPH(
        _,M,_,_,_,_,
        _,_,M,_,_,_,
        _,_,_,M,_,_,
        _,_,_,M,_,_,
        _,_,_,M,_,_,
        _,_,M,_,_,_,
        _,M,_,_,_,_,
        _,_,_,_,_,_
    ),
    // *
    MAKE_BIT_GLYPH(
        _,_,M,_,_,_,
        M,_,M,_,M,_,
        _,M,M,M,_,_,
        M,_,M,_,M,_,
        _,_,M,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // +
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        M,M,M,M,M,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // ,
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,M,_,_,_,_
    ),
    // -
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        M,M,M,M,M,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // .
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,M,_,_,_,
        _,_,_,_,_,_
    ),
    // /
    MAKE_BIT_GLYPH(
        _,_,_,_,M,_,
        _,_,_,_,M,_,
        _,_,_,M,_,_,
        _,_,M,_,_,_,
        _,M,_,_,_,_,
        M,_,_,_,_,_,
        M,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // 0
    MAKE_BIT_GLYPH(
        _,M,M,M,_,_,
        M,_,_,_,M,_,
        M,_,_,M,M,_,
        M,_,M,_,M,_,
        M,M,_,_,M,_,
        M,_,_,_,M,_,
        _,M,M,M,_,_,
        _,_,_,_,_,_
    ),
    // 1
    MAKE_BIT_GLYPH(
        _,_,M,_,_,_,
        _,M,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,M,M,M,_,_,
        _,_,_,_,_,_
    ),
    // 2
    MAKE_BIT_GLYPH(
        _,M,M,M,_,_,
        M,_,_,_,M,_,
        _,_,_,_,M,_,
        _,_,_,M,_,_,
        _,_,M,_,_,_,
        _,M,_,_,_,_,
        M,M,M,M,M,_,
        _,_,_,_,_,_
    ),
    // 3
    MAKE_BIT_GLYPH(
        _,M,M,M,_,_,
        M,_,_,_,M,_,
        _,_,_,_,M,_,
        _,_,M,M,_,_,
        _,_,_,_,M,_,
        M,_,_,_,M,_,
        _,M,M,M,_,_,
        _,_,_,_,_,_
    ),
    // 4
    MAKE_BIT_GLYPH(
        _,_,M,M,_,_,
        _,M,_,M,_,_,
        M,_,_,M,_,_,
        M,_,_,M,_,_,
        M,M,M,M,M,_,
        _,_,_,M,_,_,
        _,_,_,M,_,_,
        _,_,_,_,_,_
    ),
    // 5
    MAKE_BIT_GLYPH(
        M,M,M,M,M,_,
        M,_,_,_,_,_,
        M,M,M,M,_,_,
        _,_,_,_,M,_,
        _,_,_,_,M,_,
        M,_,_,_,M,_,
        _,M,M,M,_,_,
        _,_,_,_,_,_
    ),
    // 6
    MAKE_BIT_GLYPH(
        _,M,M,M,_,_,
        M,_,_,_,M,_,
        M,_,_,_,_,_,
        M,M,M,M,_,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        _,M,M,M,_,_,
        _,_,_,_,_,_
    ),
    // 7
    MAKE_BIT_GLYPH(
        M,M,M,M,M,_,
        _,_,_,_,M,_,
        _,_,_,M,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,M,_,_,_,_,
        _,M,_,_,_,_,
        _,_,_,_,_,_
    ),
    // 8
    MAKE_BIT_GLYPH(
        _,M,M,M,_,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        _,M,M,M,_,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        _,M,M,M,_,_,
        _,_,_,_,_,_
    ),
    // 9
    MAKE_BIT_GLYPH(
        _,M,M,M,_,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        _,M,M,M,M,_,
        _,_,_,_,M,_,
        M,_,_,_,M,_,
        _,M,M,M,_,_,
        _,_,_,_,_,_
    ),
    // :
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,M,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,M,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // ;
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,M,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,M,_,_,_,_
    ),
    // <
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,_,M,_,_,
        _,_,M,_,_,_,
        _,M,_,_,_,_,
        _,_,M,_,_,_,
        _,_,_,M,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // =
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,M,M,M,_,_,
        _,_,_,_,_,_,
        _,M,M,M,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // >
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,M,_,_,_,_,
        _,_,M,_,_,_,
        _,_,_,M,_,_,
        _,_,M,_,_,_,
        _,M,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // ?
    MAKE_BIT_GLYPH(
        _,M,M,M,_,_,
        M,_,_,_,M,_,
        _,_,_,_,M,_,
        _,_,_,M,_,_,
        _,_,M,_,_,_,
        _,_,_,_,_,_,
        _,_,M,_,_,_,
        _,_,_,_,_,_
    ),
    // @
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,M,M,M,_,_,
        M,_,_,_,M,_,
        M,_,M,_,M,_,
        M,_,M,M,_,_,
        M,_,_,_,_,_,
        _,M,M,M,_,_,
        _,_,_,_,_,_
    ),
    // A
    MAKE_BIT_GLYPH(
        _,M,M,M,_,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,M,M,M,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        _,_,_,_,_,_
    ),
    // B
    MAKE_BIT_GLYPH(
        M,M,M,M,_,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,M,M,M,_,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,M,M,M,_,_,
        _,_,_,_,_,_
    ),
    // C
    MAKE_BIT_GLYPH(
        _,M,M,M,_,_,
        M,_,_,_,M,_,
        M,_,_,_,_,_,
        M,_,_,_,_,_,
        M,_,_,_,_,_,
        M,_,_,_,M,_,
        _,M,M,M,_,_,
        _,_,_,_,_,_
    ),
    // D
    MAKE_BIT_GLYPH(
        M,M,M,M,_,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,M,M,M,_,_,
        _,_,_,_,_,_
    ),
    // E
    MAKE_BIT_GLYPH(
        M,M,M,M,M,_,
        M,_,_,_,_,_,
        M,_,_,_,_,_,
        M,M,M,M,_,_,
        M,_,_,_,_,_,
        M,_,_,_,_,_,
        M,M,M,M,M,_,
        _,_,_,_,_,_
    ),
    // F
    MAKE_BIT_GLYPH(
        M,M,M,M,M,_,
        M,_,_,_,_,_,
        M,_,_,_,_,_,
        M,M,M,M,_,_,
        M,_,_,_,_,_,
        M,_,_,_,_,_,
        M,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // G
    MAKE_BIT_GLYPH(
        _,M,M,M,_,_,
        M,_,_,_,M,_,
        M,_,_,_,_,_,
        M,_,M,M,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        _,M,M,M,_,_,
        _,_,_,_,_,_
    ),
    // H
    MAKE_BIT_GLYPH(
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,M,M,M,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        _,_,_,_,_,_
    ),
    // I
    MAKE_BIT_GLYPH(
        _,M,M,M,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,M,M,M,_,_,
        _,_,_,_,_,_
    ),
    // J
    MAKE_BIT_GLYPH(
        M,M,M,M,M,_,
        _,_,_,M,_,_,
        _,_,_,M,_,_,
        _,_,_,M,_,_,
        _,_,_,M,_,_,
        M,_,_,M,_,_,
        _,M,M,_,_,_,
        _,_,_,_,_,_
    ),
    // K
    MAKE_BIT_GLYPH(
        M,_,_,_,M,_,
        M,_,_,M,_,_,
        M,_,M,_,_,_,
        M,M,_,_,_,_,
        M,_,M,_,_,_,
        M,_,_,M,_,_,
        M,_,_,_,M,_,
        _,_,_,_,_,_
    ),
    // L
    MAKE_BIT_GLYPH(
        M,_,_,_,_,_,
        M,_,_,_,_,_,
        M,_,_,_,_,_,
        M,_,_,_,_,_,
        M,_,_,_,_,_,
        M,_,_,_,_,_,
        M,M,M,M,M,_,
        _,_,_,_,_,_
    ),
    // M
    MAKE_BIT_GLYPH(
        M,_,_,_,M,_,
        M,M,_,M,M,_,
        M,_,M,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        _,_,_,_,_,_
    ),
    // N
    MAKE_BIT_GLYPH(
        M,_,_,_,M,_,
        M,M,_,_,M,_,
        M,M,_,_,M,_,
        M,_,M,_,M,_,
        M,_,_,M,M,_,
        M,_,_,M,M,_,
        M,_,_,_,M,_,
        _,_,_,_,_,_
    ),
    // O
    MAKE_BIT_GLYPH(
        _,M,M,M,_,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        _,M,M,M,_,_,
        _,_,_,_,_,_
    ),
    // P
    MAKE_BIT_GLYPH(
        M,M,M,M,_,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,M,M,M,_,_,
        M,_,_,_,_,_,
        M,_,_,_,_,_,
        M,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // Q
    MAKE_BIT_GLYPH(
        _,M,M,M,_,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,M,_,M,_,
        M,_,_,M,M,_,
        _,M,M,M,_,_,
        _,_,_,_,M,_
    ),
    // R
    MAKE_BIT_GLYPH(
        M,M,M,M,_,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,M,M,M,_,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        _,_,_,_,_,_
    ),
    // S
    MAKE_BIT_GLYPH(
        _,M,M,M,_,_,
        M,_,_,_,M,_,
        M,_,_,_,_,_,
        _,M,M,M,_,_,
        _,_,_,_,M,_,
        M,_,_,_,M,_,
        _,M,M,M,_,_,
        _,_,_,_,_,_
    ),
    // T
    MAKE_BIT_GLYPH(
        M,M,M,M,M,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,_,_,_,_
    ),
    // U
    MAKE_BIT_GLYPH(
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        _,M,M,M,_,_,
        _,_,_,_,_,_
    ),
    // V
    MAKE_BIT_GLYPH(
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,_,M,_,_,_,
        _,_,_,_,_,_
    ),
    // W
    MAKE_BIT_GLYPH(
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,M,_,M,_,
        M,M,_,M,M,_,
        M,_,_,_,M,_,
        _,_,_,_,_,_
    ),
    // X
    MAKE_BIT_GLYPH(
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        _,M,_,M,_,_,
        _,_,M,_,_,_,
        _,M,_,M,_,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        _,_,_,_,_,_ 
    ),
    // Y
    MAKE_BIT_GLYPH(
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        _,M,_,M,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,_,_,_,_
    ),
    // Z
    MAKE_BIT_GLYPH(
        M,M,M,M,M,_,
        _,_,_,_,M,_,
        _,_,_,M,_,_,
        _,_,M,_,_,_,
        _,M,_,_,_,_,
        M,_,_,_,_,_,
        M,M,M,M,M,_,
        _,_,_,_,_,_
    ),
    // [
    MAKE_BIT_GLYPH(
        _,M,M,M,_,_,
        _,M,_,_,_,_,
        _,M,_,_,_,_,
        _,M,_,_,_,_,
        _,M,_,_,_,_,
        _,M,_,_,_,_,
        _,M,M,M,_,_,
        _,_,_,_,_,_
    ),
    // Backslash
    MAKE_BIT_GLYPH(
        M,_,_,_,_,_,
        M,_,_,_,_,_,
        _,M,_,_,_,_,
        _,_,M,_,_,_,
        _,_,_,M,_,_,
        _,_,_,_,M,_,
        _,_,_,_,M,_,
        _,_,_,_,_,_
    ),
    // ]
    MAKE_BIT_GLYPH(
        _,M,M,M,_,_,
        _,_,_,M,_,_,
        _,_,_,M,_,_,
        _,_,_,M,_,_,
        _,_,_,M,_,_,
        _,_,_,M,_,_,
        _,M,M,M,_,_,
        _,_,_,_,_,_
    ),
    // ^
    MAKE_BIT_GLYPH(
        _,_,M,_,_,_,
        _,M,_,M,_,_,
        M,_,_,_,M,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // _
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        M,M,M,M,M,M,
        _,_,_,_,_,_
    ),
    // `
    MAKE_BIT_GLYPH(
        _,M,_,_,_,_,
        _,_,M,_,_,_,
        _,_,_,M,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),    
    // a
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,M,M,M,_,_,
        _,_,_,_,M,_,
        _,M,M,M,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        _,M,M,M,M,_,
        _,_,_,_,_,_
    ),
    // b
    MAKE_BIT_GLYPH(
        M,_,_,_,_,_,
        M,_,_,_,_,_,
        M,M,M,M,_,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,M,M,M,_,_,
        _,_,_,_,_,_
    ),
    // c
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,M,M,M,_,_,
        M,_,_,_,M,_,
        M,_,_,_,_,_,
        M,_,_,_,_,_,
        M,_,_,_,M,_,
        _,M,M,M,_,_,
        _,_,_,_,_,_
    ),
    // d
    MAKE_BIT_GLYPH(
        _,_,_,_,M,_,
        _,_,_,_,M,_,
        _,M,M,M,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        _,M,M,M,M,_,
        _,_,_,_,_,_
    ),
    // e
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,M,M,M,_,_,
        M,_,_,_,M,_,
        M,M,M,M,M,_,
        M,_,_,_,_,_,
        M,_,_,_,M,_,
        _,M,M,M,_,_,
        _,_,_,_,_,_
    ),
    // f
    MAKE_BIT_GLYPH(
        _,_,M,M,M,_,
        _,M,_,_,_,_,
        _,M,_,_,_,_,
        M,M,M,M,_,_,
        _,M,_,_,_,_,
        _,M,_,_,_,_,
        _,M,_,_,_,_,
        _,_,_,_,_,_
    ),
    // g
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,M,M,M,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        _,M,M,M,M,_,
        _,_,_,_,M,_,
        _,_,_,_,M,_,
        _,M,M,M,_,_
    ),
    // h
    MAKE_BIT_GLYPH(
        M,_,_,_,_,_,
        M,_,_,_,_,_,
        M,M,M,M,_,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        _,_,_,_,_,_
    ),
    // i
    MAKE_BIT_GLYPH(
        _,_,M,_,_,_,
        _,_,_,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,_,_,_,_
    ),
    // j
    MAKE_BIT_GLYPH(
        _,_,M,_,_,_,
        _,_,_,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        M,M,_,_,_,_
    ),
    // k
    MAKE_BIT_GLYPH(
        M,_,_,_,_,_,
        M,_,_,M,_,_,
        M,_,M,_,_,_,
        M,M,_,_,_,_,
        M,_,M,_,_,_,
        M,_,_,M,_,_,
        M,_,_,M,_,_,
        _,_,_,_,_,_
    ),
    // l
    MAKE_BIT_GLYPH(
        _,M,_,_,_,_,
        _,M,_,_,_,_,
        _,M,_,_,_,_,
        _,M,_,_,_,_,
        _,M,_,_,_,_,
        _,M,_,_,_,_,
        _,_,M,M,_,_,
        _,_,_,_,_,_
    ),
    // m
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        M,M,_,M,_,_,
        M,_,M,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        _,_,_,_,_,_
    ),
    // n
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        M,M,M,M,_,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        _,_,_,_,_,_
    ),
    // o
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,M,M,M,_,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        _,M,M,M,_,_,
        _,_,_,_,_,_
    ),
    // p
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        M,M,M,M,_,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,M,M,M,_,_,
        M,_,_,_,_,_,
        M,_,_,_,_,_
    ),
    // q
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,M,M,M,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        _,M,M,M,M,_,
        _,_,_,_,M,_,
        _,_,_,_,M,_
    ),
    // r
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        M,_,M,M,M,_,
        M,M,_,_,_,_,
        M,_,_,_,_,_,
        M,_,_,_,_,_,
        M,_,_,_,_,_,
        M,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // s
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,M,M,M,_,_,
        M,_,_,_,M,_,
        _,M,M,M,_,_,
        _,_,_,_,M,_,
        M,_,_,_,M,_,
        _,M,M,M,_,_,
        _,_,_,_,_,_
    ),
    // t
    MAKE_BIT_GLYPH(
        _,M,_,_,_,_,
        _,M,_,_,_,_,
        M,M,M,M,_,_,
        _,M,_,_,_,_,
        _,M,_,_,_,_,
        _,M,_,_,_,_,
        _,_,M,M,M,_,
        _,_,_,_,_,_
    ),
    // u
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        _,M,M,M,M,_,
        _,_,_,_,_,_
    ),
    // v
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        _,M,_,M,_,_,
        _,_,M,_,_,_,
        _,_,_,_,_,_
    ),
    // w
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,M,_,M,_,
        _,M,_,M,_,_,
        _,_,_,_,_,_
    ),
    // x
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        M,_,_,_,M,_,
        _,M,_,M,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,M,_,M,_,_,
        M,_,_,_,M,_,
        _,_,_,_,_,_
    ),
    // y
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        M,_,_,_,M,_,
        _,M,M,M,M,_,
        _,_,_,_,M,_,
        _,_,_,_,M,_,
        _,M,M,M,_,_
    ),
    // z
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        M,M,M,M,M,_,
        _,_,_,_,M,_,
        _,_,_,M,_,_,
        _,_,M,_,_,_,
        _,M,_,_,_,_,
        M,M,M,M,M,_,
        _,_,_,_,_,_
    ),
    // {
    MAKE_BIT_GLYPH(
        _,_,_,M,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,M,_,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,_,M,_,_,
        _,_,_,_,_,_
    ),
    // |
    MAKE_BIT_GLYPH(
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,_,_,_,_
    ),
    // }
    MAKE_BIT_GLYPH(
        _,M,_,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,_,M,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,M,_,_,_,_,
        _,_,_,_,_,_
    ),
    // ~
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,M,_,_,M,_,
        M,_,M,_,M,_,
        M,_,_,M,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // Box drawing: Horizontal Single (Unicode equivalent: U+2500)
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        M,M,M,M,M,M,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // Box drawing: Vertical Single (Unicode equivalent: U+2502)
    MAKE_BIT_GLYPH(
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_
    ),
    // Box drawing: Down and Right Single (Unicode equivalent: U+250C)
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,M,M,M,M,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_
    ),
    // Box drawing: Down and Left Single (Unicode equivalent: U+2510)
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        M,M,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_
    ),
    // Box drawing: Up and Right Single (Unicode equivalent: U+2514)
    MAKE_BIT_GLYPH(
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,M,M,M,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // Box drawing: Up and Left Single (Unicode equivalent: U+2518)
    MAKE_BIT_GLYPH(
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        M,M,M,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // Box drawing: Vertical and Right Single (Unicode equivalent: U+251C)
    MAKE_BIT_GLYPH(
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,M,M,M,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_
    ),
    // Box drawing: Vertical and Left Single (Unicode equivalent: U+2524)
    MAKE_BIT_GLYPH(
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        M,M,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_
    ),
    // Box drawing: Horizontal and Down Single (Unicode equivalent: U+252C)
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        M,M,M,M,M,M,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_
    ),
    // Box drawing: Horizontal and Up Single (Unicode equivalent: U+2534)
    MAKE_BIT_GLYPH(
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        M,M,M,M,M,M,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // Box drawing: Horizontal and Vertical Single (Unicode equivalent: U+253C)
    MAKE_BIT_GLYPH(
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        M,M,M,M,M,M,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_
    ),
    // Box drawing: Double Horizontal (Unicode equivalent: U+2550)
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        M,M,M,M,M,M,
        _,_,_,_,_,_,
        M,M,M,M,M,M,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // Box drawing: Double Vertical (Unicode equivalent: U+2551)
    MAKE_BIT_GLYPH(
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_
    ),
    // Box drawing: Down Single and Right Double (Unicode equivalent: U+2552)
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,M,M,M,M,
        _,_,M,_,_,_,
        _,_,M,M,M,M,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_
    ),
    // Box drawing: Down Double and Right Single (Unicode equivalent: U+2553)
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,M,M,M,M,M,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_
    ),
    // Box drawing: Down and Right Double (Unicode equivalent: U+2554)
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,M,M,M,M,M,
        _,M,_,_,_,_,
        _,M,_,M,M,M,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_
    ),
    // Box drawing: Down Single and Left Double (Unicode equivalent: U+2555)
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        M,M,M,_,_,_,
        _,_,M,_,_,_,
        M,M,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_
    ),
    // Box drawing: Down Double and Left Single (Unicode equivalent: U+2556)
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        M,M,M,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_
    ),
    // Box drawing: Down and Left Double (Unicode equivalent: U+2557)
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        M,M,M,M,_,_,
        _,_,_,M,_,_,
        M,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_
    ),
    // Box drawing: Up Single and Right Double (Unicode equivalent: U+2558)
    MAKE_BIT_GLYPH(
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,M,M,M,
        _,_,M,_,_,_,
        _,_,M,M,M,M,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // Box drawing: Up Double and Right Single (Unicode equivalent: U+2559)
    MAKE_BIT_GLYPH(
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,M,M,M,M,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // Box drawing: Up and Right Double (Unicode equivalent: U+255A)
    MAKE_BIT_GLYPH(
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,M,M,
        _,M,_,_,_,_,
        _,M,M,M,M,M,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // Box drawing: Up Single and Left Double (Unicode equivalent: U+255B)
    MAKE_BIT_GLYPH(
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        M,M,M,_,_,_,
        _,_,M,_,_,_,
        M,M,M,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // Box drawing: Up Double and Left Single (Unicode equivalent: U+255C)    
    MAKE_BIT_GLYPH(
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        M,M,M,M,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // Box drawing: Up and Left Double (Unicode equivalent: U+255D)
    MAKE_BIT_GLYPH(
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        M,M,_,M,_,_,
        _,_,_,M,_,_,
        M,M,M,M,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // Box drawing: Vertical Single and Right Double (Unicode equivalent: U+255E)
    MAKE_BIT_GLYPH(
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,M,M,M,
        _,_,M,_,_,_,
        _,_,M,M,M,M,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_
    ),
    // Box drawing: Vertical Double and Right Single (Unicode equivalent: U+255F)
    MAKE_BIT_GLYPH(
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,M,M,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_
    ),
    // Box drawing: Vertical and Right Double (Unicode equivalent: U+2560)
    MAKE_BIT_GLYPH(
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,M,M,
        _,M,_,_,_,_,
        _,M,_,M,M,M,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_
    ),
    // Box drawing: Vertical Single and Left Double (Unicode equivalent: U+2561)
    MAKE_BIT_GLYPH(
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        M,M,M,_,_,_,
        _,_,M,_,_,_,
        M,M,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_
    ),
    // Box drawing: Vertical Double and Left Single (Unicode equivalent: U+2562)
    MAKE_BIT_GLYPH(
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        M,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_
    ),
    // Box drawing: Vertical and Left Double (Unicode equivalent: U+2563)
    MAKE_BIT_GLYPH(
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        M,M,_,M,_,_,
        _,_,_,M,_,_,
        M,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_
    ),
    // Box drawing: Down Single and Horizontal Double (Unicode equivalent: U+2564)
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        M,M,M,M,M,M,
        _,_,_,_,_,_,
        M,M,M,M,M,M,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_
    ),
    // Box drawing: Down Double and Horizontal Single (Unicode equivalent: U+2565)
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        M,M,M,M,M,M,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_
    ),
    // Box drawing: Down and Horizontal Double (Unicode equivalent: U+2566)
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        M,M,M,M,M,M,
        _,_,_,_,_,_,
        M,M,_,M,M,M,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_
    ),
    // Box drawing: Up Single and Horizontal Double (Unicode equivalent: U+2567)
    MAKE_BIT_GLYPH(
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        M,M,M,M,M,M,
        _,_,_,_,_,_,
        M,M,M,M,M,M,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // Box drawing: Up Double and Horizontal Single (Unicode equivalent: U+2568)
    MAKE_BIT_GLYPH(
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        M,M,M,M,M,M,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // Box drawing: Up and Horizontal Double (Unicode equivalent: U+2569)
    MAKE_BIT_GLYPH(
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        M,M,_,M,M,M,
        _,_,_,_,_,_,
        M,M,M,M,M,M,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // Box drawing: Vertical Single and Horizontal Double (Unicode equivalent: U+256A)
    MAKE_BIT_GLYPH(
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        M,M,M,M,M,M,
        _,_,M,_,_,_,
        M,M,M,M,M,M,
        _,_,M,_,_,_,
        _,_,M,_,_,_,
        _,_,M,_,_,_
    ),
    // Box drawing: Vertical Double and Horizontal Single (Unicode equivalent: U+256B)
    MAKE_BIT_GLYPH(
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        M,M,M,M,M,M,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_
    ),
    // Box drawing: Vertical and Horizontal Double (Unicode equivalent: U+256C)
    MAKE_BIT_GLYPH(
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        M,M,_,M,M,M,
        _,_,_,_,_,_,
        M,M,_,M,M,M,
        _,M,_,M,_,_,
        _,M,_,M,_,_,
        _,M,_,M,_,_
    ),
    // Box elements: Upper Half Block (Unicode equivalent: U+2580)
    MAKE_BIT_GLYPH(
        M,M,M,M,M,M,
        M,M,M,M,M,M,
        M,M,M,M,M,M,
        M,M,M,M,M,M,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    ),
    // Box elements: Lower Half Block (Unicode equivalent: U+2584)
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        M,M,M,M,M,M,
        M,M,M,M,M,M,
        M,M,M,M,M,M,
        M,M,M,M,M,M
    ),
    // Box elements: Full Block (Unicode equivalent: U+2588)
    MAKE_BIT_GLYPH(
        M,M,M,M,M,M,
        M,M,M,M,M,M,
        M,M,M,M,M,M,
        M,M,M,M,M,M,
        M,M,M,M,M,M,
        M,M,M,M,M,M,
        M,M,M,M,M,M,
        M,M,M,M,M,M
    ),
    // Box elements: Left Half Block (Unicode equivalent: U+258C)
    MAKE_BIT_GLYPH(
        M,M,M,_,_,_,
        M,M,M,_,_,_,
        M,M,M,_,_,_,
        M,M,M,_,_,_,
        M,M,M,_,_,_,
        M,M,M,_,_,_,
        M,M,M,_,_,_,
        M,M,M,_,_,_
    ),
    // Box elements: Right Half Block (Unicode equivalent: U+2590)
    MAKE_BIT_GLYPH(
        _,_,_,M,M,M,
        _,_,_,M,M,M,
        _,_,_,M,M,M,
        _,_,_,M,M,M,
        _,_,_,M,M,M,
        _,_,_,M,M,M,
        _,_,_,M,M,M,
        _,_,_,M,M,M
    ),
    // Box elements: Light Shade (Unicode equivalent: U+2591)
    MAKE_BIT_GLYPH(
        M,_,_,M,_,_,
        _,_,M,_,_,M,
        M,_,_,M,_,_,
        _,M,_,_,M,_,
        M,_,_,M,_,_,
        _,_,M,_,_,M,
        M,_,_,M,_,_,
        _,M,_,_,M,_
    ),
    // Box elements: Medium Shade (Unicode equivalent: U+2592)
    MAKE_BIT_GLYPH(
        _,M,_,M,_,M,
        M,_,M,_,M,_,
        _,M,_,M,_,M,
        M,_,M,_,M,_,
        _,M,_,M,_,M,
        M,_,M,_,M,_,
        _,M,_,M,_,M,
        M,_,M,_,M,_
    ),
    // Box elements: Dark Shade (Unicode equivalent: U+2593)
    MAKE_BIT_GLYPH(
        _,M,M,_,M,M,
        M,M,_,M,M,_,
        _,M,M,_,M,M,
        M,_,M,M,_,M,
        _,M,M,_,M,M,
        M,M,_,M,M,_,
        _,M,M,_,M,M,
        M,_,M,M,_,M
    ),
    // Geometric Shapes: Black Square (Unicode equivalent: U+25A0)
    MAKE_BIT_GLYPH(
        _,_,_,_,_,_,
        _,_,_,_,_,_,
        _,M,M,M,M,_,
        _,M,M,M,M,_,
        _,M,M,M,M,_,
        _,M,M,M,M,_,
        _,_,_,_,_,_,
        _,_,_,_,_,_
    )
};

// Remember to undefine the _ and M macros!
#undef _
#undef M

//-------------------------------------
// Returns NULL on non-printable control characters or the replacement glyph for undefined characters.
// Relies on the contents of the `bit_glyphs` array. If they're changed, make sure to update this function accordingly!
static inline const bit_glyph *
get_bit_glyph_from_codepoint(const uint32_t codepoint) {
    if (codepoint < 0x20u) {
        // Non-printable C0 control characters: we skip those when rendering, thus return NULL
        return NULL;
    }
    if (codepoint < 0x7Fu) {
        // Printable ASCII characters: map directly to the corresponding printable ASCII bit_glyph
        return &g_bit_glyphs[codepoint - 31u];
    }
    if (codepoint < 0xA0u) {
        // Non-printable control character DEL or C1 control characters: we skip those when rendering, thus return NULL
        return NULL;
    }
    switch (codepoint) {
        // Box drawing Unicode characters with single lines only: 
        // Since they're a bit scattered across the Unicode plane,
        // we map them to their corresponding bit_glyphs using a switch statement
        case 0x2500u: return &g_bit_glyphs[96];
        case 0x2502u: return &g_bit_glyphs[97];
        case 0x250Cu: return &g_bit_glyphs[98];
        case 0x2510u: return &g_bit_glyphs[99];
        case 0x2514u: return &g_bit_glyphs[100];
        case 0x2518u: return &g_bit_glyphs[101];
        case 0x251Cu: return &g_bit_glyphs[102];
        case 0x2524u: return &g_bit_glyphs[103];
        case 0x252Cu: return &g_bit_glyphs[104];
        case 0x2534u: return &g_bit_glyphs[105];
        case 0x253Cu: return &g_bit_glyphs[106];
    }
    if (codepoint >= 0x2550u && codepoint <= 0x256Cu) {
        // Box drawing Unicode characters with double lines and mixed single/double lines:
        // They're placed continuously in the Unicode plane,
        // so we can map them directly using a simple offset calculation
        return &g_bit_glyphs[107u + (codepoint - 0x2550u)];
    }
    switch (codepoint) {
        // Box elements are scattered again
        case 0x2580u: return &g_bit_glyphs[136];
        case 0x2584u: return &g_bit_glyphs[137];
        case 0x2588u: return &g_bit_glyphs[138];
        case 0x258Cu: return &g_bit_glyphs[139];
        case 0x2590u: return &g_bit_glyphs[140];
        case 0x2591u: return &g_bit_glyphs[141];
        case 0x2592u: return &g_bit_glyphs[142];
        case 0x2593u: return &g_bit_glyphs[143];
        case 0x25A0u: return &g_bit_glyphs[144]; // we treat "Black Square" as a box element for now
    }
    // Everything else: we don't have a corresponding bit_glyph, so we return the replacement glyph instead
    return &g_bit_glyphs[0];
}

//-------------------------------------
// Note: This function does NOT validate its inputs. Make sure they're sensible before calling it.
static inline void
draw_bit_glyph(
    const bit_glyph           *glyph,
          void                *target,
    const int                 target_width,
    const int                 target_height,
    const int                 scale,
    const precalculated_color *color,
    const int                 x,
    const int                 y
) {
    const uint64_t glyph_bits = (uint64_t)glyph->raw[0] | ((uint64_t)glyph->raw[1] << 16) | ((uint64_t)glyph->raw[2] << 32);    
          uint32_t *pixels    = (uint32_t*)target;

    for (int glyph_row = 0; glyph_row < MFB_DEBUG_RENDERER_GLYPH_HEIGHT; ++glyph_row) {
        int pixel_top = glyph_row * scale + y;
        if (pixel_top >= target_height) {
            break; // This and all subsequent rows are below the visible area
        }

        int pixel_bottom = pixel_top + scale;
        if (pixel_bottom <= 0) {
            continue; // This row is above the visible area
        }

        if (pixel_top < 0) {
            pixel_top = 0; // Clip the top to the visible area
        }

        if (pixel_bottom > target_height) {
            pixel_bottom = target_height; // Clip the bottom to the visible area
        }

        const uint32_t row_bits = (uint32_t)(glyph_bits >> (glyph_row * MFB_DEBUG_RENDERER_GLYPH_WIDTH)) & 0x3fu;
        
        for (int column = 0; column < MFB_DEBUG_RENDERER_GLYPH_WIDTH; ++column) {
            if (!((row_bits >> column) & 1u)) {
                continue; // Empty pixels are not drawn
            }

            int pixel_left = column * scale + x;
            if (pixel_left >= target_width) {
                break; // This and all subsequent columns are to the right of the visible area
            }

            int pixel_right = pixel_left + scale;
            if (pixel_right <= 0) {
                continue; // This column is to the left of the visible area
            }

            if (pixel_left < 0) {
                pixel_left = 0; // Clip the left to the visible area
            }

            if (pixel_right > target_width) {
                pixel_right = target_width; // Clip the right to the visible area
            }

            uint32_t *pixel_row = pixels + (size_t)pixel_top * (size_t)target_width;
            for (int pixel_y = pixel_top; pixel_y < pixel_bottom; pixel_y++, pixel_row += target_width) {
                for (int pixel_x = pixel_left; pixel_x < pixel_right; pixel_x++) {
                    pixel_row[pixel_x] = blend_precalculated_color_over(color, pixel_row[pixel_x]);
                }
            }
        }
    }
}

//-------------------------------------
// Debug renderer implementation
//-------------------------------------

// Debug renderer type definition
//-------------------------------------
typedef struct mfb_debug_renderer {
    void                        *target;
    int                         target_width;
    int                         target_height;

    int                         scale;
    int                         character_spacing;
    int                         line_spacing;
    int                         baseline;

    uint32_t                    clear_color;
    uint32_t                    draw_color;
    precalculated_color         precalculated_draw_color;

    int                         left_padding;
    int                         top_padding;
    int                         right_padding;
    int                         bottom_padding;
    mfb_debug_renderer_overflow horizontal_overflow;
    mfb_debug_renderer_overflow vertical_overflow;

    int                         x;
    int                         y;
} mfb_debug_renderer;

// Debug renderer default initialization values
//-------------------------------------
#define DEBUG_RENDERER_DEFAULT_SCALE               1
#define DEBUG_RENDERER_DEFAULT_CHARACTER_SPACING   0
#define DEBUG_RENDERER_DEFAULT_LINE_SPACING        0
#define DEBUG_RENDERER_DEFAULT_BASELINE            0
#define DEBUG_RENDERER_DEFAULT_CLEAR_COLOR         MFB_ARGB(0xFFu, 0, 0, 0)
#define DEBUG_RENDERER_DEFAULT_DRAW_COLOR          MFB_ARGB(0xFFu, 0xFFu, 0xFFu, 0xFFu)
#define DEBUG_RENDERER_DEFAULT_LEFT_PADDING        0
#define DEBUG_RENDERER_DEFAULT_RIGHT_PADDING       0
#define DEBUG_RENDERER_DEFAULT_TOP_PADDING         0
#define DEBUG_RENDERER_DEFAULT_BOTTOM_PADDING      0
#define DEBUG_RENDERER_DEFAULT_HORIZONTAL_OVERFLOW MFB_DEBUG_RENDERER_OVERFLOW_WRAP
#define DEBUG_RENDERER_DEFAULT_VERTICAL_OVERFLOW   MFB_DEBUG_RENDERER_OVERFLOW_WRAP

//-------------------------------------
struct mfb_debug_renderer *
mfb_debug_renderer_create(void) {
    struct mfb_debug_renderer *renderer = (struct mfb_debug_renderer *)malloc(sizeof(struct mfb_debug_renderer));
    if (renderer == NULL) {
        return NULL;
    }
    *renderer = (struct mfb_debug_renderer){
        NULL,                                       // target
        0,                                          // target_width
        0,                                          // target_height

        DEBUG_RENDERER_DEFAULT_SCALE,               // scale
        DEBUG_RENDERER_DEFAULT_CHARACTER_SPACING,   // character_spacing
        DEBUG_RENDERER_DEFAULT_LINE_SPACING,        // line_spacing
        DEBUG_RENDERER_DEFAULT_BASELINE,            // baseline

        DEBUG_RENDERER_DEFAULT_CLEAR_COLOR,         // clear_color
        DEBUG_RENDERER_DEFAULT_DRAW_COLOR,          // draw_color
        { { { 0, 0 } }, 0, 0, 0 },                  // precalculated_draw_color, temporarly initialized to all zero

        DEBUG_RENDERER_DEFAULT_LEFT_PADDING,        // left_padding
        DEBUG_RENDERER_DEFAULT_TOP_PADDING,         // top_padding
        DEBUG_RENDERER_DEFAULT_RIGHT_PADDING,       // right_padding
        DEBUG_RENDERER_DEFAULT_BOTTOM_PADDING,      // bottom_padding
        DEBUG_RENDERER_DEFAULT_HORIZONTAL_OVERFLOW, // horizontal_overflow
        DEBUG_RENDERER_DEFAULT_VERTICAL_OVERFLOW,   // vertical_overflow

        DEBUG_RENDERER_DEFAULT_LEFT_PADDING,        // x, initialized to the default left-side padding
        DEBUG_RENDERER_DEFAULT_TOP_PADDING          // y, initialized to the default top-side padding
    };
    // make sure to initialize the precalculated draw color correctly
    precalculate_color(renderer->draw_color, &(renderer->precalculated_draw_color));
    return renderer;
}

//-------------------------------------
struct mfb_debug_renderer *
mfb_debug_renderer_clone(const struct mfb_debug_renderer *renderer) {
    if (renderer == NULL) {
        return NULL;
    }
    struct mfb_debug_renderer *clone = (struct mfb_debug_renderer *)malloc(sizeof(struct mfb_debug_renderer));
    if (clone == NULL) {
        return NULL;
    }
    *clone = *renderer;
    return clone;
}

//-------------------------------------
void
mfb_debug_renderer_destroy(struct mfb_debug_renderer *renderer) {
    if (renderer != NULL) {
        free(renderer);
    }
}

//-------------------------------------
void
mfb_debug_renderer_set_target(struct mfb_debug_renderer *renderer, void *target, int target_width, int target_height) {
    if (renderer == NULL) {
        return;
    }
    renderer->target = target;
    if (renderer->target != NULL) {
        renderer->target_width  = (target_width > 0)  ? target_width  : 0;
        renderer->target_height = (target_height > 0) ? target_height : 0;
    } else {
        renderer->target_width  = 0;
        renderer->target_height = 0;
    }
}

//-------------------------------------
void *
mfb_debug_renderer_get_target(const struct mfb_debug_renderer *renderer) {
    if (renderer == NULL) {
        return NULL;
    }
    return renderer->target;
}

//-------------------------------------
int
mfb_debug_renderer_get_target_width(const struct mfb_debug_renderer *renderer) {
    if (renderer == NULL) {
        return 0;
    }
    return renderer->target_width;
}
//-------------------------------------
int
mfb_debug_renderer_get_target_height(const struct mfb_debug_renderer *renderer) {
    if (renderer == NULL) {
        return 0;
    }
    return renderer->target_height;
}

//-------------------------------------
void
mfb_debug_renderer_set_clear_color(struct mfb_debug_renderer *renderer, uint32_t color) {
    if (renderer == NULL) {
        return;
    }
    renderer->clear_color = color;
}

//-------------------------------------
uint32_t
mfb_debug_renderer_get_clear_color(const struct mfb_debug_renderer *renderer) {
    if (renderer == NULL) {
        return MFB_ARGB(0, 0, 0, 0);
    }
    return renderer->clear_color;
}

//-------------------------------------
void
mfb_debug_renderer_set_draw_color(struct mfb_debug_renderer *renderer, uint32_t color) {
    if (renderer == NULL) {
        return;
    }
    renderer->draw_color = color;
    precalculate_color(renderer->draw_color, &(renderer->precalculated_draw_color));
}

//-------------------------------------
uint32_t
mfb_debug_renderer_get_draw_color(const struct mfb_debug_renderer *renderer) {
    if (renderer == NULL) {
        return MFB_ARGB(0, 0, 0, 0);
    }
    return renderer->draw_color;
}

//-------------------------------------
void
mfb_debug_renderer_set_scale(struct mfb_debug_renderer *renderer, int scale) {
    if (renderer == NULL) {
        return;
    }
    renderer->scale = (scale > 0) ? scale : 0;
}

//-------------------------------------
int
mfb_debug_renderer_get_scale(const struct mfb_debug_renderer *renderer) {
    if (renderer == NULL) {
        return 0;
    }
    return renderer->scale;
}

//-------------------------------------
void
mfb_debug_renderer_set_character_spacing(struct mfb_debug_renderer *renderer, int character_spacing) {
    if (renderer == NULL) {
        return;
    }
    renderer->character_spacing = character_spacing;
}

//-------------------------------------
int
mfb_debug_renderer_get_character_spacing(const struct mfb_debug_renderer *renderer) {
    if (renderer == NULL) {
        return 0;
    }
    return renderer->character_spacing;
}

//-------------------------------------
void
mfb_debug_renderer_set_line_spacing(struct mfb_debug_renderer *renderer, int line_spacing) {
    if (renderer == NULL) {
        return;
    }
    renderer->line_spacing = line_spacing;
}

//-------------------------------------
int
mfb_debug_renderer_get_line_spacing(const struct mfb_debug_renderer *renderer) {
    if (renderer == NULL) {
        return 0;
    }
    return renderer->line_spacing;
}

//-------------------------------------
void
mfb_debug_renderer_set_baseline(struct mfb_debug_renderer *renderer, int baseline) {
    if (renderer == NULL) {
        return;
    }
    renderer->baseline = baseline;
}

//-------------------------------------
int
mfb_debug_renderer_get_baseline(const struct mfb_debug_renderer *renderer) {
    if (renderer == NULL) {
        return 0;
    }
    return renderer->baseline;
}

//-------------------------------------
void
mfb_debug_renderer_set_left_padding(struct mfb_debug_renderer *renderer, int left_padding) {
    if (renderer == NULL) {
        return;
    }
    renderer->left_padding = left_padding;
}

//-------------------------------------
int
mfb_debug_renderer_get_left_padding(const struct mfb_debug_renderer *renderer) {
    if (renderer == NULL) {
        return 0;
    }
    return renderer->left_padding;
}

//-------------------------------------
void
mfb_debug_renderer_set_top_padding(struct mfb_debug_renderer *renderer, int top_padding) {
    if (renderer == NULL) {
        return;
    }
    renderer->top_padding = top_padding;
}

//-------------------------------------
int
mfb_debug_renderer_get_top_padding(const struct mfb_debug_renderer *renderer) {
    if (renderer == NULL) {
        return 0;
    }
    return renderer->top_padding;
}

//-------------------------------------
void
mfb_debug_renderer_set_right_padding(struct mfb_debug_renderer *renderer, int right_padding) {
    if (renderer == NULL) {
        return;
    }
    renderer->right_padding = right_padding;
}

//-------------------------------------
int
mfb_debug_renderer_get_right_padding(const struct mfb_debug_renderer *renderer) {
    if (renderer == NULL) {
        return 0;
    }
    return renderer->right_padding;
}

//-------------------------------------
void
mfb_debug_renderer_set_bottom_padding(struct mfb_debug_renderer *renderer, int bottom_padding) {
    if (renderer == NULL) {
        return;
    }
    renderer->bottom_padding = bottom_padding;
}

//-------------------------------------
int
mfb_debug_renderer_get_bottom_padding(const struct mfb_debug_renderer *renderer) {
    if (renderer == NULL) {
        return 0;
    }
    return renderer->bottom_padding;
}

//-------------------------------------
void
mfb_debug_renderer_set_horizontal_overflow(struct mfb_debug_renderer *renderer, mfb_debug_renderer_overflow overflow) {
    if (renderer == NULL) {
        return;
    }
    if (!(overflow == MFB_DEBUG_RENDERER_OVERFLOW_VISIBLE || overflow == MFB_DEBUG_RENDERER_OVERFLOW_CLIP || overflow == MFB_DEBUG_RENDERER_OVERFLOW_WRAP)) {
        // sanitize to the known range of values only
        return;
    }
    renderer->horizontal_overflow = overflow;
}

//-------------------------------------
mfb_debug_renderer_overflow
mfb_debug_renderer_get_horizontal_overflow(const struct mfb_debug_renderer *renderer) {
    if (renderer == NULL) {
        return MFB_DEBUG_RENDERER_OVERFLOW_VISIBLE;
    }
    return renderer->horizontal_overflow;
}

//-------------------------------------
void
mfb_debug_renderer_set_vertical_overflow(struct mfb_debug_renderer *renderer, mfb_debug_renderer_overflow overflow) {
    if (renderer == NULL) {
        return;
    }
    if (!(overflow == MFB_DEBUG_RENDERER_OVERFLOW_VISIBLE || overflow == MFB_DEBUG_RENDERER_OVERFLOW_CLIP || overflow == MFB_DEBUG_RENDERER_OVERFLOW_WRAP)) {
        // sanitize to the known range of values only
        return;
    }
    renderer->vertical_overflow = overflow;
}

//-------------------------------------
mfb_debug_renderer_overflow
mfb_debug_renderer_get_vertical_overflow(const struct mfb_debug_renderer *renderer) {
    if (renderer == NULL) {
        return MFB_DEBUG_RENDERER_OVERFLOW_VISIBLE;
    }
    return renderer->vertical_overflow;
}

//-------------------------------------
void
mfb_debug_renderer_set_x(struct mfb_debug_renderer *renderer, int x) {
    if (renderer == NULL) {
        return;
    }
    renderer->x = x;
}

//-------------------------------------
int
mfb_debug_renderer_get_x(const struct mfb_debug_renderer *renderer) {
    if (renderer == NULL) {
        return 0;
    }
    return renderer->x;
}

//-------------------------------------
void
mfb_debug_renderer_set_y(struct mfb_debug_renderer *renderer, int y) {
    if (renderer == NULL) {
        return;
    }
    renderer->y = y;
}

//-------------------------------------
int
mfb_debug_renderer_get_y(const struct mfb_debug_renderer *renderer) {
    if (renderer == NULL) {
        return 0;
    }
    return renderer->y;
}

//-------------------------------------
// Debug rendering implementations
//-------------------------------------

//-------------------------------------
void
mfb_debug_render_clear(struct mfb_debug_renderer *renderer) {
    if (renderer == NULL) {
        return;
    }

    // capture the required parts of the current state of the debug renderer as early as possible
          void     *target       = renderer->target;
    const int      target_width  = renderer->target_width;
    const int      target_height = renderer->target_height;
    const uint32_t clear_color   = renderer->clear_color;

    if (target == NULL || target_width <= 0 || target_height <= 0) {
        return;
    }

    const size_t target_size = (size_t)target_width * (size_t)target_height;
    for (size_t i = 0; i < target_size; i++) {
        ((uint32_t *)target)[i] = clear_color;
    }
}

//-------------------------------------
static inline size_t
mfb_debug_render_text_impl(struct mfb_debug_renderer *renderer, const char *text, const bool newline) {
    if (renderer == NULL || text == NULL) {
        return 0;
    }

          void                        *target                  = renderer->target;
    const int                         target_width             = renderer->target_width;
    const int                         target_height            = renderer->target_height;
    const int                         scale                    = renderer->scale;
    if (target == NULL || target_width <= 0 || target_height <= 0 || scale <= 0) {
        return 0;
    }    
    const int                         character_advance        = (renderer->character_spacing + MFB_DEBUG_RENDERER_GLYPH_WIDTH) * scale;
    const int                         line_advance             = (renderer->line_spacing + MFB_DEBUG_RENDERER_GLYPH_HEIGHT) * scale;
    const int                         baseline_offset          = renderer->baseline * scale;
    const precalculated_color         precalculated_draw_color = renderer->precalculated_draw_color;
    const int                         left_padding             = renderer->left_padding;
    const int                         top_padding              = renderer->top_padding;
    const int                         right_limit              = target_width  - renderer->right_padding;
    const int                         bottom_limit             = target_height - renderer->bottom_padding;    
    const mfb_debug_renderer_overflow horizontal_overflow      = renderer->horizontal_overflow;
    const mfb_debug_renderer_overflow vertical_overflow        = renderer->vertical_overflow;
          int                         x                        = renderer->x;
          int                         y                        = renderer->y - baseline_offset; // the algorithm works in baseline-adjusted coordinates

    const int  glyph_width           = MFB_DEBUG_RENDERER_GLYPH_WIDTH * scale;
    const int  glyph_height          = MFB_DEBUG_RENDERER_GLYPH_HEIGHT * scale;
    const int  right_glyph_limit     = right_limit - glyph_width;
    const int  bottom_glyph_limit    = bottom_limit - glyph_height;
    const bool needs_horizontal_clip = horizontal_overflow == MFB_DEBUG_RENDERER_OVERFLOW_CLIP || left_padding > right_glyph_limit; // clipping is also needed when a single glyph can't fit the available space
    const bool needs_vertical_clip   = vertical_overflow == MFB_DEBUG_RENDERER_OVERFLOW_CLIP || (top_padding - baseline_offset) > bottom_glyph_limit;
          
    size_t index                 = 0;
    bool   drawn                 = false; // indicates whether any glyph has been drawn yet
    bool   vertical_wrap         = false; // indicates whether the text has wrapped vertically
    int    original_left_limit   = 0;
    int    original_top_limit    = 0;
    int    original_right_limit  = 0;
    int    original_bottom_limit = 0;
    while (1) {
        uint32_t codepoint  = 0;
        size_t   bytes_read = 0;
        if (!try_decode_next_utf8_codepoint(text + index, &codepoint, &bytes_read)) {
            // skip invalid UTF-8 sequences
            index += bytes_read;
            continue;
        }

        if (codepoint == (uint32_t)'\0') {
            // reached the end of the string
            break;
        }

        const bit_glyph *glyph = get_bit_glyph_from_codepoint(codepoint);
        if (glyph == NULL) {
            // skip non-printable glyphs
            index += bytes_read;
            continue;
        }

        int next_x = x;
        int next_y = y;
        if ((horizontal_overflow != MFB_DEBUG_RENDERER_OVERFLOW_VISIBLE)
            && next_x > right_glyph_limit) {
            // content exceeds the right limit
            if (needs_horizontal_clip) {
                // clip
                break;
            }
            // wrap to the next line
            next_x = left_padding;
            next_y += line_advance;
        }
        if ((vertical_overflow != MFB_DEBUG_RENDERER_OVERFLOW_VISIBLE)
            && next_y > bottom_glyph_limit) {
            // content exceeds the bottom limit
            if (needs_vertical_clip) {
                // clip
                break;
            }
            // wrap to the top line
            next_x = left_padding;
            next_y = top_padding - baseline_offset; // baseline-adjusted coordinates
            vertical_wrap = true;
        }
        if (drawn && vertical_wrap
            && next_x > original_left_limit && next_x < original_right_limit
            && next_y > original_top_limit && next_y < original_bottom_limit) {
            // When vertically wrapped, the next glyph drawn would overlap with the original position where we started rendering.
            // In that case, the remaining content is clipped and rendering stops.
            break;
        }
        // At this point `next_x` and `next_y` are safe coordinates for rendering the glyph.

        draw_bit_glyph(
            glyph,
            target,
            target_width,
            target_height,
            scale,
            &precalculated_draw_color,
            next_x,
            next_y
        );
        
        if (!drawn) {
            // The first glyph has been drawn, so record its original bounding box limits.
            original_left_limit   = next_x - glyph_width;
            original_top_limit    = next_y - glyph_height;
            original_right_limit  = next_x + glyph_width;
            original_bottom_limit = next_y + glyph_height;
            drawn = true;
        }

        x = next_x + character_advance;
        y = next_y;
        index += bytes_read;
    }

    if (newline || drawn) {
        if (newline) {
            x = left_padding;
            y += line_advance;
        }

        if ((horizontal_overflow != MFB_DEBUG_RENDERER_OVERFLOW_VISIBLE)
            && x > right_limit) {
            // current cursor exceeds the right limit
            if (needs_horizontal_clip) {
                // clip
                x = right_limit;
            } else {
                // wrap to the next line
                x = left_padding;
                y += line_advance;
            }
        }
        if ((vertical_overflow != MFB_DEBUG_RENDERER_OVERFLOW_VISIBLE)
            && y > bottom_limit) {
            // current cursor exceeds the bottom limit
            if (needs_vertical_clip) {
                // clip
                y = bottom_limit;
            } else {
                // wrap to the top line
                x = left_padding;
                y = top_padding - baseline_offset; // baseline-adjusted coordinates
            }
        }
    }

    renderer->x = x;
    renderer->y = y + baseline_offset; // convert back from baseline-adjusted coordinates
    return index;
}

//-------------------------------------
size_t
mfb_debug_render_text(struct mfb_debug_renderer *renderer, const char *text) {
    return mfb_debug_render_text_impl(renderer, text, false);
}

//-------------------------------------
size_t
mfb_debug_render_text_line(struct mfb_debug_renderer *renderer, const char *text) {
    return mfb_debug_render_text_impl(renderer, text, true);
}

#endif
