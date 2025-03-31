

#include "LobbyState.hpp"

#include <fstream>
#include <SFML/Graphics/RenderWindow.hpp>
#include "NetworkProtocol.hpp"
#include "Utility.hpp"
#include <SFML/Network/Packet.hpp>
#include <SFML/Network/TcpListener.hpp>
#include <SFML/Network/TcpSocket.hpp>


/*Code from
* Dylan Goncalves Martins (D00242562) and Paul Bichler (D00242563)
* Modified by Dawood Parhiar D00248313
*/

constexpr int TITLE_POS_Y = 30;
constexpr int UNPAIRED_POS_X = 100;
constexpr int TEAM_POS_Y = 260;
constexpr int TEAM_COL_1_POS_X = 550;
constexpr int TEAM_COL_2_POS_X = 950;
constexpr int TEAM_BUTTON_GAP = 135;
constexpr int FOOTER_POS_Y = 850;

static sf::IpAddress GetAddressFromFile()
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


LobbyState::LobbyState(StateStack& stack, Context& context, const bool is_host)
	: State(stack, context)
	, m_player_input_name("Default")
	, m_connected(false)
	, m_is_host(is_host)
	, m_unpaired_y_pos(TEAM_POS_Y - 20)
	, m_player_id(-1)
	, m_time_since_last_packet(sf::seconds(0.f))
	, m_client_timeout(sf::seconds(2.f))
	, m_lobby_time(sf::seconds(0))
	, m_send_time(sf::seconds(0.5f))
	, m_start_countdown_timer(sf::seconds(5.f))
{
	CreateUI(context);

	if (m_is_host)
	{
		context.multiplayer_manager->HostServer();
		ip = "127.0.0.1";
		// Wait a short while for the server to start listening.
		sf::sleep(sf::milliseconds(200));
	}
	else
	{
		ip = GetAddressFromFile();
	}

	m_socket = context.multiplayer_manager->ConnectToServer(ip);
	if (!m_socket)
	{
		Utility::Debug("Failed to connect to server.");
		// Handle error (retry, show error message, etc.)
	}

	// For host, immediately assign a valid player ID and update the UI.
	if (m_is_host)
	{
		m_player_id = 1;
		AddPlayer(m_player_id, m_player_input_name);
		SendPlayerName(m_player_id, m_player_input_name);
	}

	m_is_connecting = true;
	m_failed_connection_clock.restart();

	m_teams.resize(8); // Teams 1 to 8 will be stored at indices 0 to 7.
	m_player_team.clear();
}


void LobbyState::SendClientDisconnect(const sf::Int8 id) const
{
	sf::Packet packet;
	packet << static_cast<sf::Int8>(Client::PacketType::kQuit);
	packet << id;

	m_socket->send(packet);
}


auto LobbyState::HandleTeamButtonPressed(sf::Int8 id)
{
	return [this, id]
		{
			HandleTeamChoice(id);
		};
}

auto LobbyState::HandleStartGamePressed() const
{
	return [this]
		{
			SendStartGameCountdown();
		};
}

auto LobbyState::IsHostAndInTeam()
{
	return [this]
		{
			// Must be host.
			if (!m_is_host)
				return false;

			if (m_players.size() < 2)
				return false;

			// Check if this player has been assigned a team.
			auto it = m_player_team.find(m_player_id);
			if (it == m_player_team.end())
				return false;

			// Allow starting if no countdown is in progress and game hasn't begun.
			return !m_start_countdown && !m_game_started;
		};
}


auto LobbyState::HandleLeaveTeamButtonPress()
{
	return [this]
		{
			HandleTeamChoice(0);
		};
}

auto LobbyState::IsInATeam()
{
	return [this] { return m_player_team_selection[m_player_id] != 0; };
}

auto LobbyState::HandleBackButtonPressed() 
{
	return [this]
		{
			SendClientDisconnect(m_player_id);
			RequestStackPop();
			RequestStackPush(StateID::kMenu);
		};
}


