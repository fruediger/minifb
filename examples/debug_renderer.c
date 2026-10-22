// This source file uses C11's UTF-8 string literals feature,
// so make sure the file is UTF-8 encoded (preferably with BOM).
#if !(defined(MINIFB_USE_DEBUG_RENDERER) && MINIFB_USE_DEBUG_RENDERER)
#error "MINIFB_USE_DEBUG_RENDERER must be enabled for the debug renderer example"
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <MiniFB.h>
#include <MiniFB_debug_renderer.h>

//-------------------------------------
#define PI 3.14159265358979323846f

// Common colors and alpha manipulation
//-------------------------------------
#define RED        MFB_RGB(255,   0,   0)
#define YELLOW     MFB_RGB(255, 255,   0)
#define GREEN      MFB_RGB(  0, 255,   0)
#define CYAN       MFB_RGB(  0, 255, 255)
#define BLUE       MFB_RGB(  0,   0, 255)
#define MAGENTA    MFB_RGB(255,   0, 255)
#define BLACK      MFB_RGB(  0,   0,   0)
#define DARK_GRAY  MFB_RGB( 64,  64,  64)
#define GRAY       MFB_RGB(128, 128, 128)
#define LIGHT_GRAY MFB_RGB(192, 192, 192)
#define WHITE      MFB_RGB(255, 255, 255)

#define SET_ALPHA(color, alpha) MFB_ARGB(alpha, MFB_GET_R(color), MFB_GET_G(color), MFB_GET_B(color))

//-------------------------------------
#define WIDTH 800
#define HEIGHT 600

// (UI) Rendering styles
//-------------------------------------
typedef enum {
    NONE              = 0,
    PLASMA_BACKGROUND = 1 << 0
} style_flags;

typedef struct {
    uint32_t    background_color;
    uint32_t    foreground_color;
    uint32_t    shadow_color;
    style_flags flags;
} style;

static const style g_styles[] = {
    { BLACK,   WHITE, DARK_GRAY,  NONE              },
    { RED,     BLACK, DARK_GRAY,  NONE              },
    { YELLOW,  BLACK, DARK_GRAY,  NONE              },
    { GREEN,   BLACK, DARK_GRAY,  NONE              },
    { CYAN,    BLACK, DARK_GRAY,  NONE              },
    { BLUE,    WHITE, DARK_GRAY,  NONE              },
    { MAGENTA, BLACK, DARK_GRAY,  NONE              },
    { WHITE,   BLACK, LIGHT_GRAY, NONE              },
    { BLACK,   WHITE, DARK_GRAY,  PLASMA_BACKGROUND }
};
static const size_t g_styles_count = sizeof(g_styles) / sizeof(g_styles[0]);

static size_t g_style_index = 0;

static inline void
next_style() {
    g_style_index = (g_style_index + 1) % g_styles_count;
}

// Content pages
//-------------------------------------
typedef void (*page_render_callback)(struct mfb_debug_renderer *renderer);

typedef struct {
    page_render_callback render_callback;
    const char           *title;
} page;

// Prototypes for page rendering functions (defined further down below)
static void render_glyphs(struct mfb_debug_renderer *renderer);
static void render_scaling(struct mfb_debug_renderer *renderer);
static void render_hori(struct mfb_debug_renderer *renderer);
static void render_vert_vis(struct mfb_debug_renderer *renderer);
static void render_vert_clip(struct mfb_debug_renderer *renderer);
static void render_vert_wrap(struct mfb_debug_renderer *renderer);
static void render_rgb(struct mfb_debug_renderer *renderer);
static void render_cmy(struct mfb_debug_renderer *renderer);
static void render_gray(struct mfb_debug_renderer *renderer);

static const page g_pages[] = {
    { render_glyphs,    "All Glyphs"                             },
    { render_scaling,   "Scaling"                                },
    { render_hori,      "Horizontal Overflow"                    },
    { render_vert_vis,  "Vertical Overflow (visible)"            },
    { render_vert_clip, "Vertical Overflow (clipped)"            },
    { render_vert_wrap, "Vertical Overflow (wrapped)"            },
    { render_rgb,       "Blended Colors (Red, Green, Blue)"      },
    { render_cmy,       "Blended Colors (Cyan, Magenta, Yellow)" },
    { render_gray,      "Blended Colors (Grayscale)"             },
};
static const size_t g_pages_count = sizeof(g_pages) / sizeof(g_pages[0]);

static size_t g_page_index = 0;

static inline void
prev_page() {
    g_page_index = (g_page_index + g_pages_count - 1) % g_pages_count;
}

