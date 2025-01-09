
void config_write_string(FILE* f, char* str) {
    uint32_t strl = strlen(str);
    fwrite(&strl, sizeof(strl), 1, f);
    fwrite(str, sizeof(char), strl, f);
}

void config_write_image(FILE* f, Image img) {
    int bytesize = 0;
    unsigned char* file = ExportImageToMemory(img, ".png", &bytesize);
    uint32_t size = bytesize;
    fwrite(&size, sizeof(size), 1, f);
    fwrite(file, sizeof(unsigned char), size, f);
}

char* config_read_string(FILE* f) {
    uint32_t strl = 0;
    fread(&strl, sizeof(strl), 1, f);
    char* str = malloc(strl+1);
    fread(str, sizeof(char), strl, f);
    str[strl] = 0;
    return str;
}

Image config_read_image(FILE* f) {
    uint32_t size = 0;
    fread(&size, sizeof(size), 1, f);
    unsigned char str[size];
    fread(str, sizeof(unsigned char), size, f);
    return LoadImageFromMemory(".png", str, size);
}

void config_save_albums(FILE* f) {
    uint32_t album_count = da_length(albums)-1;
    fwrite(&album_count, sizeof(album_count), 1, f);
    for (size_t i = 1; i < da_length(albums); i++) {
        Album album = albums[i];
        config_write_string(f, album.name);
        config_write_string(f, album.artists);
        config_write_string(f, album.genres);
        config_write_image(f, album.cover_image);
        uint32_t album_size = da_length(album.playlist);
        fwrite(&album_size, sizeof(album_size), 1, f);
        for (size_t i = 0; i < da_length(album.playlist); i++) config_write_string(f, album.playlist[i]);
    }
}

void config_save_playlist(FILE* f) {
    uint32_t size = da_length(playlist);
    fwrite(&size, sizeof(size), 1, f);
    for (size_t i = 0; i < da_length(playlist); i++) config_write_string(f, playlist[i]);
}

void config_save() {
    FILE* f = fopen(".mus-savestate", "wb");
    config_save_albums(f);
    config_save_playlist(f);
    fclose(f);
}

void config_load_albums(FILE* f) {
    uint32_t album_count = 0;
    fread(&album_count, sizeof(uint32_t), 1, f);
    for (uint32_t i = 0; i < album_count; i++) {
        Album album = {0};
        album.name = config_read_string(f);
        album.artists = config_read_string(f);
        album.genres = config_read_string(f);
        album.cover_image = config_read_image(f);
        Image img = ImageCopy(album.cover_image);
        ImageResize(&img, font_size*6.f, font_size*6.f);
        album.cover = LoadTextureFromImage(img);
        UnloadImage(img);
        uint32_t album_size = 0;
        fread(&album_size, sizeof(album_size), 1, f);
        album.playlist = da_new(char*);
        for (size_t i = 0; i < album_size; i++) {
            char* str = config_read_string(f);
            da_push(album.playlist, str);
        }
        da_push(albums, album);
    }
}

void config_load_playlist(FILE* f) {
    uint32_t size = 0;
    fread(&size, sizeof(size), 1, f);
    for (size_t i = 0; i < size; i++) music_add_to_playlist(config_read_string(f));
    music_unload();
    playlist_position = -1;
}

void config_load() {
    FILE* f = fopen(".mus-savestate", "rb");
    config_load_albums(f);
    config_load_playlist(f);
    fclose(f);
}