void LobbyState::CreateUI(Context& context)
{
	int y = context.window->getSize().y / 2;
	int x = context.window->getSize().x / 2;

	Utility::CreateLabel(context, m_failed_connection_text, x, y, "Attempting to connect...", 35);
	Utility::CentreOrigin(m_failed_connection_text->GetText());
	m_gui_fail_container.Pack(m_failed_connection_text);

	std::shared_ptr<gui::Label> title_label;
	Utility::CreateLabel(context, title_label, UNPAIRED_POS_X, TITLE_POS_Y, "Lobby", 100);
	m_gui_container.Pack(title_label);

	Utility::CreateButton(context, m_change_name_button, TEAM_COL_1_POS_X, TITLE_POS_Y + 10, "Name",
		true);
	m_gui_container.Pack(m_change_name_button);

	Utility::CreateLabel(context, m_current_name_label, TEAM_COL_1_POS_X + 215, TITLE_POS_Y + 25,
		m_player_input_name, 20);
	m_gui_container.Pack(m_current_name_label);


	std::shared_ptr<gui::Label> unpaired_label;
	Utility::CreateLabel(context, unpaired_label, UNPAIRED_POS_X, TEAM_POS_Y - 50,
		"Unpaired Players", 30);
	m_gui_container.Pack(unpaired_label);

	for (sf::Int8 id = 1; id <= 8; ++id)
	{
		std::shared_ptr<gui::Button> team_button;
		y = TEAM_POS_Y + TEAM_BUTTON_GAP * ((id - 1 - (id - 1) % 2) / 2);
		x = id % 2 == 0 ? TEAM_COL_2_POS_X : TEAM_COL_1_POS_X;
		auto label = "Team " + std::to_string(id);
		Utility::CreateButton(context, team_button, x, y, label,HandleTeamButtonPressed(id),nullptr);
		m_gui_container.Pack(team_button);
	}

	std::shared_ptr<gui::Button> start_game_button;
	Utility::CreateButton(context, start_game_button, UNPAIRED_POS_X, FOOTER_POS_Y, "Start game",
		HandleStartGamePressed(), IsHostAndInTeam());
	m_gui_container.Pack(start_game_button);


	std::shared_ptr<gui::Button> leave_team_button;
	Utility::CreateButton(context, leave_team_button, TEAM_COL_1_POS_X, FOOTER_POS_Y, "Leave Team",
		HandleLeaveTeamButtonPress(), IsInATeam());
	m_gui_container.Pack(leave_team_button);

	std::shared_ptr<gui::Button> back_button;
	Utility::CreateButton(context, back_button, TEAM_COL_2_POS_X + 150, FOOTER_POS_Y, "Leave",
		HandleBackButtonPressed(),nullptr);
	m_gui_container.Pack(back_button);

	std::shared_ptr<gui::Label> start_countdown_text_label;
	Utility::CreateLabel(context, start_countdown_text_label, UNPAIRED_POS_X, FOOTER_POS_Y + 50, "Game starts in...",
		30);
	start_countdown_text_label->SetDrawPredicate([this] { return m_start_countdown; });
	m_gui_container.Pack(start_countdown_text_label);

	Utility::CreateLabel(context, m_start_countdown_label, UNPAIRED_POS_X + 250, FOOTER_POS_Y + 50,
		std::to_string(m_start_countdown_timer.asSeconds()), 30);
	m_start_countdown_label->SetDrawPredicate([this] { return m_start_countdown; });
	m_gui_container.Pack(m_start_countdown_label);
}


bool LobbyState::TeamHasPlace(const sf::Int8 team_id)
{
	int index = team_id - 1;
	return m_teams[index].hasSpace();
}

void LobbyState::AssignPlayerToTeam(sf::Int8 player_id, sf::Int8 team_id) {

	// Ensure team_id is valid (teams are numbered 1..m_teams.size())
	if (team_id < 1 || team_id > static_cast<sf::Int8>(m_teams.size())) {
		Utility::Debug("AssignPlayerToTeam: Invalid team_id: " + std::to_string(team_id));
		return;
	}

	int index = team_id - 1;
	// If pilot slot is free, assign there; otherwise use gunner slot.
	if (m_teams[index].pilot == -1) {
		m_teams[index].pilot = player_id;
	}
	else {
		m_teams[index].gunner = player_id;
	}
	m_player_team[player_id] = team_id;

	// Update UI position based on team button location.
	sf::Vector2f pos = GetTeamPos(team_id);
	float y = pos.y;
	// If the assigned player is not in the pilot slot, offset further down.
	if (m_teams[index].pilot != player_id)
		y += 85;
	else
		y += 60;

	// Check if the player's UI element exists.
	auto it = m_players.find(player_id);
	if (it != m_players.end() && it->second) {
		it->second->setPosition(pos.x, y);
	}
	else {
		Utility::Debug("AssignPlayerToTeam: UI element for player " + std::to_string(player_id) + " not found.");
		 //Optionally: call
		AddPlayer(player_id, "Default");// here to create the UI element.
	}
}

