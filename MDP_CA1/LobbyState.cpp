//#include "LobbyState.hpp"
//
//#include <iostream>
//#include <SFML/Network/Packet.hpp>
//
//#include "FontID.hpp"
//#include "NetworkProtocol.hpp"
//#include "TextNode.hpp"
//
//
//LobbyState::LobbyState(StateStack& stack, Context context)
//    : State(stack, context)
//    , m_thread(&LobbyState::ExecutionThread, this)
//    , m_listening_state(false),
//    m_client_timeout(sf::seconds(10.f)),
//    m_waiting_thread_end(false),
//    m_connected_players(0),
//    m_ships_assigned(0),
//    m_max_connected_players(30),
//    m_ship_id_counter(0)
//
//{
//    m_background_sprite.setTexture(context.textures->Get(TextureID::kLobbyBg));
//
//
//    // Title label
//    auto title_label = std::make_shared<gui::Label>("Lobby - Waiting for Players", *context.fonts, *context.textures);
//    title_label->setPosition(200.f, 50.f);
//    m_gui_container.Pack(title_label);
//
//    // Start button (disabled until 2 players are assigned)
//    m_start_button = std::make_shared<gui::Button>(context);
//    m_start_button->setPosition(1600.f, 900.f);
//    m_start_button->SetText("Start Game");
//    m_start_button->SetCallback([this]() { RequestStackPop(); RequestStackPush(StateID::kGame); });
//   // m_start_button->SetEnabled(true); // Initially disabled
//    
//
//    auto back_button = std::make_shared<gui::Button>(context);
//    back_button->setPosition(1600.f, 750.f);
//    back_button->SetText("Return");
//    back_button->SetCallback([this] { RequestStackPop(); RequestStackPush(StateID::kMenu); });
//
//    m_gui_container.Pack(m_start_button);
//    m_gui_container.Pack(back_button);
//
//    SetListening(true);
//    m_thread.launch();
//}
//
//LobbyState::~LobbyState()
//{
//    m_waiting_thread_end = true;
//    m_thread.wait();
//}
//
//void LobbyState::Draw()
//{
//    sf::RenderWindow& window = *GetContext().window;
//    window.draw(m_background_sprite);
//    window.draw(m_gui_container);
//
//    for (const auto& label : m_player_labels)
//    {
//        window.draw(*label);
//    }
//}
//
//bool LobbyState::Update(sf::Time dt)
//{
//    return true;
//}
//
//void LobbyState::UpdatePlayerList()
//{
//    m_player_labels.clear();
//    for (size_t i = 0; i < m_players.size(); ++i)
//    {
//        auto label = std::make_shared<gui::Label>("Player " + std::to_string(m_players[i]), *GetContext().fonts, *GetContext().textures);
//        label->setPosition(100.f, 50.f + i * 30.f);
//        m_player_labels.push_back(label);
//    }
//}
//
//void LobbyState::HandlePlayerJoin(int player_id)
//{
//    m_players.push_back(player_id);
//    UpdatePlayerList();
//    TryAssignShips();
//}
//
//void LobbyState::TryAssignShips()
//{
//    while (m_players.size() >= 2)
//    {
//        int p1 = m_players.back(); m_players.pop_back();
//        int p2 = m_players.back(); m_players.pop_back();
//        m_assigned_ships.emplace_back(std::to_string(p1), std::to_string(p2));
//        std::cout << "Assigned Players " << p1 << " and " << p2 << " to a ship." << std::endl;
//    }
//}
//
//bool LobbyState::HandleEvent(const sf::Event& event)
//{
//    m_gui_container.HandleEvent(event);
//    return true;
//}
//
//void LobbyState::AddPlayer(const std::string& playerName)
//{
//    m_players.push_back(m_connected_players);
//    m_connected_players++;
//
//    auto label = std::make_shared<gui::Label>(playerName, *GetContext().fonts, *GetContext().textures);
//    label->setPosition(100.f, 200.f + m_players.size() * 50);
//    m_player_labels.push_back(label);
//}
//
//void LobbyState::AssignShips()
//{
//    for (size_t i = 0; i < m_players.size(); i += 2)
//    {
//        if (i + 1 < m_players.size())
//        {
//            std::string ship_id = "Ship_" + std::to_string(m_ship_id_counter++);
//            m_assigned_ships.push_back({ std::to_string(m_players[i]), std::to_string(m_players[i + 1]) });
//            std::cout << "Assigned Ship " << ship_id << " to Players " << m_players[i] << " and " << m_players[i + 1] << "\n";
//        }
//    }
//}
//
//LobbyState::RemotePeer::RemotePeer(): m_ready(false), m_timed_out(false)
//{
//    m_socket.setBlocking(false);
//}
//
//void LobbyState::SetListening(bool enable)
//{
//    if (enable)
//    {
//        if (!m_listening_state)
//        {
//            m_listening_state = (m_listener_socket.listen(SERVER_PORT) == sf::Socket::Done);
//        }
//    }
//    else
//    {
//        m_listener_socket.close();
//        m_listening_state = false;
//    }
//}
//
//void LobbyState::ExecutionThread()
//{
//    while (!m_waiting_thread_end)
//    {
//        HandleIncomingConnections();
//        HandleIncomingPackets();
//        sf::sleep(sf::milliseconds(10)); // Prevents tight infinite loop
//    }
//}
//
//
//void LobbyState::Tick()
//{
//}
//
//sf::Time LobbyState::Now() const
//{
//    return m_clock.getElapsedTime();
//}
//
//void LobbyState::HandleIncomingPackets()
//{
//    for (auto& peer : m_peers)
//    {
//        sf::Packet packet;
//        if (peer->m_socket.receive(packet) == sf::Socket::Done)
//        {
//            int packet_type;
//            packet >> packet_type;
//
//            switch (packet_type)
//            {
//            case static_cast<int>(Server::PacketType::kPlayerConnect):
//            {
//                int player_id;
//                packet >> player_id;
//                HandlePlayerJoin(player_id);
//                UpdateClientState(); // Notify all players
//                break;
//            }
//            case static_cast<int>(Server::PacketType::kBroadcastMessage):
//            {
//                std::string message;
//                packet >> message;
//                BroadcastMessage(message);
//                break;
//            }
//            case static_cast<int>(Server::PacketType::kPlayerReady):
//            {
//                peer->m_ready = true;
//                std::cout <<"Player " << peer->m_socket.getRemotePort() <<" is ready!" << std::endl;
//                break;
//            }
//            default:
//                std::cout << "Received unknown packet type." << std::endl;
//                break;
//            }
//        }
//    }
//}
//
//
//void LobbyState::HandleIncomingPackets(sf::Packet& packet, RemotePeer& receiving_peer, bool& detected_timeout)
//{
//    for (auto& peer : m_peers)
//    {
//        sf::Packet packet;
//        if (peer->m_socket.receive(packet) == sf::Socket::Done)
//        {
//            int player_id;
//            packet >> player_id;
//            HandlePlayerJoin(player_id);
//        }
//    }
//}
//
//void LobbyState::HandleIncomingConnections()
//{
//    m_peers.emplace_back(std::make_unique<RemotePeer>());
//    if (m_listener_socket.accept(m_peers.back()->m_socket) == sf::Socket::Done)
//    {
//        std::cout << "New player joined!" << std::endl;
//        int new_player_id = static_cast<int>(m_peers.size()); // Assign a player ID
//        HandlePlayerJoin(new_player_id);
//        UpdateClientState(); // Notify all clients
//    }
//    else
//    {
//        m_peers.pop_back(); // Remove the failed connection
//    }
//}
//
//
//
//void LobbyState::HandleDisconnections()
//{
//    for (auto it = m_peers.begin(); it != m_peers.end();)
//    {
//        if ((*it)->m_socket.getRemoteAddress() == sf::IpAddress::None)
//        {
//            std::cout << "Player disconnected!" << std::endl;
//            it = m_peers.erase(it);
//        }
//        else
//        {
//            ++it;
//        }
//    }
//}
//
//
//void LobbyState::InformWorldState(sf::TcpSocket& socket)
//{
//}
//
//void LobbyState::BroadcastMessage(const std::string& message)
//{
//    sf::Packet packet;
//    packet << message;
//    SendToAll(packet);
//}
//
//
//void LobbyState::SendToAll(sf::Packet& packet)
//{
//    for (auto& peer : m_peers)
//    {
//        if (peer->m_socket.send(packet) != sf::Socket::Done)
//        {
//            std::cout << "Failed to send packet to a peer." << std::endl;
//        }
//    }
//}
//
//
//void LobbyState::UpdateClientState()
//{
//    sf::Packet packet;
//    packet << static_cast<int>(m_players.size()); // Send number of players
//    for (int player_id : m_players)
//    {
//        packet << player_id; // Send each player ID
//    }
//
//    SendToAll(packet);
//}
//

