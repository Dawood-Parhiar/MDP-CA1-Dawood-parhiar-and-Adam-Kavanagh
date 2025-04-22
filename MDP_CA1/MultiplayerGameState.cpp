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

//Debuging help Chatgpt
//https://chatgpt.com/share/67eb1d07-0914-800c-b587-17327464c1a9

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
	, m_local_player_id(1)
    
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

		bool found_local_ship = (m_local_player_id != -1);
		// Iterate over all players.
		for (auto itr = m_players.begin(); itr != m_players.end(); )
		{
			// Check if this is our local player.
			if (m_local_player_id && !m_playerShip.empty())
			{
				found_local_ship = true;
			}

			sf::Int8 player_id = itr->first;

			// Look up the ship id for this player.
			auto mapIt = m_playerShip.find(player_id);
			Ship* ship = nullptr;
			if (mapIt != m_playerShip.end())
			{
				ship = m_world.GetShip(mapIt->second);
			}
			// If the ship no longer exists, remove this player.
			if (!ship)
			{
				itr = m_players.erase(itr);
				if (m_players.empty())
				{
					RequestStackPush(StateID::kGameOver);
				}
			}
			else
			{
				++itr;
			}
		}

		if (!found_local_ship && m_game_started)
		{
			Utility::Debug("No Ships found in the game");
			RequestStackPush(StateID::kGameOver);
		}

		// Handle real-time input if active and the window has focus.
		if (m_active_state && m_has_focus)
		{
			CommandQueue& commands = m_world.GetCommandQueue();
			auto it = m_players.find(static_cast<sf::Int8>(m_local_player_id));
			if (it != m_players.end())
			{
				it->second->HandleRealtimeInput(commands);
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
			//// Check for timeout.
			//if (m_time_since_last_packet > m_client_timeout)
			//{
			//	m_connected = false;
			//	m_failed_connection_text.setString("Lost connection to the server");
			//	Utility::CentreOrigin(m_failed_connection_text);
			//	m_failed_connection_clock.restart();
			//}
		}

		UpdateBroadcastMessage(dt);

		// Blink the invitation text.
		/*m_player_invitation_time += dt;
		if (m_player_invitation_time > sf::seconds(1.f))
		{
			m_player_invitation_time = sf::Time::Zero;
		}*/

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
			if (m_local_player_id != -1)
			{
				position_update_packet << static_cast<sf::Int32>(1);
				// Use the mapping to get the ship for the local player.
				Ship* ship = m_world.GetShip(m_playerShip[m_local_player_id]);
				if (ship)
				{
					position_update_packet << m_local_player_id
						<< ship->getPosition().x
						<< ship->getPosition().y
						<< static_cast<sf::Int32>(ship->GetHitPoints())
						<< static_cast<sf::Int32>(ship->GetMissileAmmo());
				}
			}
			else
			{
				position_update_packet << static_cast<sf::Int32>(0);
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
		//fire missile
		else if (event.key.code == sf::Keyboard::Enter)
		{
			sf::Packet packet;
			packet << static_cast<sf::Int32>(Client::PacketType::kPlayerEvent) << m_local_player_id << static_cast<sf::Int32>(Action::kMissileFire);
			m_socket.send(packet);
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
	if (m_local_player_id != -1 && m_players.find(m_local_player_id) != m_players.end())
	{
		m_players[m_local_player_id]->DisableAllRealtimeActions();
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
	sf::Int8 player_id, ship_id, role;
	sf::Vector2f ship_position;
	packet >> player_id >> ship_id >> role >> ship_position.x >> ship_position.y;

	Ship* existingShip = m_world.GetShip(ship_id);
	if (!existingShip)
	{
		Ship* ship = m_world.AddShip(ship_id);
		ship->setPosition(ship_position);
	}


	// Create local player with controls based on role
	if (role == static_cast<sf::Int8>(Role::Captain))
	{
		m_players[player_id] = std::make_unique<Player>(&m_socket, player_id, GetContext().keys1);
	}
	else if (role == static_cast<sf::Int8>(Role::Gunner))
	{
		m_players[player_id] = std::make_unique<Player>(&m_socket, player_id, GetContext().keys2);
	}
	else
	{
		m_players[player_id] = std::make_unique<Player>(&m_socket, player_id, nullptr);
	}

	m_local_player_id = player_id;
	m_playerShip[player_id] = ship_id;
	m_game_started = true;
}



void MultiplayerGameState::HandlePlayerConnect(sf::Packet& packet)
{
	sf::Int8 player_id, ship_id, role;
	sf::Vector2f ship_position;
	packet >> player_id >> ship_id >> role >> ship_position.x >> ship_position.y;

	Ship* existingShip = m_world.GetShip(ship_id);
	if (!existingShip)
	{
		Ship* ship = m_world.AddShip(ship_id);
		ship->setPosition(ship_position);
	}


	if (role == static_cast<sf::Int8>(Role::Captain))
	{
		m_players[player_id] = std::make_unique<Player>(&m_socket, player_id, GetContext().keys1);
	}
	else if (role == static_cast<sf::Int8>(Role::Gunner))
	{
		m_players[player_id] = std::make_unique<Player>(&m_socket, player_id, GetContext().keys2);
	}
	else
	{
		m_players[player_id] = std::make_unique<Player>(&m_socket, player_id, nullptr);
	}

	m_playerShip[player_id] = ship_id;
}


void MultiplayerGameState::HandlePlayerDisconnect(sf::Packet& packet)
{
	sf::Int8 player_id;
	packet >> player_id;

	std::cerr << "Player " << player_id << " Disconnected" << std::endl;

	// Check if player exists before removing
	auto player_it = m_players.find(player_id);
	if (player_it == m_players.end())
	{
		std::cerr << "Error: Player " << player_id << " not found in game." << std::endl;
		return;
	}

	// Remove from assigned ship
	for (auto& ship : m_world.GetShips())
	{
		if (ship->GetPilot() == player_id)
		{
			std::cerr << "Pilot " << player_id << " removed from ship " << ship->GetIdentifier() << std::endl;
			ship->SetPilot(-1);
		}
		else if (ship->HasGunner() == player_id)
		{
			std::cerr << "Gunner " << player_id << " removed from ship " << ship->GetIdentifier() << std::endl;
			ship->SetGunner(-1);
		}

		// If ship is now empty, delete it
		if (ship->GetPilot() == -1 && ship->HasGunner() == -1)
		{
			std::cerr << "Ship " << ship->GetIdentifier() << " removed (empty)." << std::endl;
			m_world.RemoveShip(ship->GetIdentifier());
		}
	}

	// Remove player from tracking
	m_players.erase(player_id);
}


void MultiplayerGameState::HandleInitialState(sf::Packet& packet)
{
	// World settings
	float world_height, current_scroll;
	packet >> world_height >> current_scroll;
	m_world.SetWorldHeight(world_height);
	m_world.SetCurrentBattleFieldPosition(current_scroll);

	// Number of ships
	sf::Int32 ship_count;
	packet >> ship_count;

	for (sf::Int32 i = 0; i < ship_count; ++i)
	{
		sf::Int8 ship_id;
		sf::Vector2f ship_position;
		sf::Int8 hitpoints, missile_ammo;
		sf::Int8 pilot_id, gunner_id;

		packet >> ship_id
			>> ship_position.x >> ship_position.y
			>> hitpoints >> missile_ammo
			>> pilot_id >> gunner_id;

		// Check if the ship already exists before adding it
		Ship* ship = m_world.GetShip(ship_id);
		if (!ship)
		{
			ship = m_world.AddShip(ship_id);
			ship->setPosition(ship_position);
		}
		ship->SetHitpoints(hitpoints);
		ship->SetMissileAmmo(missile_ammo);
		ship->SetPilot(pilot_id);
		ship->SetGunner(gunner_id);


		// Add player objects for roles if assigned
		if (pilot_id != -1)
		{
			m_players[pilot_id] = std::make_unique<Player>(&m_socket, pilot_id, GetContext().keys1);
			m_playerShip[pilot_id] = ship_id;
		}
		if (gunner_id != -1)
		{
			m_players[gunner_id] = std::make_unique<Player>(&m_socket, gunner_id, GetContext().keys2);
			m_playerShip[gunner_id] = ship_id;
		}
	}

	m_game_started = true;
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
		bool is_local_plane = (ship_identifier == static_cast<sf::Int32>(m_local_player_id));

		if (ship && !is_local_plane)
		{
			sf::Vector2f interpolated_position = ship->getPosition() + (ship_position - ship->getPosition()) * 0.1f;
			ship->setPosition(interpolated_position);
			ship->SetHitpoints(hitpoints);
			ship->SetMissileAmmo(ammo);
		}
	}
}

void MultiplayerGameState::HandlePacket(sf::Int32 packet_type, sf::Packet& packet)
{
	switch (static_cast<Server::PacketType>(packet_type))
	{
		// Player readied up
	/*case Server::PacketType::kPlayerReady:
	{
		sf::Int32 player_id;
		bool is_ready;
		packet >> player_id >> is_ready;
		m_ready_players[player_id] = is_ready;
	}
	break;*/

	// Server starts the game
	/*case Server::PacketType::kGameStart:
	{
		m_game_started = true;
	}
	break;*/

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

	case Server::PacketType::kAcceptCoopPartner:
	{
		/*sf::Int32 ship_identifier;
		packet >> ship_identifier;*/

		//m_world.AddShip(ship_identifier);
		//m_players[ship_identifier].reset(new Player(&m_socket, ship_identifier, GetContext().keys2));
		//m_local_player_identifiers.emplace_back(ship_identifier);
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
		sf::Int32 type;
		sf::Vector2f position;
		packet >> type >> position.x >> position.y;
		m_world.CreatePickup(position, static_cast<PickupType>(type));
	}
	break;

	case Server::PacketType::kUpdateClientState:
	{
		HandleUpdateClient(packet);
	}
	break;
	}
}

