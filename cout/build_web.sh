#!/bin/bash

set -ex

rm -f build/{*.wasm,*.js,*.html}

mkdir -p build/

emcc -o build/cout.js \
    cout.c \
    -Os -Wall \
    -lm \
    -I/usr/include/SDL2 -D_REENTRANT -lSDL2 -lSDL2_ttf \
    -sUSE_GLFW=3 -sUSE_SDL=2 -sUSE_SDL_TTF=2 -sASYNCIFY -sMODULARIZE=1 -sEXPORT_ES6=1 -sEXPORT_NAME=createCout -sEXPORTED_FUNCTIONS=_runGame,_sdlSendLeftMouseButtonPressed,_sendShouldStop \
    --embed-file ./Lato-Regular.ttf

cp build/cout.js build/cout.wasm .

python3 -m http.server 3000
