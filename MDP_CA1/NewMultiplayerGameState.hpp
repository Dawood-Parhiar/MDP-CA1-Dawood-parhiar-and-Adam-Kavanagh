#pragma once

#include "State.hpp"
#include "World.hpp"
#include "CommandQueue.hpp"
#include "Player.hpp"
#include "MultiplayerManager.hpp"
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Clock.hpp>
#include <map>
#include <memory>
#include <vector>

class NewMultiplayerGameState : public State
{
public:
    NewMultiplayerGameState(StateStack& stack, Context context);
    virtual ~NewMultiplayerGameState();

    virtual void Draw();
    virtual bool Update(sf::Time dt);
    virtual bool HandleEvent(const sf::Event& event);
    virtual void OnActivate();
    virtual void OnDestroy();
    void DisableAllRealtimeActions();

private:
    // Packet handling functions.
    void HandlePacket(sf::Int32 packet_type, sf::Packet& packet);
    void HandleSpawnSelf(sf::Packet& packet);
    void HandlePlayerConnect(sf::Packet& packet);
    void HandlePlayerDisconnect(sf::Packet& packet);
    void HandleBroadcastMessage(sf::Packet& packet);
    void HandleInitialState(sf::Packet& packet);
    void HandlePlayerEvent(sf::Packet& packet);
    void HandleRealTimeChange(sf::Packet& packet);
    void HandleSpawnEnemy(sf::Packet& packet);
    void HandleUpdateClient(sf::Packet& packet);

    void UpdateBroadcastMessage(sf::Time elapsed_time);

    // Member variables.
    sf::RenderWindow& m_window;
    World m_world;
    MultiplayerManager* m_multiplayerManager;

    bool m_connected;
    bool m_active_state;
    bool m_has_focus;
    bool m_game_started;
    sf::Time m_client_timeout;
    sf::Time m_time_since_last_packet;
    sf::Clock m_tick_clock;
    sf::Clock m_failed_connection_clock;
    sf::Text m_broadcast_text;
    sf::Text m_failed_connection_text;
    std::vector<std::string> m_broadcasts;
    sf::Time m_broadcast_elapsed_time;

    // Mapping of players and ship IDs.
    std::map<sf::Int8, std::unique_ptr<Player>> m_players;
    std::map<sf::Int8, sf::Int8> m_playerShip; // maps player_id to ship_id
    sf::Int8 m_local_player_id;

    // For invitation blinking (if used).
    sf::Time m_player_invitation_time;
};
