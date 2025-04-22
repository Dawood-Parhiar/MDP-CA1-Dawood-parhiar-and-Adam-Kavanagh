#pragma once
#include <SFML/System/Vector2.hpp>
#include <SFML/Config.hpp>
#include <SFML/Network/TcpSocket.hpp>
#include <SFML/Network/TcpListener.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Thread.hpp>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <SFML/Graphics/Rect.hpp>

enum class Role { Captain, Gunner };

class GameServer
{
public:
	explicit GameServer(sf::Vector2f battlefield_size);
	~GameServer();
	void NotifyPlayerSpawn(sf::Int32 ship_identifier);
	void NotifyPlayerRealtimeChange(sf::Int32 ship_identifier, sf::Int32 action, bool action_enabled);
	void NotifyPlayerEvent(sf::Int32 ship_identifier, sf::Int32 action);
	

private:
	struct RemotePeer
	{
		RemotePeer();
		sf::TcpSocket m_socket;
		sf::Time m_last_packet_time;
		sf::Int8 m_identifier{};
		bool m_ready;
		bool m_timed_out;
	};

	struct ShipInfo
	{
		sf::Vector2f m_position;
		sf::Int8 m_hitpoints;
		sf::Int8 m_missile_ammo;
		std::map<sf::Int32, bool> m_realtime_actions;

		sf::Int8 m_ship_id;
		sf::Int8 m_Captain_id = -1;  // Default to no pilot
		sf::Int8 m_gunner_id = -1; // Default to no gunner

		// Check if the ship has an available seat
		bool HasCaptain() const { return m_Captain_id != -1; }
		bool HasGunner() const { return m_gunner_id != -1; }
		bool IsFull() const { return HasCaptain() && HasGunner(); }
	};

	typedef std::unique_ptr<RemotePeer> PeerPtr;

private:
	void SetListening(bool enable);
	void UpdateLobbyState();
	void ExecutionThread();
	void Tick();
	sf::Time Now() const;

	void HandleIncomingPackets();
	void PlayerEvent(sf::Packet& packet);
	void RealTimeChange(sf::Packet& packet);
	//void RequestCoopPartner(RemotePeer& receiving_peer);
	void StateUpdate(sf::Packet& packet);
	void GameEvent(sf::Packet& packet, RemotePeer& receiving_peer);
	void NotifyTeamChange(sf::Int8 id, sf::Int8 ship_id, sf::Int8 gunner_id, sf::Int8 pilot_id);
	void PlayerTeamChange(sf::Packet& packet);
	void HandlePlayerUpdate(sf::Packet& packet);
	void StartGameCountdownStart();
	void NotifyGameStart();
	void HanldePlayerNameChange(sf::Packet& packet);
	void HandleIncomingPackets(sf::Packet& packet, RemotePeer& receiving_peer, bool& detected_timeout);

	void GetAndSetID(sf::Int8& int8);
	void HandleIncomingConnections();
	void HandleDisconnections();

	void InformWorldState(sf::TcpSocket& socket);
	void BroadcastMessage(const std::string& message);
	void SendToAll(sf::Packet& packet);
	void UpdateClientState();

private:
	sf::Thread m_thread;
	sf::Clock m_clock;
	sf::TcpListener m_listener_socket;
	bool m_listening_state;
	sf::Time m_client_timeout;

	std::size_t m_max_connected_players;
	std::size_t m_connected_players;

	float m_world_height;
	sf::FloatRect m_battlefield_rect;
	float m_battlefield_scrollspeed;

	std::size_t m_ship_count;
	std::map<sf::Int8, ShipInfo> m_ship_info;

	std::vector<PeerPtr> m_peers;
	sf::Int32 m_ship_identifier_counter;
	bool m_waiting_thread_end;
	sf::Int8 m_player_id_counter = 1;


	sf::Time m_last_spawn_time;
	sf::Time m_time_for_next_spawn;
	bool m_game_started;

	bool m_inLobby = false;
};

