#include "MultiplayerGameState.hpp"
#include "MusicPlayer.hpp"
#include "Utility.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Network/Packet.hpp>
#include <SFML/Network/IpAddress.hpp>

#include <fstream>
#include "PickupType.hpp"
#include <iostream>

sf::IpAddress GetAddressFromFile()
{
	{
		//Try to open existing file
		std::ifstream input_file("ip.txt");
		std::string ip_address;
		if (input_file >> ip_address)
		{
			return ip_address;
		}
	}

	//If the open/read failed, create a new file
	std::ofstream output_file("ip.txt");
	std::string local_address = "127.0.0.1";
	output_file << local_address;
	return local_address;

}

MultiplayerGameState::MultiplayerGameState(StateStack& stack, Context context , bool is_host)
	:State(stack, context)
	, m_world(*context.window, *context.fonts, *context.sounds, true)
	, m_window(*context.window)
	, m_texture_holder(*context.textures)
	, m_connected(false)
	, m_game_server(nullptr)
	, m_active_state(true)
	, m_has_focus(true)
	, m_host(is_host)
	, m_game_started(false)
	, m_client_timeout(sf::seconds(5.f))
	, m_time_since_last_packet(sf::seconds(0.f))
    
{
	m_broadcast_text.setFont(context.fonts->Get(Font::kMain));
	m_broadcast_text.setPosition(1024.f / 2, 100.f);


	//Use this for "Attempt to connect" and "Failed to connect" messages
	m_failed_connection_text.setFont(context.fonts->Get(Font::kMain));
	m_failed_connection_text.setCharacterSize(35);
	m_failed_connection_text.setFillColor(sf::Color::White);
	m_failed_connection_text.setString("Attempting to connect...");
	Utility::CentreOrigin(m_failed_connection_text);
	m_failed_connection_text.setPosition(m_window.getSize().x / 2.f, m_window.getSize().y / 2.f);

	//Render an establishing connection frame for user feedback
	m_window.clear(sf::Color::Black);
	m_window.draw(m_failed_connection_text);
	m_window.display();
	m_failed_connection_text.setString("Failed to connect to server");
	Utility::CentreOrigin(m_failed_connection_text);

	

	//If this is the host, create a server
	sf::IpAddress ip;

	if (m_host)
	{
		m_game_server.reset(new GameServer(sf::Vector2f(m_window.getSize())));
		ip = "127.0.0.1";
	}
	else
	{
		ip = GetAddressFromFile();
	}

	if (m_socket.connect(ip, SERVER_PORT, sf::seconds(5.f)) == sf::TcpSocket::Done)
	{
		m_connected = true;
	}
	else
	{
		m_failed_connection_clock.restart();
	}

	//Set socket to non-blocking
	m_socket.setBlocking(false);

	context.music->Play(MusicThemes::kMissionTheme);
}
void MultiplayerGameState::Draw()
{
	if (m_connected)
	{
		m_world.Draw();

		// Show the broadcast message in the default view.
		m_window.setView(m_window.getDefaultView());

		if (!m_broadcasts.empty())
		{
			m_window.draw(m_broadcast_text);
		}

	}
	else
	{
		m_window.draw(m_failed_connection_text);
	}
}


