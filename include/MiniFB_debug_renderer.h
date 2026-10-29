#pragma once

#if defined(MINIFB_USE_DEBUG_RENDERER) && MINIFB_USE_DEBUG_RENDERER

#include <stddef.h>
#include <stdint.h>

// Glyph dimensions at scale 1, in pixels
//-------------------------------------
#define MFB_DEBUG_RENDERER_GLYPH_WIDTH            (6)
#define MFB_DEBUG_RENDERER_GLYPH_HEIGHT           (8)
#define MFB_DEBUG_RENDERER_GLYPH_NATURAL_BASELINE (6)

// Behavior when rendering reaches the padded boundary
//-------------------------------------
typedef enum {
    // Ignore padding and continue.
    // It's safe for rendering to even extend virtually beyond the target boundary.
    MFB_DEBUG_RENDERER_OVERFLOW_VISIBLE,

    // Stop before rendering that would extend beyond the padded boundary.
    MFB_DEBUG_RENDERER_OVERFLOW_CLIP,

    // Wrap to the next line horizontally or to the top left vertically
    // when rendering would extend beyond the padded boundary.
    // Stop if rendering cannot fit or vertical wrapping would overlap the starting position.
    MFB_DEBUG_RENDERER_OVERFLOW_WRAP,
} mfb_debug_renderer_overflow;

//-------------------------------------
struct mfb_debug_renderer;

#ifdef __cplusplus
extern "C" {
#endif

// Create a renderer. Destroy it with `mfb_debug_renderer_destroy`. Returns NULL if allocation fails.
struct mfb_debug_renderer * mfb_debug_renderer_create(void);

// Destroy a renderer. NULL is allowed.
void                        mfb_debug_renderer_destroy(struct mfb_debug_renderer *renderer);

// Copy all renderer state, including the target pointer. NULL source or destination is ignored.
void                         mfb_debug_renderer_copy(struct mfb_debug_renderer *dest, const struct mfb_debug_renderer *src);

// Set or get the target buffer and its pixel dimensions.
// Rendering requires a non-NULL buffer and positive dimensions.
void                        mfb_debug_renderer_set_target(struct mfb_debug_renderer *renderer, void *target, int target_width, int target_height);
void *                      mfb_debug_renderer_get_target(const struct mfb_debug_renderer *renderer);
int                         mfb_debug_renderer_get_target_width(const struct mfb_debug_renderer *renderer);
int                         mfb_debug_renderer_get_target_height(const struct mfb_debug_renderer *renderer);

// Set or get the glyph scale. Values below 1 disable text rendering.
void                        mfb_debug_renderer_set_scale(struct mfb_debug_renderer *renderer, int scale);
int                         mfb_debug_renderer_get_scale(const struct mfb_debug_renderer *renderer);

// Set or get the extra horizontal spacing between glyphs, in pixels at scale 1.
// It is multiplied by the current scale.
void                        mfb_debug_renderer_set_character_spacing(struct mfb_debug_renderer *renderer, int character_spacing);
int                         mfb_debug_renderer_get_character_spacing(const struct mfb_debug_renderer *renderer);

// Set or get the extra vertical spacing between lines, in pixels at scale 1.
// It is multiplied by the current scale.
void                        mfb_debug_renderer_set_line_spacing(struct mfb_debug_renderer *renderer, int line_spacing);
int                         mfb_debug_renderer_get_line_spacing(const struct mfb_debug_renderer *renderer);

// Set or get the baseline offset from the top of each glyph, in pixels at scale 1.
// The offset is multiplied by the current scale.
// `MFB_DEBUG_RENDERER_GLYPH_NATURAL_BASELINE` gives the glyph's natural baseline.
void                        mfb_debug_renderer_set_baseline(struct mfb_debug_renderer *renderer, int baseline);
int                         mfb_debug_renderer_get_baseline(const struct mfb_debug_renderer *renderer);

// Set or get the color for clearing the target. The clear functions replace pixels without blending.
void                        mfb_debug_renderer_set_clear_color(struct mfb_debug_renderer *renderer, uint32_t clear_color);
uint32_t                    mfb_debug_renderer_get_clear_color(const struct mfb_debug_renderer *renderer);

// Set or get the color for drawing text. Non-opaque colors are alpha-blended over the target.
void                        mfb_debug_renderer_set_draw_color(struct mfb_debug_renderer *renderer, uint32_t draw_color);
uint32_t                    mfb_debug_renderer_get_draw_color(const struct mfb_debug_renderer *renderer);

// Set or get the left padding in pixels. Rendering returns here after a line break or horizontal wrap.
// Changing padding does not move the cursor.
void                        mfb_debug_renderer_set_left_padding(struct mfb_debug_renderer *renderer, int left_padding);
int                         mfb_debug_renderer_get_left_padding(const struct mfb_debug_renderer *renderer);

// Set or get the top padding in pixels. Vertical wrapping returns rendering here.
// Changing padding does not move the cursor.
void                        mfb_debug_renderer_set_top_padding(struct mfb_debug_renderer *renderer, int top_padding);
int                         mfb_debug_renderer_get_top_padding(const struct mfb_debug_renderer *renderer);

// Set or get the right padding in pixels. With clipping or wrapping,
// rendering must fit before the boundary measured inward from the target's right edge.
void                        mfb_debug_renderer_set_right_padding(struct mfb_debug_renderer *renderer, int right_padding);
int                         mfb_debug_renderer_get_right_padding(const struct mfb_debug_renderer *renderer);

// Set or get the bottom padding in pixels. With clipping or wrapping,
// rendering must fit above the boundary measured inward from the target's bottom edge.
void                        mfb_debug_renderer_set_bottom_padding(struct mfb_debug_renderer *renderer, int bottom_padding);
int                         mfb_debug_renderer_get_bottom_padding(const struct mfb_debug_renderer *renderer);

// Set or get the behavior at the horizontal boundary. Clipping and wrapping use the right padding.
void                        mfb_debug_renderer_set_horizontal_overflow(struct mfb_debug_renderer *renderer, mfb_debug_renderer_overflow overflow);
mfb_debug_renderer_overflow mfb_debug_renderer_get_horizontal_overflow(const struct mfb_debug_renderer *renderer);

// Set or get the behavior at the vertical boundary. Clipping and wrapping use the bottom padding.
void                        mfb_debug_renderer_set_vertical_overflow(struct mfb_debug_renderer *renderer, mfb_debug_renderer_overflow overflow);
mfb_debug_renderer_overflow mfb_debug_renderer_get_vertical_overflow(const struct mfb_debug_renderer *renderer);

// Set or get the next horizontal rendering position in pixels.
// Position changes are not constrained by padding or overflow settings.
void                        mfb_debug_renderer_set_x(struct mfb_debug_renderer *renderer, int x);
int                         mfb_debug_renderer_get_x(const struct mfb_debug_renderer *renderer);

// Set or get the next vertical rendering position in pixels.
// Text rendering is adjusted according to the current baseline setting.
// Position changes are not constrained by padding or overflow settings.
void                        mfb_debug_renderer_set_y(struct mfb_debug_renderer *renderer, int y);
int                         mfb_debug_renderer_get_y(const struct mfb_debug_renderer *renderer);

// Clear the area inside the padding with the clear color. The color replaces existing pixels.
void                        mfb_debug_render_clear(struct mfb_debug_renderer *renderer);

// Clear the entire target with the clear color, ignoring padding. The color replaces existing pixels.
void                        mfb_debug_render_clear_full(struct mfb_debug_renderer *renderer);

// Render a null-terminated UTF-8 string at the current position and update the cursor.
// Invalid UTF-8 and C0 or C1 control characters (including '\n' and '\r') are skipped.
// Characters without an associated glyph are drawn as a replacement glyph.
// Stops before drawing a glyph that would overflow, depending on the current overflow setting.
// Returns the number of input bytes processed before stopping, including skipped bytes,
// excluding the null terminator.
size_t                      mfb_debug_render_text(struct mfb_debug_renderer *renderer, const char *text);

// Render text like `mfb_debug_render_text`, then advance the cursor to the next line,
// even if no glyph was drawn, except in case of error.
size_t                      mfb_debug_render_text_line(struct mfb_debug_renderer *renderer, const char *text);

#ifdef __cplusplus
}
#endif