static inline void
next_page() {
    g_page_index = (g_page_index + 1) % g_pages_count;
}

//-------------------------------------
static uint32_t g_palette[512];

static void
initialize_palette() {
    float inc = 90.0f / 64.0f;
    for (uint32_t c=0; c<64; ++c) {
        int32_t col = (int32_t) ((255.0f * sinf(c * inc * PI / 180.0f)) + 0.5f);
        g_palette[64 * 0 + c] = MFB_RGB(    col,       0,       0);
        g_palette[64 * 1 + c] = MFB_RGB(    255,     col,       0);
        g_palette[64 * 2 + c] = MFB_RGB(255-col,     255,       0);
        g_palette[64 * 3 + c] = MFB_RGB(      0,     255,     col);
        g_palette[64 * 4 + c] = MFB_RGB(      0, 255-col,     255);
        g_palette[64 * 5 + c] = MFB_RGB(    col,       0,     255);
        g_palette[64 * 6 + c] = MFB_RGB(    255,       0, 255-col);
        g_palette[64 * 7 + c] = MFB_RGB(255-col,       0,       0);
    }
}

// Rendering implementation for the UI
//-------------------------------------
static void
render_background(struct mfb_debug_renderer *renderer, struct mfb_timer *timer) {
    mfb_debug_renderer_set_clear_color(renderer, g_styles[g_style_index].background_color);
    mfb_debug_render_clear            (renderer);

    if (g_styles[g_style_index].flags & PLASMA_BACKGROUND) {        
        uint32_t *buffer = (uint32_t *)mfb_debug_renderer_get_target       (renderer);
        const int width  =             mfb_debug_renderer_get_target_width (renderer);
        const int height =             mfb_debug_renderer_get_target_height(renderer);

        // Copied and adapted from the example in `multiple_windows.c`
        float time = (float)mfb_timer_now(timer);
        float time_x = sinf(time * PI / 180.0f);
        float time_y = cosf(time * PI / 180.0f);
        int i = 0;
        for (int y = 0; y < height; ++y) {
            float dy = cosf((y * time_y) * PI / 180.0f);                // [-1, 1]
            for (int x=0; x < width; ++x) {
                float dx = sinf((x * time_x) * PI / 180.0f);            // [-1, 1]
                int index = (int) ((2.0f + dx + dy) * 0.25f * 511.0f);  // [0, 511]
                buffer[i++] = g_palette[index];
            }
        }
    }
}

static void
render_page(struct mfb_debug_renderer *renderer) {
    g_pages[g_page_index].render_callback(renderer);
}

static void
render_ui(struct mfb_debug_renderer *renderer) {
    mfb_debug_renderer_set_padding          (renderer, 4, 4, 4, 4);
    mfb_debug_renderer_set_character_spacing(renderer, 0);
    mfb_debug_renderer_set_line_spacing     (renderer, 0);
    mfb_debug_renderer_set_draw_color       (renderer, g_styles[g_style_index].foreground_color);
    mfb_debug_renderer_reset_position       (renderer);

    mfb_debug_renderer_set_scale            (renderer, 2);
    mfb_debug_render_text_line              (renderer, u8"╔╡████████╞════════════════════════════════════════════════════╤═╗");
    mfb_debug_render_text_line              (renderer, u8"║                                                              │ ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                              └─╢");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"╠════════════════════════════════════════════════════════════════╣");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"╟────────────────────────────────────────────────────────────────╢");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"║                                                                ║");
    mfb_debug_render_text_line              (renderer, u8"╟────────────────────────────────┬─────────────────┬─────────────╢");   
    mfb_debug_render_text_line              (renderer, u8"║ Change [S]tyle                 │ [P]revious Page │ [N]ext Page ║"); 
    mfb_debug_render_text_line              (renderer, u8"╚════════════════════════════════╧═════════════════╧═════════════╝");

    mfb_debug_renderer_set_position         (renderer, 770, 20);
    mfb_debug_render_text                   (renderer, u8"■");

    mfb_debug_renderer_set_scale            (renderer, 4);
    mfb_debug_renderer_set_draw_color       (renderer, g_styles[g_style_index].shadow_color);
    mfb_debug_renderer_set_position         (renderer, 38, 37);
    mfb_debug_render_text_line              (renderer, "Debug Renderer Examples");
    mfb_debug_renderer_set_draw_color       (renderer, g_styles[g_style_index].foreground_color);
    mfb_debug_renderer_set_position         (renderer, 36, 35);
    mfb_debug_render_text_line              (renderer, "Debug Renderer Examples");

    mfb_debug_renderer_set_scale            (renderer, 3);
    mfb_debug_renderer_set_draw_color       (renderer, g_styles[g_style_index].shadow_color);
    mfb_debug_renderer_set_position         (renderer, 38, 106);
    mfb_debug_render_text_line              (renderer, g_pages[g_page_index].title);
    mfb_debug_renderer_set_draw_color       (renderer, g_styles[g_style_index].foreground_color);
    mfb_debug_renderer_set_position         (renderer, 36, 104);
    mfb_debug_render_text_line              (renderer, g_pages[g_page_index].title);
}

