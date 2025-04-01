#include <iostream>
#include <SFML/Network/Packet.hpp>
#include "NetworkProtocol.hpp"
#include <SFML/System/Sleep.hpp>
#include "Utility.hpp"
#include "PickupType.hpp"
#include "ShipType.hpp"

GameServer::GameServer(sf::Vector2f battlefield_size)
    : m_thread(&GameServer::ExecutionThread, this)
    , m_listening_state(false)
    , m_client_timeout(sf::seconds(2.f))
    , m_max_connected_players(16)
    , m_connected_players(0)
    , m_world_height(5000.f)
    , m_battlefield_rect(0.f, m_world_height - battlefield_size.y, battlefield_size.x, battlefield_size.y)
    , m_battlefield_scrollspeed(-30.f)
    , m_ship_count(16)
    , m_peers(1)
    , m_ship_identifier_counter(1)
    , m_waiting_thread_end(false)
    , m_last_spawn_time(sf::Time::Zero)
    , m_time_for_next_spawn(sf::seconds(5.f))
    , m_game_started(false)
    //,m_in_lobby(true)
{
    m_listener_socket.setBlocking(false);
    m_peers[0].reset(new RemotePeer());
    m_thread.launch();
}

GameServer::~GameServer()
{
    m_waiting_thread_end = true;
    m_thread.wait();
}

void GameServer::NotifyPlayerSpawn(sf::Int32 ship_identifier)
{
    sf::Packet packet;
    // Use kPlayerConnect for the broadcast; individual connections already get their tailored spawn packet.
    packet << static_cast<sf::Int32>(Server::PacketType::kPlayerConnect);

    // Compute the player ID.
    sf::Int8 player_id = (m_connected_players == 0) ? 1 : (m_connected_players + 1);

    // Determine the role based on your ShipInfo.
    sf::Int8 role = (!m_ship_info[ship_identifier].HasPilot()) ?
        static_cast<sf::Int8>(Role::Pilot) :
        static_cast<sf::Int8>(Role::Gunner);

    // Build the packet with only the necessary information.
    packet << player_id           // New player's id.
        << ship_identifier     // Ship id.
        << role                // Role: Pilot (0) or Gunner (1).
        << m_ship_info[ship_identifier].m_position.x   // Ship position x.
        << m_ship_info[ship_identifier].m_position.y;  // Ship position y.

    SendToAll(packet);
}


void GameServer::NotifyPlayerRealtimeChange(sf::Int32 ship_identifier, sf::Int32 action, bool action_enabled)
{
    sf::Packet packet;
    //First thing in every packets is what type of packet it is
    packet << static_cast<sf::Int32>(Server::PacketType::kPlayerRealtimeChange);
    packet << ship_identifier;
    packet << action;
    packet << action_enabled;
    SendToAll(packet);
}

void GameServer::NotifyPlayerEvent(sf::Int32 ship_identifier, sf::Int32 action)
{
    sf::Packet packet;
    //First thing in every packets is what type of packet it is
    packet << static_cast<sf::Int32>(Server::PacketType::kPlayerEvent);
    packet << ship_identifier;
    packet << action;
    SendToAll(packet);
}

void GameServer::SetListening(bool enable)
{
    //Check is the server is already listening
    if (enable)
    {
        if (!m_listening_state)
        {
            m_listening_state = (m_listener_socket.listen(SERVER_PORT) == sf::TcpListener::Done);
        }
    }
    else
    {
        m_listener_socket.close();
        m_listening_state = false;
    }
}

void GameServer::ExecutionThread()
{
    //Initialisation
    SetListening(true);

    sf::Time frame_rate = sf::seconds(1.f / 60.f);
    sf::Time frame_time = sf::Time::Zero;
    sf::Time tick_rate = sf::seconds(1.f / 20.f);
    sf::Time tick_time = sf::Time::Zero;
    sf::Clock frame_clock, tick_clock;

    while (!m_waiting_thread_end)
    {
        //This is the game loop
        HandleIncomingConnections();
        HandleIncomingPackets();

        frame_time += frame_clock.getElapsedTime();
        frame_clock.restart();

        tick_time += tick_clock.getElapsedTime();
        tick_clock.restart();

        //Fixed time step
        while (frame_time >= frame_rate)
        {
            m_battlefield_rect.top += m_battlefield_scrollspeed * frame_rate.asSeconds();
            frame_time -= frame_rate;
        }

        //Fixed time step
        while (tick_time >= tick_rate)
        {
            Tick();
            tick_time -= tick_rate;
        }
        //sleep
        sf::sleep(sf::milliseconds(50));
    }
}