// Remove a player from whichever team they are in.
void LobbyState::RemovePlayerFromTeam(sf::Int8 player_id)
{
	auto it = m_player_team.find(player_id);
	if (it != m_player_team.end()) {
		int team_id = it->second;
		int index = team_id - 1;
		if (m_teams[index].pilot == player_id)
			m_teams[index].pilot = -1;
		if (m_teams[index].gunner == player_id)
			m_teams[index].gunner = -1;
		m_player_team.erase(it);

		// Reset the player's UI position to the unpaired area.
		m_players[player_id]->setPosition(GetUnpairedPos(player_id));
	}
}



sf::Vector2f LobbyState::GetTeamPos(const int i)
{
	const int y = TEAM_POS_Y + TEAM_BUTTON_GAP * ((i - 1 - (i - 1) % 2) / 2);
	const int x = i % 2 == 0 ? TEAM_COL_2_POS_X : TEAM_COL_1_POS_X;

	return { static_cast<float>(x), static_cast<float>(y) };
}

sf::Vector2f LobbyState::GetUnpairedPos(const int i) const
{
	const int y = m_unpaired_y_pos + 30 * i;
	const int x = UNPAIRED_POS_X;

	return { static_cast<float>(x), static_cast<float>(y) };
}

void LobbyState::MovePlayer(const sf::Int8 id, const sf::Int8 team_id)
{
	RemovePlayerFromTeam(id);
	AssignPlayerToTeam(id, team_id);
}

void LobbyState::MovePlayerBack(const sf::Int8 id)
{
	RemovePlayerFromTeam(id);
}

void LobbyState::HandleTeamChoice(const sf::Int8 team_id)
{
	// team_id == 0 means the player wants to leave their team.
	if (team_id == 0) 
	{
		RemovePlayerFromTeam(m_player_id);
		// Notify server that this player left the team.
		sf::Packet packet;
		packet << static_cast<sf::Int8>(Client::PacketType::kTeamChange)
			<< m_player_id << team_id;
		// (You might add an extra flag if needed; here we send 0)
		packet << static_cast<sf::Int8>(0);
		m_socket->send(packet);
	}
	else 
	{
		// Only assign if there is space.
		if (TeamHasPlace(team_id)) 
		{
			// Remove from current team (if any) then assign to the new team.
			RemovePlayerFromTeam(m_player_id);
			AssignPlayerToTeam(m_player_id, team_id);

			// Notify the server of the change.
			sf::Packet packet;
			packet << static_cast<sf::Int8>(Client::PacketType::kTeamChange)
				<< m_player_id << team_id;
			// Send flag: 0 if this is the first in the team, 1 if second.
			int flag = (m_teams[team_id - 1].pilot == m_player_id) ? 0 : 1;
			packet << static_cast<sf::Int8>(flag);
			m_socket->send(packet);
		}
	}
}


void LobbyState::Draw()
{
	sf::RenderWindow& window = *GetContext().window;
	window.clear(sf::Color(45, 37, 97));

	if (m_connected)
	{
		window.draw(m_gui_container);
	}
	else
	{
		window.draw(m_gui_fail_container);
	}
}


void LobbyState::NotifyServerOfExistence() const
{
	sf::Packet packet;
	m_socket->send(packet);
}

bool LobbyState::Update(const sf::Time dt)
{
	
	if (m_is_connecting)
	{
		
		sf::Packet packet;
		if (m_socket->send(packet) == sf::Socket::Done)
		{
			m_is_connecting = false;
			m_connected = true;
			return true;
		}
		
		if (m_failed_connection_clock.getElapsedTime() >= sf::seconds(5.f))
		{
			m_is_connecting = false;
			m_failed_connection_text->SetText("No servers available");
			Utility::CentreOrigin(m_failed_connection_text->GetText());
			m_failed_connection_clock.restart();
		}

		return true;
	}

	
	if (m_connected)
	{
		
		if (m_lobby_time > m_send_time)
		{
			m_lobby_time = sf::seconds(0.f);
			NotifyServerOfExistence();
		}

		sf::Packet packet;
		if (m_socket->receive(packet) == sf::Socket::Done)
		{
			m_time_since_last_packet = sf::seconds(0.f);
			sf::Int8 packet_type;
			packet >> packet_type;
			HandlePacket(packet_type, packet);
		}
		else
		{
			
			if (m_time_since_last_packet > m_client_timeout)
			{
				m_connected = false;
				m_failed_connection_text->SetText("Lost connection to the server");
				Utility::CentreOrigin(m_failed_connection_text->GetText());

				m_failed_connection_clock.restart();
			}
		}
		m_time_since_last_packet += dt;
	}
	//Failed to connect and waited for more than 5 seconds: Back to menu
	else if (m_failed_connection_clock.getElapsedTime() >= sf::seconds(5.f))
	{
		RequestStackClear();
		RequestStackPush(StateID::kMenu);
	}

	m_lobby_time += dt;

	//Update the start game countdown (if the game has been started by the host)
	if (m_start_countdown)
	{
		if (m_start_countdown_timer.asSeconds() > 0)
		{
			m_start_countdown_timer -= dt;
			m_start_countdown_label->SetText(std::to_string(static_cast<int>(m_start_countdown_timer.asSeconds())));
		}
		else if (m_is_host)
		{
			SendStartGame();
			m_start_countdown = false;
		}
	}

	return true;
}

