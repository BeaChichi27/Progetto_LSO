FROM ubuntu:24.04

RUN apt-get update && \
    apt-get install -y \
    build-essential \
    gcc \
    make

WORKDIR /app
COPY . .

RUN gcc -Wall -Wextra -std=c99 -g src/*.c -o client -Isrc/headers

CMD ["./client"]