void GameServer::Tick()
{
    UpdateClientState();

    //Check if the game is over = all planes postion.y < offset

    bool all_aircraft_done = true;
    for (const auto& current : m_ship_info)
    {
        //As long as one player has not crossed the finish line the game is still live
        if (current.second.m_position.y > 0.f)
        {
            all_aircraft_done = false;
            break;
        }
    }

    if (all_aircraft_done)
    {
        sf::Packet mission_success_packet;
        mission_success_packet << static_cast<sf::Int32>(Server::PacketType::kMissionSuccess);
        SendToAll(mission_success_packet);
    }

    //Remove aircraft that have been destroyed
    for (auto itr = m_ship_info.begin(); itr != m_ship_info.end();)
    {
        if (itr->second.m_hitpoints <= 0)
        {
            m_ship_info.erase(itr++);
        }
        else
        {
            ++itr;
        }
    }

    //Check if it is time to spawn enemies
    if (Now() >= m_time_for_next_spawn + m_last_spawn_time)
    {
        //Not going to spawn enemies near the end
        if (m_battlefield_rect.top > 600.f)
        {
            std::size_t enemy_count = 1 + Utility::RandomInt(2);
            float spawn_centre = static_cast<float>(Utility::RandomInt(500) - 250);

            //If there is only one enemy it is at spawn_centre
            float plane_distance = 0.f;
            float next_spawn_position = spawn_centre;

            //If there are two enemies they should be centred on the spawn centre
            if (enemy_count == 2)
            {
                plane_distance = static_cast<float>(150 + Utility::RandomInt(250));
                next_spawn_position = spawn_centre - plane_distance / 2.f;
            }

            //Send the spawn packets to the clients
            for (std::size_t i = 0; i < enemy_count; ++i)
            {
                sf::Packet packet;
                packet << static_cast<sf::Int32>(Server::PacketType::kSpawnEnemy);
                packet << static_cast<sf::Int32>(1 + Utility::RandomInt(static_cast<int>(ShipType::kShipCount) - 1));

                packet << m_world_height - m_battlefield_rect.top + 500;
                packet << next_spawn_position;

                next_spawn_position += plane_distance / 2.f;
                SendToAll(packet);
            }
            m_last_spawn_time = Now();
            m_time_for_next_spawn = sf::milliseconds(2000 + Utility::RandomInt(6000));
        }
    }
}

sf::Time GameServer::Now() const
{
    return m_clock.getElapsedTime();
}

void GameServer::HandleIncomingPackets()
{
    bool detected_timeout = false;

    for (PeerPtr& peer : m_peers)
    {
        if (peer->m_ready)
        {
            sf::Packet packet;
            while (peer->m_socket.receive(packet) == sf::Socket::Done)
            {
                //Interpret the packet and react to it
                HandleIncomingPackets(packet, *peer, detected_timeout);

                peer->m_last_packet_time = Now();
                packet.clear();
            }

            if (Now() > peer->m_last_packet_time + m_client_timeout)
            {
                peer->m_timed_out = true;
                detected_timeout = true;
            }

        }
    }

    if (detected_timeout)
    {
        HandleDisconnections();
    }
}

void GameServer::PlayerEvent(sf::Packet& packet)
{
    sf::Int32 aircraft_identifier;
    sf::Int32 action;
    packet >> aircraft_identifier >> action;
    NotifyPlayerEvent(aircraft_identifier, action);
}

void GameServer::RealTimeChange(sf::Packet& packet)
{
    sf::Int32 aircraft_identifier;
    sf::Int32 action;
    bool action_enabled;
    packet >> aircraft_identifier >> action >> action_enabled;
    NotifyPlayerRealtimeChange(aircraft_identifier, action, action_enabled);
}


void GameServer::StateUpdate(sf::Packet& packet)
{
    sf::Int32 num_aircraft;
    packet >> num_aircraft;

    for (sf::Int32 i = 0; i < num_aircraft; ++i)
    {
        sf::Int32 aircraft_identifier;
        sf::Int32 aircraft_hitpoints;
        sf::Int32 missile_ammo;
        sf::Vector2f aircraft_position;
        packet >> aircraft_identifier >> aircraft_position.x >> aircraft_position.y >> aircraft_hitpoints >> missile_ammo;
        m_ship_info[aircraft_identifier].m_position = aircraft_position;
        m_ship_info[aircraft_identifier].m_hitpoints = aircraft_hitpoints;
        m_ship_info[aircraft_identifier].m_missile_ammo = missile_ammo;
    }
}

