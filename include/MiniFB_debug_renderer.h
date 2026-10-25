#pragma once

#if defined(MINIFB_USE_DEBUG_RENDERER) && MINIFB_USE_DEBUG_RENDERER

#include <stddef.h>
#include <stdint.h>

// Debug renderer base glyph dimensions
//-------------------------------------
#define MFB_DEBUG_RENDERER_GLYPH_WIDTH            (6)
#define MFB_DEBUG_RENDERER_GLYPH_HEIGHT           (8)
#define MFB_DEBUG_RENDERER_GLYPH_NATURAL_BASELINE (6)

// Debug renderer overflow behavior
//-------------------------------------
typedef enum {
    // Overflows the visible area if the content exceeds the available space.
    // Ignores all padding constraints.
    MFB_DEBUG_RENDERER_OVERFLOW_VISIBLE,

    // Clips the content at the character level if it exceeds the available space.
    MFB_DEBUG_RENDERER_OVERFLOW_CLIP,

    // If used for horizontal overflow:
    //   Wraps the content partially to the next line if it exceeds the available horizontal space.
    //   If a single glyph can't fit within the available space,
    //   the content will be clipped at character level at that point.
    // If used for vertical overflow:
    //   Wraps the content partially to the top if it exceeds the available vertical space.
    //   If the content would overlap with where it started (i.e., the original position),
    //   or a single glyph can't fit within the available space,
    //   the content will be clipped at character level at that point.
    MFB_DEBUG_RENDERER_OVERFLOW_WRAP,
} mfb_debug_renderer_overflow;

// Debug renderer opaque pointer
//-------------------------------------
struct mfb_debug_renderer;

