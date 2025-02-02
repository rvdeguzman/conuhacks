#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include <enet/enet.h>
#include <cmath>
#include <vector>
#include <iostream>
#include "common.h"
#include <SDL2/SDL_image.h>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int MAP_WIDTH = 8;
const int MAP_HEIGHT = 8;
const int MINIMAP_SIZE = 120; // Size of the minimap
const char* SERVER_HOST = "127.0.0.1";
//const char* SERVER_HOST = "192.168.163.247";
const int SERVER_PORT = 1234;
const double MOUSE_SENSITIVITY = 0.003;

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

class GameClient {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    ENetHost* client;
    ENetPeer* server;
    std::vector<PlayerState> players;
    size_t playerID;
    bool isRunning;
    SDL_Texture* playerTexture;
    double pitch = 0.0;


    void handleInput() {
        const Uint8* state = SDL_GetKeyboardState(NULL);
        
        InputPacket input;
        input.up = state[SDL_SCANCODE_UP] || state[SDL_SCANCODE_W];
        input.down = state[SDL_SCANCODE_DOWN] || state[SDL_SCANCODE_S];
        
        // Calculate the perpendicular direction vector for strafing
        if (state[SDL_SCANCODE_LEFT] || state[SDL_SCANCODE_A]) {
            input.strafeLeft = true;
            input.strafeRight = false;
        } else if (state[SDL_SCANCODE_RIGHT] || state[SDL_SCANCODE_D]) {
            input.strafeLeft = false;
            input.strafeRight = true;
        } else {
            input.strafeLeft = false;
            input.strafeRight = false;
        }
        
        // Handle mouse movement for rotation
        int mouseXRel, mouseYRel;
        SDL_GetRelativeMouseState(&mouseXRel, &mouseYRel);

        // Horizontal rotation (left/right)
        double angle = -mouseXRel * MOUSE_SENSITIVITY;

        // Rotate the player's direction and camera plane
        double oldDirX = players[playerID].dirX;
        players[playerID].dirX = players[playerID].dirX * cos(angle) - players[playerID].dirY * sin(angle);
        players[playerID].dirY = oldDirX * sin(angle) + players[playerID].dirY * cos(angle);

        double oldPlaneX = players[playerID].planeX;
        players[playerID].planeX = players[playerID].planeX * cos(angle) - players[playerID].planeY * sin(angle);
        players[playerID].planeY = oldPlaneX * sin(angle) + players[playerID].planeY * cos(angle);

        // Vertical rotation (up/down)
        pitch += mouseYRel * MOUSE_SENSITIVITY;

        // Clamp the pitch to avoid extreme angles
        const double MAX_PITCH = 1.5; // Limit pitch to ±1.5 radians (~85 degrees)
        if (pitch > MAX_PITCH) pitch = MAX_PITCH;
        if (pitch < -MAX_PITCH) pitch = -MAX_PITCH;

        // Send input to the server
        input.mouseRotation = angle; // Send horizontal rotation to the server
        input.mousePitch = pitch;    // Send vertical rotation to the server

        ENetPacket* packet = enet_packet_create(
            &input, 
            sizeof(InputPacket), 
            ENET_PACKET_FLAG_RELIABLE
        );
        enet_peer_send(server, 0, packet);
    }

    void renderMinimap() {
        if (players.empty()) return;

        const int cellSize = MINIMAP_SIZE / MAP_WIDTH;
        const int minimapX = SCREEN_WIDTH - MINIMAP_SIZE - 10;
        const int minimapY = 10;

        // Draw background
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
        SDL_Rect minimapBg = {minimapX - 2, minimapY - 2, MINIMAP_SIZE + 4, MINIMAP_SIZE + 4};
        SDL_RenderFillRect(renderer, &minimapBg);

        // Draw walls
        for (int x = 0; x < MAP_WIDTH; x++) {
            for (int y = 0; y < MAP_HEIGHT; y++) {
                if (worldMap[x][y] == 1) {
                    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
                    SDL_Rect wallRect = {
                        minimapX + x * cellSize,
                        minimapY + y * cellSize,
                        cellSize,
                        cellSize
                    };
                    SDL_RenderFillRect(renderer, &wallRect);
                }
            }
        }

        // Draw players
        for (size_t i = 0; i < players.size(); i++) {
            const PlayerState& player = players[i];
            int playerMinimapX = minimapX + int(player.posX * cellSize);
            int playerMinimapY = minimapY + int(player.posY * cellSize);

            // Draw player dot
            SDL_SetRenderDrawColor(renderer, 
                i == playerID ? 0 : 255,  // Red
                i == playerID ? 255 : 0,  // Green
                0,                        // Blue
                255);                     // Alpha

            SDL_Rect playerRect = {
                playerMinimapX - 2,
                playerMinimapY - 2,
                4,
                4
            };
            SDL_RenderFillRect(renderer, &playerRect);

            // Draw direction line
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_RenderDrawLine(
                renderer,
                playerMinimapX,
                playerMinimapY,
                playerMinimapX + int(player.dirX * cellSize),
                playerMinimapY + int(player.dirY * cellSize)
            );
        }
    }

