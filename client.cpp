#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include <enet/enet.h>
#include <cmath>
#include <vector>
#include <iostream>
#include "common.h"
#include <SDL2/SDL_image.h>
#include <algorithm>
#include "SpriteSheet.h"

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int MAP_WIDTH = 8;
const int MAP_HEIGHT = 8;
const char* SERVER_HOST = "127.0.0.1";
//const char* SERVER_HOST = "192.168.163.247";
const int SERVER_PORT = 1234;

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
    // SDL_Texture* playerTexture;
    SpriteSheet playerSprite;


    // struct Sprite {
    //     double x, y, distance;
    //     SDL_Texture* texture;
    // };


    void handleInput() {
        const Uint8* state = SDL_GetKeyboardState(NULL);
        
        InputPacket input;
        input.up = state[SDL_SCANCODE_UP];
        input.down = state[SDL_SCANCODE_DOWN];
        input.left = state[SDL_SCANCODE_LEFT];
        input.right = state[SDL_SCANCODE_RIGHT];

        ENetPacket* packet = enet_packet_create(
            &input, 
            sizeof(InputPacket), 
            ENET_PACKET_FLAG_RELIABLE
        );
        enet_peer_send(server, 0, packet);
    }

    void render() {
        if (players.empty() || playerID >= players.size()) {
            std::cerr << "Error: No valid player data. Skipping rendering." << std::endl;
            return;
        }


        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Render from current player's perspective
        const PlayerState& currentPlayer = players[playerID];

        std::vector<double> zBuffer(SCREEN_WIDTH, 1e30); // Large initial depth

        for (int x = 0; x < SCREEN_WIDTH; x++) {
            double cameraX = 2 * x / double(SCREEN_WIDTH) - 1;
            double rayDirX = currentPlayer.dirX + currentPlayer.planeX * cameraX;
            double rayDirY = currentPlayer.dirY + currentPlayer.planeY * cameraX;

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

            zBuffer[x] = perpWallDist; // Store depth buffer
        }

        
        // Raycasting
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            double cameraX = 2 * x / double(SCREEN_WIDTH) - 1;
            double rayDirX = currentPlayer.dirX + currentPlayer.planeX * cameraX;
            double rayDirY = currentPlayer.dirY + currentPlayer.planeY * cameraX;

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

        // Sort sprites by distance (furthest first)
        std::vector<Sprite> spriteList;
        for (size_t i = 0; i < players.size(); i++) {
            if (i != playerID) {
                double dx = players[i].posX - currentPlayer.posX;
                double dy = players[i].posY - currentPlayer.posY;
                double distance = dx * dx + dy * dy; // Use squared distance for efficiency
                
                spriteList.push_back(Sprite(players[i].posX, players[i].posY, distance, &playerSprite));
                // spriteList.push_back({players[i].posX, players[i].posY, distance, playerSprite});
            }
        }

        std::sort(spriteList.begin(), spriteList.end(), [](const Sprite& a, const Sprite& b) {
            return a.distance > b.distance; // Render closest last
        });

        // Render sprites with per-pixel visibility check
        for (const auto& sprite : spriteList) {
            double spriteX = sprite.x - currentPlayer.posX;
            double spriteY = sprite.y - currentPlayer.posY;

            double invDet = 1.0 / (currentPlayer.planeX * currentPlayer.dirY - currentPlayer.dirX * currentPlayer.planeY);
            double transformX = invDet * (currentPlayer.dirY * spriteX - currentPlayer.dirX * spriteY);
            double transformY = invDet * (-currentPlayer.planeY * spriteX + currentPlayer.planeX * spriteY);

            if (transformY <= 0) continue; // Don't draw if behind the player

            int spriteScreenX = int((SCREEN_WIDTH / 2) * (1 + transformX / transformY));

            int spriteHeight = abs(int(SCREEN_HEIGHT / transformY)); // Scale with distance
            int spriteWidth = spriteHeight; // Maintain square proportions

            int drawStartY = -spriteHeight / 2 + SCREEN_HEIGHT / 2;
            int drawEndY = spriteHeight / 2 + SCREEN_HEIGHT / 2;
            drawStartY = std::max(0, drawStartY);
            drawEndY = std::min(SCREEN_HEIGHT - 1, drawEndY);

            int drawStartX = -spriteWidth / 2 + spriteScreenX;
            int drawEndX = spriteWidth / 2 + spriteScreenX;
            drawStartX = std::max(0, drawStartX);
            drawEndX = std::min(SCREEN_WIDTH - 1, drawEndX);

            for (int stripe = drawStartX; stripe < drawEndX; stripe++) {
                if (transformY > zBuffer[stripe]) continue; // Only remove parts behind walls

                // int frameIndex = (SDL_GetTicks() / 100) % (sprite.spriteSheet->cols * sprite.spriteSheet->rows);
                // SDL_Rect srcRect = getFrameRect(*sprite.spriteSheet, frameIndex);
                SDL_Rect srcRect = getWalkingFrame(playerSprite);


                
                SDL_Rect destRect = {drawStartX, drawStartY, spriteWidth, spriteHeight};

                SDL_RenderCopy(renderer, playerSprite.texture, &srcRect, &destRect);
            }


            // for (int stripe = drawStartX; stripe < drawEndX; stripe++) {
            //     if (transformY > zBuffer[stripe]) continue; // Only remove parts behind walls

            //     // int frameIndex = (SDL_GetTicks() / 100) % (playerSprite.cols * playerSprite.rows);
            //     // SDL_Rect srcRect = getFrameRect(playerSprite, frameIndex);
            //     SDL_Rect destRect = {drawStartX, drawStartY, spriteWidth, spriteHeight};

            //     // SDL_RenderCopy(renderer, playerSprite.texture, &srcRect, &destRect);
            //     int frameIndex = (SDL_GetTicks() / 100) % (sprite.spriteSheet->cols * sprite.spriteSheet->rows);
            //     SDL_Rect srcRect = getFrameRect(*sprite.spriteSheet, frameIndex);
            //     SDL_RenderCopy(renderer, sprite.spriteSheet->texture, &srcRect, &destRect);


            //     // SDL_Rect destRect = {stripe, drawStartY, 1, spriteHeight}; // Draw one column
            //     // SDL_RenderCopy(renderer, sprite.texture, NULL, &destRect);
            // }
        }

        SDL_RenderPresent(renderer);
    }

