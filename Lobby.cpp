#include "Lobby.h"
#include <iostream>
#include <SDL2/SDL2_gfxPrimitives.h>

Lobby::Lobby(SDL_Renderer* renderer) : renderer(renderer), isAdmin(false), isHovering(false), isClicked(false) {
    font = TTF_OpenFont("arial.ttf", 24);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
    }

    // Play button size and position
    playButtonRect.w = 200;
    playButtonRect.h = 50;
    playButtonRect.x = (SCREEN_WIDTH - playButtonRect.w) / 2;
    playButtonRect.y = (SCREEN_HEIGHT - playButtonRect.h) / 2;
}

Lobby::~Lobby() {
    TTF_CloseFont(font);
}

void Lobby::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // Black background
    SDL_RenderClear(renderer);
    
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, NULL);  // Fills screen with gray

    // Render lobby text
    SDL_Color textColor = {255, 255, 255, 255};
    std::string lobbyText = "Lobby - Waiting for Players... ";
    if (isAdmin) {
        lobbyText += "You are the admin. Press 'S' to start the game.\n";
    }
    lobbyText += "Players in Lobby: " + std::to_string(playersInLobby.size()) + "/4. Press Enter to Start Match.";

    SDL_Surface* surfaceMessage = TTF_RenderText_Solid(font, lobbyText.c_str(), textColor);
    SDL_Texture* message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

    SDL_Rect messageRect = {50, 100, surfaceMessage->w, surfaceMessage->h};
    SDL_RenderCopy(renderer, message, NULL, &messageRect);
    if (SDL_GetError()[0] != '\0') {
        std::cerr << "SDL Error after rendering text: " << SDL_GetError() << std::endl;
        SDL_ClearError();
    }

    SDL_FreeSurface(surfaceMessage);
    SDL_DestroyTexture(message);

    // ✅ Ensure only one `SDL_RenderPresent()` call at the end
    SDL_RenderPresent(renderer);
}


SDL_Keycode Lobby::handleInput() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == END_GAME_INPUT) {
            std::cout << "exiting" << std::endl;
            return END_GAME_INPUT; // Exit game
        }
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == START_GAME_INPUT) {
            std::cout << "starting game" << std::endl;
            return START_GAME_INPUT; // Exit game
        }
    }
    return NULL;
}

void Lobby::updatePlayerList(const std::vector<PlayerState>& players) {
    std::cout << "Updating lobby player list: " << players.size() << " players." << std::endl;

    // ✅ Store the latest list of players
    playersInLobby = players;

    // ✅ Print player IDs for debugging
    for (size_t i = 0; i < playersInLobby.size(); i++) {
        std::cout << "Player ID: " << i 
                  << " | Position: (" << playersInLobby[i].posX << ", " << playersInLobby[i].posY << ")"
                  << " | Admin: " << (playersInLobby[i].isAdmin ? "Yes" : "No")
                  << std::endl;
    }
}

