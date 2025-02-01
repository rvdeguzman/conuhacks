
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
};

struct InputPacket {
    uint8_t type = PLAYER_INPUT;
    bool up;
    bool down;
    bool left;
    bool right;
};

struct PositionPacket {
    uint8_t type = PLAYER_POSITION;
    uint8_t playerID;
    PlayerState state;
};