void GameServer::GameEvent(sf::Packet& packet, GameServer::RemotePeer& receiving_peer)
{
    sf::Int32 action;
    float x;
    float y;

    packet >> action;
    packet >> x;
    packet >> y;

    NotifyPlayerEvent(receiving_peer.m_identifier, action);

    //Enemy explodes, with a certain probability, drop a pickup
    //To avoid multiple messages only listen to the first peer (host)
    if (action == GameActions::kEnemyExplode && Utility::RandomInt(3) == 0 && &receiving_peer == m_peers[0].get())
    {
        sf::Packet packet;
        packet << static_cast<sf::Int32>(Server::PacketType::kSpawnPickup);
        packet << static_cast<sf::Int32>(Utility::RandomInt(static_cast<int>(PickupType::kPickupCount)));
        packet << x;
        packet << y;

        SendToAll(packet);
    }
}

void GameServer::NotifyTeamChange(sf::Int8 id, sf::Int8 ship_id, sf::Int8 gunner_id, sf::Int8 pilot_id)
{
    sf::Packet packet;
    packet << static_cast<sf::Int32>(Server::PacketType::kTeamSelection);
    packet << id << ship_id << gunner_id << pilot_id;
    Utility::Debug("Team Change: ");
    SendToAll(packet);
}

void GameServer::PlayerTeamChange(sf::Packet& packet)
{
    sf::Int8 id;
    sf::Int8 ship_id;
    sf::Int8 gunner_id;
    sf::Int8 pilot_id;
    packet >> id >> ship_id >> gunner_id >> pilot_id;

    m_ship_info[id].m_ship_id = ship_id;
    m_ship_info[id].m_gunner_id = gunner_id;
    m_ship_info[id].m_pilot_id = pilot_id;

    NotifyTeamChange(id, ship_id, gunner_id, pilot_id);
}

void GameServer::HandlePlayerUpdate(sf::Packet& packet)
{
    sf::Int8 aircraft_identifier;
    sf::Int8 ship_id;
    sf::Int8 gunner_id;
    sf::Int8 pilot_id;
    packet >> aircraft_identifier >> ship_id >> gunner_id >> pilot_id;

    m_ship_info[aircraft_identifier].m_ship_id = ship_id;
    m_ship_info[aircraft_identifier].m_gunner_id = gunner_id;
    m_ship_info[aircraft_identifier].m_pilot_id = pilot_id;
    sf::Packet notify_packet;
    notify_packet << static_cast<sf::Int32>(Server::PacketType::kPlayerUpdate);
    notify_packet << aircraft_identifier << gunner_id << pilot_id;

    SendToAll(notify_packet);
}

void GameServer::StartGameCountdownStart()
{
    sf::Packet packet;
    packet << static_cast<sf::Int32>(Server::PacketType::kStartNetworkGameCountdown);
    SendToAll(packet);
}

void GameServer::NotifyGameStart()
{
    m_game_started = true;

    // First, send a kGameStart packet to notify everyone.
    sf::Packet startPacket;
    startPacket << static_cast<sf::Int8>(Server::PacketType::kGameStart);
    SendToAll(startPacket);

    // For each connected peer, send a spawn packet with team (ship) info.
    for (int i = 0; i < m_connected_players; ++i)
    {
        if (m_peers[i]->m_ready)
        {
            sf::Packet packet;
            packet << static_cast<sf::Int8>(Server::PacketType::kSpawnSelf);

            // Get the local player's ID from the peer.
            sf::Int8 localPlayerId = m_peers[i]->m_identifier;
            packet << localPlayerId;  // Write local player id.

            // Find the ship that contains this player.
            sf::Int8 shipId = -1;
            for (const auto& pair : m_ship_info)
            {
                const ShipInfo& ship = pair.second;
                if (ship.m_pilot_id == localPlayerId || ship.m_gunner_id == localPlayerId)
                {
                    shipId = pair.first;
                    break;
                }
            }
            packet << shipId;  // Write ship id.

            if (shipId != -1)
            {
                // Get ship details.
                const ShipInfo& ship = m_ship_info[shipId];
                packet << ship.m_position.x << ship.m_position.y;
                packet << ship.m_pilot_id << ship.m_gunner_id;
            }
            else
            {
                // Fallback values if ship not found.
                packet << 0.f << 0.f;
                packet << static_cast<sf::Int8>(-1) << static_cast<sf::Int8>(-1);
            }

            // Send the packet to this peer.
            m_peers[i]->m_socket.send(packet);
        }
    }

    Utility::Debug("Start game on all sockets");
    SetListening(false);
}

void GameServer::HanldePlayerNameChange(sf::Packet& packet)
{
    sf::Int8 id;
    std::string name;
    packet >> id >> name;
    name = name.substr(0, 20);
}


