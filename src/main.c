
#include "raylib.h"
#include "id3v2lib.h"

#define UC_IMPL
#include "uc.h"

#define DA_IMPL
#include "da.h"

#include <stddef.h>
#include <stdio.h>
#include <assert.h>

#include "assets.h"

#include "win32.h"

typedef struct {
    Color bg, mg_off, mg_on, fg_off, fg;
} Colors;

Colors theme = {
    {0x18, 0x18, 0x18, 0xFF},
    {0x50, 0x50, 0x50, 0xFF},
    {0x60, 0x60, 0x60, 0xFF},
    {0xD0, 0xD0, 0xD0, 0xFF},
    {0xFF, 0xFF, 0xFF, 0xFF},
};

int font_size = 24;

#ifdef _WIN32
char* GetShortPath(char *path) {
    static char shortPath[MAX_PATH];
    wchar_t wPath[MAX_PATH];
    wchar_t wShortPath[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wPath, MAX_PATH);
    GetShortPathNameW(wPath, wShortPath, MAX_PATH);
    WideCharToMultiByte(CP_UTF8, 0, wShortPath, -1, shortPath, MAX_PATH, NULL, NULL);
    return shortPath;
}

char* to_utf8(char *path) {
    static char shortPath[MAX_PATH];
    wchar_t wPath[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, path, -1, wPath, MAX_PATH);
    WideCharToMultiByte(CP_UTF8, 0, wPath, -1, shortPath, MAX_PATH, NULL, NULL);
    return shortPath;
}

#define get_path(path) GetShortPath(path)
#define norm_text(text) to_utf8(text)
#else
#define get_path(path) path
#define norm_text(text) text
#endif

#include "music.c"
#include "ui.c"
#include "config.c"

Font load_font(char* ptr, size_t size) {
    int codepoints[516] = {0};
    
    for (int i = 0; i < 95; i++) codepoints[i] = 32 + i;          // ascii charachters
    for (int i = 0; i < 255; i++) codepoints[96 + i] = 0x400 + i; // cyrillic charachters
    
    Font font = LoadFontFromMemory(".ttf", (const unsigned char*) ptr, size, font_size, codepoints, 516);
    return font;
}

void generate_and_set_icon() {
    Image icon = LoadImageFromMemory(".png", (const unsigned char*) _ICON_PNG, _ICON_PNG_LENGTH);
    ImageColorTint(&icon, theme.fg);
    ImageResize(&icon, 64, 64);
    Image icon_bg = GenImageColor(64, 64, (Color) {0, 0, 0, 0});
    ImageDrawCircle(&icon_bg, 32, 32, 32, theme.bg);
    ImageDraw(&icon_bg, icon, (Rectangle) {0, 0, 64, 64}, (Rectangle) {0, 0, 64, 64}, (Color) {0xff, 0xff, 0xff, 255});
    SetWindowIcon(icon_bg);
    UnloadImage(icon);
    UnloadImage(icon_bg);
}

