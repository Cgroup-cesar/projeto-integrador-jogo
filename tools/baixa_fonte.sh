#!/bin/sh
# Baixa a fonte VT323 (licença SIL OFL 1.1, de Peter Hull) para assets/fontes/.
# É opcional: sem ela, o jogo usa a fonte padrão da raylib e translitera os acentos (á→a).
set -e
cd "$(dirname "$0")/.."
DESTINO=assets/fontes
BASE=https://github.com/google/fonts/raw/main/ofl/vt323
mkdir -p "$DESTINO"
if command -v curl >/dev/null 2>&1; then
    curl -fL -o "$DESTINO/VT323-Regular.ttf" "$BASE/VT323-Regular.ttf"
    curl -fL -o "$DESTINO/OFL-VT323.txt" "$BASE/OFL.txt"
elif command -v wget >/dev/null 2>&1; then
    wget -O "$DESTINO/VT323-Regular.ttf" "$BASE/VT323-Regular.ttf"
    wget -O "$DESTINO/OFL-VT323.txt" "$BASE/OFL.txt"
else
    echo "Instale o curl ou o wget, ou baixe a fonte à mão de $BASE" >&2
    exit 1
fi
echo "Fonte salva em $DESTINO/VT323-Regular.ttf (licença em $DESTINO/OFL-VT323.txt)"