// Create a renderer with the same state and target pointer as another renderer.
// Returns NULL if the input is NULL or allocation fails.
static inline struct mfb_debug_renderer *
mfb_debug_renderer_clone(const struct mfb_debug_renderer *renderer) {
    if (renderer == NULL) {
        return NULL;
    }
    struct mfb_debug_renderer *clone = mfb_debug_renderer_create();
    mfb_debug_renderer_copy(clone, renderer);
    return clone;
}

// Get both target dimensions. Either output pointer may be NULL.
static inline void
mfb_debug_renderer_get_target_dimensions(const struct mfb_debug_renderer *renderer, int *width, int *height) {
    if (width != NULL) {
        *width = mfb_debug_renderer_get_target_width(renderer);
    }
    if (height != NULL) {
        *height = mfb_debug_renderer_get_target_height(renderer);
    }
}

// Set or get both horizontal padding values. Either output pointer may be NULL.
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

// Set or get both vertical padding values. Either output pointer may be NULL.
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

// Set or get all four padding values. Any output pointer may be NULL.
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

// Set or get both overflow values. Either output pointer may be NULL.
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

// Reset the position to the current left and top padding values.
static inline void
mfb_debug_renderer_reset_position(struct mfb_debug_renderer *renderer) {
    mfb_debug_renderer_set_x(renderer, mfb_debug_renderer_get_left_padding(renderer));
    mfb_debug_renderer_set_y(renderer, mfb_debug_renderer_get_top_padding(renderer));
}

// Set or get both position values. Either output pointer may be NULL.
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