static void render_fps(struct mfb_debug_renderer *renderer, struct mfb_timer *timer) {
    mfb_debug_renderer_set_draw_color(renderer, g_styles[g_style_index].background_color);
    mfb_debug_renderer_set_scale     (renderer, 1);
    mfb_debug_renderer_set_overflow  (renderer, MFB_DEBUG_RENDERER_OVERFLOW_VISIBLE, MFB_DEBUG_RENDERER_OVERFLOW_VISIBLE);

    // The debug renderer API does not (yet?) string formatting with its `mfb_debug_render_text` function,
    // so the string needs to be formatted manually before rendering.
    static char text[256];
    const double delta = mfb_timer_delta(timer);
    int length = snprintf(text, sizeof(text), "FPS: %.1f", (isfinite(delta) && delta >= 1.0e-5) ? 1.0 / delta : 0.0);

    mfb_debug_renderer_set_position  (renderer, 28 + (96 - length * MFB_DEBUG_RENDERER_GLYPH_WIDTH) / 2, 8);
    mfb_debug_render_text            (renderer, text);
}

// Actual rendering implementations for the various pages
//-------------------------------------
static void
render_glyphs(struct mfb_debug_renderer *renderer) {
    // The string is led by a ÿ character (U+00FF), to demonstrate that printable
    // yet for our mapping unknown characters are handled by rendering a replacement glyph instead.
    static const char *glyphs = u8"ÿ !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~─│┌┐└┘├┤┬┴┼═║╒╓╔╕╖╗╘╙╚╛╜╝╞╟╠╡╢╣╤╥╦╧╨╩╪╫╬▀▄█▌▐░▒▓■";

    mfb_debug_renderer_set_padding          (renderer, 36, 160, 36, 48);
    mfb_debug_renderer_set_overflow         (renderer, MFB_DEBUG_RENDERER_OVERFLOW_WRAP, MFB_DEBUG_RENDERER_OVERFLOW_CLIP);
    mfb_debug_renderer_set_draw_color       (renderer, g_styles[g_style_index].foreground_color);
    mfb_debug_renderer_set_character_spacing(renderer, 0);
    mfb_debug_renderer_set_line_spacing     (renderer, 0);

    mfb_debug_renderer_set_scale            (renderer, 1);
    mfb_debug_renderer_set_position         (renderer, 36, 160);
    mfb_debug_render_text_line              (renderer, glyphs);

    mfb_debug_renderer_set_scale            (renderer, 2);
    mfb_debug_renderer_set_position         (renderer, 36, 184);
    mfb_debug_render_text_line              (renderer, glyphs);

    mfb_debug_renderer_set_scale            (renderer, 3);
    mfb_debug_renderer_set_position         (renderer, 36, 240);
    mfb_debug_render_text_line              (renderer, glyphs);

    mfb_debug_renderer_set_scale            (renderer, 4);
    mfb_debug_renderer_set_position         (renderer, 36, 344);
    mfb_debug_render_text_line              (renderer, glyphs);
}

