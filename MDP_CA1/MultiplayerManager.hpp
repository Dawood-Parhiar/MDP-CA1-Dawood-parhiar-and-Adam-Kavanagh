#pragma once
#include <functional>
#include <memory>
#include <vector>
#include <SFML/Network/TcpSocket.hpp>

#include "GameServer.hpp"

class MultiplayerManager
{
public:
	MultiplayerManager() = default;
	void HostServer();
	sf::TcpSocket* ConnectToServer(sf::IpAddress ip_address, float timeout = 5.f, bool blocking_mode = false);
	sf::TcpSocket* GetSocket() const;
	void Disconnect();

	std::vector<std::pair<sf::Int8, sf::Time>>& GetLeaderboard();
	void AddToLeaderboard(sf::Int8 team_id, sf::Time completion_time);
	void SetLeaderboardChangeCallback(std::function<void()> callback);
	void AddPlayer(sf::Int8 team_id, sf::Int8 id, std::string name);
	std::vector<std::string> GetPlayerNamesOfTeam(sf::Int8 team_id);
	int GetNumberOfTeams() const;
	void SetPassFocus(bool pass_focus);
	bool GetPassFocus() const;

private:
	std::unique_ptr<GameServer> m_game_server;
	std::unique_ptr<sf::TcpSocket> m_socket;

	std::map<sf::Int8, std::vector<sf::Int8>> m_teams;
	std::map<sf::Int8, std::string> m_players;
	std::vector<std::pair<sf::Int8, sf::Time>> m_leaderboard;
	std::function<void()> m_leaderboard_change_callback;
	bool m_pass_focus;
};

