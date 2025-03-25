#include "GameServer.hpp"
#include <SFML/Network/Packet.hpp>
#include "NetworkProtocol.hpp"
#include <SFML/System/Sleep.hpp>
#include "Utility.hpp"
#include "PickupType.hpp"
#include "ShipType.hpp"

GameServer::GameServer(sf::Vector2f battlefield_size)
    : m_thread(&GameServer::ExecutionThread, this)
    , m_listening_state(false)
    , m_client_timeout(sf::seconds(5.f))
    , m_max_connected_players(15)
    , m_connected_players(0)
    , m_world_height(5000.f)
    , m_battlefield_rect(0.f, m_world_height - battlefield_size.y, battlefield_size.x, battlefield_size.y)
    , m_battlefield_scrollspeed(-50.f)
    , m_ship_count(0)
    , m_peers(1)
    , m_ship_identifier_counter(1)
    , m_waiting_thread_end(false)
    , m_last_spawn_time(sf::Time::Zero)
    , m_time_for_next_spawn(sf::seconds(5.f))
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
    //First thing in every packets is what type of packet it is
    packet << static_cast<sf::Int32>(Server::PacketType::kPlayerConnect);
    packet << ship_identifier << m_ship_info[ship_identifier].m_position.x << m_ship_info[ship_identifier].m_position.y;
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

void GameServer::RequestCoopPartner(GameServer::RemotePeer& receiving_peer)
{
	receiving_peer.m_ship_identifiers.emplace_back(m_ship_identifier_counter);
	m_ship_info[m_ship_identifier_counter].m_position = sf::Vector2f(m_battlefield_rect.width / 2, m_battlefield_rect.top + m_battlefield_rect.height / 2);
	m_ship_info[m_ship_identifier_counter].m_hitpoints = 100;
	m_ship_info[m_ship_identifier_counter].m_missile_ammo = 2;

	sf::Packet request_packet;
	request_packet << static_cast<sf::Int32>(Server::PacketType::kAcceptCoopPartner);
	request_packet << m_ship_identifier_counter;
	request_packet << m_ship_info[m_ship_identifier_counter].m_position.x;
	request_packet << m_ship_info[m_ship_identifier_counter].m_position.y;

	receiving_peer.m_socket.send(request_packet);
	m_ship_count++;

	// Tell everyone else about the new plane
	sf::Packet notify_packet;
	notify_packet << static_cast<sf::Int32>(Server::PacketType::kPlayerConnect);
	notify_packet << m_ship_identifier_counter;
	notify_packet << m_ship_info[m_ship_identifier_counter].m_position.x;
	notify_packet << m_ship_info[m_ship_identifier_counter].m_position.y;

	for (PeerPtr& peer : m_peers)
	{
		if (peer.get() != &receiving_peer && peer->m_ready)
		{

			peer->m_socket.send(notify_packet);
		}
	}

	m_ship_identifier_counter++;
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
    Utility::Debug("Team change");
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
    sf::Packet packet;
    packet << static_cast<sf::Int8>(Server::PacketType::kGameStart);
    SendToAll(packet);

    for (int i = 0; i < m_connected_players; ++i)
    {
        if (m_peers[i]->m_ready)
        {
            packet.clear();

            packet << static_cast<sf::Int8>(Server::PacketType::kSpawnSelf);

            packet << static_cast<sf::Int8>(m_ship_info.size() - 1);

            sf::Int8 id = m_peers[i]->m_ship_identifiers.at(i);

            for (const auto& player_info : m_ship_info)
            {
                if (player_info.first == id)
                {
                    continue;
                }

                packet << player_info.first;
                packet << player_info.second.m_ship_id;
                packet << player_info.second.m_gunner_id;
                packet << player_info.second.m_pilot_id;
                
            }

            packet << id;
            packet << m_ship_info[id].m_ship_id;
            packet << m_ship_info[id].m_gunner_id;
            packet << m_ship_info[id].m_pilot_id;

            m_peers[i]->m_socket.send(packet);
        }
    }

    Utility::Debug("Start game on all sockets");
    SetListening(false);
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
            RequestCoopPartner(receiving_peer);
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
		case Client::PacketType::kStartNetworkGame:
            NotifyGameStart();
		case Client::PacketType::kStartNetworkGameCountdown:
            StartGameCountdownStart();
			break;
    }
}

