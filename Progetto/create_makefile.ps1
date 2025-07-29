function ConvertTo-MakefileFormat {
    param([string]$content)
    $content -replace '    ', "`t"
}

$clientMakefile = @'
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -D_WIN32_WINNT=0x0600
LDFLAGS = -lws2_32 -lconio
SRCDIR = src
HEADERDIR = $(SRCDIR)/headers
TARGET = client.exe

all: $(TARGET)

$(TARGET):
    $(CC) $(CFLAGS) $(SRCDIR)/*.c -o $@ -I$(HEADERDIR) $(LDFLAGS)

clean:
    del $(TARGET) 2>nul || true

.PHONY: all clean
'@

$serverMakefile = @'
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
'@

New-Item -ItemType Directory -Force -Path "client" | Out-Null
New-Item -ItemType Directory -Force -Path "server" | Out-Null

ConvertTo-MakefileFormat $clientMakefile | Set-Content -Path "client\Makefile" -NoNewline
ConvertTo-MakefileFormat $serverMakefile | Set-Content -Path "server\Makefile" -NoNewline

Write-Host "Makefile creati con successo!"