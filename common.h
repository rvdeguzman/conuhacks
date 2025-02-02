#pragma once
#include <enet/enet.h>
#include <cstdint>

enum PacketType
{
    PLAYER_POSITION = 1,
    PLAYER_INPUT
};

struct PlayerState
{
    double posX;
    double posY;
    double dirX;
    double dirY;
    double planeX;
    double planeY;

    PlayerState() : posX(2.0), posY(2.0),
                    dirX(-1.0), dirY(0.0),
                    planeX(0.0), planeY(0.66) {}
};

struct InputPacket
{
    uint8_t type = PLAYER_INPUT;
    bool up;
    bool down;
    bool left;
    bool right;
    double mouseRotation;
    bool shoot;
};

struct PositionPacket
{
    uint8_t type = PLAYER_POSITION;
    uint8_t playerID;
    PlayerState state;
};

struct Bullet
{
    double posX, posY; // Position of the bullet
    double dirX, dirY; // Direction of the bullet
    bool active;       // Whether the bullet is active
};

struct BulletPacket
{
    uint8_t type; // Optional: Add a packet type if needed
    double posX;
    double posY;
    double dirX;
    double dirY;
    bool active;
};

constexpr int MAP_WIDTH = 8;
constexpr int MAP_HEIGHT = 8;
const int worldMap[MAP_WIDTH][MAP_HEIGHT] = {
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 0, 0, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 0, 0, 1, 0, 1},
    {1, 0, 1, 0, 0, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1}};