public:
    GameClient() : isRunning(false) {
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

        playerSprite = loadSpriteSheet(renderer, "msgunner.info", "msgunner.bmp");

        if (playerSprite.texture == nullptr) {
            std::cerr << "Error: Failed to load sprite sheet!" << std::endl;
        } else {
            std::cout << "Sprite sheet loaded successfully!" << std::endl;
            std::cout << "Sprite Info: " << playerSprite.cols << " cols, " 
                    << playerSprite.rows << " rows, " 
                    << playerSprite.frameWidth << "x" << playerSprite.frameWidth << std::endl;
        }



        // Initialize players vector with default states
        players.resize(2);
        playerID = 0;  // Will be set properly when connecting to server
        isRunning = true;


        // // Load player texture
        // SDL_Surface* tempSurface = IMG_Load("player_texture.png");
        // if (!tempSurface) {
        //     throw std::runtime_error("Failed to load player texture: " + std::string(IMG_GetError()));
        // }
        // playerTexture = SDL_CreateTextureFromSurface(renderer, tempSurface);
        // if (!playerTexture) {
        //     std::cerr << "Failed to create player texture: " << SDL_GetError() << std::endl;
        //     return;
        // } else {
        //     std::cout << "Player texture loaded successfully!" << std::endl;
        // }

        // SDL_FreeSurface(tempSurface);
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
        // SDL_DestroyTexture(playerTexture);
        // playerSprite.free();

        if (playerSprite.texture) {
            SDL_DestroyTexture(playerSprite.texture);
            playerSprite.texture = nullptr;
        }


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
