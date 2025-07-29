CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
LDFLAGS = -lws2_32
SRCDIR = src
HEADERDIR = $(SRCDIR)/headers
TARGET = tris_server.exe

SOURCES = $(SRCDIR)/main.c $(SRCDIR)/network.c $(SRCDIR)/lobby.c $(SRCDIR)/game_manager.c
HEADERS = $(HEADERDIR)/network.h $(HEADERDIR)/lobby.h $(HEADERDIR)/game_manager.h
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
    $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(SRCDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
    $(CC) $(CFLAGS) -I$(HEADERDIR) -c $< -o $@

clean:
    del /Q $(SRCDIR)\*.o $(TARGET) 2>nul || true

rebuild: clean all

run: $(TARGET)