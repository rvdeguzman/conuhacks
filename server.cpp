#include <enet/enet.h>
#include <iostream>
#include <vector>
#include <cmath>
#include "common.h"

const int MAX_CLIENTS = 2;
const int PORT = 1234;

class GameServer
{
private:
    ENetHost *server;
    std::vector<ENetPeer *> clients;
    std::vector<PlayerState> players;
    std::vector<Bullet> bullets;

    void updatePlayerState(size_t playerIndex, const InputPacket &input)
    {
        PlayerState &player = players[playerIndex];
        const double moveSpeed = 0.05;
        const double rotSpeed = 0.03;

        if (input.up)
        {
            player.posX += player.dirX * moveSpeed;
            player.posY += player.dirY * moveSpeed;
        }
        if (input.down)
        {
            player.posX -= player.dirX * moveSpeed;
            player.posY -= player.dirY * moveSpeed;
        }
        if (input.right)
        {
            double oldDirX = player.dirX;
            player.dirX = player.dirX * cos(-rotSpeed) - player.dirY * sin(-rotSpeed);
            player.dirY = oldDirX * sin(-rotSpeed) + player.dirY * cos(-rotSpeed);
            double oldPlaneX = player.planeX;
            player.planeX = player.planeX * cos(-rotSpeed) - player.planeY * sin(-rotSpeed);
            player.planeY = oldPlaneX * sin(-rotSpeed) + player.planeY * cos(-rotSpeed);
        }
        if (input.left)
        {
            double oldDirX = player.dirX;
            player.dirX = player.dirX * cos(rotSpeed) - player.dirY * sin(rotSpeed);
            player.dirY = oldDirX * sin(rotSpeed) + player.dirY * cos(rotSpeed);
            double oldPlaneX = player.planeX;
            player.planeX = player.planeX * cos(rotSpeed) - player.planeY * sin(rotSpeed);
            player.planeY = oldPlaneX * sin(rotSpeed) + player.planeY * cos(rotSpeed);
        }
        if (input.shoot)
        {
            Bullet bullet;
            bullet.posX = players[playerIndex].posX;
            bullet.posY = players[playerIndex].posY;
            bullet.dirX = players[playerIndex].dirX;
            bullet.dirY = players[playerIndex].dirY;
            bullet.active = true;
            bullets.push_back(bullet);
        }
    }

    void updateBullets()
    {
        const double bulletSpeed = 0.1;
        for (auto &bullet : bullets)
        {
            if (!bullet.active)
                continue;

            bullet.posX += bullet.dirX * bulletSpeed;
            bullet.posY += bullet.dirY * bulletSpeed;

            // Check for collision with walls
            if (worldMap[int(bullet.posX)][int(bullet.posY)] == 1)
            {
                bullet.active = false;
            }

            // Check for collision with players
            for (size_t i = 0; i < players.size(); i++)
            {
                if (std::hypot(players[i].posX - bullet.posX, players[i].posY - bullet.posY) < 0.5)
                {
                    bullet.active = false;
                    // Handle player hit (e.g., reduce health)
                }
            }
        }
    }

    void broadcastBulletStates()
    {
        for (const auto &bullet : bullets)
        {
            if (!bullet.active)
                continue;

            BulletPacket bulletPacket;
            bulletPacket.posX = bullet.posX;
            bulletPacket.posY = bullet.posY;
            bulletPacket.dirX = bullet.dirX;
            bulletPacket.dirY = bullet.dirY;

            ENetPacket *packet = enet_packet_create(
                &bulletPacket,
                sizeof(BulletPacket),
                ENET_PACKET_FLAG_UNSEQUENCED);
            enet_host_broadcast(server, 0, packet);
        }
    }

public:
    GameServer()
    {
        if (enet_initialize() != 0)
        {
            throw std::runtime_error("Failed to initialize ENet");
        }

        ENetAddress address;
        address.host = ENET_HOST_ANY;
        address.port = PORT;

        server = enet_host_create(&address, MAX_CLIENTS, 2, 0, 0);
        if (!server)
        {
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

    void run()
    {
        std::cout << "Server running on port " << PORT << std::endl;

        while (true)
        {
            ENetEvent event;
            while (enet_host_service(server, &event, 10) > 0)
            {
                switch (event.type)
                {
                case ENET_EVENT_TYPE_CONNECT:
                {
                    std::cout << "Client connected from "
                              << event.peer->address.host << ":"
                              << event.peer->address.port << std::endl;
                    clients.push_back(event.peer);
                    event.peer->data = (void *)(clients.size() - 1);
                    break;
                }
                case ENET_EVENT_TYPE_RECEIVE:
                {
                    InputPacket *input = (InputPacket *)event.packet->data;
                    size_t playerIndex = (size_t)event.peer->data;
                    updatePlayerState(playerIndex, *input);

                    // Broadcast updated positions to all clients
                    for (size_t i = 0; i < players.size(); i++)
                    {
                        PositionPacket posPacket;
                        posPacket.playerID = i;
                        posPacket.state = players[i];

                        ENetPacket *packet = enet_packet_create(
                            &posPacket,
                            sizeof(PositionPacket),
                            ENET_PACKET_FLAG_RELIABLE);
                        enet_host_broadcast(server, 0, packet);
                    }

                    enet_packet_destroy(event.packet);
                    break;
                }
                case ENET_EVENT_TYPE_DISCONNECT:
                {
                    std::cout << "Client disconnected" << std::endl;
                    size_t playerIndex = (size_t)event.peer->data;
                    clients[playerIndex] = nullptr;
                    break;
                }
                default:
                    break;
                }
            }
            updateBullets();
            broadcastBulletStates();
        }
    }

    ~GameServer()
    {
        enet_host_destroy(server);
        enet_deinitialize();
    }
};

int main()
{
    try
    {
        GameServer server;
        server.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
