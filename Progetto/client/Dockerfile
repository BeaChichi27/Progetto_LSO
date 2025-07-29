FROM ubuntu:24.04

RUN apt-get update && \
    apt-get install -y build-essential

WORKDIR /app

COPY src/ ./src/
COPY Makefile ./

RUN make

CMD ["./src/client"]