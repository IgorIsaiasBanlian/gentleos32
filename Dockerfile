FROM --platform=linux/amd64 alpine:3.20
RUN apk add --no-cache bash clang nasm binutils make perl mtools gzip
WORKDIR /src
