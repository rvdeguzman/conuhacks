#include "Menu.h"
#include "common.h"
#include <iostream>
#include <random>

Menu::Menu(SDL_Renderer* renderer) : renderer(renderer) {
    std::random_device rd; // obtain a random number from hardware
    std::mt19937 gen(rd()); // seed the generator
    std::uniform_int_distribution<> distr(1, 4); // define the range

    font = TTF_OpenFont("arial.ttf", 24);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
    }

    std::string filePath = "wall" + std::to_string(distr(gen)) + ".png";
    SDL_Surface* bgSurface = IMG_Load(filePath.c_str()); // Change file name if needed
    if (!bgSurface) {
        std::cerr << "Failed to load background image: " << IMG_GetError() << std::endl;
    } else {
        backgroundTexture = SDL_CreateTextureFromSurface(renderer, bgSurface);
        SDL_FreeSurface(bgSurface);
    }

    playButtonRect.w = 200;  // Width
    playButtonRect.h = 50;   // Height
    playButtonRect.x = (SCREEN_WIDTH - playButtonRect.w) / 2;  // Center X
    playButtonRect.y = (SCREEN_HEIGHT - playButtonRect.h) / 2;  // Center Y
}

Menu::~Menu() {
    if (backgroundTexture) {
        SDL_DestroyTexture(backgroundTexture); // ✅ Free the background texture
    }
    TTF_CloseFont(font);
}

void Menu::render() {
    SDL_RenderClear(renderer);

    if (backgroundTexture) {
        int textureWidth, textureHeight;
        SDL_QueryTexture(backgroundTexture, NULL, NULL, &textureWidth, &textureHeight); // Get image size

        int scaledWidth = textureWidth * 3;
        int scaledHeight = textureHeight * 3;

        // ✅ Loop to fill the screen with repeated images
        for (int y = 0; y < SCREEN_HEIGHT; y += scaledHeight) {  // Adjust to screen height
            for (int x = 0; x < SCREEN_WIDTH; x += scaledWidth) {  // Adjust to screen width
                SDL_Rect destRect = {x, y, scaledWidth, scaledHeight};
                SDL_RenderCopy(renderer, backgroundTexture, NULL, &destRect);
            }
        }
    } else {
        // Fallback: Draw a solid color if no image is loaded
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
    }

    // ✅ Set Button Color Based on Hover & Click State
    if (isClicked) {
        SDL_SetRenderDrawColor(renderer, 100, 100, 255, 255); // Blue when clicked
    } else if (isHovering) {
        SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255); // Light Gray when hovered
    } else {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White when normal
    }

    // ✅ Draw the Filled Button
    SDL_RenderFillRect(renderer, &playButtonRect);

    // ✅ Draw Button Outline (Black)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &playButtonRect);

    // ✅ Render "PLAY" text
    SDL_Color textColor = {0, 0, 0, 255}; // Black text
    SDL_Surface* surfaceMessage = TTF_RenderText_Solid(font, "PLAY", textColor);
    SDL_Texture* message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

    // ✅ Center text inside button
    SDL_Rect messageRect;
    messageRect.w = surfaceMessage->w;
    messageRect.h = surfaceMessage->h;
    messageRect.x = playButtonRect.x + (playButtonRect.w - messageRect.w) / 2;
    messageRect.y = playButtonRect.y + (playButtonRect.h - messageRect.h) / 2;

    SDL_RenderCopy(renderer, message, NULL, &messageRect);


    SDL_FreeSurface(surfaceMessage);
    SDL_DestroyTexture(message);
    
    SDL_RenderPresent(renderer);
}

SDL_Keycode Menu::handleInput() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == END_GAME_INPUT) {
            std::cout << "exiting" << std::endl;
            return END_GAME_INPUT; // Exit game
        }
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        // ✅ Check if mouse is over the button
        isHovering = (mouseX >= playButtonRect.x && mouseX <= playButtonRect.x + playButtonRect.w &&
                      mouseY >= playButtonRect.y && mouseY <= playButtonRect.y + playButtonRect.h);


        if (e.type == SDL_MOUSEBUTTONDOWN && isHovering) {
            isClicked = true;
            return ENTER_LOBBY_INPUT; // ✅ Start the game when button is clicked
        } else {
            isClicked = false;
        }
    }
    return NULL;
}
