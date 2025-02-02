#ifndef MENU_H
#define MENU_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <SDL2/SDL_image.h>
#include "common.h"


class Menu {
public:
    Menu(SDL_Renderer* renderer);
    ~Menu();

    void render();
    SDL_Keycode handleInput(); // Returns true if the player pressed Enter to start

private:
    SDL_Renderer* renderer;
    TTF_Font* font;
    SDL_Texture* backgroundTexture;
    SDL_Rect playButtonRect;

    bool isHovering = false;  // ✅ Track hover state
    bool isClicked = false;   // ✅ Track click state
};

#endif