//Code from
// Dylan Goncalves Martins (D00242562) and Paul Bichler (D00242563)
//
//#include "LobbyState.hpp"
//#include <SFML/Graphics/RenderWindow.hpp>
//#include "NetworkProtocol.hpp"
//#include "Utility.hpp"
//#include <SFML/Network/Packet.hpp>
//#include <SFML/Network/TcpSocket.hpp>
//
//constexpr int TITLE_POS_Y = 30;
//constexpr int UNPAIRED_POS_X = 100;
//constexpr int TEAM_POS_Y = 260;
//constexpr int TEAM_COL_1_POS_X = 550;
//constexpr int TEAM_COL_2_POS_X = 950;
//constexpr int TEAM_BUTTON_GAP = 135;
//constexpr int FOOTER_POS_Y = 850;
//
//LobbyState::LobbyState(StateStack& stack, Context& context, const bool is_host)
//	: State(stack, context)
//	, m_player_input_name()
//	, m_connected(false)
//	, m_is_host(is_host)
//	, m_unpaired_y_pos(TEAM_POS_Y - 20)
//	, m_player_id(-1)
//	, m_time_since_last_packet(sf::seconds(0.f))
//	, m_client_timeout(sf::seconds(2.f))
//	, m_lobby_time(sf::seconds(0))
//	, m_send_time(sf::seconds(0.5f))
//	, m_start_countdown_timer(sf::seconds(5.f))
//{
//	CreateUI(context);
//
//	if (m_is_host)
//	{
//		context.m_multiplayer_manager->HostServer();
//		ip = "127.0.0.1";
//	}
//	else
//	{
//		ip = context.m_player_data_manager->GetData().m_ip_address;
//		//ip = GetAddressFromFile();
//	}
//
//	m_socket = context.m_multiplayer_manager->ConnectToServer(ip);
//	m_is_connecting = true;
//	m_failed_connection_clock.restart();
//
//
//	for (sf::Int8 i = 0; i < 8; ++i)
//	{
//		m_team_selections.try_emplace(i, std::vector<sf::Int8>());
//	}
//}
//
//void LobbyState::SendClientDisconnect(const sf::Int8 id) const
//{
//	sf::Packet packet;
//	packet << static_cast<sf::Int8>(Client::PacketType::kQuit);
//	packet << id;
//
//	m_socket->send(packet);
//}
//
//auto LobbyState::HandleTutorialPress() const
//{
//	return [this]
//		{
//			RequestStackPush(StateID::kTutorial);
//		};
//}
//
//auto LobbyState::HandleTeamButtonPressed(sf::Int8 id)
//{
//	return [this, id]
//		{
//			HandleTeamChoice(id);
//		};
//}
//
//auto LobbyState::HandleStartGamePressed() const
//{
//	return [this]
//		{
//			SendStartGameCountdown();
//		};
//}
//
//auto LobbyState::IsHostAndInTeam()
//{
//	return [this]
//		{
//			return m_is_host && m_team_selections[m_player_team_selection[m_player_id]].size() == 2 && !m_start_countdown &&
//				!m_game_started;
//		};
//}
//
//auto LobbyState::HandleLeaveTeamButtonPress()
//{
//	return [this]
//		{
//			HandleTeamChoice(0);
//		};
//}
//
//auto LobbyState::IsInATeam()
//{
//	return [this] { return m_player_team_selection[m_player_id] != 0; };
//}
//
//auto LobbyState::HandleBackButtonPressed() const
//{
//	return [this]
//		{
//			SendClientDisconnect(m_player_id);
//			RequestStackPop();
//			RequestStackPush(StateID::kMenu);
//		};
//}
//
///**
//* Dylan Goncalves Martins (D00242562)
//* Creates the lobby UI
//*/
//void LobbyState::CreateUI(Context& context)
//{
//	int y = context.m_window->getSize().y / 2;
//	int x = context.m_window->getSize().x / 2;
//
//	Utility::CreateLabel(context, m_failed_connection_text, x, y, "Attempting to connect...", 35);
//	Utility::CentreOrigin(m_failed_connection_text->GetText());
//	m_gui_fail_container.Pack(m_failed_connection_text);
//
//	std::shared_ptr<gui::Label> title_label;
//	Utility::CreateLabel(context, title_label, UNPAIRED_POS_X, TITLE_POS_Y, "Lobby", 100);
//	m_gui_container.Pack(title_label);
//
//	Utility::CreateButton(context, m_change_name_button, TEAM_COL_1_POS_X, TITLE_POS_Y + 10, "Name",
//		true);
//	m_gui_container.Pack(m_change_name_button);
//
//	Utility::CreateLabel(context, m_current_name_label, TEAM_COL_1_POS_X + 215, TITLE_POS_Y + 25,
//		m_player_input_name, 20);
//	m_gui_container.Pack(m_current_name_label);
//
//	std::shared_ptr<gui::Button> tutorial_button;
//	Utility::CreateButton(context, tutorial_button, TEAM_COL_1_POS_X, TITLE_POS_Y + 85,
//		"How to Play", HandleTutorialPress());
//	m_gui_container.Pack(tutorial_button);
//
//	std::shared_ptr<gui::Label> unpaired_label;
//	Utility::CreateLabel(context, unpaired_label, UNPAIRED_POS_X, TEAM_POS_Y - 50,
//		"Unpaired Players", 30);
//	m_gui_container.Pack(unpaired_label);
//
//	for (sf::Int8 id = 1; id <= 8; ++id)
//	{
//		std::shared_ptr<gui::Button> team_button;
//		y = TEAM_POS_Y + TEAM_BUTTON_GAP * ((id - 1 - (id - 1) % 2) / 2);
//		x = id % 2 == 0 ? TEAM_COL_2_POS_X : TEAM_COL_1_POS_X;
//		auto label = "Team " + std::to_string(id);
//		Utility::CreateButton(context, team_button, x, y, label, HandleTeamButtonPressed(id));
//		m_gui_container.Pack(team_button);
//	}
//
//	std::shared_ptr<gui::Button> start_game_button;
//	Utility::CreateButton(context, start_game_button, UNPAIRED_POS_X, FOOTER_POS_Y, "Start game",
//		HandleStartGamePressed(), IsHostAndInTeam());
//	m_gui_container.Pack(start_game_button);
//
//	std::shared_ptr<gui::Button> leave_team_button;
//	Utility::CreateButton(context, leave_team_button, TEAM_COL_1_POS_X, FOOTER_POS_Y, "Leave Team",
//		HandleLeaveTeamButtonPress(), IsInATeam());
//	m_gui_container.Pack(leave_team_button);
//
//	std::shared_ptr<gui::Button> back_button;
//	Utility::CreateButton(context, back_button, TEAM_COL_2_POS_X + 150, FOOTER_POS_Y, "Leave",
//		HandleBackButtonPressed());
//	m_gui_container.Pack(back_button);
//
//	std::shared_ptr<gui::Label> start_countdown_text_label;
//	Utility::CreateLabel(context, start_countdown_text_label, UNPAIRED_POS_X, FOOTER_POS_Y + 15, "Game starts in...",
//		30);
//	start_countdown_text_label->SetDrawPredicate([this] { return m_start_countdown; });
//	m_gui_container.Pack(start_countdown_text_label);
//
//	Utility::CreateLabel(context, m_start_countdown_label, UNPAIRED_POS_X + 250, FOOTER_POS_Y + 15,
//		std::to_string(m_start_countdown_timer.asSeconds()), 30);
//	m_start_countdown_label->SetDrawPredicate([this] { return m_start_countdown; });
//	m_gui_container.Pack(m_start_countdown_label);
//}
//
//bool LobbyState::TeamHasPlace(const sf::Int8 id)
//{
//	if (m_team_selections[id].size() < 2)
//	{
//		return true;
//	}
//
//	return false;
//}
//
///**
//* Dylan Goncalves Martins (D00242562)
//* Gets the appropriate screen pos for the label
//*/
//sf::Vector2f LobbyState::GetTeamPos(const int i)
//{
//	const int y = TEAM_POS_Y + TEAM_BUTTON_GAP * ((i - 1 - (i - 1) % 2) / 2);
//	const int x = i % 2 == 0 ? TEAM_COL_2_POS_X : TEAM_COL_1_POS_X;
//
//	return { static_cast<float>(x), static_cast<float>(y) };
//}
//
//sf::Vector2f LobbyState::GetUnpairedPos(const int i) const
//{
//	const int y = m_unpaired_y_pos + 30 * i;
//	const int x = UNPAIRED_POS_X;
//
//	return { static_cast<float>(x), static_cast<float>(y) };
//}
//
///**
//* Dylan Goncalves Martins (D00242562)
//* Moves a label to the right pos
//*/
//void LobbyState::MovePlayer(const sf::Int8 id, const sf::Int8 team_id)
//{
//	/**
//	* Dylan Goncalves Martins (D00242562)
//	* Removes from previous team pos
//	*/
//	if (m_player_team_selection[id] != 0)
//	{
//		m_team_selections[m_player_team_selection[id]].erase(
//			std::remove(m_team_selections[m_player_team_selection[id]].begin(),
//				m_team_selections[m_player_team_selection[id]].end(), id),
//			m_team_selections[m_player_team_selection[id]].end());
//	}
//
//	m_team_selections[team_id].emplace_back(id);
//
//	const sf::Vector2f pos = GetTeamPos(team_id);
//	float y = pos.y;
//
//	if (m_team_selections[team_id].front() != id)
//	{
//		y += 85;
//	}
//	else
//	{
//		y += 60;
//	}
//
//	m_players[id]->setPosition(pos.x, y);
//
//	m_player_team_selection[id] = team_id;
//}
//
//void LobbyState::MovePlayerBack(const sf::Int8 id)
//{
//	if (m_player_team_selection[id] != 0)
//	{
//		m_team_selections[m_player_team_selection[id]].erase(
//			std::remove(m_team_selections[m_player_team_selection[id]].begin(),
//				m_team_selections[m_player_team_selection[id]].end(), id),
//			m_team_selections[m_player_team_selection[id]].end());
//	}
//
//	m_players[id]->setPosition(GetUnpairedPos(id));
//	m_player_team_selection[id] = 0;
//}
//
//
///**
// * Dylan Goncalves Martins (D00242562)
// * Sends a packet to the server on team choice
// */
//void LobbyState::HandleTeamChoice(const sf::Int8 id)
//{
//	if (TeamHasPlace(id) || id == 0)
//	{
//		sf::Packet packet;
//		packet << static_cast<sf::Int8>(Client::PacketType::kTeamChange);
//		packet << m_player_id;
//		packet << id;
//		if (m_team_selections[id].empty())
//		{
//			packet << static_cast<sf::Int8>(0);
//		}
//		else
//		{
//			packet << static_cast<sf::Int8>(1);
//		}
//
//		m_socket->send(packet);
//	}
//}
//
//void LobbyState::Draw()
//{
//	sf::RenderWindow& window = *GetContext().m_window;
//	window.clear(sf::Color(0, 37, 97));
//
//	if (m_connected)
//	{
//		window.draw(m_gui_container);
//	}
//	else
//	{
//		window.draw(m_gui_fail_container);
//	}
//}
//
///**
// * Dylan Goncalves Martins (D00242562)
// * Sends a packet to the server so the server doesn't disconnect him
// */
//void LobbyState::NotifyServerOfExistence() const
//{
//	sf::Packet packet;
//	m_socket->send(packet);
//}
//
//bool LobbyState::Update(const sf::Time dt)
//{
//	/**
//	* Dylan Goncalves Martins (D00242562)
//	* This runs at the start until a succesfull conection between the client and the server have benn made
//	*/
//	if (m_is_connecting)
//	{
//		/**
//		* Dylan Goncalves Martins (D00242562)
//		* Sends a packet to the server to see if server succsefully connected
//		*/
//		sf::Packet packet;
//		if (m_socket->send(packet) == sf::Socket::Done)
//		{
//			m_is_connecting = false;
//			m_connected = true;
//			return true;
//		}
//		/**
//		* Dylan Goncalves Martins (D00242562)
//		* After some time and no succesfull connection we tell the client that he hasnt found a server
//		*/
//		if (m_failed_connection_clock.getElapsedTime() >= sf::seconds(5.f))
//		{
//			m_is_connecting = false;
//			m_failed_connection_text->SetText("No servers available");
//			Utility::CentreOrigin(m_failed_connection_text->GetText());
//			m_failed_connection_clock.restart();
//		}
//
//		return true;
//	}
//
//	/**
//	* Dylan Goncalves Martins (D00242562)
//	* As long as the player is conected this will be executed
//	*/
//	if (m_connected)
//	{
//		/**
//		* Dylan Goncalves Martins (D00242562)
//		* Every few frames send a packet to the server so he knows of the players existance
//		*/
//		if (m_lobby_time > m_send_time)
//		{
//			m_lobby_time = sf::seconds(0.f);
//			NotifyServerOfExistence();
//		}
//
//		sf::Packet packet;
//		if (m_socket->receive(packet) == sf::Socket::Done)
//		{
//			m_time_since_last_packet = sf::seconds(0.f);
//			sf::Int8 packet_type;
//			packet >> packet_type;
//			HandlePacket(packet_type, packet);
//		}
//		else
//		{
//			/**
//			* Dylan Goncalves Martins (D00242562)
//			* If there is no response from the server after some time set connected to false
//			*/
//			if (m_time_since_last_packet > m_client_timeout)
//			{
//				m_connected = false;
//				m_failed_connection_text->SetText("Lost connection to the server");
//				Utility::CentreOrigin(m_failed_connection_text->GetText());
//
//				m_failed_connection_clock.restart();
//			}
//		}
//		m_time_since_last_packet += dt;
//	}
//	//Failed to connect and waited for more than 5 seconds: Back to menu
//	else if (m_failed_connection_clock.getElapsedTime() >= sf::seconds(5.f))
//	{
//		RequestStackClear();
//		RequestStackPush(StateID::kMenu);
//	}
//
//	m_lobby_time += dt;
//
//	//Update the start game countdown (if the game has been started by the host)
//	if (m_start_countdown)
//	{
//		if (m_start_countdown_timer.asSeconds() > 0)
//		{
//			m_start_countdown_timer -= dt;
//			m_start_countdown_label->SetText(std::to_string(static_cast<int>(m_start_countdown_timer.asSeconds())));
//		}
//		else if (m_is_host)
//		{
//			SendStartGame();
//			m_start_countdown = false;
//		}
//	}
//
//	return true;
//}
//
//bool LobbyState::HandleEvent(const sf::Event& event)
//{
//	if (m_game_started)
//		return false;
//
//	if (m_change_name_button->IsActive())
//	{
//		//Name Input
//		if (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::Return)
//		{
//			m_change_name_button->Deactivate();
//			GetContext().m_player_data_manager->GetData().m_player_name = m_player_input_name;
//			GetContext().m_player_data_manager->Save();
//
//			m_players[m_player_id]->SetText(m_player_input_name);
//
//			SendPlayerName(m_player_id, m_player_input_name);
//		}
//		else if (event.type == sf::Event::TextEntered)
//		{
//			if (event.text.unicode == '\b')
//			{
//				if (!m_player_input_name.empty())
//					m_player_input_name.erase(m_player_input_name.size() - 1, 1);
//			}
//			else if (event.text.unicode != '\n' && event.text.unicode != '\r')
//			{
//				m_player_input_name += event.text.unicode;
//				m_player_input_name = m_player_input_name.substr(0, 15);
//			}
//
//			m_current_name_label->SetText(m_player_input_name);
//		}
//	}
//	else
//	{
//		m_gui_container.HandleEvent(event);
//	}
//
//	if (event.type == sf::Event::GainedFocus)
//	{
//		GetContext().m_multiplayer_manager->SetPassFocus(true);
//	}
//	else if (event.type == sf::Event::LostFocus)
//	{
//		GetContext().m_multiplayer_manager->SetPassFocus(false);
//	}
//
//	return false;
//}
//
////Written by Paul Bichler (D00242563)
////This method is called right before the state is popped
//void LobbyState::OnStackPopped()
//{
//	//disconnect the player if the state was popped (except when it was popped because the game started)
//	if (!m_game_started)
//		GetContext().m_multiplayer_manager->Disconnect();
//}
//
//void LobbyState::HandleTeamSelection(sf::Packet& packet)
//{
//	sf::Int8 identifier;
//	sf::Int8 team_identifier;
//	packet >> identifier >> team_identifier;
//
//	//move the player to the selected team (team_id 0 means leave current team)
//	if (team_identifier == 0)
//		MovePlayerBack(identifier);
//	else
//		MovePlayer(identifier, team_identifier);
//}
//
//
//void LobbyState::HandleGameStart()
//{
//	if (m_team_selections[m_player_team_selection[m_player_id]].size() == 2)
//	{
//		m_game_started = true;
//		RequestStackClear();
//		RequestStackPush(StateID::kNetworkGame);
//		return;
//	}
//
//	RequestStackPop();
//	RequestStackPush(StateID::kMenu);
//}
//
//void LobbyState::HandleGameStartCountdown()
//{
//	m_start_countdown = true;
//}
//
//void LobbyState::HandlePacket(sf::Int8 packet_type, sf::Packet& packet)
//{
//	switch (static_cast<server::PacketType>(packet_type))
//	{
//	case Server::PacketType::kSpawnSelf:
//		HandleSpawnSelf(packet);
//		break;
//	case Server::PacketType::kPlayerConnect:
//		HandlePlayerConnect(packet);
//		break;
//	case Server::PacketType::kPlayerDisconnect:
//		HandlePlayerDisconnect(packet);
//		break;
//	case Server::PacketType::kInitialState:
//		HandleInitialState(packet);
//		break;
//	case Server::PacketType::kUpdatePlayer:
//		HandleUpdatePlayer(packet);
//		break;
//	case Server::PacketType::kTeamSelection:
//		HandleTeamSelection(packet);
//		break;
//	case Server::PacketType::kStartGame:
//		HandleGameStart();
//		break;
//	case Server::PacketType::kStartGameCountdown:
//		m_start_countdown = true;
//		break;
//	default:
//		break;
//	}
//}
//
//void LobbyState::HandlePlayerConnect(sf::Packet& packet)
//{
//	sf::Int8 identifier;
//	packet >> identifier;
//	AddPlayer(identifier, "Default");
//}
//
//void LobbyState::HandlePlayerDisconnect(sf::Packet& packet)
//{
//	sf::Int8 id;
//	packet >> id;
//
//	auto& team_selection = m_team_selections[m_player_team_selection[id]];
//	const auto remove = std::remove(team_selection.begin(), team_selection.end(), id);
//	team_selection.erase(remove, team_selection.end());
//
//	m_player_team_selection.erase(id);
//	m_gui_container.Pull(m_players[id]);
//	m_players[id].reset();
//	m_players.erase(id);
//}
//
//void LobbyState::HandleUpdatePlayer(sf::Packet& packet)
//{
//	sf::Int8 identifier;
//	std::string name;
//
//	packet >> identifier >> name;
//	m_players[identifier]->SetText(name);
//}
//
///**
//* Dylan Goncalves Martins (D00242562)
//* Handles the initial state packet
//*/
//void LobbyState::HandleInitialState(sf::Packet& packet)
//{
//	sf::Int8 player_count;
//	packet >> player_count;
//	for (sf::Int8 i = 0; i < player_count; ++i)
//	{
//		sf::Int8 identifier;
//		sf::Int8 team_identifier;
//		std::string name;
//
//		packet >> identifier >> team_identifier >> name;
//
//		AddPlayer(identifier, name);
//
//		if (team_identifier != 0)
//		{
//			if (identifier != m_player_id)
//			{
//				MovePlayer(identifier, team_identifier);
//			}
//		}
//	}
//}
//
////Written by Paul Bichler (D00242563)
////Client has updated their name (inform all clients)
//void LobbyState::SendPlayerName(const sf::Int8 id, const std::string& name) const
//{
//	std::string display_name = name;
//	display_name.append(m_is_host ? " (Host)" : "");
//
//	sf::Packet packet;
//	packet << static_cast<sf::Int8>(Client::PacketType::kPlayerUpdate);
//	packet << id;
//	packet << display_name;
//
//	m_socket->send(packet);
//}
//
////Written by Paul Bichler (D00242563)
////Game was started by Host (Tell all the clients to start the game countdown)
//void LobbyState::SendStartGameCountdown() const
//{
//	sf::Packet packet;
//	packet << static_cast<sf::Int8>(Client::PacketType::kStartNetworkGameCountdown);
//	m_socket->send(packet);
//}
//
///**
//* Dylan Goncalves Martins (D00242562)
//* Sends Start game packet
//*/
//void LobbyState::SendStartGame() const
//{
//	sf::Packet packet;
//	packet << static_cast<sf::Int8>(Client::PacketType::kStartNetworkGame);
//	m_socket->send(packet);
//}
//
///**
//* Dylan Goncalves Martins (D00242562)
//* Adds a label to the lobby for a player
//*/
//void LobbyState::AddPlayer(const sf::Int8 id, const std::string& label_text)
//{
//	gui::Label::Ptr name;
//	Utility::CreateLabel(GetContext(), name, UNPAIRED_POS_X, m_unpaired_y_pos + 30 * id,
//		label_text, 20);
//	m_gui_container.Pack(name);
//	m_players.try_emplace(id, name);
//	m_player_team_selection.try_emplace(id, 0);
//}
//
///**
//* Dylan Goncalves Martins (D00242562)
//* Handles self spawn packet
//*/
//void LobbyState::HandleSpawnSelf(sf::Packet& packet)
//{
//	sf::Int8 identifier;
//	packet >> identifier;
//
//	m_player_id = identifier;
//
//	Utility::Debug("Player connected.");
//	AddPlayer(identifier, GetContext().m_player_data_manager->GetData().m_player_name);
//	SendPlayerName(identifier, GetContext().m_player_data_manager->GetData().m_player_name);
//}
