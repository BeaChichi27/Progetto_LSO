FROM ubuntu:24.04

RUN apt-get update && \
    apt-get install -y \
    build-essential \
    gcc \
    make

WORKDIR /app

COPY src/ ./src/
COPY Makefile .

RUN make

EXPOSE 8080

CMD ["./server"]