static void
render_scaling(struct mfb_debug_renderer *renderer) {
    mfb_debug_renderer_set_padding          (renderer, 36, 160, 36, 48);
    mfb_debug_renderer_set_overflow         (renderer, MFB_DEBUG_RENDERER_OVERFLOW_WRAP, MFB_DEBUG_RENDERER_OVERFLOW_CLIP);
    mfb_debug_renderer_set_draw_color       (renderer, g_styles[g_style_index].foreground_color);
    mfb_debug_renderer_set_character_spacing(renderer, 0);
    mfb_debug_renderer_set_line_spacing     (renderer, 0);

    mfb_debug_renderer_set_scale            (renderer, 1);
    mfb_debug_renderer_set_position         (renderer, 36, 160);
    mfb_debug_render_text_line              (renderer, "Scale 1x");

    mfb_debug_renderer_set_scale            (renderer, 2);
    mfb_debug_renderer_set_position         (renderer, 36, 176);
    mfb_debug_render_text_line              (renderer, "Scale 2x");

    mfb_debug_renderer_set_scale            (renderer, 3);
    mfb_debug_renderer_set_position         (renderer, 36, 200);
    mfb_debug_render_text_line              (renderer, "Scale 3x");

    mfb_debug_renderer_set_scale            (renderer, 4);
    mfb_debug_renderer_set_position         (renderer, 36, 232);
    mfb_debug_render_text_line              (renderer, "Scale 4x");

    mfb_debug_renderer_set_scale            (renderer, 5);
    mfb_debug_renderer_set_position         (renderer, 36, 272);
    mfb_debug_render_text_line              (renderer, "Scale 5x");

    mfb_debug_renderer_set_scale            (renderer, 6);
    mfb_debug_renderer_set_position         (renderer, 36, 320);
    mfb_debug_render_text_line              (renderer, "Scale 6x");

    mfb_debug_renderer_set_scale            (renderer, 7);
    mfb_debug_renderer_set_position         (renderer, 36, 376);
    mfb_debug_render_text_line              (renderer, "Scale 7x");

    mfb_debug_renderer_set_scale            (renderer, 8);
    mfb_debug_renderer_set_position         (renderer, 36, 440);
    mfb_debug_render_text_line              (renderer, "Scale 8x");
}

static void
render_hori(struct mfb_debug_renderer *renderer) {
    mfb_debug_renderer_set_scale              (renderer, 2);
    mfb_debug_renderer_set_padding            (renderer, 36, 160, 36, 48);
    mfb_debug_renderer_set_vertical_overflow  (renderer, MFB_DEBUG_RENDERER_OVERFLOW_CLIP);
    mfb_debug_renderer_set_draw_color         (renderer, g_styles[g_style_index].foreground_color);
    mfb_debug_renderer_set_character_spacing  (renderer, 0);
    mfb_debug_renderer_set_line_spacing       (renderer, 4);

    mfb_debug_renderer_set_horizontal_overflow(renderer, MFB_DEBUG_RENDERER_OVERFLOW_CLIP);
    mfb_debug_renderer_set_position           (renderer, 36, 160);
    mfb_debug_render_text                     (renderer, "If `horizontal_overflow` is set to ...");

    mfb_debug_renderer_set_horizontal_overflow(renderer, MFB_DEBUG_RENDERER_OVERFLOW_VISIBLE);
    mfb_debug_renderer_set_position           (renderer, 36, 192);
    mfb_debug_render_text                     (renderer, " ... `MFB_DEBUG_RENDERER_OVERFLOW_VISIBLE`, the rendered text can exceed the set right limit.");

    mfb_debug_renderer_set_horizontal_overflow(renderer, MFB_DEBUG_RENDERER_OVERFLOW_CLIP);
    mfb_debug_renderer_set_position           (renderer, 36, 224);
    mfb_debug_render_text                     (renderer, " ... `MFB_DEBUG_RENDERER_OVERFLOW_CLIP`, the rendered text is clipped at the set right limit at character boundaries.");

    mfb_debug_renderer_set_horizontal_overflow(renderer, MFB_DEBUG_RENDERER_OVERFLOW_WRAP);
    mfb_debug_renderer_set_position           (renderer, 36, 256);
    mfb_debug_render_text                     (renderer, " ... `MFB_DEBUG_RENDERER_OVERFLOW_WRAP`, the rendered text is wrapped to the next line at the set right limit at character boundaries.");
}

static void
render_vert_vis(struct mfb_debug_renderer *renderer) {
    mfb_debug_renderer_set_scale              (renderer, 2);
    mfb_debug_renderer_set_padding            (renderer, 36, 160, 36, 48);
    mfb_debug_renderer_set_horizontal_overflow(renderer, MFB_DEBUG_RENDERER_OVERFLOW_WRAP);
    mfb_debug_renderer_set_draw_color         (renderer, g_styles[g_style_index].foreground_color);
    mfb_debug_renderer_set_character_spacing  (renderer, 0);
    mfb_debug_renderer_set_line_spacing       (renderer, 4);

    mfb_debug_renderer_set_vertical_overflow  (renderer, MFB_DEBUG_RENDERER_OVERFLOW_VISIBLE);
    mfb_debug_renderer_set_position           (renderer, 36, 520);
    mfb_debug_render_text                     (renderer, "If `vertical_overflow` is set to `MFB_DEBUG_RENDERER_OVERFLOW_VISIBLE`, the rendered text can exceed the set bottom limit.");

    mfb_debug_renderer_set_position           (renderer, 36, 240);
    mfb_debug_render_text                     (renderer, "*`MFB_DEBUG_RENDERER_OVERFLOW_VISIBLE`");
}