bool LobbyState::HandleEvent(const sf::Event& event)
{
	if (m_game_started)
		return false;

	if (m_change_name_button->IsActive())
	{
		// Name Input
		if (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::Return)
		{
			m_change_name_button->Deactivate();
			// Only update the UI if m_player_id is valid.
			if (m_player_id != -1 && m_players.find(m_player_id) != m_players.end())
			{
				m_players[m_player_id]->SetText(m_player_input_name);
				SendPlayerName(m_player_id, m_player_input_name);
			}
			else
			{
				Utility::Debug("Cannot update name: m_player_id is not set yet.");
			}
		}
		else if (event.type == sf::Event::TextEntered)
		{
			if (event.text.unicode == '\b')
			{
				if (!m_player_input_name.empty())
					m_player_input_name.erase(m_player_input_name.size() - 1, 1);
			}
			else if (event.text.unicode != '\n' && event.text.unicode != '\r')
			{
				m_player_input_name += event.text.unicode;
				m_player_input_name = m_player_input_name.substr(0, 15);
			}

			m_current_name_label->SetText(m_player_input_name);
		}
	}
	else
	{
		m_gui_container.HandleEvent(event);
	}

	if (event.type == sf::Event::GainedFocus)
	{
		GetContext().multiplayer_manager->SetPassFocus(true);
	}
	else if (event.type == sf::Event::LostFocus)
	{
		GetContext().multiplayer_manager->SetPassFocus(false);
	}

	return false;
}



void LobbyState::OnStackPopped()
{
	//disconnect the player if the state was popped (except when it was popped because the game started)
	if (!m_game_started)
		GetContext().multiplayer_manager->Disconnect();
}

void LobbyState::HandleTeamSelection(sf::Packet& packet)
{
	sf::Int8 identifier;
	sf::Int8 team_identifier;
	packet >> identifier >> team_identifier;

	//move the player to the selected team (team_id 0 means leave current team)
	if (team_identifier == 0)
		MovePlayerBack(identifier);
	else
		MovePlayer(identifier, team_identifier);
}

void LobbyState::HandleGameStart()
{
	// Ensure at least 2 players are connected.
	if (m_players.size() < 2)
	{
		Utility::Debug("Not enough players to start the game.");
		RequestStackPop();
		RequestStackPush(StateID::kMenu);
		return;
	}

	// Verify that the host is assigned to a team.
	auto it = m_player_team.find(m_player_id);
	if (it == m_player_team.end())
	{
		Utility::Debug("Local player is not assigned to a team.");
		RequestStackPop();
		RequestStackPush(StateID::kMenu);
		return;
	}

	// Conditions met: start the game.
	m_game_started = true;
	RequestStackClear();
	RequestStackPush(StateID::kNetworkGame);
}


void LobbyState::HandleGameStartCountdown()
{
	m_start_countdown = true;
}

void LobbyState::HandlePacket(sf::Int8 packet_type, sf::Packet& packet)
{
	switch (static_cast<Server::PacketType>(packet_type))
	{
	case Server::PacketType::kSpawnSelf:
		HandleSpawnSelf(packet);
		break;
	case Server::PacketType::kPlayerConnect:
		HandlePlayerConnect(packet);
		break;
	case Server::PacketType::kPlayerDisconnect:
		HandlePlayerDisconnect(packet);
		break;
	case Server::PacketType::kInitialState:
		HandleInitialState(packet);
		break;
	case Server::PacketType::kPlayerUpdate:
		HandleUpdatePlayer(packet);
		break;
	case Server::PacketType::kTeamSelection:
		HandleTeamSelection(packet);
		break;
	case Server::PacketType::kGameStart:
		HandleGameStart();
		break;
	case Server::PacketType::kStartGameCountdown:
		Utility::Debug("Received start game countdown.");
		m_start_countdown = true;
		break;
	case Server::PacketType::kLobbyStateUpdate:
		HandleLobbyStateUpdate(packet);
		break;
	default:
		break;
	}
}

