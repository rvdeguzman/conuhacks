#include <SDL2/SDL.h>
#include <cmath>
#include <vector>
#include <iostream>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int MAP_WIDTH = 8;
const int MAP_HEIGHT = 8;

// Define the map (1 represents walls, 0 represents empty space)
const int worldMap[MAP_WIDTH][MAP_HEIGHT] = {
    {1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,1},
    {1,0,1,0,0,1,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,1,0,0,1,0,1},
    {1,0,1,0,0,1,0,1},
    {1,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1}
};

struct Player {
    double posX = 3.0;    // Player X position
    double posY = 3.0;    // Player Y position
    double dirX = -1.0;   // Initial direction vector
    double dirY = 0.0;
    double planeX = 0.0;  // Camera plane
    double planeY = 0.66;
    double moveSpeed = 0.05;
    double rotSpeed = 0.03;
};

int main(int argc, char* args[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Raycasting Engine",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    if (!window) {
        std::cout << "Window creation failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cout << "Renderer creation failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    Player player;
    bool quit = false;
    SDL_Event e;

    while (!quit) {
        // Handle events
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        // Handle keyboard input
        const Uint8* state = SDL_GetKeyboardState(NULL);
        if (state[SDL_SCANCODE_UP]) {
            if (!worldMap[int(player.posX + player.dirX * player.moveSpeed)][int(player.posY)])
                player.posX += player.dirX * player.moveSpeed;
            if (!worldMap[int(player.posX)][int(player.posY + player.dirY * player.moveSpeed)])
                player.posY += player.dirY * player.moveSpeed;
        }
        if (state[SDL_SCANCODE_DOWN]) {
            if (!worldMap[int(player.posX - player.dirX * player.moveSpeed)][int(player.posY)])
                player.posX -= player.dirX * player.moveSpeed;
            if (!worldMap[int(player.posX)][int(player.posY - player.dirY * player.moveSpeed)])
                player.posY -= player.dirY * player.moveSpeed;
        }
        if (state[SDL_SCANCODE_RIGHT]) {
            double oldDirX = player.dirX;
            player.dirX = player.dirX * cos(-player.rotSpeed) - player.dirY * sin(-player.rotSpeed);
            player.dirY = oldDirX * sin(-player.rotSpeed) + player.dirY * cos(-player.rotSpeed);
            double oldPlaneX = player.planeX;
            player.planeX = player.planeX * cos(-player.rotSpeed) - player.planeY * sin(-player.rotSpeed);
            player.planeY = oldPlaneX * sin(-player.rotSpeed) + player.planeY * cos(-player.rotSpeed);
        }
        if (state[SDL_SCANCODE_LEFT]) {
            double oldDirX = player.dirX;
            player.dirX = player.dirX * cos(player.rotSpeed) - player.dirY * sin(player.rotSpeed);
            player.dirY = oldDirX * sin(player.rotSpeed) + player.dirY * cos(player.rotSpeed);
            double oldPlaneX = player.planeX;
            player.planeX = player.planeX * cos(player.rotSpeed) - player.planeY * sin(player.rotSpeed);
            player.planeY = oldPlaneX * sin(player.rotSpeed) + player.planeY * cos(player.rotSpeed);
        }

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Raycasting
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            double cameraX = 2 * x / double(SCREEN_WIDTH) - 1;
            double rayDirX = player.dirX + player.planeX * cameraX;
            double rayDirY = player.dirY + player.planeY * cameraX;

            int mapX = int(player.posX);
            int mapY = int(player.posY);

            double sideDistX, sideDistY;
            double deltaDistX = std::abs(1 / rayDirX);
            double deltaDistY = std::abs(1 / rayDirY);
            double perpWallDist;

            int stepX, stepY;
            int hit = 0;
            int side;

            if (rayDirX < 0) {
                stepX = -1;
                sideDistX = (player.posX - mapX) * deltaDistX;
            } else {
                stepX = 1;
                sideDistX = (mapX + 1.0 - player.posX) * deltaDistX;
            }
            if (rayDirY < 0) {
                stepY = -1;
                sideDistY = (player.posY - mapY) * deltaDistY;
            } else {
                stepY = 1;
                sideDistY = (mapY + 1.0 - player.posY) * deltaDistY;
            }

            // DDA algorithm
            while (hit == 0) {
                if (sideDistX < sideDistY) {
                    sideDistX += deltaDistX;
                    mapX += stepX;
                    side = 0;
                } else {
                    sideDistY += deltaDistY;
                    mapY += stepY;
                    side = 1;
                }
                if (worldMap[mapX][mapY] > 0)
                    hit = 1;
            }

            if (side == 0)
                perpWallDist = (mapX - player.posX + (1 - stepX) / 2) / rayDirX;
            else
                perpWallDist = (mapY - player.posY + (1 - stepY) / 2) / rayDirY;

            int lineHeight = (int)(SCREEN_HEIGHT / perpWallDist);
            int drawStart = -lineHeight / 2 + SCREEN_HEIGHT / 2;
            if (drawStart < 0) drawStart = 0;
            int drawEnd = lineHeight / 2 + SCREEN_HEIGHT / 2;
            if (drawEnd >= SCREEN_HEIGHT) drawEnd = SCREEN_HEIGHT - 1;

            // Choose wall color
            SDL_SetRenderDrawColor(renderer, 
                side == 1 ? 155 : 255,  // Red
                side == 1 ? 155 : 255,  // Green
                side == 1 ? 155 : 255,  // Blue
                255);                   // Alpha

            // Draw the vertical line
            SDL_RenderDrawLine(renderer, x, drawStart, x, drawEnd);
        }

        // Update screen
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

