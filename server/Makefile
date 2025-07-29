CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
LDFLAGS = -lws2_32
SRCDIR = src
HEADERDIR = $(SRCDIR)/headers
TARGET = server.exe

all: $(TARGET)

$(TARGET):
    $(CC) $(CFLAGS) $(SRCDIR)/*.c -o $@ -I$(HEADERDIR) $(LDFLAGS)

clean:
    del $(TARGET) 2>nul || true

.PHONY: all clean