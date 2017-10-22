emcc asteroids.c -o2 -s WASM=1 --preload-files assets -s USE_SDL=2 -s USE_ZLIB=1 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS="['png']" -s ALLOW_MEMORY_GROWTH=1 --no-heap-copy -o asteroids.html