#ifdef __cplusplus
extern "C" {
#endif

// Creates a new debug renderer instance.
// Must be destroyed with `mfb_debug_renderer_destroy` when it's no longer needed.
// Returns NULL in case of an error.
struct mfb_debug_renderer * mfb_debug_renderer_create(void);

// Destroys a debug renderer instance.
// It's safe to pass a NULL pointer; in that case, the function will do nothing.
void                        mfb_debug_renderer_destroy(struct mfb_debug_renderer *renderer);

// Copies the state of one debug renderer instance to another.
// Can be useful to preserve the state of a renderer to reset the working renderer to at the beginning of a frame, for example.
void                         mfb_debug_renderer_copy(struct mfb_debug_renderer *dest, const struct mfb_debug_renderer *src);

// Sets or gets the target rendering buffer. The target's dimensions are specified in pixels.
// If the buffer is NULL or the dimensions are not positive integers, no rendering will occur.
void                        mfb_debug_renderer_set_target(struct mfb_debug_renderer *renderer, void *target, int target_width, int target_height);
void *                      mfb_debug_renderer_get_target(const struct mfb_debug_renderer *renderer);
int                         mfb_debug_renderer_get_target_width(const struct mfb_debug_renderer *renderer);
int                         mfb_debug_renderer_get_target_height(const struct mfb_debug_renderer *renderer);

// Sets or gets the scale used to draw text to the rendering target when rendering text.
// Must be a positive integer; if it's not, no rendering will occur.
void                        mfb_debug_renderer_set_scale(struct mfb_debug_renderer *renderer, int scale);
int                         mfb_debug_renderer_get_scale(const struct mfb_debug_renderer *renderer);

// Sets or gets the spacing between characters, in pixels, when rendering text.
// The spacing is measured as a base value at a scale of `1`, and will be adapted according to the current scale when rendering text.
void                        mfb_debug_renderer_set_character_spacing(struct mfb_debug_renderer *renderer, int character_spacing);
int                         mfb_debug_renderer_get_character_spacing(const struct mfb_debug_renderer *renderer);

// Sets or gets the spacing between lines, in pixels, when rendering text.
// The spacing is measured as a base value at a scale of `1`, and will be adapted according to the current scale when rendering text.
void                        mfb_debug_renderer_set_line_spacing(struct mfb_debug_renderer *renderer, int line_spacing);
int                         mfb_debug_renderer_get_line_spacing(const struct mfb_debug_renderer *renderer);

// Sets or gets the baseline, in pixels, that determines the vertical position of origin of each glyph when rendering text.
// The baseline is relative to the top of the glyph, where `0` represents the topmost pixel
// and `MFB_DEBUG_RENDERER_GLYPH_HEIGHT - 1` represents the bottommost pixel.
// It is measured as a base value at a scale of `1`, and will be adapted according to the current scale when rendering text.
// Defaults to `0`; use `MFB_DEBUG_RENDERER_GLYPH_NATURAL_BASELINE` for the natural baseline of the glyph.
void                        mfb_debug_renderer_set_baseline(struct mfb_debug_renderer *renderer, int baseline);
int                         mfb_debug_renderer_get_baseline(const struct mfb_debug_renderer *renderer);

// Sets or gets the color used to reset a portion of the rendering target to when clearing it.
void                        mfb_debug_renderer_set_clear_color(struct mfb_debug_renderer *renderer, uint32_t clear_color);
uint32_t                    mfb_debug_renderer_get_clear_color(const struct mfb_debug_renderer *renderer);

// Sets or gets the color used when rendering text to the rendering target.
void                        mfb_debug_renderer_set_draw_color(struct mfb_debug_renderer *renderer, uint32_t draw_color);
uint32_t                    mfb_debug_renderer_get_draw_color(const struct mfb_debug_renderer *renderer);

// Sets or gets the left-side padding, in pixels, used when rendering text to the rendering target.
// When a whole line is rendered or horizontal wrapping occurs, the horizontal position will be reset to the specified value.
// Notice that changing the padding will not automatically adjust the current rendering position.
void                        mfb_debug_renderer_set_left_padding(struct mfb_debug_renderer *renderer, int left_padding);
int                         mfb_debug_renderer_get_left_padding(const struct mfb_debug_renderer *renderer);

// Sets or gets the top-side padding, in pixels, used when rendering text to the rendering target.
// When vertical wrapping occurs, the vertical position will be reset to the specified value.
// Notice that changing the padding will not automatically adjust the current rendering position.
void                        mfb_debug_renderer_set_top_padding(struct mfb_debug_renderer *renderer, int top_padding);
int                         mfb_debug_renderer_get_top_padding(const struct mfb_debug_renderer *renderer);

// Sets or gets the right-side padding, in pixels, used when rendering text to the rendering target.
// If horizontal wrapping or clipping is enabled, the specified value, relative from the right edge of the rendering target,
// will determine the maximum horizontal position for rendering text.
void                        mfb_debug_renderer_set_right_padding(struct mfb_debug_renderer *renderer, int right_padding);
int                         mfb_debug_renderer_get_right_padding(const struct mfb_debug_renderer *renderer);

// Sets or gets the bottom-side padding, in pixels, used when rendering text to the rendering target.
// If vertical wrapping or clipping is enabled, the specified value, relative from the bottom edge of the rendering target,
// will determine the maximum vertical position for rendering text.
void                        mfb_debug_renderer_set_bottom_padding(struct mfb_debug_renderer *renderer, int bottom_padding);
int                         mfb_debug_renderer_get_bottom_padding(const struct mfb_debug_renderer *renderer);

// Sets or gets the horizontal overflow behavior, determining what happens when content exceeds the horizontal bounds when rendering text.
void                        mfb_debug_renderer_set_horizontal_overflow(struct mfb_debug_renderer *renderer, mfb_debug_renderer_overflow overflow);
mfb_debug_renderer_overflow mfb_debug_renderer_get_horizontal_overflow(const struct mfb_debug_renderer *renderer);

// Sets or gets the vertical overflow behavior, determining what happens when content exceeds the vertical bounds when rendering text.
void                        mfb_debug_renderer_set_vertical_overflow(struct mfb_debug_renderer *renderer, mfb_debug_renderer_overflow overflow);
mfb_debug_renderer_overflow mfb_debug_renderer_get_vertical_overflow(const struct mfb_debug_renderer *renderer);

// Sets or gets the horizontal position, in pixels, where text will be rendered next.
// Updates automatically when rendering text.
// Can be manually set ignoring the current padding or overflow constraints, if needed.
void                        mfb_debug_renderer_set_x(struct mfb_debug_renderer *renderer, int x);
int                         mfb_debug_renderer_get_x(const struct mfb_debug_renderer *renderer);

// Sets or gets the vertical position, in pixels, where text will be rendered next.
// Updates automatically when rendering text.
// Can be manually set ignoring the current padding or overflow constraints, if needed.
void                        mfb_debug_renderer_set_y(struct mfb_debug_renderer *renderer, int y);
int                         mfb_debug_renderer_get_y(const struct mfb_debug_renderer *renderer);

// Clears a portion of the rendering target with the current clear color, constrained by the current padding.
// The portion of the rendering target specified by the current padding will be overwritten with the current clear color.
// No alpha blending will occur, and the content will be fully replaced with the clear color.
void                        mfb_debug_render_clear(struct mfb_debug_renderer *renderer);

// Clears the entire rendering target with the current clear color, ignoring the current padding.
// The entire rendering target will be overwritten with the current clear color.
// No alpha blending will occur, and the content will be fully replaced with the clear color.
void                        mfb_debug_render_clear_full(struct mfb_debug_renderer *renderer);

// Renders the specified text at the current position using the debug renderer's current state.
// Returns the amount of input text, in bytes, that was consumed during rendering and before any clipping occurred.
// If the content is rendered entirely without clipping, the return value will be the total length of the input text, in bytes,
// excluding the null-terminator.
// Uses the current draw color as the color for rendering the text. If the draw color has an alpha component different from `255`,
// alpha blending with the existing content of the rendering target will occur.
// Notice that any control characters, including line feed (`'\n'`) and carriage return (`'\r'`), are ignored and skipped during rendering.
// If you want to force a line break at the end of the given text, use `mfb_debug_render_text_line` instead.
size_t                      mfb_debug_render_text(struct mfb_debug_renderer *renderer, const char *text);

// Renders the specified text at the current position using the debug renderer's current state
// and advances the position to the beginning of the next line after rendering.
// Returns the amount of input text, in bytes, that was consumed during rendering and before any clipping occurred.
// If the content is rendered entirely without clipping, the return value will be the total length of the input text, in bytes,
// excluding the null-terminator.
// Uses the current draw color as the color for rendering the text. If the draw color has an alpha component different from `255`,
// alpha blending with the existing content of the rendering target will occur.
// Notice that any control characters, including line feed (`'\n'`) and carriage return (`'\r'`), are ignored and skipped during rendering.
// This function always automatically advances the position to the beginning of the next line after rendering,
// except in cases where vertical clipping occurs.
size_t                      mfb_debug_render_text_line(struct mfb_debug_renderer *renderer, const char *text);

#ifdef __cplusplus
}
#endif

