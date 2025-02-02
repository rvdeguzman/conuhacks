
#pragma once
#include <cstdint>
#include <enet/enet.h>

enum PacketType { PLAYER_POSITION = 1, PLAYER_INPUT };

struct PlayerState {
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