bool MultiplayerGameState::Update(sf::Time dt)
{
	if (m_connected)
	{
		m_world.Update(dt);

		bool found_local_ship = false; 
		
		// Iterate over all players.
		for (auto itr = m_players.begin(); itr != m_players.end(); )
		{
			if (std::find(m_local_player_identifiers.begin(), m_local_player_identifiers.end(), itr->first) != m_local_player_identifiers.end())
			{
				found_local_ship = true;
			}

			if (!m_world.GetShip(itr->first))
			{
				itr = m_players.erase(itr);
				if (m_players.empty())
				{
					RequestStackPush(StateID::kGameOver);
					return true;
				}
			}
			else
			{
				++itr;
			}
		}

		if (!found_local_ship && m_game_started && m_local_player_spawned)
		{
			Utility::Debug("No Ships found in the game");
			RequestStackPush(StateID::kGameOver);
		}

		// Handle real-time input if active and the window has focus.
		if (m_active_state && m_has_focus)
		{
			CommandQueue& commands = m_world.GetCommandQueue();
			for (auto& pair: m_players)
			{
				pair.second->HandleRealtimeInput(commands);
			}
		}

		// Always handle network input.
		CommandQueue& commands = m_world.GetCommandQueue();
		for (auto& pair : m_players)
		{
			pair.second->HandleRealtimeNetworkInput(commands);
		}

		// Process incoming packets.
		sf::Packet packet;
		if (m_socket.receive(packet) == sf::Socket::Done)
		{
			m_time_since_last_packet = sf::seconds(0.f);
			sf::Int32 packet_type;
			packet >> packet_type;
			HandlePacket(packet_type, packet);
		}
		else
		{
			// Check for timeout.
			if (m_time_since_last_packet > m_client_timeout)
			{
				m_connected = false;
				m_failed_connection_text.setString("Lost connection to the server");
				Utility::CentreOrigin(m_failed_connection_text);
				m_failed_connection_clock.restart();
			}
		}

		UpdateBroadcastMessage(dt);

		// Process game events.
		GameActions::Action game_action;
		while (m_world.PollGameAction(game_action))
		{
			sf::Packet packet;
			packet << static_cast<sf::Int32>(Client::PacketType::kGameEvent)
				<< static_cast<sf::Int32>(game_action.type)
				<< game_action.position.x
				<< game_action.position.y;
			m_socket.send(packet);
		}

		// Regular position updates.
		if (m_tick_clock.getElapsedTime() > sf::seconds(1.f / 20.f))
		{
			sf::Packet position_update_packet;

			position_update_packet << static_cast<sf::Int32>(Client::PacketType::kStateUpdate);

			/*position_update_packet << static_cast<sf::Int32>(m_local_player_identifiers.size());
			for (auto& id : m_local_player_identifiers)
			{
				if (Ship* ship = m_world.GetShip(id))
				{
				position_update_packet << id << ship->getPosition().x << ship->getPosition().y << static_cast<sf::Int32>(ship->GetHitPoints()) << static_cast<sf::Int32>(ship->GetMissileAmmo());
				}
			}*/

			std::vector<sf::Int32> validIds;
			for (auto id : m_local_player_identifiers)
				if (m_world.GetShip(id))
					validIds.push_back(id);

			// now emit the true count
			position_update_packet << (sf::Int32)validIds.size();
			for (auto id : validIds)
			{
				auto* ship = m_world.GetShip(id);
				position_update_packet
					<< id
					<< ship->getPosition().x << ship->getPosition().y
					<< ship->GetHitPoints()
					<< ship->GetMissileAmmo();
			}


			m_socket.send(position_update_packet);
			m_tick_clock.restart();
		}
		m_time_since_last_packet += dt;
	}
	else if (m_failed_connection_clock.getElapsedTime() >= sf::seconds(5.f))
	{
		RequestStackClear();
		RequestStackPush(StateID::kMenu);
	}
	return true;
}


bool MultiplayerGameState::HandleEvent(const sf::Event& event)
{
	//Game input handling
	CommandQueue& commands = m_world.GetCommandQueue();

	//Forward events to all players
	for (auto& pair : m_players)
	{
		pair.second->HandleEvent(event, commands);
	}

	if (event.type == sf::Event::KeyPressed)
	{
		
		if (event.key.code == sf::Keyboard::Escape)
		{
			DisableAllRealtimeActions();
			RequestStackPush(StateID::kNetworkPause);
		}
		
	}
	else if (event.type == sf::Event::GainedFocus)
	{
		m_has_focus = true;
	}
	else if (event.type == sf::Event::LostFocus)
	{
		m_has_focus = false;
	}
	return true;
}


void MultiplayerGameState::OnActivate()
{
	m_active_state = true;
}

void MultiplayerGameState::OnDestroy()
{
	if (!m_host && m_connected)
	{
		//Inform server this client is dying
		sf::Packet packet;
		packet << static_cast<sf::Int32>(Client::PacketType::kQuit);
		m_socket.send(packet);
	}
}

void MultiplayerGameState::DisableAllRealtimeActions()
{
	m_active_state = false;
	for (sf::Int32 identifier : m_local_player_identifiers)
	{
		m_players[identifier]->DisableAllRealtimeActions();
	}
	
}

void MultiplayerGameState::UpdateBroadcastMessage(sf::Time elapsed_time)
{
	if (m_broadcasts.empty())
	{
		return;
	}

	//Update broadcast timer
	m_broadcast_elapsed_time += elapsed_time;
	if (m_broadcast_elapsed_time > sf::seconds(2.f))
	{
		//If message has expired, remove it
		m_broadcasts.erase(m_broadcasts.begin());

		//Continue to display the next broadcast message
		if (!m_broadcasts.empty())
		{
			m_broadcast_text.setString(m_broadcasts.front());
			Utility::CentreOrigin(m_broadcast_text);
			m_broadcast_elapsed_time = sf::Time::Zero;
		}
	}
}