void LobbyState::HandlePlayerConnect(sf::Packet& packet)
{
	sf::Int8 identifier;
	packet >> identifier;
	if (m_player_id == -1)
	{
		m_player_id = identifier;
		Utility::Debug("Guest assigned id: " + std::to_string(identifier));
		AddPlayer(identifier, "Default");
		// Optionally update name:
		SendPlayerName(identifier, m_player_input_name);
	}
	else
	{
		// For new players joining the lobby, just add them.
		AddPlayer(identifier, "Default");
	}
}


void LobbyState::HandlePlayerDisconnect(sf::Packet& packet)
{
	sf::Int8 id;
	packet >> id;

	// Remove the player from their assigned team, if any.
	RemovePlayerFromTeam(id);

	// Remove the player's UI element and clear their record.
	m_gui_container.Pull(m_players[id]);
	m_players[id].reset();
	m_players.erase(id);
}


void LobbyState::HandleUpdatePlayer(sf::Packet& packet)
{
	sf::Int8 identifier;
	std::string name;

	packet >> identifier >> name;
	m_players[identifier]->SetText(name);
}

void LobbyState::HandleInitialState(sf::Packet& packet)
{
	sf::Int8 player_count;
	packet >> player_count;
	for (sf::Int8 i = 0; i < player_count; ++i)
	{
		sf::Int8 identifier;
		sf::Int8 team_identifier;
		std::string name;

		packet >> identifier >> team_identifier >> name;

		AddPlayer(identifier, name);

		if (team_identifier != 0)
		{
			if (identifier != m_player_id)
			{
				MovePlayer(identifier, team_identifier);
			}
		}
	}
}

void LobbyState::SendPlayerName(const sf::Int8 id, const std::string& name) const
{

	if (!m_socket)
	{
		Utility::Debug("SendPlayerName: m_socket is null.");
		return;
	}

	std::string display_name = name;
	display_name.append(m_is_host ? " (Host)" : "");

	sf::Packet packet;
	packet << static_cast<sf::Int8>(Client::PacketType::kPlayerUpdate);
	packet << id;
	packet << display_name;

	m_socket->send(packet);
}

void LobbyState::SendStartGameCountdown() const
{
	sf::Packet packet;
	packet << static_cast<sf::Int8>(Client::PacketType::kStartNetworkGameCountdown);
	m_socket->send(packet);
}

void LobbyState::SendStartGame() const
{
	sf::Packet packet;
	packet << static_cast<sf::Int8>(Client::PacketType::kStartNetworkGame);
	m_socket->send(packet);
}


void LobbyState::AddPlayer(const sf::Int8 id, const std::string& label_text)
{
	gui::Label::Ptr name;
	Utility::CreateLabel(GetContext(), name, UNPAIRED_POS_X, m_unpaired_y_pos + 30 * id,
		label_text, 20);
	m_gui_container.Pack(name);
	m_players.try_emplace(id, name);
	m_player_team_selection.try_emplace(id, 0);
}


void LobbyState::HandleSpawnSelf(sf::Packet& packet)
{
	sf::Int8 id;
	packet >> id; // Read the player's id.
	m_player_id = id; // Now m_player_id is set.
	

	sf::Int8 ship_id, role;
	sf::Vector2f ship_position;
	packet >> ship_id >> role >> ship_position.x >> ship_position.y;

	Utility::Debug("Player spawned with id: " + std::to_string(id) +
		", ship: " + std::to_string(ship_id) +
		", role: " + std::to_string(role));

	AddPlayer(id, m_player_input_name);
	SendPlayerName(id, m_player_input_name);
}


void LobbyState::HandleLobbyStateUpdate(sf::Packet& packet)
{
	// The packet layout is:
	// [kLobbyStateUpdate][teamCount][for each team: teamId, pilotId, gunnerId]
	sf::Int32 teamCount;
	packet >> teamCount;

	// Loop over each team (or ship) entry in the lobby.
	for (int i = 0; i < teamCount; ++i)
	{
		sf::Int8 teamId, pilotId, gunnerId;
		packet >> teamId >> pilotId >> gunnerId;

		// Assuming your m_teams vector is 0-indexed and team IDs are 1-indexed:
		int index = teamId - 1;
		if (index >= 0 && index < static_cast<int>(m_teams.size()))
		{
			m_teams[index].pilot = pilotId;
			m_teams[index].gunner = gunnerId;
		}

		// Update the mapping of players to teams:
		if (pilotId != -1)
		{
			m_player_team[pilotId] = teamId;
		}
		if (gunnerId != -1)
		{
			m_player_team[gunnerId] = teamId;
		}
	}

	// (Optional) Update your lobby UI here so that team labels, player lists, etc.
	// reflect the current state. For example:
	// UpdateTeamUI();
}
