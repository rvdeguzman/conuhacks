#include "common.h"
#include <cmath>
#include <enet/enet.h>
#include <iostream>
#include <vector>

const int MAX_CLIENTS = 2;
const int PORT = 1234;
PlayerState p1;
PlayerState p2;


class GameServer {
private:
  ENetHost *server;
  std::vector<ENetPeer *> clients;
  std::vector<PlayerState> players;
  const double PLAYER_RADIUS = 0.2; // Collision radius for players
  const double WALL_BUFFER = 0.1;   // Extra buffer space from walls

  double lastTime;

  bool checkCollision(double x, double y, size_t currentPlayerIndex) {
    // Check map boundaries
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) {
      return true;
    }

    // Check the 4 cells around the player's position (including buffer)
    int minX = static_cast<int>(x - PLAYER_RADIUS - WALL_BUFFER);
    int maxX = static_cast<int>(x + PLAYER_RADIUS + WALL_BUFFER);
    int minY = static_cast<int>(y - PLAYER_RADIUS - WALL_BUFFER);
    int maxY = static_cast<int>(y + PLAYER_RADIUS + WALL_BUFFER);

    // Clamp to map boundaries
    minX = std::max(0, minX);
    maxX = std::min(MAP_WIDTH - 1, maxX);
    minY = std::max(0, minY);
    maxY = std::min(MAP_HEIGHT - 1, maxY);

    // Check each cell in the area
    for (int checkX = minX; checkX <= maxX; checkX++) {
      for (int checkY = minY; checkY <= maxY; checkY++) {
        if (worldMap[checkX][checkY] > 0) { // If there's a wall
          // Calculate detailed collision with wall boundaries
          double wallMinX = checkX;
          double wallMaxX = checkX + 1.0;
          double wallMinY = checkY;
          double wallMaxY = checkY + 1.0;

          // Check if player's collision circle intersects with wall square
          double closestX = std::max(wallMinX, std::min(wallMaxX, x));
          double closestY = std::max(wallMinY, std::min(wallMaxY, y));

          double distanceX = x - closestX;
          double distanceY = y - closestY;
          double distanceSquared =
              (distanceX * distanceX) + (distanceY * distanceY);

          if (distanceSquared <
              (PLAYER_RADIUS + WALL_BUFFER) * (PLAYER_RADIUS + WALL_BUFFER)) {
            return true; // Collision detected
          }
        }
      }
    }

    // Check collision with other players
    for (size_t i = 0; i < players.size(); i++) {
      // Skip checking collision with self
      if (i == currentPlayerIndex)
        continue;

      const PlayerState &otherPlayer = players[i];

      // Quick AABB check first for performance
      if (std::abs(otherPlayer.posX - x) < PLAYER_RADIUS * 2 &&
          std::abs(otherPlayer.posY - y) < PLAYER_RADIUS * 2) {

        // More precise circle collision check
        double dx = otherPlayer.posX - x;
        double dy = otherPlayer.posY - y;
        double distanceSquared = dx * dx + dy * dy;

        if (distanceSquared < (PLAYER_RADIUS * 2) * (PLAYER_RADIUS * 2)) {
          return true; // Player collision detected
        }
      }
    }

