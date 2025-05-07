#include <iostream>
#include <SFML/Network/Packet.hpp>
#include "NetworkProtocol.hpp"
#include <SFML/System/Sleep.hpp>

#include "Action.hpp"
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
    , m_ship_count(0)
    , m_peers(1)
    , m_ship_identifier_counter(1)
    , m_waiting_thread_end(false)
    , m_last_spawn_time(sf::Time::Zero)
    , m_time_for_next_spawn(sf::seconds(5.f))
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
    packet << ship_identifier << m_ship_info[ship_identifier].m_position.x << m_ship_info[ship_identifier].m_position.y << m_ship_info[ship_identifier].m_hitpoints << m_ship_info[ship_identifier].m_missile_ammo << m_ship_info[ship_identifier].m_cannon_angle;

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

    sf::Time FRAME_RATE = sf::seconds(1.f / 60.f);
    //sf::Time frame_time = sf::Time::Zero;
    sf::Time TICK_RATE = sf::seconds(1.f / 20.f);

    sf::Clock clock;
    sf::Time next_frame = clock.getElapsedTime() + FRAME_RATE;
    sf:: Time next_tick = clock.getElapsedTime() + TICK_RATE;

    while (!m_waiting_thread_end)
    {
        //This is the game loop
        HandleIncomingConnections();
        HandleIncomingPackets();

        
        for (auto& peer : m_peers)
        {
            if (peer->m_ready)
                peer->FlushSendQueue();
        }


        sf::Time now = clock.getElapsedTime();
        //Update rendering scroll in fixed 60HZ steps
        while (now >= next_frame)
        {
	        m_battlefield_rect.top += m_battlefield_scrollspeed * FRAME_RATE.asSeconds();
            next_frame += FRAME_RATE;
        }

        //Frun game logic in fixed 20HZ steps
        while (now >= next_tick)
        {
            Tick();
            next_tick += TICK_RATE;
        }
        //sleep just until the next frame or tick is due
        sf::Time wakeup_call = std::min(next_frame, next_tick);
        if (wakeup_call > now)
            sf::sleep(wakeup_call - now);
    }
}

