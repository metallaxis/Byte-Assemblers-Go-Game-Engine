FROM gcc:latest AS builder

WORKDIR /engine

COPY . .

RUN make

FROM alpine:3.21

CMD ["bash"]
