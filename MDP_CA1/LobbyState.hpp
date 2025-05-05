
#pragma once
#include <SFML/Network/IpAddress.hpp>
#include <SFML/Network/Packet.hpp>

#include "Button.hpp"
#include "Container.hpp"
#include "GameServer.hpp"
#include "Label.hpp"
#include "State.hpp"

struct Team {
	int pilot = -1;   // first player (or “pilot”)
	int gunner = -1;  // second player (or “gunner”)

	bool isComplete() const { return pilot != -1 && gunner != -1; }
	bool hasSpace() const { return pilot == -1 || gunner == -1; }
};


class LobbyState : public State
{
public:
	LobbyState(StateStack& stack, Context& context, bool is_host);
	void CreateUI(Context& context);
	bool TeamHasPlace(sf::Int8 id);
	void AssignPlayerToTeam(sf::Int8 player_id, sf::Int8 team_id);
	void RemovePlayerFromTeam(sf::Int8 player_id);
	static sf::Vector2f GetTeamPos(int i);
	sf::Vector2f GetUnpairedPos(int i) const;
	void MovePlayer(sf::Int8 id, sf::Int8 team_id);
	void MovePlayerBack(sf::Int8 id);
	void HandleTeamChoice(sf::Int8 id);
	void Draw() override;
	void NotifyServerOfExistence() const;
	bool Update(sf::Time dt) override;
	bool HandleEvent(const sf::Event& event) override;
	void OnStackPopped() override;
	

private:
	void HandleTeamSelection(sf::Packet& packet);
	void SendClientDisconnect(sf::Int8 id) const;
	//auto HandleTutorialPress() const;
	auto HandleTeamButtonPressed(sf::Int8 id);
	auto HandleStartGamePressed() const;
	auto IsHostAndInTeam();
	auto HandleLeaveTeamButtonPress();
	auto IsInATeam();
	auto HandleBackButtonPressed();
	void HandleGameStart();
	void HandleGameStartCountdown();
	void HandlePacket(sf::Int8 packet_type, sf::Packet& packet);
	void HandlePlayerConnect(sf::Packet& packet);
	void HandlePlayerDisconnect(sf::Packet& packet);
	void HandleUpdatePlayer(sf::Packet& packet);
	void HandleInitialState(sf::Packet& packet);
	void SendPlayerName(sf::Int8 id, const std::string& name) const;
	void SendStartGameCountdown() const;
	void SendStartGame() const;
	void AddPlayer(sf::Int8 id, const std::string& label_text);
	void HandleSpawnSelf(sf::Packet& packet);
	void HandleLobbyStateUpdate(sf::Packet& packet);

private:
	sf::TcpSocket* m_socket;

	gui::Container m_gui_container;
	gui::Container m_gui_fail_container;

	gui::Button::Ptr m_change_name_button;
	gui::Label::Ptr m_current_name_label;
	std::string m_player_input_name;

	gui::Label::Ptr m_failed_connection_text;
	sf::Clock m_failed_connection_clock;

	bool m_connected;
	bool m_is_host;
	bool m_game_started{};
	bool m_start_countdown{};
	int m_unpaired_y_pos;

	std::shared_ptr<gui::Label> m_start_countdown_label;

	std::map<sf::Int8, gui::Label::Ptr> m_players;
	//std::map<sf::Int8, std::vector<sf::Int8>> m_team_selections;
	std::map<sf::Int8, sf::Int8> m_player_team_selection;
	sf::Int8 m_player_id{};

	sf::Time m_time_since_last_packet;
	sf::Time m_client_timeout;
	sf::Time m_lobby_time;
	sf::Time m_send_time;
	sf::Time m_start_countdown_timer;
	bool m_is_connecting;
	sf::IpAddress ip;

	std::vector<Team> m_teams;
	std::unordered_map<sf::Int8, sf::Int8> m_player_team;

};
