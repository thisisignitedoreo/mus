
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

Font font;
Texture play, pause, back, forward, repeat, repeat_one, tdelete;
float font_spacing = 0;
int font_size = 24;
int cursor = MOUSE_CURSOR_ARROW;

float music_seek_temp = 0.0f;

#define DRAW_STACK_SIZE 100
Rectangle draw_stack[DRAW_STACK_SIZE] = {0};
size_t draw_stack_size = 0;

bool seeking_music = false;

float clamp(float x, float a, float b) {
    if (x < a) return a;
    if (x > b) return b;
    return x;
}

Rectangle get_draw_box() {
    Rectangle draw_box = {.x = 0, .y = 0, .width = GetScreenWidth(), .height = GetScreenHeight()};
    for (size_t i = 0; i < draw_stack_size; i++) {
        draw_box.x += draw_stack[i].x;
        draw_box.y += draw_stack[i].y;
        draw_box.width = draw_stack[i].width;
        draw_box.height = draw_stack[i].height;
    }
    return draw_box;
}

void draw_box(Rectangle rect) {
    draw_stack[draw_stack_size++] = rect;
}

void drop_draw_box() {
    draw_stack_size--;
}

void clear_box(Color color) {
    DrawRectangleRec(get_draw_box(), color);
}

int draw_text_box_anchor_sized(char* text, int max_size, Vector2 pos, Color color, Color bg_color, Vector2 anchor) {
    Rectangle draw_box = get_draw_box();
    
    BeginScissorMode(draw_box.x, draw_box.y, draw_box.width, draw_box.height);

    int text_size = MeasureTextEx(font, text, font_size, font_spacing).x;
    if (max_size) BeginScissorMode(draw_box.x + pos.x - text_size*anchor.x, draw_box.y + pos.y - font_size*anchor.y, max_size, font_size);
    DrawTextEx(font, text, (Vector2) {draw_box.x + pos.x - text_size*anchor.x, draw_box.y + pos.y - font_size*anchor.y}, font_size, font_spacing, color);
    if (max_size && text_size > max_size) {
        EndScissorMode(); DrawRectangleGradientH(draw_box.x + pos.x - text_size*anchor.x + max_size - font_size, draw_box.y + pos.y - font_size*anchor.y, font_size, font_size, (Color) {bg_color.r, bg_color.g, bg_color.b, 0}, bg_color);
    }
    
    EndScissorMode();

    return text_size;
}

int measure_text(char* text) {
    return MeasureTextEx(font, text, font_size, font_spacing).x;
}

int draw_text_box_anchor(char* text, Vector2 pos, Color color, Vector2 anchor) {
    return draw_text_box_anchor_sized(text, 0, pos, color, theme.bg, anchor);
}

int draw_text_box(char* text, Vector2 pos, Color color) {
    return draw_text_box_anchor(text, pos, color, (Vector2) {0.5, 0.5});
}

bool draw_button_bg(Rectangle rect, Texture texture, Color color, Color bg_col, Color hov_col, bool active) {
    draw_box(rect);

    Rectangle draw_box = get_draw_box();

    bool is_hovered = GetMouseX() >= draw_box.x && GetMouseX() < draw_box.x + draw_box.width && GetMouseY() >= draw_box.y && GetMouseY() < draw_box.y + draw_box.height;
    if (is_hovered && active) cursor = MOUSE_CURSOR_POINTING_HAND;
    
    bool mouse_pressed = IsMouseButtonDown(0);
                      
    Color bg = is_hovered && active ? mouse_pressed ? color : hov_col : bg_col;
    Color fg = is_hovered && active && mouse_pressed ? bg_col : color;
    
    clear_box(bg);
    DrawTexture(texture, draw_box.x, draw_box.y, fg);

    drop_draw_box();

    return is_hovered && IsMouseButtonPressed(0);
}

bool draw_button(Rectangle rect, Texture texture, Color color) {
    return draw_button_bg(rect, texture, color, theme.mg_off, theme.mg_on, music_loaded);
}

void draw_rectangle_box(Rectangle rect, Color color) {
    Rectangle draw_box = get_draw_box();

    BeginScissorMode(draw_box.x, draw_box.y, draw_box.width, draw_box.height);
    DrawRectangle(rect.x + draw_box.x, rect.y + draw_box.y, rect.width, rect.height, color);
    EndScissorMode();
}

void draw_circle_box(Vector2 pos, float radius, Color color) {
    Rectangle draw_box = get_draw_box();

    BeginScissorMode(draw_box.x, draw_box.y, draw_box.width, draw_box.height);
    DrawCircle(pos.x + draw_box.x, pos.y + draw_box.y, radius, color);
    EndScissorMode();
}