int main(void) {
    SetTraceLogLevel(LOG_NONE);
    // Because loading a texture somewhy logs stuff
    
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);
    InitWindow(600, 400, "mus");
    SetExitKey(0);
    
    generate_and_set_icon();
    
    playlist = da_new(char*);
    albums = da_new(Album);
    
    font = load_font(_FONT_TTF, _FONT_TTF_LENGTH);

    Image iplay = LoadImageFromMemory(".png", (const unsigned char*) _PLAY_PNG, _PLAY_PNG_LENGTH);
    Image ipause = LoadImageFromMemory(".png", (const unsigned char*) _PAUSE_PNG, _PAUSE_PNG_LENGTH);
    Image iback = LoadImageFromMemory(".png", (const unsigned char*) _BACK_PNG, _BACK_PNG_LENGTH);
    Image iforward = LoadImageFromMemory(".png", (const unsigned char*) _FORWARD_PNG, _FORWARD_PNG_LENGTH);
    Image irepeat = LoadImageFromMemory(".png", (const unsigned char*) _REPEAT_PNG, _REPEAT_PNG_LENGTH);
    Image irepeat_one = LoadImageFromMemory(".png", (const unsigned char*) _REPEAT_ONE_PNG, _REPEAT_ONE_PNG_LENGTH);
    Image idelete = LoadImageFromMemory(".png", (const unsigned char*) _DELETE_PNG, _DELETE_PNG_LENGTH);
    Image igo_back = LoadImageFromMemory(".png", (const unsigned char*) _GO_BACK_PNG, _GO_BACK_PNG_LENGTH);

    ImageResize(&iplay, font_size, font_size);
    ImageResize(&ipause, font_size, font_size);
    ImageResize(&iback, font_size, font_size);
    ImageResize(&iforward, font_size, font_size);
    ImageResize(&irepeat, font_size, font_size);
    ImageResize(&irepeat_one, font_size, font_size);
    ImageResize(&idelete, font_size, font_size);
    ImageResize(&igo_back, font_size, font_size);

    play = LoadTextureFromImage(iplay);
    pause = LoadTextureFromImage(ipause);
    back = LoadTextureFromImage(iback);
    forward = LoadTextureFromImage(iforward);
    repeat = LoadTextureFromImage(irepeat);
    repeat_one = LoadTextureFromImage(irepeat_one);
    tdelete = LoadTextureFromImage(idelete);
    go_back = LoadTextureFromImage(igo_back);
    
    UnloadImage(iplay);
    UnloadImage(ipause);
    UnloadImage(iback);
    UnloadImage(iforward);
    UnloadImage(irepeat);
    UnloadImage(irepeat_one);
    UnloadImage(idelete);
    UnloadImage(igo_back);

    InitAudioDevice();
    
    Image empty_cover = GenImageColor(font_size*6.f, font_size*6.f, theme.mg_off);
    Album empty_album = {.name = "<not specified>", .year = 0, .genres = "", .artists = "", .cover = LoadTextureFromImage(empty_cover), .playlist = da_new(char*)};
    UnloadImage(empty_cover);
    da_push(albums, empty_album);

    if (FileExists(".mus-savestate")) config_load();

    SetTargetFPS(60);
    
    while (!WindowShouldClose()) {
        cursor = MOUSE_CURSOR_ARROW;
        
        scroll_factor = GetScreenHeight()*0.1f;
        
        if (music_loaded) UpdateMusicStream(music);
        music_update();

        if (IsKeyPressed(KEY_SPACE)) music_play_pause();
        else if (IsKeyPressed(KEY_R)) music_toggle_repeat();
        else if (IsKeyPressed(KEY_LEFT)) music_playlist_previous();
        else if (IsKeyPressed(KEY_RIGHT)) music_playlist_next();
        
        BeginDrawing();

        ClearBackground(theme.bg);

        draw_box((Rectangle) {0, font_size*1.5f, GetScreenWidth(), GetScreenHeight() - font_size*5.f});
        draw_main_ui();
        drop_draw_box();
        
        draw_box((Rectangle) {0, 0, GetScreenWidth(), font_size*1.5f});
        draw_menu_bar();
        drop_draw_box();

        draw_box((Rectangle) {0, GetScreenHeight() - font_size*3.5f, GetScreenWidth(), font_size*3.5f});
        draw_status_bar();
        drop_draw_box();

        SetMouseCursor(cursor);
        EndDrawing();
    }

    CloseAudioDevice();

    UnloadFont(font);
    UnloadTexture(back);
    UnloadTexture(forward);
    UnloadTexture(pause);
    UnloadTexture(play);
    UnloadTexture(repeat);
    UnloadTexture(repeat_one);
    UnloadTexture(tdelete);
    UnloadTexture(go_back);

    CloseWindow();

    config_save();
    
    while (da_length(albums) != 0) pop_album();
    
    return 0;
}