    return false; // No collision
  }

  void updatePlayerState(size_t playerIndex, const InputPacket &input) {
    PlayerState &player = players[playerIndex];
    double prevX = player.posX;
    double prevY = player.posY;

    // Calculate delta time in seconds
    double currentTime = enet_time_get() / 1000.0;
    double deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    const double BASE_MOVE_SPEED = 6.0;
    const double BASE_ROT_SPEED = 3.0;

    const double moveSpeed = BASE_MOVE_SPEED * deltaTime;
    const double rotSpeed = BASE_ROT_SPEED * deltaTime;

    // Store original position for collision resolution
    double newX = player.posX;
    double newY = player.posY;

    if (input.mouseRotation != 0.0) {
      double rotAmount =
          -input.mouseRotation; // Negative because screen coordinates
      double oldDirX = player.dirX;
      player.dirX = player.dirX * cos(rotAmount) - player.dirY * sin(rotAmount);
      player.dirY = oldDirX * sin(rotAmount) + player.dirY * cos(rotAmount);
      double oldPlaneX = player.planeX;
      player.planeX =
          player.planeX * cos(rotAmount) - player.planeY * sin(rotAmount);
      player.planeY =
          oldPlaneX * sin(rotAmount) + player.planeY * cos(rotAmount);
    }

    // Handle rotation (no collision check needed)
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

    // Handle movement with collision detection
    if (input.forward) {
      std::cout << "moving forward" << std::endl;
      newX = player.posX + player.dirX * moveSpeed;
      newY = player.posY + player.dirY * moveSpeed;
    }
    if (input.backward) {
      newX = player.posX - player.dirX * moveSpeed;
      newY = player.posY - player.dirY * moveSpeed;
    }
    if (input.strafeRight) {
      newX = player.posX + player.dirY * moveSpeed;
      newY = player.posY - player.dirX * moveSpeed;
    }
    if (input.strafeLeft) {
      newX = player.posX - player.dirY * moveSpeed;
      newY = player.posY + player.dirX * moveSpeed;
    }

    // Try to move with collision detection
    // First try the full movement
    if (!checkCollision(newX, newY, playerIndex)) {
      player.posX = newX;
      player.posY = newY;
    } else {
      // If collision, try moving along X axis only
      if (!checkCollision(newX, player.posY, playerIndex)) {
        player.posX = newX;
      }
      // Try moving along Y axis only
      else if (!checkCollision(player.posX, newY, playerIndex)) {
        player.posY = newY;
      }
      // If both failed, player stays in current position
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
    p1.posX = 7.0;
    p1.posY = 7.0;
    // PlayerState p1;
    // p1.name = "Player 1";
    p1.isAdmin = true;
    p1.dirX = -1.0;
    p1.dirY = 0.0;
    p1.planeX = 0.0;
    p1.planeY = 0.66;

    PlayerState p2;
    p2.posX = 14.0;
    p2.posY = 14.0;
    p2.isAdmin = false;
    p2.dirX = 1.0;
    p2.dirY = 0.0;
    p2.planeX = 0.0;
    p2.planeY = 0.66;
    // players.push_back(p1);
    // players.push_back(p2);
  }

  bool hasWallBetweenPoints(double startX, double startY, double endX,
                            double endY) {
    // Implementation of Digital Differential Analyzer (DDA) algorithm
    double dirX = endX - startX;
    double dirY = endY - startY;
    double distance = sqrt(dirX * dirX + dirY * dirY);

    // Normalize direction vector
    dirX /= distance;
    dirY /= distance;

    // Starting map cell
    int mapX = int(startX);
    int mapY = int(startY);

    // Length of ray from one x or y-side to next x or y-side
    double deltaDistX = std::abs(1.0 / dirX);
    double deltaDistY = std::abs(1.0 / dirY);

    // Calculate step and initial sideDist
    double sideDistX, sideDistY;
    int stepX, stepY;

    if (dirX < 0) {
      stepX = -1;
      sideDistX = (startX - mapX) * deltaDistX;
    } else {
      stepX = 1;
      sideDistX = (mapX + 1.0 - startX) * deltaDistX;
    }

    if (dirY < 0) {
      stepY = -1;
      sideDistY = (startY - mapY) * deltaDistY;
    } else {
      stepY = 1;
      sideDistY = (mapY + 1.0 - startY) * deltaDistY;
    }

    // Perform DDA
    double rayLength = 0.0;
    while (rayLength < distance) {
      // Jump to next map square
      if (sideDistX < sideDistY) {
        rayLength = sideDistX;
        sideDistX += deltaDistX;
        mapX += stepX;
      } else {
        rayLength = sideDistY;
        sideDistY += deltaDistY;
        mapY += stepY;
      }

      // Check if ray has hit a wall
      if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT) {
        return true; // Hit map boundary
      }

      if (worldMap[mapX][mapY] > 0) {
        return true; // Hit a wall
      }
    }

    return false; // No walls between points
  }

  bool isPlayerHit(const PlayerState &shooter, const PlayerState &target) {
    // Calculate vector from shooter to target
    double dx = target.posX - shooter.posX;
    double dy = target.posY - shooter.posY;

    // Calculate distance
    double distance = sqrt(dx * dx + dy * dy);
    if (distance > 8.0)
      return false; // Maximum shooting distance

    // Normalize direction vectors
    double normalizedToTargetX = dx / distance;
    double normalizedToTargetY = dy / distance;

    double dirLength =
        sqrt(shooter.dirX * shooter.dirX + shooter.dirY * shooter.dirY);
    double normalizedDirX = shooter.dirX / dirLength;
    double normalizedDirY = shooter.dirY / dirLength;

    // Calculate dot product to get angle
    double dotProduct = normalizedDirX * normalizedToTargetX +
                        normalizedDirY * normalizedToTargetY;
    if (dotProduct <= 0.984)
      return false; // cos(10°) ≈ 0.984

    // Now check for walls using a more precise approach
    double currX = shooter.posX;
    double currY = shooter.posY;
    const double STEP_SIZE = 0.1; // Small steps for precise collision

    // Calculate step vectors
    double stepX = normalizedToTargetX * STEP_SIZE;
    double stepY = normalizedToTargetY * STEP_SIZE;

    // Check points along the line between shooter and target
    int numSteps = static_cast<int>(distance / STEP_SIZE);

    for (int i = 0; i < numSteps; i++) {
      // Move along the line
      currX += stepX;
      currY += stepY;

      // Get current map cell
      int mapX = static_cast<int>(currX);
      int mapY = static_cast<int>(currY);

      // Check if we've reached the target (with some tolerance)
      double distToTarget =
          sqrt(pow(target.posX - currX, 2) + pow(target.posY - currY, 2));
      if (distToTarget < PLAYER_RADIUS) {
        return true; // Hit the target!
      }

      // Check for wall collision
      if (mapX >= 0 && mapX < MAP_WIDTH && mapY >= 0 && mapY < MAP_HEIGHT) {
        if (worldMap[mapX][mapY] > 0) {
          // Additional check for corner cases
          // If we're very close to the target when we hit a wall, still count
          // it as a hit
          if (distToTarget < PLAYER_RADIUS * 2) {
            return true;
          }
          return false; // Hit a wall
        }
      }
    }

    // If we got here, we're in range and no walls are in the way
    return true;
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
            // std::cout << "📩 Received Packet of type: " << event.type << std::endl;
            // std::cout << "ENET_EVENT_TYPE_CONNECT: " << ENET_EVENT_TYPE_CONNECT << std::endl;
            // std::cout << "ENET_EVENT_TYPE_RECEIVE: " << ENET_EVENT_TYPE_RECEIVE << std::endl;
            // std::cout << "ENET_EVENT_TYPE_DISCONNECT: " << ENET_EVENT_TYPE_DISCONNECT << std::endl;
        switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT: {
          std::cout << "Client connected from " << event.peer->address.host
                    << ":" << event.peer->address.port << std::endl;

            switch (players.size()) {
                case 0: {
                    players.push_back(p1);
                    // std::cout << "1 player" << std::endl;
                    break;
                }
                case 1: {
                    players.push_back(p2);
                    // std::cout << "2 players" << std::endl;
                    break;
                }
            }

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

            std::cout << "Sending PositionPacket - Player ID: " << (int)posPacket.playerID 
                  << " | X: " << posPacket.state.posX
                  << " | Y: " << posPacket.state.posY << std::endl;

            packet = enet_packet_create(&posPacket, sizeof(PositionPacket),
                                        ENET_PACKET_FLAG_RELIABLE);

            if (!packet) {
                std::cerr << "Error: Failed to create position packet!" << std::endl;
                continue;
            }

            enet_peer_send(event.peer, 0, packet);
          }

          break;
        }
        case ENET_EVENT_TYPE_RECEIVE: {
            InputPacket* input = (InputPacket*)event.packet->data;
            size_t playerIndex = (size_t)event.peer->data;
            updatePlayerState(playerIndex, *input);
            
            // Broadcast updated positions to all clients
            for (size_t i = 0; i < players.size(); i++) {
                PositionPacket posPacket;
                posPacket.playerID = i;
                posPacket.state = players[i];
                
                ENetPacket* packet = enet_packet_create(
                    &posPacket, 
                    sizeof(PositionPacket), 
                    ENET_PACKET_FLAG_RELIABLE
                );
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

  void broadcastLobbyUpdate() {
    if (players.empty()) {
        std::cerr << "Error: No players in the lobby!" << std::endl;
        return;
    }

    LobbyUpdatePacket lobbyPacket;
    lobbyPacket.numPlayers = players.size();

    std::cout << "Broadcasting lobby update with " << (int)lobbyPacket.numPlayers << " players." << std::endl;

    for (size_t i = 0; i < players.size(); i++) {
        lobbyPacket.players[i] = players[i];  // ✅ Now only sending IDs and positions
    }

    ENetPacket* packet = enet_packet_create(&lobbyPacket, sizeof(LobbyUpdatePacket), ENET_PACKET_FLAG_RELIABLE);
    if (!packet) {
        std::cerr << "Error: Failed to create lobby update packet!" << std::endl;
        return;
    }

    enet_host_broadcast(server, 0, packet);
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