void GameServer::Tick()
{
    UpdateClientState();

    /*Check if the game is over = all planes postion.y < offset
    if (!m_ship_info.empty())
    {
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
    }*/

    //Remove ships that have been destroyed
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

    // 2) Only check for last‐man‐standing if the match has started
    if (m_matchStarted && !m_missionSuccessSent)
    {
        std::size_t aliveCount = m_ship_info.size();

        // Only declare success if we began with 2+ players, and now only 1 or 0 remain
        if (m_initialShipCount >= 2 && aliveCount <= 1)
        {
            sf::Int32 winnerId = aliveCount;
            sf::Packet packet;
            packet << static_cast<sf::Int32>(Server::PacketType::kMissionSuccess)
        	<< winnerId;
            SendToAll(packet);

            m_missionSuccessSent = true;
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
    sf::Int32 ship_identifier;
    sf::Int32 action;
    packet >> ship_identifier >> action;
    NotifyPlayerEvent(ship_identifier, action);
    
}

// In GameServer.cpp, replace your old RealTimeChange with this:
void GameServer::RealTimeChange(sf::Packet& packet)
{
    // 1) Read the client’s input sequence first:
    sf::Int32 seq;
    sf::Int32 ship_identifier;
    sf::Int32 action;
    bool      action_enabled;

    packet >> seq
        >> ship_identifier
        >> action
        >> action_enabled;

    // 2) Record which input the server has now processed for this ship
    auto& info = m_ship_info[ship_identifier];
    info.m_lastProcessedInput = seq;

    // 3) Broadcast the game action unchanged to all clients
    NotifyPlayerRealtimeChange(ship_identifier, action, action_enabled);

    // 3) **Apply** rotation to the server’s ShipData
    auto& data = m_ship_info[ship_identifier];
    const float rotationSpeed = 0.5f;      
    const float tickSec = 1.f / 20.f; 

    if (action == static_cast<sf::Int32>(Action::kRotateLeft) && action_enabled)
        data.m_cannon_angle -= rotationSpeed * tickSec;
    else if (action == static_cast<sf::Int32>(Action::kRotateRight) && action_enabled)
        data.m_cannon_angle += rotationSpeed * tickSec;

    if (data.m_cannon_angle < 0.f)   data.m_cannon_angle += 360.f;
    if (data.m_cannon_angle >= 360.f) data.m_cannon_angle -= 360.f;

}



void GameServer::StateUpdate(sf::Packet& packet)
{
    sf::Int32 numb_ships;
    packet >> numb_ships;

    for (sf::Int32 i = 0; i < numb_ships; ++i)
    {
        sf::Int32 ship_identifier;
        sf::Int32 ship_hitpoints;
        sf::Int32 missile_ammo;
        sf::Vector2f ship_position;
        float cannon_angle;
        packet >> ship_identifier >> ship_position.x >> ship_position.y >> ship_hitpoints >> missile_ammo >> cannon_angle;
        m_ship_info[ship_identifier].m_position = ship_position;
        m_ship_info[ship_identifier].m_hitpoints = ship_hitpoints;
        m_ship_info[ship_identifier].m_missile_ammo = missile_ammo;
        m_ship_info[ship_identifier].m_cannon_angle = cannon_angle;
    }
}

void GameServer::GameEvent(sf::Packet& packet, GameServer::RemotePeer& receiving_peer)
{
    sf::Int32 action;
    float x, y;
    packet >> action >> x >> y;

    if (action == GameActions::kEnemyExplode
        && Utility::RandomInt(3) == 0)
    {
        sf::Packet spawnPacket;
        spawnPacket << static_cast<sf::Int32>(Server::PacketType::kSpawnPickup)
            << static_cast<sf::Int32>(Utility::RandomInt(static_cast<int>(PickupType::kPickupCount)))
            << x << y;
        SendToAll(spawnPacket);
    }
}

void GameServer::HandlePlayerUpdate(sf::Packet& packet)
{
    /*sf::Int8 ship_id;
    packet >> ship_id;

    sf::Packet notify_packet;
    notify_packet << static_cast<sf::Int32>(Server::PacketType::kPlayerUpdate);
    notify_packet << ship_id;

    SendToAll(notify_packet);*/
}

void GameServer::StartGameCountdownStart()
{
    /*sf::Packet packet;
    packet << static_cast<sf::Int32>(Server::PacketType::kStartNetworkGameCountdown);
    SendToAll(packet);*/
}

void GameServer::NotifyGameStart()
{
    //m_game_started = true;

    //// First, send a kGameStart packet to notify everyone.
    //sf::Packet startPacket;
    //startPacket << static_cast<sf::Int8>(Server::PacketType::kGameStart);
    //SendToAll(startPacket);

    //// For each connected peer, send a spawn packet with ship info.
    //for (int i = 0; i < m_connected_players; ++i)
    //{
    //    if (m_peers[i]->m_ready)
    //    {
    //        sf::Packet packet;
    //        packet << static_cast<sf::Int8>(Server::PacketType::kSpawnSelf);

    //        // Send the packet to this peer.
    //        m_peers[i]->m_socket.send(packet);
    //    }
    //}

    //Utility::Debug("Start game on all sockets");
    //SetListening(false);
}

void GameServer::HandlePlayerNameChange(sf::Packet& packet)
{
    sf::Int32 shipId;
    std::string name;
    packet >> shipId >> name;

    // Store it on the server if you want:
    m_ship_names[shipId] = name;
    // Now broadcast to everyone
    sf::Packet out;
    out << static_cast<sf::Int32>(Server::PacketType::kPlayerName)
        << shipId
        << name;
    SendToAll(out);
}


void GameServer::HandleIncomingPackets(sf::Packet& packet, RemotePeer& receiving_peer, bool& detected_timeout)
{

    //use of chatgpt can be seen here
    // https://chatgpt.com/share/681be6d4-0a6c-800c-977f-f82a6a5ad6f1
    sf::Int32 packet_type;
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
    case Client::PacketType::kStateUpdate:
        StateUpdate(packet);
        break;
    case Client::PacketType::kGameEvent:
        GameEvent(packet, receiving_peer);
        break;
    case Client::PacketType::kTeamChange:
        //PlayerTeamChange(packet);
        Utility::Debug("Void");
        break;
    case Client::PacketType::kPlayerUpdate:
       // HandlePlayerUpdate(packet);
        Utility::Debug("Void");
        break;
    case Client::PacketType::kNameChange:
        HandlePlayerNameChange(packet);
        
        break;
    case Client::PacketType::kStartNetworkGame:
        //NotifyGameStart();
        Utility::Debug("Void");
        break;
    case Client::PacketType::kStartNetworkGameCountdown:
        //StartGameCountdownStart();
        Utility::Debug("Void");
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

        // 1) Figure out our grid dimensions
   //    E.g. 5 columns per row, up to 3 rows → 15 slots total
        const int maxPerRow = 5;
        int index = m_connected_players;     // 0 for first, 1 for second, etc
        int col = index % maxPerRow;       // x slot
        int row = index / maxPerRow;       // y slot

        // 2) Compute your battlefield extents
        float left = m_battlefield_rect.left;
        //float top = m_battlefield_rect.top;
        float width = m_battlefield_rect.width;
        float height = m_battlefield_rect.height;

        float scrollBottom = m_battlefield_rect.top + height;

        // 3) Determine how many rows are actually in use so far
        int totalPlayers = m_connected_players + 1;
        int rowsInUse = (totalPlayers + maxPerRow - 1) / maxPerRow; // ceil

        // 4) Compute spawnX and spawnY with equal margins
        float segmentX = width / (maxPerRow + 1);
        float segmentY = height / (rowsInUse + 1);

        float spawnX = left + segmentX * (col + 1);
        float spawnY = scrollBottom - segmentY * (row + 1);


        //Order the new client to spawn its player 
        sf::Int32 newId = m_ship_identifier_counter;
        m_ship_info[newId].m_position = {spawnX,spawnY};
        m_ship_info[newId].m_hitpoints = 200;
        m_ship_info[newId].m_missile_ammo = 20;


        // Build the spawn packet with the ship's information 
        sf::Packet packet;
    	packet << static_cast<sf::Int32>(Server::PacketType::kSpawnSelf);

        packet << m_ship_identifier_counter
            << m_ship_info[m_ship_identifier_counter].m_position.x
            << m_ship_info[m_ship_identifier_counter].m_position.y
            << m_ship_info[m_ship_identifier_counter].m_hitpoints
            << m_ship_info[m_ship_identifier_counter].m_missile_ammo
            << m_ship_info[m_ship_identifier_counter].m_cannon_angle;

        m_peers[m_connected_players]->m_ship_identifiers.emplace_back(m_ship_identifier_counter);

        BroadcastMessage("New player");
        InformWorldState(m_peers[m_connected_players]->m_socket);

        // Send every stored name to the newcomer
        for (auto it = m_ship_names.begin(); it != m_ship_names.end(); ++it)
        {
            sf::Int32 id = it->first;
            const std::string& name = it->second;

            sf::Packet namePkt;
            namePkt << static_cast<sf::Int32>(Server::PacketType::kPlayerName)
                << id
                << name;
            m_peers[m_connected_players]->m_socket.send(namePkt);
        }

        NotifyPlayerSpawn(m_ship_identifier_counter++);

        m_peers[m_connected_players]->m_socket.send(packet);
        m_peers[m_connected_players]->m_ready = true;
        m_peers[m_connected_players]->m_last_packet_time = Now();
        
        m_ship_count++;
        m_connected_players++;

        
        m_initialShipCount = static_cast<sf::Int32>(m_ship_info.size());
        m_matchStarted = true;
        m_missionSuccessSent = false;

        // Prepare a new RemotePeer if there is still capacity.
        if (m_connected_players >= m_max_connected_players)
        {
            SetListening(false);
        }
        else
        {
            m_peers.emplace_back(PeerPtr(new RemotePeer()));
        }
    }
}

void GameServer::HandleDisconnections()
{
    for (auto itr = m_peers.begin(); itr != m_peers.end(); )
    {
        if ((*itr)->m_timed_out)
        {
            for (sf::Int32 identifier: (*itr)->m_ship_identifiers)
            {
                SendToAll((sf::Packet() << static_cast <sf::Int32> (Server::PacketType::kPlayerDisconnect) << identifier));
                m_ship_info.erase(identifier);
            }
            m_connected_players--;
            m_ship_count -= (*itr)->m_ship_identifiers.size();

            itr = m_peers.erase(itr);

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
    // 1) Packet type
    packet << static_cast<sf::Int32>(Server::PacketType::kInitialState)

    
        //<< m_packet_sequence++           // sequence #
        //<< Now().asSeconds()          // server timestamp


    // 2) World geometry
    << m_world_height
        << (m_battlefield_rect.top + m_battlefield_rect.height);

    // 3) How many ships are we actually going to send?
    sf::Int32 shipsToSend = 0;
    for (std::size_t i = 0; i < m_connected_players; ++i)
        if (m_peers[i]->m_ready)
            shipsToSend += static_cast<sf::Int32>(m_peers[i]->m_ship_identifiers.size());

    packet << shipsToSend;

    // 4) Now serialize exactly that many
    for (std::size_t i = 0; i < m_connected_players; ++i)
    {
        if (!m_peers[i]->m_ready)
            continue;

        for (sf::Int32 id : m_peers[i]->m_ship_identifiers)
        {
            const auto& info = m_ship_info[id];
            packet << id
                << info.m_position.x
                << info.m_position.y
                << info.m_hitpoints
                << info.m_missile_ammo
            	<< info.m_cannon_angle;

            // Look up the name (or empty string)
            const auto it = m_ship_names.find(id);
            packet << (it != m_ship_names.end() ? it->second : std::string{});
        }
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
    for (auto& peer : m_peers)
    {
        if (peer->m_ready)
            peer->QueuePacket(packet);
    }
}

void GameServer::UpdateClientState()
{
    sf::Packet packet;

    // Header
    packet << static_cast<sf::Int32>(Server::PacketType::kUpdateClientState)
           << m_packet_sequence++           // sequence #
           << Now().asSeconds();            // server timestamp

    // Scroll + ship count
    float scrollBottom = m_battlefield_rect.top + m_battlefield_rect.height;
    packet << scrollBottom
           << static_cast<sf::Int32>(m_ship_info.size());

    //Per‐ship state + ack
    for (const auto& kv : m_ship_info)
    {
        sf::Int32 id   = static_cast<sf::Int32>(kv.first);
        const auto& s  = kv.second;

        packet << id
               << s.m_position.x
               << s.m_position.y
               << s.m_hitpoints
               << s.m_missile_ammo
               << s.m_cannon_angle
               << s.m_lastProcessedInput;
    }

    // 4) Broadcast
    SendToAll(packet);
}



//It is essential to set the sockets to non-blocking - m_socket.setBlocking(false)
//otherwise the server will hang waiting to read input from a connection

GameServer::RemotePeer::RemotePeer() : m_ready(false), m_timed_out(false)
{
    m_socket.setBlocking(false);
}

void GameServer::RemotePeer::QueuePacket(const sf::Packet& packet)
{
    m_send_queue.push_back(packet);
}

void GameServer::RemotePeer::FlushSendQueue()
{
    while (!m_send_queue.empty())
    {
        auto& front = m_send_queue.front();
        auto status = m_socket.send(front);
        if (status == sf::Socket::Done)
        {
            m_send_queue.pop_front();
        }
        else if (status == sf::Socket::Partial || status == sf::Socket::NotReady)
        {
            // OS buffer still full — stop and retry later
            break;
        }
        else
        {
            // Error or disconnect → mark timed out and drop rest
            m_timed_out = true;
            break;
        }
    }
}
