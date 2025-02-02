#include "SpriteSheet.h"
#include <SDL2/SDL_image.h>
#include <fstream>
#include <iostream>
#include <vector>

// Helper function to read `.info` file
void parseSpriteInfo(const std::string& infoFile, SpriteSheet& sheet) {
    std::ifstream file(infoFile);
    if (!file) {
        throw std::runtime_error("Failed to open sprite info file: " + infoFile);
    }

    file >> sheet.cols >> sheet.rows >> sheet.frameWidth >> sheet.useTransparency;
    std::string hexColor;
    file >> hexColor;

    if (sheet.useTransparency) {
        // Convert hex string to integer
        unsigned int hexValue = std::stoul(hexColor, nullptr, 16);
        
        // Extract RGB values
        sheet.transparentColor.r = (hexValue >> 16) & 0xFF;
        sheet.transparentColor.g = (hexValue >> 8) & 0xFF;
        sheet.transparentColor.b = hexValue & 0xFF;
        sheet.transparentColor.a = 255;
    }

    std::cout << "Loaded sprite info file: " << infoFile << std::endl;
    std::cout << "Columns: " << sheet.cols << ", Rows: " << sheet.rows 
                << ", Frame Size: " << sheet.frameWidth << "x" << sheet.frameWidth << std::endl;


    file.close();
}

// Load sprite sheet and split it into individual frames
SpriteSheet loadSpriteSheet(SDL_Renderer* renderer, const std::string& infoFile, const std::string& imageFile) {
    SpriteSheet sheet;

    // Read sprite sheet metadata
    parseSpriteInfo(infoFile, sheet);

    // Load sprite sheet image
    SDL_Surface* tempSurface = IMG_Load(imageFile.c_str());
    if (!tempSurface) {
        throw std::runtime_error("Failed to load sprite sheet: " + std::string(SDL_GetError()));
    }

    // Apply transparency if needed
    if (sheet.useTransparency) {
        SDL_SetColorKey(tempSurface, SDL_TRUE, SDL_MapRGB(tempSurface->format, 
                        sheet.transparentColor.r, 
                        sheet.transparentColor.g, 
                        sheet.transparentColor.b));
    }

    if (sheet.useTransparency) {
        std::cout << "Applying transparency: R=" << (int)sheet.transparentColor.r
                << " G=" << (int)sheet.transparentColor.g
                << " B=" << (int)sheet.transparentColor.b << std::endl;
    }


    // Convert surface to texture
    sheet.texture = SDL_CreateTextureFromSurface(renderer, tempSurface);
    SDL_FreeSurface(tempSurface);

    if (!sheet.texture) {
        throw std::runtime_error("Failed to create texture from sprite sheet: " + std::string(SDL_GetError()));
    }

    // Extract all frames from the sprite sheet
    for (int row = 0; row < sheet.rows; row++) {
        for (int col = 0; col < sheet.cols; col++) {
            SDL_Rect frame;
            frame.x = col * sheet.frameWidth;
            frame.y = row * sheet.frameWidth;
            frame.w = sheet.frameWidth;
            frame.h = sheet.frameWidth;
            sheet.frames.push_back(frame);
        }
    }

    return sheet;
}

SDL_Rect getWalkingFrame(const SpriteSheet& sheet, bool isMoving) {
    static const int columnIndex = 2; // 3rd column (0-based index)
    static const int startRow = 0;    // Row 1 (0-based index)
    static const int endRow = 4;      // Row 5 (0-based index)
    static const Uint32 timePerFrame = 100; // Frame duration in milliseconds

    if (!isMoving) {
        return sheet.frames[startRow * sheet.cols + columnIndex];
    }

    int totalFrames = (endRow - startRow + 1);
    int frameRow = (SDL_GetTicks() / timePerFrame) % totalFrames + startRow;
    
    int frameIndex = frameRow * sheet.cols + columnIndex; // Convert row, col to 1D index
    return sheet.frames[frameIndex];
}