    void render() {
        if (players.empty()) return;

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Render from current player's perspective
        const PlayerState& currentPlayer = players[playerID];
        
        // Raycasting
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            double cameraX = 2 * x / double(SCREEN_WIDTH) - 1;
            double rayDirX = currentPlayer.dirX + currentPlayer.planeX * cameraX;
            double rayDirY = currentPlayer.dirY + currentPlayer.planeY * cameraX;

            // Apply vertical rotation (pitch)
            double rayDirZ = -pitch; // Adjust ray direction for vertical look

            int mapX = int(currentPlayer.posX);
            int mapY = int(currentPlayer.posY);

            double sideDistX, sideDistY;
            double deltaDistX = std::abs(1 / rayDirX);
            double deltaDistY = std::abs(1 / rayDirY);
            double perpWallDist;

            int stepX, stepY;
            int hit = 0;
            int side;

            if (rayDirX < 0) {
                stepX = -1;
                sideDistX = (currentPlayer.posX - mapX) * deltaDistX;
            } else {
                stepX = 1;
                sideDistX = (mapX + 1.0 - currentPlayer.posX) * deltaDistX;
            }
            if (rayDirY < 0) {
                stepY = -1;
                sideDistY = (currentPlayer.posY - mapY) * deltaDistY;
            } else {
                stepY = 1;
                sideDistY = (mapY + 1.0 - currentPlayer.posY) * deltaDistY;
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
                perpWallDist = (mapX - currentPlayer.posX + (1.0 - stepX) / 2.0) / rayDirX;
            else
                perpWallDist = (mapY - currentPlayer.posY + (1.0 - stepY) / 2.0) / rayDirY;

            int lineHeight = (int)(SCREEN_HEIGHT / perpWallDist);
            int drawStart = -lineHeight / 2 + SCREEN_HEIGHT / 2 + (int)(rayDirZ * SCREEN_HEIGHT / 2);
            if (drawStart < 0) drawStart = 0;
            int drawEnd = lineHeight / 2 + SCREEN_HEIGHT / 2 + (int)(rayDirZ * SCREEN_HEIGHT / 2);
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

        // Render players as 3D blocks
        for (size_t i = 0; i < players.size(); i++) {
            if (i != playerID) {
                double spriteX = players[i].posX - currentPlayer.posX;
                double spriteY = players[i].posY - currentPlayer.posY;

                double invDet = 1.0 / (currentPlayer.planeX * currentPlayer.dirY - currentPlayer.dirX * currentPlayer.planeY);

                double transformX = invDet * (currentPlayer.dirY * spriteX - currentPlayer.dirX * spriteY);
                double transformY = invDet * (-currentPlayer.planeY * spriteX + currentPlayer.planeX * spriteY);

                int spriteScreenX = int((SCREEN_WIDTH / 2) * (1 + transformX / transformY));

                int spriteHeight = abs(int(SCREEN_HEIGHT / transformY));
                int drawStartY = -spriteHeight / 2 + SCREEN_HEIGHT / 2 + (int)(pitch * SCREEN_HEIGHT / 2);
                if (drawStartY < 0) drawStartY = 0;
                int drawEndY = spriteHeight / 2 + SCREEN_HEIGHT / 2 + (int)(pitch * SCREEN_HEIGHT / 2);
                if (drawEndY >= SCREEN_HEIGHT) drawEndY = SCREEN_HEIGHT - 1;

                int spriteWidth = abs(int(SCREEN_HEIGHT / transformY));
                int drawStartX = -spriteWidth / 2 + spriteScreenX;
                if (drawStartX < 0) drawStartX = 0;
                int drawEndX = spriteWidth / 2 + spriteScreenX;
                if (drawEndX >= SCREEN_WIDTH) drawEndX = SCREEN_WIDTH - 1;

                SDL_Rect destRect = {drawStartX, drawStartY, spriteWidth, spriteHeight};
                SDL_RenderCopy(renderer, playerTexture, NULL, &destRect);

            }
        }

        renderMinimap();

        SDL_RenderPresent(renderer);
    }

public:
    GameClient() : isRunning(false){
        if (SDL_Init(SDL_INIT_VIDEO) < 0 || enet_initialize() != 0) {
            throw std::runtime_error("Failed to initialize SDL or ENet");
        }

        if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
            throw std::runtime_error("SDL2_image initialization failed: " + std::string(IMG_GetError()));
        }

        std::cout << "Initializing SDL2..." << std::endl;
        window = SDL_CreateWindow("Raycasting Client",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        
        if (!window) {
            std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
            return;
        } else {
            std::cout << "SDL window created successfully!" << std::endl;
        }


        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
            return;
        } else {
            std::cout << "SDL renderer created successfully!" << std::endl;
        }

        client = enet_host_create(NULL, 1, 2, 0, 0);
        if (!client) {
            throw std::runtime_error("Failed to create ENet client");
        }

        ENetAddress address;
        enet_address_set_host(&address, SERVER_HOST);
        address.port = SERVER_PORT;

        server = enet_host_connect(client, &address, 2, 0);
        if (!server) {
            throw std::runtime_error("Failed to connect to server");
        }

        // Initialize players vector with default states
        players.resize(2);
        playerID = 0;  // Will be set properly when connecting to server
        isRunning = true;

        // Load player texture
        SDL_Surface* tempSurface = IMG_Load("player_texture.png");
        if (!tempSurface) {
            throw std::runtime_error("Failed to load player texture: " + std::string(IMG_GetError()));
        }
        playerTexture = SDL_CreateTextureFromSurface(renderer, tempSurface);
        if (!playerTexture) {
            std::cerr << "Failed to create player texture: " << SDL_GetError() << std::endl;
            return;
        } else {
            std::cout << "Player texture loaded successfully!" << std::endl;
        }

        SDL_FreeSurface(tempSurface);
        SDL_SetRelativeMouseMode(SDL_TRUE);
    }


