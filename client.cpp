#include "SpriteSheet.h"
#include "common.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_timer.h>
#include <algorithm>
#include <cmath>
#include <enet/enet.h>
#include <iostream>
#include <vector>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const char *SERVER_HOST = "127.0.0.1";
// const char* SERVER_HOST = "192.168.163.247";
const int SERVER_PORT = 1234;

class GameClient {
private:
  SDL_Window *window;
  SDL_Renderer *renderer;
  ENetHost *client;
  ENetPeer *server;
  std::vector<PlayerState> players;
  size_t playerID;
  bool isRunning;
  SDL_Texture *playerTexture;

  // Weapon state
  int currentWeapon = 0; // Current weapon type
  bool isShooting = false;
  Uint32 lastShotTime = 0;
  const int SHOOT_ANIMATION_MS = 500; // Animation duration in milliseconds

  static const int NUM_TEXTURES = 4;
  SDL_Texture *wallTextures[NUM_TEXTURES];
  static const int TEX_WIDTH = 64;
  static const int TEX_HEIGHT = 64;
  // SDL_Texture* playerTexture;
  SpriteSheet playerSprite;
  SpriteSheet weaponSprite;

  void handleInput() {
    const Uint8 *state = SDL_GetKeyboardState(NULL);

    InputPacket input;
    input.forward = state[SDL_SCANCODE_W];
    input.backward = state[SDL_SCANCODE_S];
    input.strafeLeft = state[SDL_SCANCODE_A];
    input.strafeRight = state[SDL_SCANCODE_D];
    input.turnLeft = state[SDL_SCANCODE_LEFT];
    input.turnRight = state[SDL_SCANCODE_RIGHT];

    // Handle weapon input
    if (state[SDL_SCANCODE_SPACE] && !isShooting) {
      isShooting = true;
      lastShotTime = SDL_GetTicks();
      isShooting = true;
      lastShotTime = SDL_GetTicks();

      // Send shot attempt to server
      ShotAttemptPacket shotPacket;
      shotPacket.shooterID = playerID;
      shotPacket.shooterPosX = players[playerID].posX;
      shotPacket.shooterPosY = players[playerID].posY;
      shotPacket.shooterDirX = players[playerID].dirX;
      shotPacket.shooterDirY = players[playerID].dirY;

      ENetPacket *packet = enet_packet_create(
          &shotPacket, sizeof(ShotAttemptPacket), ENET_PACKET_FLAG_RELIABLE);
      enet_peer_send(server, 0, packet);
    }

    // Weapon switching
    if (state[SDL_SCANCODE_1])
      currentWeapon = 0;
    if (state[SDL_SCANCODE_2])
      currentWeapon = 1;
    if (state[SDL_SCANCODE_3])
      currentWeapon = 2;
    if (state[SDL_SCANCODE_4])
      currentWeapon = 3;

    ENetPacket *packet = enet_packet_create(&input, sizeof(InputPacket),
                                            ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(server, 0, packet);
  }

void renderMinimap() {
    const int MINIMAP_SIZE = 150;  // Size of the minimap in pixels
    const int MINIMAP_X = SCREEN_WIDTH - MINIMAP_SIZE - 10;  // Position from right
    const int MINIMAP_Y = 10;  // Position from top
    const int CELL_SIZE = MINIMAP_SIZE / MAP_WIDTH;  // Size of each map cell
    const int PLAYER_DOT_SIZE = 4;  // Size of player dots on minimap
    const int DIRECTION_LINE_LENGTH = 8;  // Length of direction indicator

    // Draw minimap background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 192);  // Semi-transparent black
    SDL_Rect minimapBG = {MINIMAP_X, MINIMAP_Y, MINIMAP_SIZE, MINIMAP_SIZE};
    SDL_RenderFillRect(renderer, &minimapBG);

    // Draw walls
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (worldMap[x][y] > 0) {
                // Choose color based on wall type
                switch(worldMap[x][y]) {
                    case 1: SDL_SetRenderDrawColor(renderer, 192, 192, 192, 255); break;  // Gray
                    case 2: SDL_SetRenderDrawColor(renderer, 192, 0, 0, 255); break;      // Red
                    case 3: SDL_SetRenderDrawColor(renderer, 0, 192, 0, 255); break;      // Green
                    case 4: SDL_SetRenderDrawColor(renderer, 0, 0, 192, 255); break;      // Blue
                    default: SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);        // White
                }

                SDL_Rect wallRect = {
                    MINIMAP_X + (x * CELL_SIZE),
                    MINIMAP_Y + (y * CELL_SIZE),
                    CELL_SIZE,
                    CELL_SIZE
                };
                SDL_RenderFillRect(renderer, &wallRect);
            }
        }
    }

    // Draw players
    for (size_t i = 0; i < players.size(); i++) {
        const PlayerState& player = players[i];
        
        // Calculate player position on minimap
        int playerMinimapX = MINIMAP_X + static_cast<int>(player.posX * CELL_SIZE);
        int playerMinimapY = MINIMAP_Y + static_cast<int>(player.posY * CELL_SIZE);

        // Draw player dot
        if (i == playerID) {
            // Current player in yellow
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        } else {
            // Other players in red
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        }

        SDL_Rect playerRect = {
            playerMinimapX - PLAYER_DOT_SIZE/2,
            playerMinimapY - PLAYER_DOT_SIZE/2,
            PLAYER_DOT_SIZE,
            PLAYER_DOT_SIZE
        };
        SDL_RenderFillRect(renderer, &playerRect);

        // Draw direction indicator for current player
        if (i == playerID) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_RenderDrawLine(renderer,
                playerMinimapX,
                playerMinimapY,
                playerMinimapX + static_cast<int>(player.dirX * DIRECTION_LINE_LENGTH),
                playerMinimapY + static_cast<int>(player.dirY * DIRECTION_LINE_LENGTH)
            );
        }
    }

    // Draw minimap border
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &minimapBG);
}
  void render() {
    if (players.empty() || playerID >= players.size()) {
      std::cerr << "Error: No valid player data. Skipping rendering."
                << std::endl;
      return;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Render from current player's perspective
    const PlayerState &currentPlayer = players[playerID];

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
        perpWallDist =
            (mapX - currentPlayer.posX + (1.0 - stepX) / 2.0) / rayDirX;
      else
        perpWallDist =
            (mapY - currentPlayer.posY + (1.0 - stepY) / 2.0) / rayDirY;

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
        perpWallDist =
            (mapX - currentPlayer.posX + (1.0 - stepX) / 2.0) / rayDirX;
      else
        perpWallDist =
            (mapY - currentPlayer.posY + (1.0 - stepY) / 2.0) / rayDirY;

      int lineHeight = (int)(SCREEN_HEIGHT / perpWallDist);
      int drawStart = -lineHeight / 2 + SCREEN_HEIGHT / 2;
      if (drawStart < 0)
        drawStart = 0;
      int drawEnd = lineHeight / 2 + SCREEN_HEIGHT / 2;
      if (drawEnd >= SCREEN_HEIGHT)
        drawEnd = SCREEN_HEIGHT - 1;

      // Calculate texture coordinates
      double wallX;
      if (side == 0) {
        wallX = currentPlayer.posY + perpWallDist * rayDirY;
      } else {
        wallX = currentPlayer.posX + perpWallDist * rayDirX;
      }
      wallX -= floor(wallX);

      // x coordinate on the texture
      int texX = int(wallX * TEX_WIDTH);
      if (side == 0 && rayDirX > 0)
        texX = TEX_WIDTH - texX - 1;
      if (side == 1 && rayDirY < 0)
        texX = TEX_WIDTH - texX - 1;

      // Choose texture based on wall type (1-4)
      int texNum = worldMap[mapX][mapY] - 1;
      // Manual clamp implementation
      texNum = (texNum < 0)
                   ? 0
                   : ((texNum > NUM_TEXTURES - 1) ? NUM_TEXTURES - 1 : texNum);

      // Calculate texture scaling
      double step = 1.0 * TEX_HEIGHT / lineHeight;
      double texPos = (drawStart - SCREEN_HEIGHT / 2 + lineHeight / 2) * step;

      // Draw the textured vertical line pixel by pixel
      for (int y = drawStart; y < drawEnd; y++) {
        int texY = (int)texPos & (TEX_HEIGHT - 1);
        texPos += step;

        SDL_Rect srcRect = {texX, texY, 1, 1};
        SDL_Rect destRect = {x, y, 1, 1};
        SDL_RenderCopy(renderer, wallTextures[texNum], &srcRect, &destRect);
      }
    }

        // Sort sprites by distance (furthest first)
        std::vector<Sprite> spriteList;
        for (size_t i = 0; i < players.size(); i++) {
            if (i != playerID) {
                double dx = players[i].posX - currentPlayer.posX;
                double dy = players[i].posY - currentPlayer.posY;
                double distance = dx * dx + dy * dy; // Use squared distance for efficiency
                
                spriteList.push_back(Sprite(players[i].posX, players[i].posY, distance, &playerSprite, i));
                // spriteList.push_back(Sprite(players[i].posX, players[i].posY, distance, &playerSprite));
                // spriteList.push_back({players[i].posX, players[i].posY, distance, playerSprite});
            }
        }

        std::sort(spriteList.begin(), spriteList.end(), [](const Sprite& a, const Sprite& b) {
            return a.distance > b.distance; // Render closest last
        });

        SDL_Rect srcRect, destRect; // ✅ Declare rects before using them

    // Render sprites with per-pixel visibility check
    for (const auto &sprite : spriteList) {
      double spriteX = sprite.x - currentPlayer.posX;
      double spriteY = sprite.y - currentPlayer.posY;

      double invDet = 1.0 / (currentPlayer.planeX * currentPlayer.dirY -
                             currentPlayer.dirX * currentPlayer.planeY);
      double transformX = invDet * (currentPlayer.dirY * spriteX -
                                    currentPlayer.dirX * spriteY);
      double transformY = invDet * (-currentPlayer.planeY * spriteX +
                                    currentPlayer.planeX * spriteY);

      if (transformY <= 0)
        continue; // Don't draw if behind the player

      int spriteScreenX =
          int((SCREEN_WIDTH / 2) * (1 + transformX / transformY));

      int spriteHeight =
          abs(int(SCREEN_HEIGHT / transformY)); // Scale with distance
      int spriteWidth = spriteHeight;           // Maintain square proportions

      int drawStartY = -spriteHeight / 2 + SCREEN_HEIGHT / 2;
      int drawEndY = spriteHeight / 2 + SCREEN_HEIGHT / 2;
      drawStartY = std::max(0, drawStartY);
      drawEndY = std::min(SCREEN_HEIGHT - 1, drawEndY);

      int drawStartX = -spriteWidth / 2 + spriteScreenX;
      int drawEndX = spriteWidth / 2 + spriteScreenX;
      drawStartX = std::max(0, drawStartX);
      drawEndX = std::min(SCREEN_WIDTH - 1, drawEndX);

            // for (const auto& sprite : spriteList) {
                int playerIndex = sprite.playerIndex;

                // Get the other player's direction
                double otherDirX = players[playerIndex].dirX;
                double otherDirY = players[playerIndex].dirY;

                // Get current player's right direction
                double cameraRightX = currentPlayer.planeX;
                double cameraRightY = currentPlayer.planeY;

                // Compute dot product to determine flipping
                double dotProduct = (otherDirX * cameraRightX) + (otherDirY * cameraRightY);
                bool shouldFlip = dotProduct < 0;  // ✅ Flip if looking left relative to us

                // ✅ Get the correct walking frame
                SDL_Rect srcRect = getWalkingFrame(*sprite.spriteSheet, players[playerIndex].isMoving);

                // ✅ Ensure valid dimensions
                if (srcRect.w == 0 || srcRect.h == 0) {
                    std::cerr << "Warning: srcRect has invalid dimensions!" << std::endl;
                    continue;
                }

                // ✅ Ensure `SDL_RenderCopyEx` is actually being used
                SDL_RendererFlip flipType = shouldFlip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

                // std::cout << "Rendering Player " << playerIndex << " Flip: " << (shouldFlip ? "YES" : "NO") << std::endl;

                // ✅ Apply the flipping transformation
                SDL_RenderCopyEx(renderer, sprite.spriteSheet->texture, &srcRect, &destRect, 0, NULL, flipType);
            // }
            for (int stripe = drawStartX; stripe < drawEndX; stripe++) {
                if (stripe < 0 || stripe >= SCREEN_WIDTH) continue;  // Skip out-of-bounds pixels

                if (transformY > zBuffer[stripe]) continue;  // ✅ Skip if behind a wall

                // ✅ Reverse texture column selection when flipping
                int texX;
                if (shouldFlip) {
                    texX = srcRect.x + srcRect.w - (int)((stripe - drawStartX) * (float)srcRect.w / (drawEndX - drawStartX)) - 1;
                } else {
                    texX = srcRect.x + (int)((stripe - drawStartX) * (float)srcRect.w / (drawEndX - drawStartX));
                }

                SDL_Rect srcColumn = {texX, srcRect.y, 1, srcRect.h}; // Select 1px slice of sprite
                SDL_Rect columnRect = {stripe, drawStartY, 1, spriteHeight}; // Render 1px wide column

                SDL_RenderCopy(renderer, sprite.spriteSheet->texture, &srcColumn, &columnRect);
            }





      // for (int stripe = drawStartX; stripe < drawEndX; stripe++) {
      //     if (transformY > zBuffer[stripe]) continue; // Only remove parts
      //     behind walls

      //     // int frameIndex = (SDL_GetTicks() / 100) % (playerSprite.cols *
      //     playerSprite.rows);
      //     // SDL_Rect srcRect = getFrameRect(playerSprite, frameIndex);
      //     SDL_Rect destRect = {drawStartX, drawStartY, spriteWidth,
      //     spriteHeight};

      //     // SDL_RenderCopy(renderer, playerSprite.texture, &srcRect,
      //     &destRect); int frameIndex = (SDL_GetTicks() / 100) %
      //     (sprite.spriteSheet->cols * sprite.spriteSheet->rows); SDL_Rect
      //     srcRect = getFrameRect(*sprite.spriteSheet, frameIndex);
      //     SDL_RenderCopy(renderer, sprite.spriteSheet->texture, &srcRect,
      //     &destRect);
    }
    // Render weapon
    int weaponFrame = 0;
    if (isShooting) {
      Uint32 elapsedTime = SDL_GetTicks() - lastShotTime;
      if (elapsedTime < SHOOT_ANIMATION_MS) {
        // Calculate animation frame based on elapsed time
        weaponFrame = (elapsedTime * weaponSprite.cols) / SHOOT_ANIMATION_MS;
        weaponFrame = std::min(weaponFrame, weaponSprite.cols - 1);
      } else {
        isShooting = false;
      }
    }

    // Calculate weapon position (centered at bottom of screen)
    int weaponWidth = weaponSprite.frameWidth * 6; // Scale up the weapon
    int weaponHeight = 384;
    int weaponX = (SCREEN_WIDTH - weaponWidth) / 2;
    int weaponY = SCREEN_HEIGHT - weaponHeight;

    // Get the correct weapon frame from spritesheet
    SDL_Rect weaponSrcRect = {
        weaponFrame * weaponSprite.frameWidth,
        currentWeapon * 64,
        weaponSprite.frameWidth,
        64,
    };

    SDL_Rect weaponDestRect = {weaponX, weaponY, weaponWidth, weaponHeight};

    // Add slight weapon bob based on time
    float bobAmount = 5.0f;  // Adjust this for more/less bob
    float bobSpeed = 0.004f; // Adjust this for faster/slower bob
    weaponDestRect.y +=
        static_cast<int>(sin(SDL_GetTicks() * bobSpeed) * bobAmount);

    SDL_RenderCopy(renderer, weaponSprite.texture, &weaponSrcRect,
                   &weaponDestRect);
    
    renderMinimap();

    SDL_RenderPresent(renderer);
  }

