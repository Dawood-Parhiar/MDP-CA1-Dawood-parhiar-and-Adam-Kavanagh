#pragma once
#include <deque>
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
#include "SFML/Network/Packet.hpp"


class GameServer
{
private:
	struct RemotePeer
	{
		RemotePeer();
		sf::TcpSocket m_socket;
		sf::Time m_last_packet_time;
		std::vector<sf::Int32> m_ship_identifiers;

		bool m_ready;
		bool m_timed_out;
		std::deque<sf::Packet> m_send_queue;

		void QueuePacket(const sf::Packet& packet);
		void FlushSendQueue();
	};

	struct ShipInfo
	{
		sf::Vector2f m_position;
		sf::Int32 m_hitpoints;
		sf::Int32 m_missile_ammo;
		std::map<sf::Int32, bool> m_realtime_actions;
		sf::Int32    m_lastProcessedInput = -1;
		sf::Int32 m_cannon_angle = 0.f;
	};

	typedef std::unique_ptr<RemotePeer> PeerPtr;
	


public:
	explicit GameServer(sf::Vector2f battlefield_size);
	~GameServer();
	void NotifyPlayerSpawn(sf::Int32 ship_identifier);
	void NotifyPlayerRealtimeChange(sf::Int32 ship_identifier, sf::Int32 action, bool action_enabled);
	void NotifyPlayerEvent(sf::Int32 ship_identifier, sf::Int32 action);
	

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
	void HandlePlayerUpdate(sf::Packet& packet);
	void StartGameCountdownStart();
	void NotifyGameStart();
	void HandlePlayerNameChange(sf::Packet& packet);
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
	std::map<sf::Int32, ShipInfo> m_ship_info;

	std::vector<PeerPtr> m_peers;
	sf::Int32 m_ship_identifier_counter;
	bool m_waiting_thread_end;

	sf::Time m_last_spawn_time;
	sf::Time m_time_for_next_spawn;

	sf::Int32 m_packet_sequence = 0;
	std::unordered_map<sf::Int32, std::string> m_ship_names;
};