    void run() {
        SDL_Event e;
        const int FPS = 60;
        const int FRAME_DELAY = 1000 / FPS;
        Uint32 frameStart;
        int frameTime;
        
        while (isRunning) {
            frameStart = SDL_GetTicks();
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) {
                    isRunning = false;
                }
            }

            handleInput();

            ENetEvent event;
            while (enet_host_service(client, &event, 0) > 0) {
                switch (event.type) {
                    case ENET_EVENT_TYPE_RECEIVE: {
                        if (event.packet->dataLength == sizeof(uint8_t)) {
                            // This is the initial player ID assignment
                            playerID = *(uint8_t*)event.packet->data;
                            std::cout << "Assigned player ID: " << (int)playerID << std::endl;
                        } else {
                            // This is a position update
                            PositionPacket* pos = (PositionPacket*)event.packet->data;
                            players[pos->playerID] = pos->state;
                        }
                        enet_packet_destroy(event.packet);
                        break;
                    }
                    case ENET_EVENT_TYPE_DISCONNECT:
                        std::cout << "Disconnected from server" << std::endl;
                        isRunning = false;
                        break;
                    default:
                        break;
                }
            }

            render();

            frameTime = SDL_GetTicks() - frameStart;
            if (FRAME_DELAY > frameTime) {
                SDL_Delay(FRAME_DELAY - frameTime);
            }
        }
    }

    ~GameClient() {
        SDL_DestroyTexture(playerTexture);
        enet_peer_disconnect(server, 0);
        
        ENetEvent event;
        while (enet_host_service(client, &event, 3000) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_RECEIVE:
                    enet_packet_destroy(event.packet);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    std::cout << "Disconnection succeeded." << std::endl;
                    goto cleanup;
                default:
                    break;
            }
        }
    cleanup:
        enet_host_destroy(client);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();  // ✅ Properly quit SDL2_Image
        enet_deinitialize();
        SDL_Quit();
    }

};

int main(int argc, char* args[]) {
    try {
        GameClient client;
        client.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}