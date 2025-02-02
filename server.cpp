#include "common.h"
#include <cmath>
#include <enet/enet.h>
#include <iostream>
#include <vector>

const int MAX_CLIENTS = 2;
const int PORT = 1234;

class GameServer {
private:
  ENetHost *server;
  std::vector<ENetPeer *> clients;
  std::vector<PlayerState> players;

  double lastTime;

  void updatePlayerState(size_t playerIndex, const InputPacket &input) {
    PlayerState &player = players[playerIndex];

        double prevX = player.posX;
        double prevY = player.posY;
        
    // Calculate delta time in seconds
    double currentTime = enet_time_get() / 1000.0; // Convert to seconds
    double deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    const double BASE_MOVE_SPEED = 6.0; // Units per second
    const double BASE_ROT_SPEED = 3.0;  // Radians per second

    // Apply delta time to make movement frame-rate independent
    const double moveSpeed = BASE_MOVE_SPEED * deltaTime;
    const double rotSpeed = BASE_ROT_SPEED * deltaTime;

    if (input.forward) {
      player.posX += player.dirX * moveSpeed;
      player.posY += player.dirY * moveSpeed;
    }
    if (input.backward) {
      player.posX -= player.dirX * moveSpeed;
      player.posY -= player.dirY * moveSpeed;
    }
    if (input.turnRight) {
      double oldDirX = player.dirX;
      player.dirX = player.dirX * cos(-rotSpeed) - player.dirY * sin(-rotSpeed);
      player.dirY = oldDirX * sin(-rotSpeed) + player.dirY * cos(-rotSpeed);
      double oldPlaneX = player.planeX;
      player.planeX =
          player.planeX * cos(-rotSpeed) - player.planeY * sin(-rotSpeed);
      player.planeY =
          oldPlaneX * sin(-rotSpeed) + player.planeY * cos(-rotSpeed);
    }
    if (input.turnLeft) {
      double oldDirX = player.dirX;
      player.dirX = player.dirX * cos(rotSpeed) - player.dirY * sin(rotSpeed);
      player.dirY = oldDirX * sin(rotSpeed) + player.dirY * cos(rotSpeed);
      double oldPlaneX = player.planeX;
      player.planeX =
          player.planeX * cos(rotSpeed) - player.planeY * sin(rotSpeed);
      player.planeY = oldPlaneX * sin(rotSpeed) + player.planeY * cos(rotSpeed);
    }
if (input.strafeRight) {
      // Move perpendicular to direction vector (rotated 90 degrees counterclockwise)
      player.posX += player.dirY * moveSpeed;
      player.posY -= player.dirX * moveSpeed;
    }
    if (input.strafeLeft) {
      // Move perpendicular to direction vector (rotated 90 degrees clockwise)
      player.posX -= player.dirY * moveSpeed;
      player.posY += player.dirX * moveSpeed;
    }
            player.isMoving = (player.posX != prevX || player.posY != prevY);

  }

public:
  GameServer() {
    if (enet_initialize() != 0) {
      throw std::runtime_error("Failed to initialize ENet");
    }

    lastTime = enet_time_get() / 1000.0;

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = PORT;

    server = enet_host_create(&address, MAX_CLIENTS, 2, 0, 0);
    if (!server) {
      throw std::runtime_error("Failed to create ENet server");
    }

    // Initialize starting positions for players
    PlayerState p1;
    p1.posX = 2.0;
    p1.posY = 2.0;
    p1.dirX = -1.0;
    p1.dirY = 0.0;
    p1.planeX = 0.0;
    p1.planeY = 0.66;

    PlayerState p2;
    p2.posX = 6.0;
    p2.posY = 6.0;
    p2.dirX = 1.0;
    p2.dirY = 0.0;
    p2.planeX = 0.0;
    p2.planeY = 0.66;
    players.push_back(p1);
    players.push_back(p2);
  }

  bool isPlayerHit(const PlayerState &shooter, const PlayerState &target) {
    // Calculate vector from shooter to target
    double dx = target.posX - shooter.posX;
    double dy = target.posY - shooter.posY;

    // Calculate distance
    double distance = sqrt(dx * dx + dy * dy);
    if (distance > 8.0)
      return false; // Maximum shooting distance

    // Normalize the direction vector
    double dirLength =
        sqrt(shooter.dirX * shooter.dirX + shooter.dirY * shooter.dirY);
    double normalizedDirX = shooter.dirX / dirLength;
    double normalizedDirY = shooter.dirY / dirLength;

    // Normalize the vector to target
    double normalizedDx = dx / distance;
    double normalizedDy = dy / distance;

    // Calculate dot product to get cosine of angle
    double dotProduct =
        normalizedDirX * normalizedDx + normalizedDirY * normalizedDy;

    // Check if target is within 30-degree cone (cos(30°) ≈ 0.866)
    return dotProduct > 0.866;
  }

