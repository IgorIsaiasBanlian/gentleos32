FROM --platform=linux/amd64 alpine:3.20
RUN apk add --no-cache bash gcc nasm binutils make perl mtools gzip
WORKDIR /src
