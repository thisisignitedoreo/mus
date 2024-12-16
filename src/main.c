
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

#include "music.c"
#include "ui.c"

#ifdef _WIN32
char *GetShortPath(char *path) {
    static char shortPath[MAX_PATH];
    wchar_t wPath[MAX_PATH];
    wchar_t wShortPath[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wPath, MAX_PATH);
    GetShortPathNameW(wPath, wShortPath, MAX_PATH);
    WideCharToMultiByte(CP_UTF8, 0, wShortPath, -1, shortPath, MAX_PATH, NULL, NULL);
    return shortPath;
}

#define get_path(path) GetShortPath(path)
#else
#define get_path(path) path
#endif

Font load_font(char* ptr, size_t size) {
    int codepoints[516] = {0};
    
    for (int i = 0; i < 95; i++) codepoints[i] = 32 + i;          // ascii charachters
    for (int i = 0; i < 255; i++) codepoints[96 + i] = 0x400 + i; // cyrillic charachters
    codepoints[512] = 0x25B6;
    codepoints[513] = 0x23E9;
    codepoints[514] = 0x23EA;
    codepoints[515] = 0xFE0F;
    
    Font font = LoadFontFromMemory(".ttf", (const unsigned char*) ptr, size, font_size, codepoints, 516);
    return font;
}

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);
    InitWindow(500, 400, "mus");
    SetExitKey(0);
    
    Image icon = LoadImageFromMemory(".png", (const unsigned char*) _ICON_PNG, _ICON_PNG_LENGTH);
    ImageColorTint(&icon, (Color) {0, 0, 0, 255});
    ImageResize(&icon, 64, 64);
    SetWindowIcon(icon);
    UnloadImage(icon);
    
    playlist = da_new(char*);
    
    font = load_font(_FONT_TTF, _FONT_TTF_LENGTH);

    Image iplay = LoadImageFromMemory(".png", (const unsigned char*) _PLAY_PNG, _PLAY_PNG_LENGTH);
    Image ipause = LoadImageFromMemory(".png", (const unsigned char*) _PAUSE_PNG, _PAUSE_PNG_LENGTH);
    Image iback = LoadImageFromMemory(".png", (const unsigned char*) _BACK_PNG, _BACK_PNG_LENGTH);
    Image iforward = LoadImageFromMemory(".png", (const unsigned char*) _FORWARD_PNG, _FORWARD_PNG_LENGTH);
    Image irepeat = LoadImageFromMemory(".png", (const unsigned char*) _REPEAT_PNG, _REPEAT_PNG_LENGTH);
    Image irepeat_one = LoadImageFromMemory(".png", (const unsigned char*) _REPEAT_ONE_PNG, _REPEAT_ONE_PNG_LENGTH);
    Image idelete = LoadImageFromMemory(".png", (const unsigned char*) _DELETE_PNG, _DELETE_PNG_LENGTH);

    ImageResize(&iplay, font_size, font_size);
    ImageResize(&ipause, font_size, font_size);
    ImageResize(&iback, font_size, font_size);
    ImageResize(&iforward, font_size, font_size);
    ImageResize(&irepeat, font_size, font_size);
    ImageResize(&irepeat_one, font_size, font_size);
    ImageResize(&idelete, font_size, font_size);

    play = LoadTextureFromImage(iplay);
    pause = LoadTextureFromImage(ipause);
    back = LoadTextureFromImage(iback);
    forward = LoadTextureFromImage(iforward);
    repeat = LoadTextureFromImage(irepeat);
    repeat_one = LoadTextureFromImage(irepeat_one);
    tdelete = LoadTextureFromImage(idelete);
    
    UnloadImage(iplay);
    UnloadImage(ipause);
    UnloadImage(iback);
    UnloadImage(iforward);
    UnloadImage(irepeat);
    UnloadImage(irepeat_one);
    UnloadImage(idelete);

    InitAudioDevice();

    SetTargetFPS(60);
    
    while (!WindowShouldClose()) {
        cursor = MOUSE_CURSOR_ARROW;

        if (IsFileDropped()) {
            FilePathList files = LoadDroppedFiles();
            for (size_t i = 0; i < files.count; i++) {
                if (music_ismusic(get_path(files.paths[i]))) music_add_to_playlist(get_path(files.paths[i]));
            }
            UnloadDroppedFiles(files);
        }
        
        if (music_loaded) UpdateMusicStream(music);
        music_update();
        
        BeginDrawing();
        
        ClearBackground(theme.bg);

        draw_box((Rectangle) {0, font_size*1.5f, GetScreenWidth(), GetScreenHeight() - font_size*4.5f});
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

    CloseWindow();
    
    return 0;
}