void GameServer::HandleIncomingConnections()
{
    if (!m_listening_state)
    {
        return;
    }

    if (m_listener_socket.accept(m_peers[m_connected_players]->m_socket) == sf::TcpListener::Done)
    {
        sf::Int32 assigned_ship_id = -1;
        bool is_pilot = false;

        // Find an existing ship that has space
        for (auto& ship_pair : m_ship_info)  
        {
            sf::Int32 ship_id = ship_pair.first;  // Extract key
            ShipInfo& ship = ship_pair.second;    // Extract value

            if (!ship.HasGunner())  // Ship has an empty gunner seat
            {
                assigned_ship_id = ship_id;
                is_pilot = false;
                break;
            }
        }

        // If no existing ship has space, create a new one
        if (assigned_ship_id == -1)
        {
            assigned_ship_id = m_ship_identifier_counter;
            is_pilot = true;

            // Initialize new ship info
            ShipInfo new_ship;
            new_ship.m_position = sf::Vector2f(m_battlefield_rect.width / 2,
                m_battlefield_rect.top + m_battlefield_rect.height / 2);
            new_ship.m_hitpoints = 100;
            new_ship.m_missile_ammo = 20;

            m_ship_info[assigned_ship_id] = new_ship;
            m_ship_identifier_counter++;
        }

        // Assign player to their role
        if (is_pilot)
        {
            m_ship_info[assigned_ship_id].m_pilot_id = m_connected_players;
        }
        else
        {
            m_ship_info[assigned_ship_id].m_gunner_id = m_connected_players;
        }

        // Notify client
        sf::Packet packet;
        packet << static_cast<sf::Int32>(Server::PacketType::kSpawnSelf);
        packet << assigned_ship_id;
        packet << m_ship_info[assigned_ship_id].m_position.x;
        packet << m_ship_info[assigned_ship_id].m_position.y;

        m_peers[m_connected_players]->m_ship_identifiers.emplace_back(assigned_ship_id);
        BroadcastMessage("New player joined");
        InformWorldState(m_peers[m_connected_players]->m_socket);
        NotifyPlayerSpawn(assigned_ship_id);
        m_peers[m_connected_players]->m_socket.send(packet);
        m_peers[m_connected_players]->m_ready = true;
        m_peers[m_connected_players]->m_last_packet_time = Now();

        // Update connected players
        m_connected_players++;
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
    for (auto itr = m_peers.begin(); itr != m_peers.end();)
    {
        if ((*itr)->m_timed_out)
        {
            //Inform everyone of a disconnection, erase
            for (sf::Int32 identifer : (*itr)->m_ship_identifiers)
            {
                SendToAll((sf::Packet() << static_cast<sf::Int32>(Server::PacketType::kPlayerDisconnect) << identifer));
                m_ship_info.erase(identifer);
            }

            m_connected_players--;
            m_ship_count -= (*itr)->m_ship_identifiers.size();

            itr = m_peers.erase(itr);

            //If the number of peers has dropped below max_connections
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

    for (std::size_t i = 0; i < m_connected_players; ++i)
    {
        if (m_peers[i]->m_ready)
        {
            for (sf::Int32 identifier : m_peers[i]->m_ship_identifiers)
            {
                packet << identifier << m_ship_info[identifier].m_position.x << m_ship_info[identifier].m_position.y << m_ship_info[identifier].m_hitpoints << m_ship_info[identifier].m_missile_ammo;
            }
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

    for (const auto& aircraft : m_ship_info)
    {
        update_client_state_packet << aircraft.first << aircraft.second.m_position.x << aircraft.second.m_position.y << aircraft.second.m_hitpoints << aircraft.second.m_missile_ammo;
    }

    SendToAll(update_client_state_packet);
}


//It is essential to set the sockets to non-blocking - m_socket.setBlocking(false)
//otherwise the server will hang waiting to read input from a connection

GameServer::RemotePeer::RemotePeer() : m_ready(false), m_timed_out(false)
{
    m_socket.setBlocking(false);
}