// Convenience function for creating a new debug renderer instance by copying the state of an existing one
// Like `mfb_debug_renderer_copy`, this is a shallow copy: the clone refers to the same target buffer as the original.
// Returns NULL in case of an error.
static inline struct mfb_debug_renderer *
mfb_debug_renderer_clone(const struct mfb_debug_renderer *renderer) {
    if (renderer == NULL) {
        return NULL;
    }
    struct mfb_debug_renderer *clone = mfb_debug_renderer_create(); // maybe NULL
    mfb_debug_renderer_copy(clone, renderer); // it's fine to pass `clone` here, even if it's NULL, `mfb_debug_renderer_copy` does nothing if any input is NULL
    return clone;
}

// Convenience function for getting both target dimensions at once
static inline void
mfb_debug_renderer_get_target_dimensions(const struct mfb_debug_renderer *renderer, int *width, int *height) {
    if (width != NULL) {
        *width = mfb_debug_renderer_get_target_width(renderer);
    }
    if (height != NULL) {
        *height = mfb_debug_renderer_get_target_height(renderer);
    }
}

// Convenience functions for setting or getting both horizontal padding values at once
static inline void
mfb_debug_renderer_set_horizontal_padding(struct mfb_debug_renderer *renderer, int left_padding, int right_padding) {
    mfb_debug_renderer_set_left_padding(renderer, left_padding);
    mfb_debug_renderer_set_right_padding(renderer, right_padding);
}