void GameServer::HandleIncomingPackets(sf::Packet& packet, RemotePeer& receiving_peer, bool& detected_timeout)
{
    sf::Int8 packet_type;
    packet >> packet_type;

    switch (static_cast<Client::PacketType> (packet_type))
    {
    case Client::PacketType::kQuit:
        receiving_peer.m_timed_out = true;
        detected_timeout = true;
        break;
    case Client::PacketType::kPlayerEvent:
        PlayerEvent(packet);
        break;
    case Client::PacketType::kPlayerRealtimeChange:
        RealTimeChange(packet);
        break;
    case Client::PacketType::kRequestCoopPartner:
        //RequestCoopPartner(receiving_peer);
        Utility::Debug("Void");
        break;
    case Client::PacketType::kStateUpdate:
        StateUpdate(packet);
        break;
    case Client::PacketType::kGameEvent:
        GameEvent(packet, receiving_peer);
        break;
    case Client::PacketType::kTeamChange:
        PlayerTeamChange(packet);
        break;
    case Client::PacketType::kPlayerUpdate:
        HandlePlayerUpdate(packet);
        break;
    case Client::PacketType::kNameChange:
        HanldePlayerNameChange(packet);
        break;
    case Client::PacketType::kStartNetworkGame:
        NotifyGameStart();
        break;
    case Client::PacketType::kStartNetworkGameCountdown:
        StartGameCountdownStart();
        break;
    }
}

void GameServer::GetAndSetID(sf::Int8& int8)
{

}
void GameServer::HandleIncomingConnections()
{
    if (!m_listening_state)
        return;

    // Try to accept a new connection into the next available RemotePeer.
    if (m_listener_socket.accept(m_peers[m_connected_players]->m_socket) == sf::TcpListener::Done)
    {
        // Limit check.
        if (m_connected_players >= m_max_connected_players)
        {
            SetListening(false);
            return;
        }

        // Assign a unique player ID to the new connection.
        // (Here we simply use m_connected_players + 1; you can use a dedicated counter if needed.)
        sf::Int8 player_id = m_player_id_counter++;

        m_peers[m_connected_players]->m_identifier = player_id;

        // Look for an existing ship with an open seat.
        ShipInfo* ship = nullptr;
        for (auto& pair : m_ship_info)
        {
            if (!pair.second.IsFull())
            {
                ship = &pair.second;
                break;
            }
        }

        // If no ship is available, create a new one.
        if (ship == nullptr)
        {
            ShipInfo newShip;
            newShip.m_ship_id = static_cast<sf::Int8>(m_ship_count++);
            // Initialize the ship state (for example, centered on the battlefield).
            newShip.m_position = sf::Vector2f(m_battlefield_rect.width / 2,
                m_battlefield_rect.top + m_battlefield_rect.height / 2);
            newShip.m_hitpoints = 100;
            newShip.m_missile_ammo = 20;
            // Ensure seats are marked as available.
            newShip.m_pilot_id = -1;
            newShip.m_gunner_id = -1;

            // Insert the new ship into the map.
            sf::Int8 shipID = newShip.m_ship_id;
            m_ship_info[shipID] = newShip;
            ship = &m_ship_info[shipID];
        }

        // Determine which role to assign: if there is no pilot, assign as pilot; otherwise assign as gunner.
        sf::Int8 role = -1;
        if (!ship->HasPilot())
        {
            ship->m_pilot_id = player_id;
            role = static_cast<sf::Int8>(Role::Pilot); // e.g., 0
        }
        else if (!ship->HasGunner())
        {
            ship->m_gunner_id = player_id;
            role = static_cast<sf::Int8>(Role::Gunner); // e.g., 1
        }

        // Build the spawn packet with the ship's information and the assigned role.
        sf::Packet packet;
        // Use kSpawnSelf for the local (pilot) player and kPlayerConnect for the remote (gunner) player.
        if (role == static_cast<sf::Int8>(Role::Pilot))
        {
            packet << static_cast<sf::Int32>(Server::PacketType::kSpawnSelf);
        }
        else if (role == static_cast<sf::Int8>(Role::Gunner))
        {
            packet << static_cast<sf::Int32>(Server::PacketType::kPlayerConnect);
        }
        // Packet layout: [player id] [ship_id][role][position.x][position.y]
        packet << player_id;
    	packet << ship->m_ship_id;
        packet << role;
        packet << ship->m_position.x;
        packet << ship->m_position.y;

        // Send the spawn packet directly to the new connection.
        m_peers[m_connected_players]->m_socket.send(packet);

        // Optionally, notify all clients that a new player has joined this ship.
        BroadcastMessage("New player joined ship " + std::to_string(ship->m_ship_id));
        // Optionally, send the current world state to this new connection.
        InformWorldState(m_peers[m_connected_players]->m_socket);

        // Mark this peer as ready and update its last packet time.
        m_peers[m_connected_players]->m_ready = true;
        m_peers[m_connected_players]->m_last_packet_time = Now();

        // Increment the connected player count.
        m_connected_players++;

        // Prepare a new RemotePeer if there is still capacity.
        if (m_connected_players < m_max_connected_players)
        {
            m_peers.emplace_back(PeerPtr(new RemotePeer()));
        }
        else
        {
            SetListening(false);
        }
    }
}