  void handleShot(const ShotAttemptPacket &shotPacket,
                  std::vector<PlayerState> &players) {
    if (shotPacket.shooterID >= players.size())
      return;

    // Update shooter's state with the position from packet
    PlayerState shooter = players[shotPacket.shooterID];
    shooter.posX = shotPacket.shooterPosX;
    shooter.posY = shotPacket.shooterPosY;
    shooter.dirX = shotPacket.shooterDirX;
    shooter.dirY = shotPacket.shooterDirY;

    // Check for hits on other players
    for (size_t i = 0; i < players.size(); i++) {
      if (i != shotPacket.shooterID) {
        if (isPlayerHit(shooter, players[i])) {
          // Player was hit! Send hit notification to all clients
          HitNotificationPacket hitPacket;
          hitPacket.shooterID = shotPacket.shooterID;
          hitPacket.targetID = i;

          // Broadcast hit notification to all clients
          ENetPacket *packet =
              enet_packet_create(&hitPacket, sizeof(HitNotificationPacket),
                                 ENET_PACKET_FLAG_RELIABLE);
          enet_host_broadcast(server, 0, packet);

          std::cout << "Player " << shotPacket.shooterID << " hit player " << i
                    << std::endl;
        }
      }
    }
  }
  void run() {
    std::cout << "Server running on port " << PORT << std::endl;

    while (true) {
      ENetEvent event;
      while (enet_host_service(server, &event, 10) > 0) {
        switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT: {
          std::cout << "Client connected from " << event.peer->address.host
                    << ":" << event.peer->address.port << std::endl;

          // Find first available player slot
          size_t newPlayerID = clients.size();
          clients.push_back(event.peer);
          event.peer->data = (void *)newPlayerID;

          // Send the player their ID
          uint8_t idPacket = (uint8_t)newPlayerID;
          ENetPacket *packet = enet_packet_create(&idPacket, sizeof(uint8_t),
                                                  ENET_PACKET_FLAG_RELIABLE);
          enet_peer_send(event.peer, 0, packet);

          // Send initial positions of all players to the new client
          for (size_t i = 0; i < players.size(); i++) {
            PositionPacket posPacket;
            posPacket.playerID = i;
            posPacket.state = players[i];

            packet = enet_packet_create(&posPacket, sizeof(PositionPacket),
                                        ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(event.peer, 0, packet);
          }

          break;
        }
        case ENET_EVENT_TYPE_RECEIVE: {
          if (event.packet->dataLength == sizeof(InputPacket)) {
            InputPacket *input = (InputPacket *)event.packet->data;
            size_t playerIndex = (size_t)event.peer->data;
            updatePlayerState(playerIndex, *input);
          } else if (event.packet->dataLength == sizeof(ShotAttemptPacket)) {
            ShotAttemptPacket *shotPacket =
                (ShotAttemptPacket *)event.packet->data;
            handleShot(*shotPacket, players);
          }
          // Broadcast updated positions to all clients
          for (size_t i = 0; i < players.size(); i++) {
            PositionPacket posPacket;
            posPacket.playerID = i;
            posPacket.state = players[i];

            ENetPacket *packet = enet_packet_create(
                &posPacket, sizeof(PositionPacket), ENET_PACKET_FLAG_RELIABLE);
            enet_host_broadcast(server, 0, packet);
          }

          enet_packet_destroy(event.packet);
          break;
        }
        case ENET_EVENT_TYPE_DISCONNECT: {
          std::cout << "Client disconnected" << std::endl;
          size_t playerIndex = (size_t)event.peer->data;
          clients[playerIndex] = nullptr;

          // Reset the disconnected player's position
          players[playerIndex] = PlayerState();

          // Notify other clients about the disconnection
          PositionPacket posPacket;
          posPacket.playerID = playerIndex;
          posPacket.state = players[playerIndex];

          ENetPacket *packet = enet_packet_create(
              &posPacket, sizeof(PositionPacket), ENET_PACKET_FLAG_RELIABLE);
          enet_host_broadcast(server, 0, packet);
          break;
        }
        default:
          break;
        }
      }
    }
  }

  ~GameServer() {
    enet_host_destroy(server);
    enet_deinitialize();
  }
};

int main() {
  try {
    GameServer server;
    server.run();
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
