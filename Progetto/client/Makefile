CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -I$(HEADERDIR)
LDFLAGS = -lws2_32
SRCDIR = src
HEADERDIR = $(SRCDIR)/headers
TARGET = client.exe

SOURCES = $(SRCDIR)/main.c $(SRCDIR)/network.c
HEADERS = $(HEADERDIR)/network.h
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
    $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(SRCDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
    $(CC) $(CFLAGS) -c $< -o $@

clean:
    del /Q $(SRCDIR)\*.o $(TARGET) 2>nul || true

rebuild: clean all

run: $(TARGET)
    .\$(TARGET)

.PHONY: all clean rebuild run