static void
render_vert_clip(struct mfb_debug_renderer *renderer) {
    mfb_debug_renderer_set_scale              (renderer, 2);
    mfb_debug_renderer_set_padding            (renderer, 36, 160, 36, 48);
    mfb_debug_renderer_set_horizontal_overflow(renderer, MFB_DEBUG_RENDERER_OVERFLOW_WRAP);
    mfb_debug_renderer_set_draw_color         (renderer, g_styles[g_style_index].foreground_color);
    mfb_debug_renderer_set_character_spacing  (renderer, 0);
    mfb_debug_renderer_set_line_spacing       (renderer, 4);

    mfb_debug_renderer_set_vertical_overflow  (renderer, MFB_DEBUG_RENDERER_OVERFLOW_CLIP);
    mfb_debug_renderer_set_position           (renderer, 36, 520);
    mfb_debug_render_text                     (renderer, "If `vertical_overflow` is set to `MFB_DEBUG_RENDERER_OVERFLOW_CLIP`, the rendered text is clipped at the set bottom limit at character boundaries.");

    mfb_debug_renderer_set_position           (renderer, 36, 240);
    mfb_debug_render_text                     (renderer, "*`MFB_DEBUG_RENDERER_OVERFLOW_CLIP`");
}

static void
render_vert_wrap(struct mfb_debug_renderer *renderer) {
    mfb_debug_renderer_set_scale              (renderer, 2);
    mfb_debug_renderer_set_padding            (renderer, 36, 160, 36, 48);
    mfb_debug_renderer_set_horizontal_overflow(renderer, MFB_DEBUG_RENDERER_OVERFLOW_WRAP);
    mfb_debug_renderer_set_draw_color         (renderer, g_styles[g_style_index].foreground_color);
    mfb_debug_renderer_set_character_spacing  (renderer, 0);
    mfb_debug_renderer_set_line_spacing       (renderer, 4);

    mfb_debug_renderer_set_vertical_overflow  (renderer, MFB_DEBUG_RENDERER_OVERFLOW_WRAP);
    mfb_debug_renderer_set_position           (renderer, 36, 520);
    mfb_debug_render_text                     (renderer, "If `vertical_overflow` is set to `MFB_DEBUG_RENDERER_OVERFLOW_WRAP`, the rendered text is wrapped to the top line at the set bottom limit at character boundaries.");

    mfb_debug_renderer_set_position           (renderer, 36, 240);
    mfb_debug_render_text                     (renderer, "*`MFB_DEBUG_RENDERER_OVERFLOW_WRAP`");
}

static void
render_rgb(struct mfb_debug_renderer *renderer) {
    mfb_debug_renderer_set_scale            (renderer, 3);
    mfb_debug_renderer_set_overflow         (renderer, MFB_DEBUG_RENDERER_OVERFLOW_VISIBLE, MFB_DEBUG_RENDERER_OVERFLOW_VISIBLE);
    mfb_debug_renderer_set_character_spacing(renderer, 0);

    mfb_debug_renderer_set_position         (renderer, 36, 160);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(RED, 255));
    mfb_debug_render_text                   (renderer, "Red 100%");
    mfb_debug_renderer_set_position         (renderer, 36, 190);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(RED, 204));
    mfb_debug_render_text                   (renderer, "Red  80%");
    mfb_debug_renderer_set_position         (renderer, 36, 220);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(RED, 153));
    mfb_debug_render_text                   (renderer, "Red  60%");
    mfb_debug_renderer_set_position         (renderer, 36, 250);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(RED, 102));
    mfb_debug_render_text                   (renderer, "Red  40%");
    mfb_debug_renderer_set_position         (renderer, 36, 280);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(RED, 51));
    mfb_debug_render_text                   (renderer, "Red  20%");
    mfb_debug_renderer_set_position         (renderer, 36, 310);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(RED, 0));
    mfb_debug_render_text                   (renderer, "Red   0%");

    mfb_debug_renderer_set_position         (renderer, 202, 160);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(GREEN, 255));
    mfb_debug_render_text                   (renderer, "Green 100%");
    mfb_debug_renderer_set_position         (renderer, 202, 190);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(GREEN, 204));
    mfb_debug_render_text                   (renderer, "Green  80%");
    mfb_debug_renderer_set_position         (renderer, 202, 220);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(GREEN, 153));
    mfb_debug_render_text                   (renderer, "Green  60%");
    mfb_debug_renderer_set_position         (renderer, 202, 250);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(GREEN, 102));
    mfb_debug_render_text                   (renderer, "Green  40%");
    mfb_debug_renderer_set_position         (renderer, 202, 280);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(GREEN, 51));
    mfb_debug_render_text                   (renderer, "Green  20%");
    mfb_debug_renderer_set_position         (renderer, 202, 310);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(GREEN, 0));
    mfb_debug_render_text                   (renderer, "Green   0%");

    mfb_debug_renderer_set_position         (renderer, 400, 160);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(BLUE, 255));
    mfb_debug_render_text                   (renderer, "Blue 100%");
    mfb_debug_renderer_set_position         (renderer, 400, 190);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(BLUE, 204));
    mfb_debug_render_text                   (renderer, "Blue  80%");
    mfb_debug_renderer_set_position         (renderer, 400, 220);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(BLUE, 153));
    mfb_debug_render_text                   (renderer, "Blue  60%");
    mfb_debug_renderer_set_position         (renderer, 400, 250);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(BLUE, 102));
    mfb_debug_render_text                   (renderer, "Blue  40%");
    mfb_debug_renderer_set_position         (renderer, 400, 280);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(BLUE, 51));
    mfb_debug_render_text                   (renderer, "Blue  20%");
    mfb_debug_renderer_set_position         (renderer, 400, 310);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(BLUE, 0));
    mfb_debug_render_text                   (renderer, "Blue   0%");
}

