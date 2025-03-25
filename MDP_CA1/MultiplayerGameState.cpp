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

MultiplayerGameState::MultiplayerGameState(StateStack& stack, Context context, bool is_host)
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

	
	/*m_player_invitation_text.setFont(context.fonts->Get(Font::kMain));
	m_player_invitation_text.setCharacterSize(20);
	m_player_invitation_text.setFillColor(sf::Color::White);
	m_player_invitation_text.setString("Press Enter to spawn player 2");
	m_player_invitation_text.setPosition(1000 - m_player_invitation_text.getLocalBounds().width, 760 - m_player_invitation_text.getLocalBounds().height);*/
	

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

	//if (m_connected && !m_game_started)
	//{
	//	m_window.clear(sf::Color::Cyan);
	//	m_window.setView(m_window.getDefaultView());
	//	// Draw lobby UI
	//	sf::RectangleShape lobby_container(sf::Vector2f(600.f, 400.f));
	//	lobby_container.setFillColor(sf::Color(50, 50, 50, 200));
	//	lobby_container.setOutlineColor(sf::Color::White);
	//	lobby_container.setOutlineThickness(3.f);
	//	lobby_container.setPosition((m_window.getSize().x - 600.f) / 2, (m_window.getSize().y - 400.f) / 2);

	//	m_window.draw(lobby_container);

	//	// Draw title
	//	sf::Text title("Multiplayer Lobby", *m_failed_connection_text.getFont(), 30);
	//	title.setFillColor(sf::Color::White);
	//	Utility::CentreOrigin(title);
	//	title.setPosition(m_window.getSize().x / 2, (m_window.getSize().y / 2) - 160.f);
	//	m_window.draw(title);

	//	// Display player list
	//	float y_offset = (m_window.getSize().y / 2) - 100.f;
	//	for (const auto& player : m_players)
	//	{
	//		sf::Text player_text("Player " + std::to_string(player.first) + " - " + (m_ready_players[player.first] ? "Ready" : "Not Ready"),
	//			*m_failed_connection_text.getFont(), 20);
	//		player_text.setFillColor(m_ready_players[player.first] ? sf::Color::Green : sf::Color::Red);
	//		Utility::CentreOrigin(player_text);
	//		player_text.setPosition(m_window.getSize().x / 2, y_offset);
	//		y_offset += 40.f;
	//		m_window.draw(player_text);
	//	}

	//	// Show "Press Enter to Start" only for host
	//	if (m_host)
	//	{
	//		sf::Text start_text("Press G to Start", *m_failed_connection_text.getFont(), 25);
	//		start_text.setFillColor(sf::Color::White);
	//		Utility::CentreOrigin(start_text);
	//		start_text.setPosition(m_window.getSize().x / 2, y_offset + 50.f);
	//		m_window.draw(start_text);
	//	}

	//	m_window.display();
	//}

	//Play the game music
	context.music->Play(MusicThemes::kMissionTheme);
}

void MultiplayerGameState::Draw()
{

	if (m_connected)
	{
		m_world.Draw();

		//Show the broadcast message in default view
		m_window.setView(m_window.getDefaultView());

		if (!m_broadcasts.empty())
		{
			m_window.draw(m_broadcast_text);
		}

		if (m_local_player_identifiers.size() < 2 && m_player_invitation_time < sf::seconds(0.5f))
		{
			m_window.draw(m_player_invitation_text);
		}
	}
	else
	{
		m_window.draw(m_failed_connection_text);
	}
}

