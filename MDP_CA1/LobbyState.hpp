//#pragma once
//#include <SFML/Network/TcpListener.hpp>
//#include <SFML/Network/TcpSocket.hpp>
//
//#include "Container.hpp"
//#include "State.hpp"
//#include "Label.hpp"
//#include "ResourceHolder.hpp"
//#include "Utility.hpp"
//#include "Button.hpp"
//class LobbyState : public State
//{
//public:
//    explicit LobbyState(StateStack& stack, Context context);
//    ~LobbyState();
//
//    void Draw() override;
//    bool Update(sf::Time dt) override;
//    void UpdatePlayerList();
//    void HandlePlayerJoin(int player_id);
//    void TryAssignShips();
//    bool HandleEvent(const sf::Event& event) override;
//
//    void AddPlayer(const std::string& playerName);
//    void AssignShips();
//
//
//private:
//    struct RemotePeer
//    {
//        RemotePeer();
//        sf::TcpSocket m_socket;
//        sf::Time m_last_packet_time;
//        std::vector<sf::Int32> m_ship_identifiers;
//        bool m_ready;
//        bool m_timed_out;
//    };
//
//    struct ShipInfo
//    {
//        sf::Vector2f m_position;
//        sf::Int32 m_hitpoints;
//        sf::Int32 m_missile_ammo;
//        std::map<sf::Int32, bool> m_realtime_pilot_actions;
//        std::map<sf::Int32, bool> m_realtime_gunner_actions;
//    };
//
//    typedef std::unique_ptr<RemotePeer> PeerPtr;
//
//    void SetListening(bool enable);
//    void ExecutionThread();
//    void Tick();
//    sf::Time Now() const;
//
//    void HandleIncomingPackets();
//    void HandleIncomingPackets(sf::Packet& packet, RemotePeer& receiving_peer, bool& detected_timeout);
//
//    void HandleIncomingConnections();
//    void HandleDisconnections();
//
//    void InformWorldState(sf::TcpSocket& socket);
//    void BroadcastMessage(const std::string& message);
//    void SendToAll(sf::Packet& packet);
//    void UpdateClientState();
//
//private:
//    sf::Sprite m_background_sprite;
//    gui::Container m_gui_container;
//    std::vector<int> m_players;
//    std::vector<std::shared_ptr<gui::Label>> m_player_labels;
//    std::vector<std::pair<std::string, std::string>> m_assigned_ships;
//    std::shared_ptr<gui::Button> m_start_button;
//
//    std::size_t m_max_connected_players;
//    std::size_t m_connected_players;
//
//    sf::TcpListener m_listener_socket;
//    bool m_listening_state;
//    sf::Time m_client_timeout;
//    sf::Thread m_thread;
//    sf::Clock m_clock;
//
//    std::size_t m_ships_assigned;
//    std::map<sf::Int32, ShipInfo> m_ship_info;
//
//    std::vector<PeerPtr> m_peers;
//    sf::Int32 m_ship_id_counter;
//    bool m_waiting_thread_end;
//};
//

/*
#pragma once
#include <SFML/Network/IpAddress.hpp>
#include <SFML/Network/Packet.hpp>

#include "Button.hpp"
#include "Container.hpp"
#include "Label.hpp"
#include "State.hpp"

class LobbyState : public State
{
public:
	LobbyState(StateStack& stack, Context& context, bool is_host);
	void CreateUI(Context& context);
	bool TeamHasPlace(sf::Int8 id);
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
	auto HandleBackButtonPressed() const;
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
	std::map<sf::Int8, std::vector<sf::Int8>> m_team_selections;
	std::map<sf::Int8, sf::Int8> m_player_team_selection;
	sf::Int8 m_player_id{};

	sf::Time m_time_since_last_packet;
	sf::Time m_client_timeout;
	sf::Time m_lobby_time;
	sf::Time m_send_time;
	sf::Time m_start_countdown_timer;
	bool m_is_connecting;
	sf::IpAddress ip;
};
*/