static void
render_cmy(struct mfb_debug_renderer *renderer) {
    mfb_debug_renderer_set_scale            (renderer, 3);
    mfb_debug_renderer_set_overflow         (renderer, MFB_DEBUG_RENDERER_OVERFLOW_VISIBLE, MFB_DEBUG_RENDERER_OVERFLOW_VISIBLE);
    mfb_debug_renderer_set_character_spacing(renderer, 0);

    mfb_debug_renderer_set_position         (renderer, 36, 160);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(CYAN, 255));
    mfb_debug_render_text                   (renderer, "Cyan 100%");
    mfb_debug_renderer_set_position         (renderer, 36, 190);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(CYAN, 204));
    mfb_debug_render_text                   (renderer, "Cyan  80%");
    mfb_debug_renderer_set_position         (renderer, 36, 220);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(CYAN, 153));
    mfb_debug_render_text                   (renderer, "Cyan  60%");
    mfb_debug_renderer_set_position         (renderer, 36, 250);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(CYAN, 102));
    mfb_debug_render_text                   (renderer, "Cyan  40%");
    mfb_debug_renderer_set_position         (renderer, 36, 280);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(CYAN, 51));
    mfb_debug_render_text                   (renderer, "Cyan  20%");
    mfb_debug_renderer_set_position         (renderer, 36, 310);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(CYAN, 0));
    mfb_debug_render_text                   (renderer, "Cyan  0%");

    mfb_debug_renderer_set_position         (renderer, 220, 160);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(MAGENTA, 255));
    mfb_debug_render_text                   (renderer, "Magenta 100%");
    mfb_debug_renderer_set_position         (renderer, 220, 190);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(MAGENTA, 204));
    mfb_debug_render_text                   (renderer, "Magenta  80%");
    mfb_debug_renderer_set_position         (renderer, 220, 220);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(MAGENTA, 153));
    mfb_debug_render_text                   (renderer, "Magenta  60%");
    mfb_debug_renderer_set_position         (renderer, 220, 250);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(MAGENTA, 102));
    mfb_debug_render_text                   (renderer, "Magenta  40%");
    mfb_debug_renderer_set_position         (renderer, 220, 280);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(MAGENTA, 51));
    mfb_debug_render_text                   (renderer, "Magenta  20%");
    mfb_debug_renderer_set_position         (renderer, 220, 310);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(MAGENTA, 0));
    mfb_debug_render_text                   (renderer, "Magenta   0%");

    mfb_debug_renderer_set_position         (renderer, 450, 160);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(YELLOW, 255));
    mfb_debug_render_text                   (renderer, "Yellow 100%");
    mfb_debug_renderer_set_position         (renderer, 450, 190);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(YELLOW, 204));
    mfb_debug_render_text                   (renderer, "Yellow  80%");
    mfb_debug_renderer_set_position         (renderer, 450, 220);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(YELLOW, 153));
    mfb_debug_render_text                   (renderer, "Yellow  60%");
    mfb_debug_renderer_set_position         (renderer, 450, 250);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(YELLOW, 102));
    mfb_debug_render_text                   (renderer, "Yellow  40%");
    mfb_debug_renderer_set_position         (renderer, 450, 280);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(YELLOW, 51));
    mfb_debug_render_text                   (renderer, "Yellow  20%");
    mfb_debug_renderer_set_position         (renderer, 450, 310);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(YELLOW, 0));
    mfb_debug_render_text                   (renderer, "Yellow   0%");
}

