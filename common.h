#pragma once
#include <enet/enet.h>
#include <cstdint>

enum PacketType {
    PLAYER_POSITION = 1,
    PLAYER_INPUT
};

struct PlayerState {
    double posX;
    double posY;
    double dirX;
    double dirY;
    double planeX;
    double planeY;

    PlayerState() : 
        posX(2.0), posY(2.0),
        dirX(-1.0), dirY(0.0),
        planeX(0.0), planeY(0.66) {}
};

struct InputPacket {
    uint8_t type = PLAYER_INPUT;
    bool up;
    bool down;
    bool left;
    bool right;
    double mouseRotation;
};

struct PositionPacket {
    uint8_t type = PLAYER_POSITION;
    uint8_t playerID;
    PlayerState state;
};

struct Bullet {
    double posX, posY;  // Position of the bullet
    double dirX, dirY;  // Direction of the bullet
    bool active;        // Whether the bullet is active
};