public:
  GameClient() : isRunning(false) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || enet_initialize() != 0) {
      throw std::runtime_error("Failed to initialize SDL or ENet");
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
      throw std::runtime_error("SDL2_image initialization failed: " +
                               std::string(IMG_GetError()));
    }

    std::cout << "Initializing SDL2..." << std::endl;
    window = SDL_CreateWindow("Raycasting Client", SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
                              SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

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
                << playerSprite.rows << " rows, " << playerSprite.frameWidth
                << "x" << playerSprite.frameWidth << std::endl;
    }

    // player weapon sprites (will be shown to the current player, so should act
    // as a ui element:)
    weaponSprite = loadSpriteSheet(renderer, "weapons.info", "weapons.bmp");

    // Initialize players vector with default states
    players.resize(2);
    playerID = 0; // Will be set properly when connecting to server
    isRunning = true;

    // Load wall textures
    const char *textureFiles[NUM_TEXTURES] = {"wall1.png", "wall2.png",
                                              "wall3.png", "wall4.png"};

    for (int i = 0; i < NUM_TEXTURES; i++) {
      SDL_Surface *tempSurface = IMG_Load(textureFiles[i]);
      if (!tempSurface) {
        throw std::runtime_error("Failed to load wall texture: " +
                                 std::string(IMG_GetError()));
      }
      wallTextures[i] = SDL_CreateTextureFromSurface(renderer, tempSurface);
      SDL_FreeSurface(tempSurface);
      if (!wallTextures[i]) {
        throw std::runtime_error("Failed to create wall texture: " +
                                 std::string(SDL_GetError()));
      }
    }

    // Load player texture
    SDL_Surface *tempSurface = IMG_Load("player_texture.png");
    if (!tempSurface) {
      throw std::runtime_error("Failed to load player texture: " +
                               std::string(IMG_GetError()));
    }
    playerTexture = SDL_CreateTextureFromSurface(renderer, tempSurface);
    if (!playerTexture) {
      std::cerr << "Failed to create player texture: " << SDL_GetError()
                << std::endl;
      return;
    } else {
      std::cout << "Player texture loaded successfully!" << std::endl;
    }

    // // Load player texture
    // SDL_Surface* tempSurface = IMG_Load("player_texture.png");
    // if (!tempSurface) {
    //     throw std::runtime_error("Failed to load player texture: " +
    //     std::string(IMG_GetError()));
    // }
    // playerTexture = SDL_CreateTextureFromSurface(renderer, tempSurface);
    // if (!playerTexture) {
    //     std::cerr << "Failed to create player texture: " << SDL_GetError() <<
    //     std::endl; return;
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
                        } else if (event.packet->dataLength == sizeof(PositionPacket)) {
                            // This is a position update
                            PositionPacket* pos = (PositionPacket*)event.packet->data;
                            players[pos->playerID] = pos->state;
          } else if (event.packet->dataLength ==
                     sizeof(HitNotificationPacket)) {
            // This is a hit notification
            HitNotificationPacket *hit =
                (HitNotificationPacket *)event.packet->data;
            if (hit->targetID == playerID) {
              std::cout << "You were hit by player " << hit->shooterID << "!"
                        << std::endl;
              // Here you can add visual/audio feedback for being hit
            } else if (hit->shooterID == playerID) {
              std::cout << "You hit player " << hit->targetID << "!"
                        << std::endl;
              // Here you can add visual/audio feedback for successful hit
            }
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
    for (int i = 0; i < NUM_TEXTURES; i++) {
      SDL_DestroyTexture(wallTextures[i]);
    }
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
    IMG_Quit(); // ✅ Properly quit SDL2_Image
    enet_deinitialize();
    SDL_Quit();
  }
};

int main(int argc, char *args[]) {
  try {
    GameClient client;
    client.run();
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
