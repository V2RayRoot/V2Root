FROM alpine:latest

ARG VERSION=1.2.0

WORKDIR /build

RUN apk add \
    gcc \
    musl-dev \
    jansson-dev \
    curl-dev \
    openssl-dev \
    zlib-dev

COPY v2root/lib/src /build/src

RUN gcc -O2 -Wall -fPIC \
    -c src/libv2root_vless.c \
    -c src/libv2root_vmess.c \
    -c src/libv2root_shadowsocks.c \
    -c src/libv2root_manage.c \
    -c src/libv2root_core.c \
    -c src/libv2root_utils.c \
    -c src/libv2root_service.c \
    -c src/libv2root_linux.c && \
    gcc -shared -fPIC -o libv2root-static-${VERSION}.so \
    libv2root_vless.o \
    libv2root_vmess.o \
    libv2root_shadowsocks.o \
    libv2root_manage.o \
    libv2root_core.o \
    libv2root_utils.o \
    libv2root_service.o \
    libv2root_linux.o \
    -ljansson -lcurl -lssl -lcrypto -lz && \
    mkdir -p /output && \
    cp libv2root-static-${VERSION}.so /output/ && \
    mkdir -p /dist && \
    cp libv2root-static-${VERSION}.so /dist/

RUN echo '#!/bin/sh' > /entrypoint.sh && \
    echo 'mkdir -p /host-dist' >> /entrypoint.sh && \
    echo 'cp /dist/* /host-dist/' >> /entrypoint.sh && \
    echo 'chown -R '"$(id -u):$(id -g)"' /host-dist' >> /entrypoint.sh && \
    echo 'echo "Build complete! Library saved to ./dist/"' >> /entrypoint.sh && \
    chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
