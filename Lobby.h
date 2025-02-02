#ifndef LOBBY_H
#define LOBBY_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include "common.h"

class Lobby {
public:
    Lobby(SDL_Renderer* renderer);
    ~Lobby();

    void render();
    SDL_Keycode handleInput(); // Handle lobby-specific events

    void updatePlayerList(const std::vector<PlayerState>& players);

private:
    SDL_Renderer* renderer;
    TTF_Font* font;

    std::vector<PlayerState> playersInLobby;
    bool isAdmin;
    bool isHovering;
    bool isClicked;
    SDL_Rect playButtonRect;
};

#endif
