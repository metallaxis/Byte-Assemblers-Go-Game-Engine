FROM gcc:latest AS builder
FROM alpine:3.21

WORKDIR /engine

COPY . .

RUN make

CMD ["bash"]
