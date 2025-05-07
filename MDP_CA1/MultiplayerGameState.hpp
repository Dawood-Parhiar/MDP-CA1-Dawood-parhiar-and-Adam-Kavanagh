#pragma once
#include "State.hpp"
#include "World.hpp"
#include "Player.hpp"
#include "GameServer.hpp"
#include "NetworkProtocol.hpp"
#include "SFML/Network/TcpSocket.hpp"
#include "SFML/Network/Packet.hpp"

struct NetworkState
{
	sf::Vector2f lastPos,    // older snapshot
		currPos;    // newer snapshot
	float         lastTime,  // server timestamp of lastPos
		currTime;  // server timestamp of currPos
	int           hitpoints,
		ammo;
};

class MultiplayerGameState : public State
{
public:
	MultiplayerGameState(StateStack& stack, Context context ,bool is_host);
	virtual void Draw();
	virtual bool Update(sf::Time dt);
	virtual bool HandleEvent(const sf::Event& event);
	virtual void OnActivate();
	void OnDestroy();
	void DisableAllRealtimeActions();
	

private:
	void UpdateBroadcastMessage(sf::Time elpased_time);
	void HandleBroadcastMessage(sf::Packet& packet);
	void HandleSpawnSelf(sf::Packet& packet);
	void HandlePlayerConnect(sf::Packet& packet);
	void HandlePlayerDisconnect(sf::Packet& packet);
	void HandleInitialState(sf::Packet& packet);
	void HandlePlayerEvent(sf::Packet& packet);
	void HandleRealTimeChange(sf::Packet& packet);
	void HandleSpawnEnemy(sf::Packet& packet);
	void HandleUpdateClient(sf::Packet& packet);
	void HandleSpawnPickup(sf::Packet& packet);
	void HandlePacket(sf::Int32 packet_type, sf::Packet& packet);
	void QueueOutgoingPacket(const sf::Packet& packet);
	void FlushSendQueue();
	void InterpolateRemoteShips();

private:
	typedef std::unique_ptr<Player> PlayerPtr;

private:
	World m_world;
	sf::RenderWindow& m_window;
	TextureHolder& m_texture_holder;

	std::map<int, PlayerPtr> m_players;
	std::vector<sf::Int32> m_local_player_identifiers;

	sf::TcpSocket m_socket;
	bool m_connected;
	std::unique_ptr<GameServer> m_game_server;
	sf::Clock m_tick_clock;

	std::vector<std::string> m_broadcasts;
	sf::Text m_broadcast_text;
	sf::Time m_broadcast_elapsed_time;

	sf::Text m_player_invitation_text;
	sf::Time m_player_invitation_time;

	sf::Text m_failed_connection_text;
	sf::Clock m_failed_connection_clock;

	bool m_active_state;
	bool m_has_focus;
	bool m_host;
	bool m_game_started;

	sf::Time m_client_timeout;
	sf::Time m_time_since_last_packet;

	bool m_local_player_spawned = false;

	//chatgpt

	sf::Int32 m_last_server_sequence = -1;
	std::deque<sf::Packet> m_send_queue;

	std::unordered_map<sf::Int32, NetworkState> m_networkStates;
	sf::Clock                                   m_clientClock;        // local clock
	const float                                 m_interpolationDelay = 0.1f;  // 100 ms buffer

	struct PendingInput {
		sf::Int32 seq;
		Action    action;
		bool      enabled;
	};

	std::deque<PendingInput> m_pendingInputs;
	sf::Int32                m_nextInputSeq = 0;     // starts at 0
	sf::Int32                m_lastServerAck = -1;
};

