
TARGET=mus
SRC=src/main.c

FLAGS=-Wall -Wextra -std=gnu99 -I./raylib/include -L./raylib/lib -I./id3v2lib/include -L./id3v2lib/lib -ggdb
LIBS=-lraylib -lopengl32 -lgdi32 -lwinmm -lid3v2

ifdef _NO_CONSOLE
FLAGS += -mwindows
endif

$(TARGET): $(SRC)
	gcc $(FLAGS) -o $(TARGET) $(SRC) $(LIBS)