void draw_volume_scrollbox() {
    Rectangle draw_box = get_draw_box();
    int margin = draw_box.height/2;
    char* text = (char*) TextFormat("vol: %.0f%%", GetMasterVolume()*100);
    int scrollbox_width = measure_text(text);

    bool hovered = GetMouseX() <= draw_box.x + draw_box.width - margin/2 && GetMouseX() >= draw_box.x + draw_box.width - margin/2 - scrollbox_width && GetMouseY() >= draw_box.y + draw_box.height/2 - font_size/2 && GetMouseY() <= draw_box.y + draw_box.height/2 + font_size/2;
    
    if (hovered) draw_rectangle_box((Rectangle) {draw_box.width - margin/2 - scrollbox_width - margin/4, draw_box.height/2 - font_size/2, scrollbox_width + margin/2, font_size}, theme.mg_on);
    if (hovered && GetMouseWheelMove() != 0.0f) {
        music_volume = clamp(music_volume + GetMouseWheelMove()*0.05f, 0.0f, 1.0f);
        SetMasterVolume(music_volume);
    }
    
    draw_text_box_anchor(text, (Vector2) {draw_box.width - margin/2, margin}, theme.fg, (Vector2) {1, 0.5});
    
}

void draw_menu_bar() {
    clear_box(theme.mg_off);

    Rectangle draw_box = get_draw_box();

    int margin = draw_box.height/2;

    draw_text_box_anchor("mus", (Vector2) {margin/2, margin}, theme.fg, (Vector2) {0, 0.5});
    draw_volume_scrollbox();
}

void draw_status_bar() {
    clear_box(theme.mg_off);

    Rectangle draw_box = get_draw_box();
    int margin_x = font_size/2;
    int margin_y = margin_x;

    if (!music_loaded) {
        draw_text_box_anchor_sized("drag-n-drop some music!", draw_box.x + draw_box.width - margin_x*2, (Vector2) {margin_x, margin_y}, theme.fg, theme.mg_off, (Vector2) {0, 0});
    } else if (tag == NULL || *(music_get_artist()) == 0) {
        draw_text_box_anchor_sized(playlist[playlist_position], draw_box.x + draw_box.width - margin_x*2, (Vector2) {margin_x, margin_y}, theme.fg, theme.mg_off, (Vector2) {0, 0});
    } else {
        int artist_size = draw_text_box_anchor_sized((char*) TextFormat("%s - ", music_get_artist()), draw_box.x + draw_box.width - margin_x*2, (Vector2) {margin_x, margin_y}, theme.fg_off, theme.mg_off, (Vector2) {0, 0});
        int title_size = draw_text_box_anchor_sized(music_get_name(), draw_box.x + draw_box.width - margin_x*2 - artist_size, (Vector2) {margin_x + artist_size, margin_y}, theme.fg, theme.mg_off, (Vector2) {0, 0});
        if (draw_box.width - margin_x - measure_text(music_get_album()) > margin_x*2 + artist_size + title_size) draw_text_box_anchor(music_get_album(), (Vector2) {draw_box.width - margin_x, margin_y}, theme.fg, (Vector2) {1, 0});
    }

    margin_y = draw_box.height - font_size/2;

    float music_current = music_get_current_time();
    float music_full = music_get_full_time();

    float play_bar_pos = 0.0f;
    if (music_loaded) play_bar_pos = music_current/(float) music_full;
    if (seeking_music) play_bar_pos = music_seek_temp/(float) music_full;
    
    int current_time_text_size = draw_text_box_anchor((char*) TextFormat("%d:%02d", (int) music_current/60, (int) music_current%60), (Vector2) {margin_x, margin_y}, theme.fg, (Vector2) {0, 1});
    int full_time_text_size = draw_text_box_anchor((char*) TextFormat("%d:%02d", (int) music_full/60, (int) music_full%60), (Vector2) {margin_x + current_time_text_size + margin_x*2, margin_y}, theme.fg, (Vector2) {0, 1});
    draw_text_box_anchor("/", (Vector2) {margin_x + current_time_text_size + margin_x, margin_y}, theme.fg_off, (Vector2) {0.5, 1});
    int time_text_size = current_time_text_size + full_time_text_size + margin_x*2;

    if (draw_button((Rectangle) {draw_box.width - margin_x - font_size*4.f, margin_y - font_size, font_size, font_size}, back, music_loaded ? theme.fg : theme.fg_off) && music_loaded)
        music_playlist_previous();
    if (draw_button((Rectangle) {draw_box.width - margin_x - font_size*3.f, margin_y - font_size, font_size, font_size}, music_playing ? pause : play, music_loaded ? theme.fg : theme.fg_off) && music_loaded)
        music_play_pause();
    if (draw_button((Rectangle) {draw_box.width - margin_x - font_size*2.f, margin_y - font_size, font_size, font_size}, forward, music_loaded ? theme.fg : theme.fg_off) && music_loaded)
        music_playlist_next();
    if (draw_button((Rectangle) {draw_box.width - margin_x - font_size*1.f, margin_y - font_size, font_size, font_size}, music_repeat == 2 ? repeat_one : repeat, music_loaded && music_repeat ? theme.fg : theme.fg_off) && music_loaded)
        music_toggle_repeat();

    int bar_height = font_size/12;
    float bar_start = margin_x*2 + time_text_size;
    float bar_width = GetScreenWidth() - margin_x*3 - time_text_size - font_size*4.5f;
    float bar_y = margin_y - font_size/2;

    bool bar_hovered = GetMouseX() >= draw_box.x + bar_start && GetMouseX() <= draw_box.x + bar_start + bar_width && GetMouseY() >= draw_box.y + bar_y - font_size/4 && GetMouseY() <= draw_box.y + bar_y + font_size/4;

    if (bar_hovered && music_loaded) {
        cursor = MOUSE_CURSOR_POINTING_HAND;
    }
    if (bar_hovered && IsMouseButtonPressed(0) && music_loaded) {
        seeking_music = true;
    }

    if (seeking_music && IsMouseButtonUp(0)) {
        music_seek(music_seek_temp);
        seeking_music = false;
    }

    if (seeking_music)
        music_seek_temp = (clamp((GetMouseX() - bar_start)/bar_width, 0.001f, 0.999f)*music_full);

    draw_rectangle_box((Rectangle) {bar_start, bar_y - bar_height/2, bar_width, bar_height}, theme.mg_on);
    if (music_loaded) draw_rectangle_box((Rectangle) {bar_start, bar_y - bar_height/2, bar_width*play_bar_pos, bar_height}, theme.fg);
    if (music_loaded) draw_circle_box((Vector2) {bar_start + bar_width*play_bar_pos, bar_y}, bar_hovered ? font_size/4 : font_size/6, theme.fg);

    if (music_loaded && seeking_music) {
        float second_seeked = clamp((GetMouseX() - bar_start)/bar_width, 0.f, 1.f)*music_full;
        char* text = (char*) TextFormat("%d:%02d", (int) second_seeked/60, (int) second_seeked%60);
        int w = measure_text(text);
        draw_rectangle_box((Rectangle) {clamp((GetMouseX() - bar_start), 0.f, bar_width)+bar_start - w/2 - font_size/4, bar_y - font_size*1.5, w + font_size/2, font_size}, theme.bg);
        draw_text_box_anchor(text, (Vector2) {clamp((GetMouseX() - bar_start), 0.f, bar_width)+bar_start, bar_y - font_size/2}, theme.fg, (Vector2) {0.5, 1});
    }
}