void MultiplayerGameState::HandleBroadcastMessage(sf::Packet& packet)
{
	std::string message;
	packet >> message;
	m_broadcasts.push_back(message);

	//Just added the first message, display immediately
	if (m_broadcasts.size() == 1)
	{
		m_broadcast_text.setString(m_broadcasts.front());
		Utility::CentreOrigin(m_broadcast_text);
		m_broadcast_elapsed_time = sf::Time::Zero;
	}
}
void MultiplayerGameState::HandleSpawnSelf(sf::Packet& packet)
{
	sf::Int32 ship_id;
	sf::Vector2f ship_position;
	packet >>  ship_id >> ship_position.x >> ship_position.y;

	Ship* ship = m_world.AddShip(ship_id);
	ship->setPosition(ship_position);
	m_players[ship_id].reset(new Player(&m_socket, ship_id, GetContext().keys1));
	m_local_player_identifiers.push_back(ship_id);
	m_game_started = true;
	m_local_player_spawned = true;
}

void MultiplayerGameState::HandlePlayerConnect(sf::Packet& packet)
{
	sf::Int32 ship_id;
	sf::Vector2f ship_position;
	packet >> ship_id >> ship_position.x >> ship_position.y;

	Ship* aircraft = m_world.AddShip(ship_id);
	aircraft->setPosition(ship_position);
	m_players[ship_id].reset(new Player(&m_socket, ship_id, nullptr));
}

void MultiplayerGameState::HandlePlayerDisconnect(sf::Packet& packet)
{
	sf::Int32 ship_id;
	packet >> ship_id;
	m_world.RemoveShip(ship_id);
	m_players.erase(ship_id);
}

//
//void MultiplayerGameState::HandleInitialState(sf::Packet& packet)
//{
//	// World settings
//	float world_height, current_scroll;
//	packet >> world_height >> current_scroll;
//	m_world.SetWorldHeight(world_height);
//	m_world.SetCurrentBattleFieldPosition(current_scroll);
//
//	// Number of ships
//	sf::Int32 ship_count;
//	packet >> ship_count;
//
//	for (sf::Int32 i = 0; i < ship_count; ++i)
//	{
//		sf::Int32 ship_id;
//		sf::Int32 hitpoints;
//		sf::Int32 missile_ammo;
//		sf::Vector2f ship_position;
//
//		packet >> ship_id
//			>> ship_position.x >> ship_position.y
//			>> hitpoints >> missile_ammo;
//
//		// Check if the ship already exists before adding it
//		Ship* ship = m_world.AddShip(ship_id);
//		ship->setPosition(ship_position);
//		ship->SetHitpoints(hitpoints);
//		ship->SetMissileAmmo(missile_ammo);
//
//		m_players[ship_id].reset(new Player(&m_socket, ship_id, nullptr));
//	}
//	
//}

void MultiplayerGameState::HandleInitialState(sf::Packet& packet)
{
	// 0) Read & verify the packet type
	sf::Int32 packetType;
	if (!(packet >> packetType)
		|| packetType != static_cast<sf::Int32>(Server::PacketType::kInitialState))
	{
		// Wrong kind of packet, or it was too short
		std::cerr << "HandleInitialState: bad packet type or truncated\n";
		return;
	}

	// 1) World settings
	float world_height, current_scroll;
	if (!(packet >> world_height >> current_scroll))
	{
		std::cerr << "HandleInitialState: truncated world data\n";
		return;
	}
	m_world.SetWorldHeight(world_height);
	m_world.SetCurrentBattleFieldPosition(current_scroll);

	// 2) Number of ships
	sf::Int32 ship_count;
	if (!(packet >> ship_count))
	{
		std::cerr << "HandleInitialState: missing ship count\n";
		return;
	}

	// 3) Wipe out any old ships before adding the new ones
	m_world.GetShips().clear();
	m_players.clear();

	// 4) Deserialize each ship
	for (sf::Int32 i = 0; i < ship_count; ++i)
	{
		sf::Int32 ship_id;
		sf::Vector2f ship_position;
		sf::Int32 hitpoints, missile_ammo;

		if (!(packet >> ship_id
			>> ship_position.x >> ship_position.y
			>> hitpoints >> missile_ammo))
		{
			std::cerr << "HandleInitialState: truncated ship entry at index " << i << "\n";
			return;
		}

		Ship* ship = m_world.AddShip(ship_id);
		ship->setPosition(ship_position);
		ship->SetHitpoints(hitpoints);
		ship->SetMissileAmmo(missile_ammo);

		m_players[ship_id] = std::make_unique<Player>(&m_socket, ship_id, nullptr);
	}
}


