#ifndef SPRITESHEET_H
#define SPRITESHEET_H

#include <SDL2/SDL.h>
#include <vector>
#include <string>

// Structure to handle a single sprite instance
struct Sprite {
    double x, y, distance;
    struct SpriteSheet* spriteSheet;  // Pointer to SpriteSheet
    int playerIndex;

    Sprite(double x, double y, double dist, SpriteSheet* sheet, int index)
        : x(x), y(y), distance(dist), spriteSheet(sheet), playerIndex(index) {}
};

// Structure to handle an entire sprite sheet
struct SpriteSheet {
    SDL_Texture* texture;
    int cols, rows;
    int frameWidth;
    bool useTransparency;
    SDL_Color transparentColor;
    std::vector<SDL_Rect> frames; // Store individual frame rects
};

// Function prototypes
SpriteSheet loadSpriteSheet(SDL_Renderer* renderer, const std::string& infoFile, const std::string& imageFile);
SDL_Rect getWalkingFrame(const SpriteSheet& sheet, bool isMoving);

#endif
