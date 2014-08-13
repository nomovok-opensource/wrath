FURYSDLKeyCode.values.tcc is machine generated with:

grep SDLK /usr/include/SDL2/SDL_keycode.h | sed 's/=/\n/g' | grep -v SDL_SCANCODE | grep SDLK | sed 's/SDLK_\(.*\)/FURYKey_\1 = SDLK_\1,/' | sed 's/=/=\n/g' | sed 's/FURYKey_\(.\)\(.*\)/\nFURYKey_\U\1\L\2/' > FURYSDLKeyCode.values.tcc



and then a little editing to remove extra ickyies.