static void
render_gray(struct mfb_debug_renderer *renderer) {
    mfb_debug_renderer_set_scale            (renderer, 2);
    mfb_debug_renderer_set_overflow         (renderer, MFB_DEBUG_RENDERER_OVERFLOW_VISIBLE, MFB_DEBUG_RENDERER_OVERFLOW_VISIBLE);
    mfb_debug_renderer_set_character_spacing(renderer, 0);

    mfb_debug_renderer_set_position         (renderer, 36, 160);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(BLACK, 255));
    mfb_debug_render_text                   (renderer, "Black 100%");
    mfb_debug_renderer_set_position         (renderer, 36, 180);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(BLACK, 204));
    mfb_debug_render_text                   (renderer, "Black  80%");
    mfb_debug_renderer_set_position         (renderer, 36, 200);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(BLACK, 153));
    mfb_debug_render_text                   (renderer, "Black  60%");
    mfb_debug_renderer_set_position         (renderer, 36, 220);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(BLACK, 102));
    mfb_debug_render_text                   (renderer, "Black  40%");
    mfb_debug_renderer_set_position         (renderer, 36, 240);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(BLACK, 51));
    mfb_debug_render_text                   (renderer, "Black  20%");
    mfb_debug_renderer_set_position         (renderer, 36, 260);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(BLACK, 0));
    mfb_debug_render_text                   (renderer, "Black   0%");

    mfb_debug_renderer_set_position         (renderer, 180, 160);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(DARK_GRAY, 255));
    mfb_debug_render_text                   (renderer, "D.Gray 100%");
    mfb_debug_renderer_set_position         (renderer, 180, 180);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(DARK_GRAY, 204));
    mfb_debug_render_text                   (renderer, "D.Gray  80%");
    mfb_debug_renderer_set_position         (renderer, 180, 200);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(DARK_GRAY, 153));
    mfb_debug_render_text                   (renderer, "D.Gray  60%");
    mfb_debug_renderer_set_position         (renderer, 180, 220);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(DARK_GRAY, 102));
    mfb_debug_render_text                   (renderer, "D.Gray  40%");
    mfb_debug_renderer_set_position         (renderer, 180, 240);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(DARK_GRAY, 51));
    mfb_debug_render_text                   (renderer, "D.Gray  20%");
    mfb_debug_renderer_set_position         (renderer, 180, 260);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(DARK_GRAY, 0));
    mfb_debug_render_text                   (renderer, "D.Gray   0%");

    mfb_debug_renderer_set_position         (renderer, 336, 160);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(GRAY, 255));
    mfb_debug_render_text                   (renderer, "Gray 100%");
    mfb_debug_renderer_set_position         (renderer, 336, 180);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(GRAY, 204));
    mfb_debug_render_text                   (renderer, "Gray  80%");
    mfb_debug_renderer_set_position         (renderer, 336, 200);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(GRAY, 153));
    mfb_debug_render_text                   (renderer, "Gray  60%");
    mfb_debug_renderer_set_position         (renderer, 336, 220);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(GRAY, 102));
    mfb_debug_render_text                   (renderer, "Gray  40%");
    mfb_debug_renderer_set_position         (renderer, 336, 240);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(GRAY, 51));
    mfb_debug_render_text                   (renderer, "Gray  20%");
    mfb_debug_renderer_set_position         (renderer, 336, 260);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(GRAY, 0));
    mfb_debug_render_text                   (renderer, "Gray   0%");

    mfb_debug_renderer_set_position         (renderer, 470, 160);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(LIGHT_GRAY, 255));
    mfb_debug_render_text                   (renderer, "L.Gray 100%");
    mfb_debug_renderer_set_position         (renderer, 470, 180);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(LIGHT_GRAY, 204));
    mfb_debug_render_text                   (renderer, "L.Gray  80%");
    mfb_debug_renderer_set_position         (renderer, 470, 200);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(LIGHT_GRAY, 153));
    mfb_debug_render_text                   (renderer, "L.Gray  60%");
    mfb_debug_renderer_set_position         (renderer, 470, 220);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(LIGHT_GRAY, 102));
    mfb_debug_render_text                   (renderer, "L.Gray  40%");
    mfb_debug_renderer_set_position         (renderer, 470, 240);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(LIGHT_GRAY, 51));
    mfb_debug_render_text                   (renderer, "L.Gray  20%");
    mfb_debug_renderer_set_position         (renderer, 470, 260);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(LIGHT_GRAY, 0));
    mfb_debug_render_text                   (renderer, "L.Gray   0%");

    mfb_debug_renderer_set_position         (renderer, 626, 160);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(WHITE, 255));
    mfb_debug_render_text                   (renderer, "White 100%");
    mfb_debug_renderer_set_position         (renderer, 626, 180);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(WHITE, 204));
    mfb_debug_render_text                   (renderer, "White  80%");
    mfb_debug_renderer_set_position         (renderer, 626, 200);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(WHITE, 153));
    mfb_debug_render_text                   (renderer, "White  60%");
    mfb_debug_renderer_set_position         (renderer, 626, 220);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(WHITE, 102));
    mfb_debug_render_text                   (renderer, "White  40%");
    mfb_debug_renderer_set_position         (renderer, 626, 240);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(WHITE, 51));
    mfb_debug_render_text                   (renderer, "White  20%");
    mfb_debug_renderer_set_position         (renderer, 626, 260);
    mfb_debug_renderer_set_draw_color       (renderer, SET_ALPHA(WHITE, 0));
    mfb_debug_render_text                   (renderer, "White   0%");
}

