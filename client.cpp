#include <SDL2/SDL.h>
#include <enet/enet.h>
#include <cmath>
#include <vector>
#include <iostream>
#include "common.h"

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int MAP_WIDTH = 8;
const int MAP_HEIGHT = 8;
const char* SERVER_HOST = "127.0.0.1";
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
                perpWallDist = (mapX - currentPlayer.posX + (1 - stepX) / 2) / rayDirX;
            else
                perpWallDist = (mapY - currentPlayer.posY + (1 - stepY) / 2) / rayDirY;

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

        // Render other players (as simple rectangles for now)
        for (size_t i = 0; i < players.size(); i++) {
            if (i != playerID) {
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                SDL_Rect rect = {
                    int(players[i].posX * 32),
                    int(players[i].posY * 32),
                    16, 16
                };
                SDL_RenderFillRect(renderer, &rect);
            }
        }

        SDL_RenderPresent(renderer);
    }

public:
    GameClient() : isRunning(false) {
        if (SDL_Init(SDL_INIT_VIDEO) < 0 || enet_initialize() != 0) {
            throw std::runtime_error("Failed to initialize SDL or ENet");
        }

        window = SDL_CreateWindow("Raycasting Client",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        
        if (!window) {
            throw std::runtime_error("Failed to create window");
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            throw std::runtime_error("Failed to create renderer");
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
        playerID = 0;
        isRunning = true;
    }

    void run() {
        SDL_Event e;
        while (isRunning) {
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
                        PositionPacket* pos = (PositionPacket*)event.packet->data;
                        players[pos->playerID] = pos->state;
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
        }
    }

    ~GameClient() {
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
