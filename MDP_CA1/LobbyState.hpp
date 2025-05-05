#pragma once

#include <SFML/Network/IpAddress.hpp>
#include <SFML/Network/TcpSocket.hpp>
#include <SFML/Network/Packet.hpp>
#include <SFML/System/Time.hpp>
#include <map>
#include <string>

#include "Container.hpp"
#include "Button.hpp"
#include "Label.hpp"
#include "State.hpp"

// Tracks information about each player in the lobby
struct PlayerInfo
{
    std::string       name;
    bool              ready = false;
    gui::Label::Ptr   label;
};

class LobbyState : public State
{
public:
    LobbyState(StateStack& stack, Context& context, bool isHost);

    // Build the lobby UI
    void CreateUI(Context& context);

    // State interface
    void Draw() override;
    bool Update(sf::Time dt) override;
    bool HandleEvent(const sf::Event& event) override;
    void OnStackPopped() override;

private:
    // Incoming packet handlers
    void HandlePacket(sf::Int8 packetType, sf::Packet& packet);
    void HandleSpawnSelf(sf::Packet& packet);
    void HandlePlayerConnect(sf::Packet& packet);
    void HandlePlayerDisconnect(sf::Packet& packet);
    void HandleUpdatePlayer(sf::Packet& packet);
    void HandleInitialState(sf::Packet& packet);
    void HandleLobbyPlayers(sf::Packet& packet);
    void HandleStartGameCountdown();
    void HandleGameStart();

    // Outgoing messages
    void SendClientDisconnect(sf::Int8 id) const;
    void SendPlayerName(sf::Int8 id, const std::string& name) const;
    void SendReadyToggle(bool isReady);
    void SendStartGameCountdown() const;
    void SendStartGame() const;

    // UI management
    void AddPlayer(sf::Int8 id, const std::string& name);
    void RemovePlayer(sf::Int8 id);
    void UpdatePlayerListUI();

private:
    sf::TcpSocket* m_socket = nullptr;
    gui::Container                      m_gui_container;
    gui::Container                      m_gui_fail_container;

    // UI elements
    gui::Button::Ptr                    m_change_name_button;
    gui::Label::Ptr                     m_current_name_label;
    gui::Button::Ptr                    m_ready_button;
    gui::Label::Ptr                     m_start_countdown_label;
    std::string                         m_player_input_name;
    gui::Label::Ptr                     m_failed_connection_text;
    sf::Clock                           m_failed_connection_clock;

    // Lobby state
    bool                                m_connected = false;
    bool                                m_is_host = false;
    bool                                m_game_started = false;
    bool                                m_start_countdown = false;
    sf::Time                            m_start_countdown_timer;

    // Player list
    std::map<sf::Int8, PlayerInfo>     m_players;
    sf::Int8                            m_player_id = -1;

    // Networking & timing
    sf::Time                            m_time_since_last_packet;
    sf::Time                            m_client_timeout;
    sf::Time                            m_lobby_time;
    sf::Time                            m_send_time;
    bool                                m_is_connecting = false;
    sf::IpAddress                       m_server_ip;
};
