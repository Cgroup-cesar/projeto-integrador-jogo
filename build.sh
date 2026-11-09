#!/bin/sh
# Atalho para Linux e macOS: só chama o make. Ex.: ./build.sh run
cd "$(dirname "$0")" && make "$@"
