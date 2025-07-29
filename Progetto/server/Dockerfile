FROM ubuntu:24.04

RUN apt-get update && \
    apt-get install -y mingw-w64 make

WORKDIR /app

COPY src/ ./src/
COPY Makefile ./

RUN make CC=x86_64-w64-mingw32-gcc TARGET=tris_server.exe LDFLAGS="-lws2_32"

CMD ["wine", "./tris_server.exe"]