void draw_main_ui() {
    Rectangle draw_box = get_draw_box();
    if (da_length(playlist) == 0) draw_text_box("whoa, quite empty here...", (Vector2) {draw_box.width/2, draw_box.height/2}, theme.mg_on);
    else {
        for (int i = 0; i < (int) da_length(playlist); i++) {
            bool hovered = GetMouseX() >= draw_box.x + font_size/2 && GetMouseX() < draw_box.x + draw_box.width - font_size*1.5f && GetMouseY() >= draw_box.y + font_size/2 + i*font_size && GetMouseY() < draw_box.y + font_size/2 + (i+1)*font_size;
            Color dark_color = i == playlist_position || hovered ? theme.fg_off : theme.mg_on;
            if (hovered) {
                draw_rectangle_box((Rectangle) {font_size/2, font_size/2 + i*font_size, draw_box.width - font_size*1.5f, font_size}, theme.mg_off);
                cursor = MOUSE_CURSOR_POINTING_HAND;
            }
            int w1 = draw_text_box_anchor_sized((char*) TextFormat("%zu. ", i+1), draw_box.width - draw_box.x - font_size*2.f, (Vector2) {font_size/2, font_size/2 + i*font_size}, dark_color, theme.bg, (Vector2) {0, 0});
            if (*(music_get_artist_playlist(i)) == 0) {
                draw_text_box_anchor_sized(playlist[i], draw_box.width - draw_box.x - font_size*2.f - w1, (Vector2) {font_size/2 + w1, font_size/2 + i*font_size}, theme.fg, theme.bg, (Vector2) {0, 0});
            } else {
                int w2 = draw_text_box_anchor_sized((char*) TextFormat("%s - ", music_get_artist_playlist(i)), draw_box.width - draw_box.x - font_size*2.f - w1, (Vector2) {font_size/2 + w1, font_size/2 + i*font_size}, theme.fg, theme.bg, (Vector2) {0, 0});
                int w3 = draw_text_box_anchor_sized((char*) TextFormat("%s ", music_get_name_playlist(i)), draw_box.width - draw_box.x - font_size*2.f - w1 - w2, (Vector2) {font_size/2 + w1 + w2, font_size/2 + i*font_size}, theme.fg, theme.bg, (Vector2) {0, 0});
                draw_text_box_anchor_sized((char*) TextFormat("(%s)", music_get_album_playlist(i)), draw_box.width - draw_box.x - font_size*2.f - w1 - w2 - w3, (Vector2) {font_size/2 + w1 + w2 + w3, font_size/2 + i*font_size}, dark_color, theme.bg, (Vector2) {0, 0});
            }
            if (hovered && IsMouseButtonPressed(0)) {
                playlist_position = i;
                if (music_loaded) music_unload();
                music_load(playlist[playlist_position]);
            }
            if (draw_button_bg((Rectangle) {draw_box.width - draw_box.x - font_size*1.5f, font_size/2 + font_size*i, font_size, font_size}, tdelete, theme.fg, theme.bg, theme.mg_off, true)) {
                music_remove_from_playlist(i);
            }
        }
    }
}