//void MultiplayerGameState::Draw()
//{
//	if (!m_game_started)
//	{
//		m_window.clear();
//		m_window.setView(m_window.getDefaultView());
//
//		// Draw lobby UI
//		sf::RectangleShape lobby_container(sf::Vector2f(600.f, 400.f));
//		lobby_container.setFillColor(sf::Color(50, 50, 50, 200));
//		lobby_container.setOutlineColor(sf::Color::White);
//		lobby_container.setOutlineThickness(3.f);
//		lobby_container.setPosition((m_window.getSize().x - 600.f) / 2, (m_window.getSize().y - 400.f) / 2);
//
//		m_window.draw(lobby_container);
//
//		// Draw title
//		sf::Text title("Multiplayer Lobby", *m_failed_connection_text.getFont(),30);
//		title.setFillColor(sf::Color::White);
//		Utility::CentreOrigin(title);
//		title.setPosition(m_window.getSize().x / 2, (m_window.getSize().y / 2) - 160.f);
//		m_window.draw(title);
//
//		// Display player list
//		float y_offset = (m_window.getSize().y / 2) - 100.f;
//		for (const auto& player : m_players)
//		{
//			sf::Text player_text("Player " + std::to_string(player.first) + " - " + (m_ready_players[player.first] ? "Ready" : "Not Ready"),
//				*m_failed_connection_text.getFont(), 20);
//			player_text.setFillColor(m_ready_players[player.first] ? sf::Color::Green : sf::Color::Red);
//			Utility::CentreOrigin(player_text);
//			player_text.setPosition(m_window.getSize().x / 2, y_offset);
//			y_offset += 40.f;
//			m_window.draw(player_text);
//		}
//
//		// Show "Press Enter to Start" only for host
//		if (m_host)
//		{
//			sf::Text start_text("Press G to Start", *m_failed_connection_text.getFont(), 25);
//			start_text.setFillColor(sf::Color::White);
//			Utility::CentreOrigin(start_text);
//			start_text.setPosition(m_window.getSize().x / 2, y_offset + 50.f);
//			m_window.draw(start_text);
//		}
//
//		m_window.display();
//	}
//	else
//	{
//		// Draw the game world when game starts
//		m_world.Draw();
//		m_window.setView(m_window.getDefaultView());
//
//		if (!m_broadcasts.empty())
//		{
//			m_window.draw(m_broadcast_text);
//		}
//
//		if (m_local_player_identifiers.size() < 2 && m_player_invitation_time < sf::seconds(0.5f))
//		{
//			m_window.draw(m_player_invitation_text);
//		}
//	}
//}

