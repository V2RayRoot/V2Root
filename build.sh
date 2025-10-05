#!/bin/bash

mkdir -p dist

VERSION=${1:-1.2.0}
docker build --build-arg VERSION=$VERSION -t v2root-build .

docker run --rm -v "$(pwd)/dist:/host-dist" v2root-build

echo "Build completed! Library is in ./dist/"
