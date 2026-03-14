FROM alpine

RUN apk add gcc make git linux-headers musl-dev

WORKDIR /engine

COPY . .

RUN make

CMD ["bash"]