void GameServer::HandleDisconnections()
{
    for (auto itr = m_peers.begin(); itr != m_peers.end(); )
    {
        if ((*itr)->m_timed_out)
        {
            sf::Int8 identifier = (*itr)->m_identifier; // Player ID
            SendToAll(sf::Packet() << static_cast<sf::Int32>(Server::PacketType::kPlayerDisconnect) << identifier);

            // Find and update the ship the player was in
            for (auto ship_itr = m_ship_info.begin(); ship_itr != m_ship_info.end(); ++ship_itr)
            {
                ShipInfo& ship = ship_itr->second;

                bool removed = false;
                if (ship.m_pilot_id == identifier)
                {
                    ship.m_pilot_id = -1;
                    removed = true;
                }
                if (ship.m_gunner_id == identifier)
                {
                    ship.m_gunner_id = -1;
                    removed = true;
                }

                // If the ship is now empty, remove it
                if (removed && !ship.HasPilot() && !ship.HasGunner())
                {
                    m_ship_info.erase(ship_itr);
                    m_ship_count--;
                    break; // stop after erasing
                }
            }

            m_connected_players--;
            itr = m_peers.erase(itr); // Remove the disconnected peer

            // Allow more connections
            if (m_connected_players < m_max_connected_players)
            {
                m_peers.emplace_back(PeerPtr(new RemotePeer()));
                SetListening(true);
            }

            BroadcastMessage("A player has disconnected");
        }
        else
        {
            ++itr;
        }
    }
}


void GameServer::InformWorldState(sf::TcpSocket& socket)
{
    sf::Packet packet;
    packet << static_cast<sf::Int32>(Server::PacketType::kInitialState);
    packet << m_world_height << m_battlefield_rect.top + m_battlefield_rect.height;
    packet << static_cast<sf::Int32>(m_ship_count);

    for (const auto& pair : m_ship_info)
    {
        const sf::Int8 ship_id = pair.first;
        const ShipInfo& ship = pair.second;

        packet << ship_id
            << ship.m_position.x
            << ship.m_position.y
            << ship.m_hitpoints
            << ship.m_missile_ammo
            << ship.m_pilot_id
            << ship.m_gunner_id;
    }



    socket.send(packet);
}

void GameServer::BroadcastMessage(const std::string& message)
{
    sf::Packet packet;
    packet << static_cast<sf::Int32>(Server::PacketType::kBroadcastMessage);
    packet << message;
    for (std::size_t i = 0; i < m_connected_players; ++i)
    {
        if (m_peers[i]->m_ready)
        {
            m_peers[i]->m_socket.send(packet);
        }
    }
}

void GameServer::SendToAll(sf::Packet& packet)
{
    for (std::size_t i = 0; i < m_connected_players; ++i)
    {
        if (m_peers[i]->m_ready)
        {
            m_peers[i]->m_socket.send(packet);
        }
    }
}

void GameServer::UpdateClientState()
{
    sf::Packet update_client_state_packet;
    update_client_state_packet << static_cast<sf::Int32>(Server::PacketType::kUpdateClientState);
    update_client_state_packet << static_cast<float>(m_battlefield_rect.top + m_battlefield_rect.height);
    update_client_state_packet << static_cast<sf::Int32>(m_ship_count);

    for (const auto& ship : m_ship_info)
    {
        update_client_state_packet << static_cast<sf::Int8>(ship.first)
            << ship.second.m_position.x
            << ship.second.m_position.y
            << ship.second.m_hitpoints
            << ship.second.m_missile_ammo
            << ship.second.m_pilot_id
            << ship.second.m_gunner_id;

    }

    SendToAll(update_client_state_packet);
}


//It is essential to set the sockets to non-blocking - m_socket.setBlocking(false)
//otherwise the server will hang waiting to read input from a connection

GameServer::RemotePeer::RemotePeer() : m_ready(false), m_timed_out(false)
{
    m_socket.setBlocking(false);
}