// Input handling and UI interactions
//-------------------------------------
static void
keyboard(struct mfb_window *window, mfb_key key, mfb_key_mod mod, bool is_pressed) {
    (void)window;
    (void)mod;

    if (key == MFB_KB_KEY_S) {        
        static bool s_held = false;
        if (!s_held && is_pressed) {
            next_style();
        }
        s_held = is_pressed;
    }
    if (key == MFB_KB_KEY_P) {
        static bool p_held = false;
        if (!p_held && is_pressed) {
            prev_page();
        }
        p_held = is_pressed;
    }
    if (key == MFB_KB_KEY_N) {
        static bool n_held = false;
        if (!n_held && is_pressed) {
            next_page();
        }
        n_held = is_pressed;
    }
}

static void
mouse_button(struct mfb_window *window, mfb_mouse_button button, mfb_key_mod mod, bool is_pressed) {
    (void)mod;

    if (button == MFB_MOUSE_LEFT) {
        static bool left_held = false;
        if (!left_held && is_pressed) {
            const int x = mfb_get_mouse_x(window);
            const int y = mfb_get_mouse_y(window);
            if (y >= 4 && y <= 52 && x >= WIDTH - 40 && x <= WIDTH - 4) {
                mfb_close(window);
            } else if (y >= HEIGHT - 44 && y <= HEIGHT - 12) {
                if (x >= 10 && x < 406) {
                    next_style();
                } else if (x >= 406 && x < 622) {
                    prev_page();
                } else if (x >= 622 && x < 790) {
                    next_page();
                }
            }
        }
        left_held = is_pressed;
    }
}

//-------------------------------------
int main() {
    initialize_palette();

    struct mfb_window *window = mfb_open("Debug Renderer Examples", WIDTH, HEIGHT);
    if (!window) {
        return -1;
    }

    mfb_set_keyboard_callback(window, keyboard);
    mfb_set_mouse_button_callback(window, mouse_button);

    uint32_t *buffer = (uint32_t *)malloc(WIDTH * HEIGHT * sizeof(uint32_t));
    if (!buffer) {
        return -1;
    }

    struct mfb_debug_renderer *ui_renderer = mfb_debug_renderer_create();
    if (!ui_renderer) {
        free(buffer);
        return -1;
    }

    mfb_debug_renderer_set_target(ui_renderer, buffer, WIDTH, HEIGHT);

    struct mfb_debug_renderer *page_renderer = mfb_debug_renderer_clone(ui_renderer);
    if (!page_renderer) {
        mfb_debug_renderer_destroy(ui_renderer);
        free(buffer);
        return -1;
    }

    struct mfb_timer *timer = mfb_timer_create();
    if (!timer) {
        mfb_debug_renderer_destroy(page_renderer);
        mfb_debug_renderer_destroy(ui_renderer);
        free(buffer);
        return -1;
    }

    do {
        render_background(page_renderer, timer);
        render_page(page_renderer);
        render_ui(ui_renderer);
        render_fps(ui_renderer, timer);

        if (mfb_update(window, buffer) != MFB_STATE_OK) {
            window = NULL;
            break;
        }
    } while (mfb_wait_sync(window));

    mfb_timer_destroy(timer);
    mfb_debug_renderer_destroy(page_renderer);
    mfb_debug_renderer_destroy(ui_renderer);
    free(buffer);
    return 0;
}