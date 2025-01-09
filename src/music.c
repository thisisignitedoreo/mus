
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

typedef struct {
    char* name;
    char* artists;
    char* genres;
    Texture cover;
    Image cover_image;
    int year;
    char** playlist;
} Album;

Album* albums;

char* utf162utf8(char* utf16str) {
    uc_utf16_to_utf8_buffered(utf16str, utf8str, 1024, 0, UC_BYTE_ORDER_BOM, false);
    return utf8str;
}

void pop_album() {
    int index = da_length(albums)-1;
    for (size_t i = 0; i < da_length(albums[index].playlist); i++) free(albums[index].playlist[i]);
    da_free(albums[index].playlist);
    free(albums[index].name);
    free(albums[index].genres);
    free(albums[index].artists);
    UnloadTexture(albums[index].cover);
    UnloadImage(albums[index].cover_image);
    da_pop(albums, NULL);
}

char* music_string_from_textframe(ID3v2_TextFrame* data) {
    if (data == NULL) return "";
    char* str;
    if (data->data->encoding == 1) str = utf162utf8(data->data->text);    else str = data->data->text;
    return str;
}

char* music_get_artist_from_path(char* path) {
    ID3v2_Tag* tag = ID3v2_read_tag(path);
    if (tag == NULL) return "";
    char* str = music_string_from_textframe(ID3v2_Tag_get_artist_frame(tag));
    ID3v2_Tag_free(tag);
    return str;
}

char* music_get_album_artists_from_path(char* path) {
    ID3v2_Tag* tag = ID3v2_read_tag(path);
    if (tag == NULL) return "";
    char* str = music_string_from_textframe(ID3v2_Tag_get_album_artist_frame(tag));
    ID3v2_Tag_free(tag);
    if (*str == 0) return music_get_artist_from_path(path);
    return str;
}

char* music_get_title_from_path(char* path) {
    ID3v2_Tag* tag = ID3v2_read_tag(path);
    if (tag == NULL) return "";
    char* str = music_string_from_textframe(ID3v2_Tag_get_title_frame(tag));
    ID3v2_Tag_free(tag);
    return str;
}

int music_get_no_from_path(char* path) {
    ID3v2_Tag* tag = ID3v2_read_tag(path);
    if (tag == NULL) return 0;
    char* str = music_string_from_textframe(ID3v2_Tag_get_track_frame(tag));
    ID3v2_Tag_free(tag);
    while (*str == '0') str++;
    return atoi(str);
}

char* music_get_album_name_from_path(char* path) {
    ID3v2_Tag* tag = ID3v2_read_tag(path);
    if (tag == NULL) return "";
    char* str = music_string_from_textframe(ID3v2_Tag_get_album_frame(tag));
    ID3v2_Tag_free(tag);
    return str;
}

char* music_get_genres_from_path(char* path) {
    ID3v2_Tag* tag = ID3v2_read_tag(path);
    if (tag == NULL) return "";
    char* str = music_string_from_textframe(ID3v2_Tag_get_genre_frame(tag));
    ID3v2_Tag_free(tag);
    return str;
}

int music_get_year_from_path(char* path) {
    ID3v2_Tag* tag = ID3v2_read_tag(path);
    if (tag == NULL) return 0;
    char* str = music_string_from_textframe(ID3v2_Tag_get_year_frame(tag));
    ID3v2_Tag_free(tag);
    int num = atoi(str);
    return num;
}

Image music_get_cover_from_path(char* path) {
    ID3v2_Tag* tag = ID3v2_read_tag(path);
    if (tag == NULL) return GenImageColor(64, 64, theme.mg_off);
    ID3v2_ApicFrame* data = ID3v2_Tag_get_album_cover_frame(tag);
    if (data == NULL) return GenImageColor(64, 64, theme.mg_off);
    if (strcmp(data->data->mime_type, ID3v2_MIME_TYPE_JPG) == 0) {
        return LoadImageFromMemory(".jpg", (const unsigned char*) data->data->data, data->data->picture_size);
    } return LoadImageFromMemory(".png", (const unsigned char*) data->data->data, data->data->picture_size);
}

void album_new(char* name, char* path) {
    char* mname    = malloc(strlen(name)    + 1); memcpy(mname,    name,    strlen(name)    + 1);
    char* artists  = music_get_album_artists_from_path(path);
    char* martists = malloc(strlen(artists) + 1); memcpy(martists, artists, strlen(artists) + 1);
    char* genres   = music_get_genres_from_path(path);
    char* mgenres  = malloc(strlen(name)    + 1); memcpy(mgenres,  genres,  strlen(genres)  + 1);
    Image cover = music_get_cover_from_path(path);
    //Image cover = GenImageColor(64, 64, theme.fg);
    Image cover_copy = ImageCopy(cover);
    ImageResize(&cover, font_size*6.f, font_size*6.f);
    Album a = {.name = mname, .artists = martists, .genres = mgenres, .cover_image = cover_copy, .cover = LoadTextureFromImage(cover), .year = music_get_year_from_path(path), .playlist = da_new(char*)};
    UnloadImage(cover);
    da_push(albums, a);
}

void album_add_song(char* path) {
    char* name = music_get_album_name_from_path(path);
    if (*name == 0) { da_push(albums[0].playlist, path); return; }
    size_t index = 0;
    bool found = false;
    for (size_t i = 0; i < da_length(albums); i++) {
        if (strcmp(albums[i].name, name) == 0) { found = true; index = i; break; }
    }
    char* mpath = malloc(strlen(path)+1); memcpy(mpath, path, strlen(path)+1);
    if (!found) {
        album_new(name, path);
        index = da_length(albums)-1;
    }
    da_push(albums[index].playlist, mpath);
}

void music_scan(char* path) {
    char* epath = get_path(path);
    FilePathList list = LoadDirectoryFilesEx(epath, ".mp3", true);
    for (size_t i = 0; i < list.count; i++) {        
        char* mpath = get_path(norm_text(list.paths[i]));
        TraceLog(LOG_INFO, "Scanning %s", mpath);
        album_add_song(mpath);
    }
    UnloadDirectoryFiles(list);
}

void music_load(char* filename) {
    music = LoadMusicStream(filename);
    PlayMusicStream(music);
    music.looping = music_repeat == 2;
    tag = ID3v2_read_tag(filename);
    music_loaded = true;
    music_playing = true;
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
    if (playlist_position > (int) indice) playlist_position--;
    else if ((int) indice == playlist_position) {
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