void MultiplayerGameState::HandlePlayerEvent(sf::Packet& packet)
{
	sf::Int32 ship_identifier;
	sf::Int32 action;
	packet >> ship_identifier >> action;

	auto itr = m_players.find(ship_identifier);
	if (itr != m_players.end())
	{
		itr->second->HandleNetworkEvent(static_cast<Action>(action), m_world.GetCommandQueue());
	}
}

void MultiplayerGameState::HandleRealTimeChange(sf::Packet& packet)
{
	sf::Int32 ship_identifier;
	sf::Int32 action;
	bool action_enabled;
	packet >> ship_identifier >> action >> action_enabled;

	auto itr = m_players.find(ship_identifier);
	if (itr != m_players.end())
	{
		itr->second->HandleNetworkRealtimeChange(static_cast<Action>(action), action_enabled);
	}
}

void MultiplayerGameState::HandleSpawnEnemy(sf::Packet& packet)
{
	float height;
	sf::Int32 type;
	float relative_x;
	packet >> type >> height >> relative_x;

	m_world.AddEnemy(static_cast<ShipType>(type), relative_x, height);
	m_world.SortEnemies();
}

void MultiplayerGameState::HandleUpdateClient(sf::Packet& packet)
{
	float current_world_position;
	sf::Int32 ship_count;
	packet >> current_world_position >> ship_count;

	float current_view_position = m_world.GetViewBounds().top + m_world.GetViewBounds().height;

	//Set the world's scroll compensation according to whether the view is behind or ahead
	m_world.SetWorldScrollCompensation(current_view_position / current_world_position);

	for (sf::Int32 i = 0; i < ship_count; ++i)
	{
		sf::Vector2f ship_position;
		sf::Int32 ship_identifier;
		sf::Int32 hitpoints;
		sf::Int32 ammo;
		packet >> ship_identifier >> ship_position.x >> ship_position.y >> hitpoints >> ammo;

		Ship* ship = m_world.GetShip(ship_identifier);
		bool is_local_plane = std::find(m_local_player_identifiers.begin(), m_local_player_identifiers.end(), ship_identifier) != m_local_player_identifiers.end();

		if (ship && !is_local_plane)
		{
			sf::Vector2f interpolated_position = ship->getPosition() + (ship_position - ship->getPosition()) * 0.1f;
			ship->setPosition(interpolated_position);
			ship->SetHitpoints(hitpoints);
			ship->SetMissileAmmo(ammo);
		}
	}
}

void MultiplayerGameState::HandleSpawnPickup(sf::Packet& packet)
{
	sf::Int32 type;
	sf::Vector2f position;
	packet >> type >> position.x >> position.y;
	m_world.CreatePickup(position, static_cast<PickupType>(type));
}

void MultiplayerGameState::HandlePacket(sf::Int32 packet_type, sf::Packet& packet)
{
	switch (static_cast<Server::PacketType>(packet_type))
	{
		//Send message to all Clients

	case Server::PacketType::kBroadcastMessage:
	{
		HandleBroadcastMessage(packet);
	}
	break;

	//Sent by the server to spawn player 1 airplane on connect
	case Server::PacketType::kSpawnSelf:
	{
		HandleSpawnSelf(packet);
	}
	break;


	case Server::PacketType::kPlayerConnect:
	{
		HandlePlayerConnect(packet);
	}
	break;


	case Server::PacketType::kPlayerDisconnect:
	{
		HandlePlayerDisconnect(packet);
	}
	break;

	case Server::PacketType::kInitialState:
	{
		HandleInitialState(packet);
	}
	break;

	//Player event, like missile fired occurs
	case Server::PacketType::kPlayerEvent:
	{
		HandlePlayerEvent(packet);
	}
	break;

	//Player's movement or fire keyboard state changes
	case Server::PacketType::kPlayerRealtimeChange:
	{
		HandleRealTimeChange(packet);
	}
	break;

	//New Enemy to be created
	case Server::PacketType::kSpawnEnemy:
	{
		HandleSpawnEnemy(packet);
	}
	break;

	//Mission Successfully completed
	case Server::PacketType::kMissionSuccess:
	{
		RequestStackPush(StateID::kMissionSuccess);
	}
	break;

	//Pickup created
	case Server::PacketType::kSpawnPickup:
	{
		HandleSpawnPickup(packet);
	}
	break;

	case Server::PacketType::kUpdateClientState:
	{
		HandleUpdateClient(packet);
	}
	break;

	}
}

