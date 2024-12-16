
ID3v2_Tag* tag;
Music music;
bool music_playing = true;
int music_repeat = 0;
// 0 - no repeat
// 1 - repeat
// 2 - repeat one
bool music_loaded = false;
float music_volume = 1.0f;

char** playlist = 0;
int playlist_position = -1;

char utf8str[1024] = {0};

char* utf162utf8(char* utf16str) {
    uc_utf16_to_utf8_buffered(utf16str, utf8str, 1024, 0, UC_BYTE_ORDER_BOM, false);
    return utf8str;
}

void music_load(char* filename) {
    music = LoadMusicStream(filename);
    PlayMusicStream(music);
    music.looping = music_repeat == 2;
    tag = ID3v2_read_tag(filename);
    music_loaded = true;
}

void music_unload() {
    UnloadMusicStream(music);
    music_loaded = false;
    ID3v2_Tag_free(tag);
    tag = ID3v2_Tag_new_empty();
}

void music_add_to_playlist(char* path) {
    char* realloced_path = malloc(strlen(path) + 1);
    memcpy(realloced_path, path, strlen(path) + 1);
    da_push(playlist, realloced_path);
    if (!music_loaded) {
        playlist_position = da_length(playlist) - 1;
        music_load(path);
    }
}

void music_remove_from_playlist(size_t indice) {
    if ((int) indice == playlist_position) {
        music_unload();
        playlist_position = -1;
    }
    free(playlist[indice]);
    memcpy(playlist + indice, playlist + indice + 1, da_stride(playlist) * (da_length(playlist) - indice));
    da_pop(playlist, 0);
}

bool music_ismusic(char* path) {
    FILE* f = fopen(path, "rb");
    if (f == NULL) return false;
    char magic[3] = {0};
    if (fread(magic, 1, 3, f) != 3) {
        fclose(f); return false;
    }
    fclose(f);
    return memcmp(magic, "\xFF\xFB", 2) == 0 || memcmp(magic, "\xFF\xF3", 2) == 0 || memcmp(magic, "\xFF\xF2", 2) == 0 || memcmp(magic, "ID3", 3) == 0; // only mp3s are supported
}

char* music_get_name() {
    if (!music_loaded || tag == NULL) return "";
    ID3v2_TextFrame* data = ID3v2_Tag_get_title_frame(tag);
    if (data == NULL) return "";
    char* str = utf162utf8(data->data->text);
    return str;
}

char* music_get_artist() {
    if (!music_loaded || tag == NULL) return "";
    ID3v2_TextFrame* data = ID3v2_Tag_get_artist_frame(tag);
    if (data == NULL) return "";
    char* str = utf162utf8(data->data->text);
    return str;
}

char* music_get_album() {
    if (!music_loaded || tag == NULL) return "";
    ID3v2_TextFrame* data = ID3v2_Tag_get_album_frame(tag);
    if (data == NULL) return "";
    char* str = utf162utf8(data->data->text);
    return str;
}

char* music_get_name_playlist(size_t indice) {
    ID3v2_Tag* tag = ID3v2_read_tag(playlist[indice]);
    if (tag == NULL) return "";
    ID3v2_TextFrame* data = ID3v2_Tag_get_title_frame(tag);
    if (data == NULL) return "";
    char* str = utf162utf8(data->data->text);
    ID3v2_Tag_free(tag);
    return str;
}

char* music_get_artist_playlist(size_t indice) {
    ID3v2_Tag* tag = ID3v2_read_tag(playlist[indice]);
    if (tag == NULL) return "";
    ID3v2_TextFrame* data = ID3v2_Tag_get_artist_frame(tag);
    if (data == NULL) return "";
    char* str = utf162utf8(data->data->text);
    ID3v2_Tag_free(tag);
    return str;
}

char* music_get_album_playlist(size_t indice) {
    ID3v2_Tag* tag = ID3v2_read_tag(playlist[indice]);
    if (tag == NULL) return "";
    ID3v2_TextFrame* data = ID3v2_Tag_get_album_frame(tag);
    if (data == NULL) return "";    
    char* str = utf162utf8(data->data->text);
    ID3v2_Tag_free(tag);
    return str;
}

void music_play_pause() {
    if (!music_loaded) return;
    if (music_playing) PauseMusicStream(music);
    else ResumeMusicStream(music);
    music_playing = !music_playing;
}

void music_toggle_repeat() {
    if (!music_loaded) return;
    music_repeat = (music_repeat + 1) % 3;
    music.looping = music_repeat == 2;
}

float music_get_current_time() {
    if (!music_loaded) return 0.0f;
    return GetMusicTimePlayed(music);
}

void music_seek(float time) {
    if (!music_loaded) return;
    SeekMusicStream(music, time);
}

float music_get_full_time() {
    if (!music_loaded) return 0.0f;
    return GetMusicTimeLength(music);
}

void music_playlist_next() {
    if (music_loaded) music_unload();
    if (playlist_position == (int) da_length(playlist)-1) {
        playlist_position = 0;
        if (music_repeat == 1) {
            music_load(playlist[playlist_position]);
        } else playlist_position = -1;
    } else {
        playlist_position++;
        music_load(playlist[playlist_position]);
    }
}

void music_playlist_previous() {
    if (playlist_position > 0) {
        if (music_loaded) music_unload();
        playlist_position--;
        music_load(playlist[playlist_position]);
    }
}

void music_update() {
    if (music_loaded && music_playing && !IsMusicStreamPlaying(music)) {
        music_playlist_next();
    }
}
