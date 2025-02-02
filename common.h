
#pragma once
#include <cstdint>
#include <enet/enet.h>
#include <SDL2/SDL.h>
#include <string>

enum PacketType { PLAYER_POSITION = 1, PLAYER_INPUT };

const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 768;

const SDL_Keycode ENTER_LOBBY_INPUT = SDLK_PLUS;
const SDL_Keycode START_GAME_INPUT = SDLK_RETURN;
const SDL_Keycode END_GAME_INPUT = SDLK_ESCAPE;


// struct Player {
//       // Indicate if the player is an admin
//     double posX, posY; // Position on the screen
//     // Other player data like team, score, etc.
// };

struct PlayerState {
    // char name[32];  // ✅ Use fixed-size array instead of std::string
    bool isAdmin;

    double posX;
    double posY;
    double dirX;
    double dirY;
    double planeX;
    double planeY;
    bool isMoving;

    PlayerState() : 
        posX(2.0), posY(2.0),
        dirX(-1.0), dirY(0.0),
        planeX(0.0), planeY(0.66),
        isMoving(false) {}
};

struct InputPacket {
  uint8_t type = PLAYER_INPUT;
  bool forward;
  bool backward;
  bool strafeLeft;
  bool strafeRight;
  bool turnLeft;
  bool turnRight;
  double mouseRotation;
};

struct ShotAttemptPacket {
  size_t shooterID;
  double shooterPosX;
  double shooterPosY;
  double shooterDirX;
  double shooterDirY;
};

struct HitNotificationPacket {
  size_t shooterID;
  size_t targetID;
};

struct PositionPacket {
  uint8_t type = PLAYER_POSITION;
  uint8_t playerID;
  PlayerState state;
};

// Packet to update the lobby with players' info
struct LobbyUpdatePacket {
    uint8_t numPlayers;  // Number of players in the lobby
    PlayerState players[4];    // Array of players in the lobby
};

// Packet to start the game
struct GameStartPacket {
    bool startGame;  // Whether to start the game or not
};