FURYSDLKeyCode.values.tcc is machine generated with:


grep SDLK_ /usr/include/SDL/SDL_keysym.h | sed 's/SDLK_\(.\)\([A-Z]*\)/FURYKey_\U\1\L\2/' > ../inc/FURY/SDL/FURYSDLKeyCode.values.tcc