bool MultiplayerGameState::Update(sf::Time dt)
{
	//Connected to the Server: Handle all the network logic
	if (m_connected)
	{
		m_world.Update(dt);

		//Remove players whose aircraft were destroyed
		bool found_local_ship = false;
		for (auto itr = m_players.begin(); itr != m_players.end();)
		{
			//Check if there are no more local planes for remote clients
			if (std::find(m_local_player_identifiers.begin(), m_local_player_identifiers.end(), itr->first) != m_local_player_identifiers.end())
			{
				found_local_ship = true;
			}

			if (!m_world.GetShip(itr->first))
			{
				itr = m_players.erase(itr);

				//No more players left : Mission failed
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
			RequestStackPush(StateID::kGameOver);
		}

		//Only handle the realtime input if the window has focus and the game is unpaused
		if (m_active_state && m_has_focus)
		{
			CommandQueue& commands = m_world.GetCommandQueue();
			for (auto& pair : m_players)
			{
				pair.second->HandleRealtimeInput(commands);
			}
		}

		//Always handle the network input
		CommandQueue& commands = m_world.GetCommandQueue();
		for (auto& pair : m_players)
		{
			pair.second->HandleRealtimeNetworkInput(commands);
		}

		//Handle messages from the server that may have arrived
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
			//Check for timeout with the server
			if (m_time_since_last_packet > m_client_timeout)
			{
				m_connected = false;
				m_failed_connection_text.setString("Lost connection to the server");
				Utility::CentreOrigin(m_failed_connection_text);

				m_failed_connection_clock.restart();
			}
		}

		UpdateBroadcastMessage(dt);

		//Time counter fro blinking second player text
		m_player_invitation_time += dt;
		if (m_player_invitation_time > sf::seconds(1.f))
		{
			m_player_invitation_time = sf::Time::Zero;
		}

		//Events occurring in the game
		GameActions::Action game_action;
		while (m_world.PollGameAction(game_action))
		{
			sf::Packet packet;
			packet << static_cast<sf::Int32>(Client::PacketType::kGameEvent);
			packet << static_cast<sf::Int32>(game_action.type);
			packet << game_action.position.x;
			packet << game_action.position.y;

			m_socket.send(packet);
		}

		//Regular position updates
		if (m_tick_clock.getElapsedTime() > sf::seconds(1.f / 20.f))
		{
			sf::Packet position_update_packet;
			position_update_packet << static_cast<sf::Int32>(Client::PacketType::kStateUpdate);
			position_update_packet << static_cast<sf::Int32>(m_local_player_identifiers.size());

			for (sf::Int32 identifier : m_local_player_identifiers)
			{
				if (Ship* ship = m_world.GetShip(identifier))
				{
					position_update_packet << identifier << ship->getPosition().x << ship->getPosition().y << static_cast<sf::Int32>(ship->GetHitPoints()) << static_cast<sf::Int32>(ship->GetMissileAmmo());
				}
			}
			m_socket.send(position_update_packet);
			m_tick_clock.restart();
		}
		m_time_since_last_packet += dt;
	}

	//Failed to connect and waited for more than 5 seconds: Back to menu
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
		//If enter pressed, add second player co-op only if there is only 1 player
		/*if (event.key.code == sf::Keyboard::Space  && m_local_player_identifiers.size() == 1)
		{
			sf::Packet packet;
			packet << static_cast<sf::Int32>(Client::PacketType::kRequestCoopPartner);
			m_socket.send(packet);
		}*/
		//If escape is pressed, show the pause screen
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
//
//bool MultiplayerGameState::HandleEvent(const sf::Event& event)
//{
//	// Game input handling
//	CommandQueue& commands = m_world.GetCommandQueue();
//
//	// Forward events to all players
//	for (auto& pair : m_players)
//	{
//		pair.second->HandleEvent(event, commands);
//	}
//
//	if (!m_game_started)
//	{
//		// Lobby-related event handling
//
//		if (event.type == sf::Event::KeyPressed)
//		{
//			// Players toggle ready status
//			if (event.key.code == sf::Keyboard::R)
//			{
//				m_ready_players[m_local_player_identifiers[0]] = !m_ready_players[m_local_player_identifiers[0]];
//
//				// Send a "Ready" update to the server
//				sf::Packet packet;
//				packet << static_cast<sf::Int32>(Client::PacketType::kPlayerReady);
//				packet << static_cast<sf::Int32>(m_local_player_identifiers[0]) << m_ready_players[m_local_player_identifiers[0]];
//				m_socket.send(packet);
//			}
//
//			// Host can start the game if all are ready
//			else if (m_host && event.key.code == sf::Keyboard::G)
//			{
//				bool all_ready = std::all_of(m_ready_players.begin(), m_ready_players.end(), [](const auto& pair) {
//					return pair.second;
//					});
//
//				if (all_ready)
//				{
//					sf::Packet start_packet;
//					start_packet << static_cast<sf::Int32>(Client::PacketType::kGameStart);
//					m_socket.send(start_packet);
//					
//				}
//			}
//
//		}
//	}// If escape is pressed, show the pause screen
//	else if (event.key.code == sf::Keyboard::Escape)
//	{
//		DisableAllRealtimeActions();
//		RequestStackPush(StateID::kNetworkPause);
//	}
//	// Focus handling
//	else if (event.type == sf::Event::GainedFocus)
//	{
//		m_has_focus = true;
//	}
//	else if (event.type == sf::Event::LostFocus)
//	{
//		m_has_focus = false;
//	}
//
//	return true;  // Event handled in lobby state
//}

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
	break;

	//Sent by the server to spawn player 1 airplane on connect
	case Server::PacketType::kSpawnSelf:
	{
			//spawn odd connecting player as player 1 with binding to keys1, even connecting player as player 2 with binding to keys2
		
		sf::Int32 player_id;
		sf::Vector2f ship_position;
		packet >> player_id >> ship_position.x >> ship_position.y;

		sf::Int32 ship_identifier = 1;
		//Determine ship ID, each ship has 2 players
		Ship* ship = m_world.AddShip(ship_identifier);
		ship->setPosition(ship_position);

		m_players[player_id].reset(new Player(&m_socket, player_id, GetContext().keys1));
			ship->SetPilot(player_id);

		m_local_player_identifiers.push_back(player_id);
		m_game_started = true;

		/*sf::Int32 aircraft_identifier;
		sf::Vector2f aircraft_position;
		packet >> aircraft_identifier >> aircraft_position.x >> aircraft_position.y;
		Ship* aircraft = m_world.AddShip(aircraft_identifier);
		aircraft->setPosition(aircraft_position);
		m_players[aircraft_identifier].reset(new Player(&m_socket, aircraft_identifier, GetContext().keys1));
		m_local_player_identifiers.push_back(aircraft_identifier);
		m_game_started = true;*/
	}
	break;

	case Server::PacketType::kPlayerConnect:
	{
		sf::Int32 player_id;
		sf::Vector2f ship_position;
		packet >> player_id >> ship_position.x >> ship_position.y;

		std::cerr << "New player connected: " << player_id << std::endl;

		// Find an available ship or create a new one
		Ship* assigned_ship = nullptr;
		sf::Int32 ship_identifier = -1;

		for (auto& ship : m_world.GetShips()) 
		{
			if (!ship->HasGunner()) // If the ship has space for a second player
			{
				assigned_ship = ship;
				ship_identifier = ship->GetIdentifier();
				break;
			}
		}

		// If no existing ship had space, create a new one
		if (!assigned_ship)
		{
			ship_identifier = m_world.GetShips().size();  // New ship ID
			assigned_ship = m_world.AddShip(ship_identifier);
			assigned_ship->setPosition(ship_position);
			std::cerr << "Created new ship (ID: " << ship_identifier << ")" << std::endl;
		}

		// Assign player as pilot or gunner
		bool is_pilot = (assigned_ship->GetPilot() == -1);
		if (is_pilot)
		{
			m_players[player_id].reset(new Player(&m_socket, player_id, GetContext().keys1));
			assigned_ship->SetPilot(player_id);
		}
		else
		{
			m_players[player_id].reset(new Player(&m_socket, player_id, GetContext().keys2));
			assigned_ship->SetGunner(player_id);
		}

		m_local_player_identifiers.push_back(player_id);
	}
	break;

	case Server::PacketType::kPlayerDisconnect:
	{
		sf::Int32 ship_identifier;
		packet >> ship_identifier;
		//m_world.RemoveShip(ship_identifier);
		m_players.erase(ship_identifier);
	}
	break;

	case Server::PacketType::kInitialState:
	{
		sf::Int32 ship_count;
		float world_height, current_scroll;
		packet >> world_height >> current_scroll;

		m_world.SetWorldHeight(world_height);
		m_world.SetCurrentBattleFieldPosition(current_scroll);

		packet >> ship_count;
		for (sf::Int32 i = 0; i < ship_count; ++i)
		{
			sf::Int32 ship_identifier;
			sf::Int32 hitpoints;
			sf::Int32 missile_ammo;
			sf::Vector2f ship_position;
			packet >> ship_identifier >> ship_position.x >> ship_position.y >> hitpoints >> missile_ammo;

			Ship* ship = m_world.AddShip(ship_identifier);
			ship->setPosition(ship_position);
			ship->SetHitpoints(hitpoints);
			ship->SetMissileAmmo(missile_ammo);

			m_players[ship_identifier].reset(new Player(&m_socket, ship_identifier, nullptr));
		}
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
		sf::Int32 ship_identifier;
		sf::Int32 action;
		packet >> ship_identifier >> action;

		auto itr = m_players.find(ship_identifier);
		if (itr != m_players.end())
		{
			itr->second->HandleNetworkEvent(static_cast<Action>(action), m_world.GetCommandQueue());
		}
	}
	break;

	//Player's movement or fire keyboard state changes
	case Server::PacketType::kPlayerRealtimeChange:
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
	break;

	//New Enemy to be created
	case Server::PacketType::kSpawnEnemy:
	{
		float height;
		sf::Int32 type;
		float relative_x;
		packet >> type >> height >> relative_x;

		m_world.AddEnemy(static_cast<ShipType>(type), relative_x, height);
		m_world.SortEnemies();
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
	break;
	}
}

