#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "game.h"

#define STB_DS_IMPLEMENTATION
#include "./stb_ds.h"

#define FACTOR 100
#define WIDTH (16*FACTOR)
#define HEIGHT (9*FACTOR)

#define ANEK_LATIN_FILE_PATH "fonts/AnekLatin-Light.ttf"

void scc(int code)
{
    if (code < 0) {
        fprintf(stderr, "SDL ERROR: %s\n", SDL_GetError());
        exit(1);
    }
}

void *scp(void *ptr)
{
    if (ptr == NULL) {
        fprintf(stderr, "SDL ERROR: %s\n", SDL_GetError());
        exit(1);
    }
    return ptr;
}

static SDL_Renderer *renderer = NULL;
static SDL_Window *window = NULL;

typedef struct {
    char *key; // text
    SDL_Surface *surface;
    SDL_Texture *texture;
} Font_Text_Surface;

typedef struct {
    int key; // ptsize
    TTF_Font *font;
    Font_Text_Surface *texts;
} Font_Cache;

static Font_Cache *font_cache = NULL;

ptrdiff_t font_cache_get_size(int size)
{
    ptrdiff_t font_index = hmgeti(font_cache, (int) size);
    if (font_index < 0) {
        Font_Cache item = {0};
        item.key = size;
        item.font = scp(TTF_OpenFont(ANEK_LATIN_FILE_PATH, size));
        hmputs(font_cache, item);
        font_index = hmgeti(font_cache, (int) size);
        assert(font_index >= 0);
        printf("[LOG] new font size %d\n", (int) size);
    }
    return font_index;
}

ptrdiff_t font_cache_get_text(ptrdiff_t font_index, const char *text)
{
    ptrdiff_t text_index = shgeti(font_cache[font_index].texts, text);
    if (text_index < 0) {
        SDL_Color fg = { .r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF, };
        Font_Text_Surface item = {0};
        item.key = (char*) text;
        item.surface = scp(TTF_RenderText_Blended(font_cache[font_index].font, text, fg));
        item.texture = SDL_CreateTextureFromSurface(renderer, item.surface);
        scc(SDL_SetTextureBlendMode(item.texture, SDL_BLENDMODE_BLEND));
        shputs(font_cache[font_index].texts, item);
        text_index = shgeti(font_cache[font_index].texts, text);
        assert(text_index >= 0);
        printf("[LOG] new text \"%s\"\n", text);
    }
    return text_index;
}

u32 platform_text_width(const char *text, u32 size)
{
    ptrdiff_t font_index = font_cache_get_size(size);
    ptrdiff_t text_index = font_cache_get_text(font_index, text);
    SDL_Surface *surface = font_cache[font_index].texts[text_index].surface;
    return surface->w;
}

void platform_fill_text(i32 x, i32 y, const char *text, u32 size, u32 c)
{
    ptrdiff_t font_index = font_cache_get_size(size);
    ptrdiff_t text_index = font_cache_get_text(font_index, text);
    SDL_Surface *surface = font_cache[font_index].texts[text_index].surface;
    SDL_Texture *texture = font_cache[font_index].texts[text_index].texture;

    int descent = TTF_FontDescent(font_cache[font_index].font);

    SDL_Rect src = { .w = surface->w, .h = surface->h, };
    SDL_Rect dst = { .x = x, .y = y - surface->h - descent, .w = surface->w, .h = surface->h, };
    scc(SDL_RenderCopy(renderer, texture, &src, &dst));

    // TODO: custom color for SDL2 platform_fill_text
    (void) c;
}

SDL_Color unpack_color(uint32_t color)
{
    return (SDL_Color) {
        .r = (color>>(8*0))&0xFF,
        .g = (color>>(8*1))&0xFF,
        .b = (color>>(8*2))&0xFF,
        .a = (color>>(8*3))&0xFF,
    };
}

void platform_fill_rect(int x, int y, int w, int h, uint32_t c)
{
    assert(renderer != NULL);
    SDL_Rect rect = {.x = x, .y = y, .w = w, .h = h,};
    SDL_Color color = unpack_color(c);
    scc(SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a));
    scc(SDL_RenderFillRect(renderer, &rect));
}

void platform_stroke_rect(int x, int y, int w, int h, uint32_t c)
{
    assert(renderer != NULL);
    SDL_Rect rect = {.x = x, .y = y, .w = w, .h = h,};
    SDL_Color color = unpack_color(c);
    scc(SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a));
    scc(SDL_RenderDrawRect(renderer, &rect));
}

void platform_stroke_line(i32 x1, i32 y1, i32 x2, i32 y2, u32 c)
{
    assert(renderer != NULL);
    SDL_Color color = unpack_color(c);
    scc(SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a));
    scc(SDL_RenderDrawLine(renderer, x1, y1, x2, y2));
}

void platform_panic(const char *file_path, int line, const char *message)
{
    fprintf(stderr, "%s:%d: GAME ASSERTION FAILED: %s\n", file_path, line, message);
    abort();
}

void platform_log(const char *message)
{
    printf("[LOG] %s\n", message);
}

int main()
{
    game_init(WIDTH, HEIGHT);

    scc(SDL_Init(SDL_INIT_VIDEO));
    scc(TTF_Init());
    window = scp(SDL_CreateWindow(
                     "Snake Native SDL",
                     0, 0,
                     WIDTH, HEIGHT,
                     SDL_WINDOW_RESIZABLE));

    renderer = scp(SDL_CreateRenderer(
                       window,
                       -1,
                       SDL_RENDERER_ACCELERATED));

    scc(SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND));

    bool quit = false;
    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT: {
                quit = true;
            }
            break;

            case SDL_KEYDOWN: {
                game_keydown(event.key.keysym.sym);
            }
            break;

            case SDL_WINDOWEVENT: {
                switch (event.window.event) {
                case SDL_WINDOWEVENT_RESIZED: {
                    game_resize(event.window.data1, event.window.data2);
                }
                break;
                }
            } break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        game_update(1.0f/60.0f);
        game_render();
        // TODO: better way to lock 60 FPS
        SDL_RenderPresent(renderer);
        SDL_Delay(1000/60);
    }

    TTF_Quit();
    SDL_Quit();

    return 0;
}