static inline void
mfb_debug_renderer_get_horizontal_padding(const struct mfb_debug_renderer *renderer, int *left_padding, int *right_padding) {
    if (left_padding != NULL) {
        *left_padding = mfb_debug_renderer_get_left_padding(renderer);
    }
    if (right_padding != NULL) {
        *right_padding = mfb_debug_renderer_get_right_padding(renderer);
    }
}

// Convenience functions for setting or getting both vertical padding values at once
static inline void
mfb_debug_renderer_set_vertical_padding(struct mfb_debug_renderer *renderer, int top_padding, int bottom_padding) {
    mfb_debug_renderer_set_top_padding(renderer, top_padding);
    mfb_debug_renderer_set_bottom_padding(renderer, bottom_padding);
}

static inline void
mfb_debug_renderer_get_vertical_padding(const struct mfb_debug_renderer *renderer, int *top_padding, int *bottom_padding) {
    if (top_padding != NULL) {
        *top_padding = mfb_debug_renderer_get_top_padding(renderer);
    }
    if (bottom_padding != NULL) {
        *bottom_padding = mfb_debug_renderer_get_bottom_padding(renderer);
    }
}

// Convenience macros for setting or getting all four padding values at once
static inline void
mfb_debug_renderer_set_padding(struct mfb_debug_renderer *renderer, int padding_left, int padding_top, int padding_right, int padding_bottom) {
    mfb_debug_renderer_set_left_padding(renderer, padding_left);
    mfb_debug_renderer_set_top_padding(renderer, padding_top);
    mfb_debug_renderer_set_right_padding(renderer, padding_right);
    mfb_debug_renderer_set_bottom_padding(renderer, padding_bottom);
}

static inline void
mfb_debug_renderer_get_padding(const struct mfb_debug_renderer *renderer, int *padding_left, int *padding_top, int *padding_right, int *padding_bottom) {
    if (padding_left != NULL) {
        *padding_left = mfb_debug_renderer_get_left_padding(renderer);
    }
    if (padding_top != NULL) {
        *padding_top = mfb_debug_renderer_get_top_padding(renderer);
    }
    if (padding_right != NULL) {
        *padding_right = mfb_debug_renderer_get_right_padding(renderer);
    }
    if (padding_bottom != NULL) {
        *padding_bottom = mfb_debug_renderer_get_bottom_padding(renderer);
    }
}

// Convenience functions for setting or getting both overflow behavior values at once
static inline void
mfb_debug_renderer_set_overflow(struct mfb_debug_renderer *renderer, mfb_debug_renderer_overflow overflow_horizontal, mfb_debug_renderer_overflow overflow_vertical) {
    mfb_debug_renderer_set_horizontal_overflow(renderer, overflow_horizontal);
    mfb_debug_renderer_set_vertical_overflow(renderer, overflow_vertical);
}

static inline void
mfb_debug_renderer_get_overflow(const struct mfb_debug_renderer *renderer, mfb_debug_renderer_overflow *overflow_horizontal, mfb_debug_renderer_overflow *overflow_vertical) {
    if (overflow_horizontal != NULL) {
        *overflow_horizontal = mfb_debug_renderer_get_horizontal_overflow(renderer);
    }
    if (overflow_vertical != NULL) {
        *overflow_vertical = mfb_debug_renderer_get_vertical_overflow(renderer);
    }
}

// Convenience function for resetting both position values to the current padding values
static inline void
mfb_debug_renderer_reset_position(struct mfb_debug_renderer *renderer) {
    mfb_debug_renderer_set_x(renderer, mfb_debug_renderer_get_left_padding(renderer));
    mfb_debug_renderer_set_y(renderer, mfb_debug_renderer_get_top_padding(renderer));
}

// Convenience functions for setting or getting both position values at once
static inline void
mfb_debug_renderer_set_position(struct mfb_debug_renderer *renderer, int x, int y) {
    mfb_debug_renderer_set_x(renderer, x);
    mfb_debug_renderer_set_y(renderer, y);
}

static inline void
mfb_debug_renderer_get_position(const struct mfb_debug_renderer *renderer, int *x, int *y) {
    if (x != NULL) {
        *x = mfb_debug_renderer_get_x(renderer);
    }
    if (y != NULL) {
        *y = mfb_debug_renderer_get_y(renderer);
    }
}

#endif
