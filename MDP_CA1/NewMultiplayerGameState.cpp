#include "NewMultiplayerGameState.hpp"
#include "Utility.hpp"
#include "MusicPlayer.hpp"
#include "StateID.hpp"
#include "NetworkProtocol.hpp"
#include <SFML/Network/Packet.hpp>
#include <SFML/Network/IpAddress.hpp>
#include <fstream>
#include <iostream>



NewMultiplayerGameState::NewMultiplayerGameState(StateStack& stack, Context context)
    : State(stack, context)
    , m_window(*context.window)
    , m_world(*context.window, *context.fonts, *context.sounds, true)
    , m_multiplayerManager(context.multiplayer_manager)  // MultiplayerManager provided via context
    , m_connected(false)
    , m_active_state(true)
    , m_has_focus(true)
    , m_game_started(false)
    , m_client_timeout(sf::seconds(5.f))
    , m_time_since_last_packet(sf::seconds(0.f))
    , m_local_player_id(-1)
    , m_player_invitation_time(sf::Time::Zero)
    , m_broadcast_elapsed_time(sf::Time::Zero)
{
    // Setup broadcast text.
    m_broadcast_text.setFont(context.fonts->Get(Font::kMain));
    m_broadcast_text.setPosition(1024.f / 2, 100.f);

    // Setup failed connection text.
    m_failed_connection_text.setFont(context.fonts->Get(Font::kMain));
    m_failed_connection_text.setCharacterSize(35);
    m_failed_connection_text.setFillColor(sf::Color::White);
    m_failed_connection_text.setString("Attempting to connect...");
    Utility::CentreOrigin(m_failed_connection_text);
    m_failed_connection_text.setPosition(m_window.getSize().x / 2.f, m_window.getSize().y / 2.f);

    // Render initial connection message.
    m_window.clear(sf::Color::Black);
    m_window.draw(m_failed_connection_text);
    m_window.display();
    m_failed_connection_text.setString("Failed to connect to server");
    Utility::CentreOrigin(m_failed_connection_text);

    // Always run as host.
    m_multiplayerManager->HostServer();
    // As host, we always use the loopback address.
    sf::IpAddress ip = "127.0.0.1";

    // Connect to the local server.
    if (m_multiplayerManager->ConnectToServer(ip, 5.f, false))
    {
        m_connected = true;
    }
    else
    {
        m_failed_connection_clock.restart();
    }

    context.music->Play(MusicThemes::kMissionTheme);
}

NewMultiplayerGameState::~NewMultiplayerGameState()
{
    // Disconnect using the MultiplayerManager.
    m_multiplayerManager->Disconnect();
}

void NewMultiplayerGameState::Draw()
{
    if (m_connected)
    {
        m_world.Draw();
        m_window.setView(m_window.getDefaultView());
        if (!m_broadcasts.empty())
        {
            m_window.draw(m_broadcast_text);
        }
    }
    else
    {
        m_window.draw(m_failed_connection_text);
    }
}

bool NewMultiplayerGameState::Update(sf::Time dt)
{
    if (m_connected)
    {
        m_world.Update(dt);

        // Clean up any players whose ships no longer exist.
        for (auto itr = m_players.begin(); itr != m_players.end(); )
        {
            sf::Int8 player_id = itr->first;
            auto mapIt = m_playerShip.find(player_id);
            Ship* ship = (mapIt != m_playerShip.end()) ? m_world.GetShip(mapIt->second) : nullptr;
            if (!ship)
            {
                itr = m_players.erase(itr);
                if (m_players.empty())
                {
                    RequestStackPush(StateID::kGameOver);
                }
            }
            else
            {
                ++itr;
            }
        }

        if (m_local_player_id == -1 && m_game_started)
        {
            Utility::Debug("No local ship found in the game");
            RequestStackPush(StateID::kGameOver);
        }

        // Process local real-time input.
        if (m_active_state && m_has_focus)
        {
            CommandQueue& commands = m_world.GetCommandQueue();
            for (auto& pair : m_players)
            {
                pair.second->HandleRealtimeInput(commands);
            }
        }

        // Process network real-time input.
        {
            CommandQueue& commands = m_world.GetCommandQueue();
            for (auto& pair : m_players)
            {
                pair.second->HandleRealtimeNetworkInput(commands);
            }
        }

        // Process incoming packets.
        sf::Packet packet;
        if (m_multiplayerManager->GetSocket()->receive(packet) == sf::Socket::Done)
        {
            m_time_since_last_packet = sf::seconds(0.f);
            sf::Int32 packet_type;
            packet >> packet_type;
            HandlePacket(packet_type, packet);
        }
        else
        {
            if (m_time_since_last_packet > m_client_timeout)
            {
                m_connected = false;
                m_failed_connection_text.setString("Lost connection to the server");
                Utility::CentreOrigin(m_failed_connection_text);
                m_failed_connection_clock.restart();
            }
        }

        UpdateBroadcastMessage(dt);

        // Process game events (e.g., missile fire) and send to server.
        GameActions::Action game_action;
        while (m_world.PollGameAction(game_action))
        {
            sf::Packet packet;
            packet << static_cast<sf::Int32>(Client::PacketType::kGameEvent)
                << static_cast<sf::Int32>(game_action.type)
                << game_action.position.x
                << game_action.position.y;
            m_multiplayerManager->GetSocket()->send(packet);
        }

        // Send regular state updates.
        if (m_tick_clock.getElapsedTime() > sf::seconds(1.f / 20.f))
        {
            sf::Packet position_update_packet;
            position_update_packet << static_cast<sf::Int32>(Client::PacketType::kStateUpdate);
            if (m_local_player_id != -1)
            {
                position_update_packet << static_cast<sf::Int32>(1);
                Ship* ship = m_world.GetShip(m_playerShip[m_local_player_id]);
                if (ship)
                {
                    position_update_packet << m_local_player_id
                        << ship->getPosition().x
                        << ship->getPosition().y
                        << static_cast<sf::Int32>(ship->GetHitPoints())
                        << static_cast<sf::Int32>(ship->GetMissileAmmo());
                }
            }
            else
            {
                position_update_packet << static_cast<sf::Int32>(0);
            }
            m_multiplayerManager->GetSocket()->send(position_update_packet);
            m_tick_clock.restart();
        }
        m_time_since_last_packet += dt;
    }
    else if (m_failed_connection_clock.getElapsedTime() >= sf::seconds(5.f))
    {
        RequestStackClear();
        RequestStackPush(StateID::kMenu);
    }
    return true;
}

bool NewMultiplayerGameState::HandleEvent(const sf::Event& event)
{
    CommandQueue& commands = m_world.GetCommandQueue();
    for (auto& pair : m_players)
    {
        pair.second->HandleEvent(event, commands);
    }

    if (event.type == sf::Event::KeyPressed)
    {
        if (event.key.code == sf::Keyboard::Escape)
        {
            DisableAllRealtimeActions();
            RequestStackPush(StateID::kNetworkPause);
        }
    }
    else if (event.type == sf::Event::GainedFocus)
    {
        m_has_focus = true;
    }
    else if (event.type == sf::Event::LostFocus)
    {
        m_has_focus = false;
    }
    return true;
}

void NewMultiplayerGameState::OnActivate()
{
    m_active_state = true;
}

void NewMultiplayerGameState::OnDestroy()
{
    sf::Packet packet;
    packet << static_cast<sf::Int32>(Client::PacketType::kQuit);
    m_multiplayerManager->GetSocket()->send(packet);
}

void NewMultiplayerGameState::DisableAllRealtimeActions()
{
    m_active_state = false;
    if (m_local_player_id != -1 && m_players.find(m_local_player_id) != m_players.end())
    {
        m_players[m_local_player_id]->DisableAllRealtimeActions();
    }
}

void NewMultiplayerGameState::UpdateBroadcastMessage(sf::Time elapsed_time)
{
    if (m_broadcasts.empty())
        return;

    m_broadcast_elapsed_time += elapsed_time;
    if (m_broadcast_elapsed_time > sf::seconds(2.f))
    {
        m_broadcasts.erase(m_broadcasts.begin());
        if (!m_broadcasts.empty())
        {
            m_broadcast_text.setString(m_broadcasts.front());
            Utility::CentreOrigin(m_broadcast_text);
            m_broadcast_elapsed_time = sf::Time::Zero;
        }
    }
}

//------------------------------------------------
// Packet Handling Functions
//------------------------------------------------

void NewMultiplayerGameState::HandleBroadcastMessage(sf::Packet& packet)
{
    std::string message;
    packet >> message;
    m_broadcasts.push_back(message);
    if (m_broadcasts.size() == 1)
    {
        m_broadcast_text.setString(m_broadcasts.front());
        Utility::CentreOrigin(m_broadcast_text);
        m_broadcast_elapsed_time = sf::Time::Zero;
    }
}

void NewMultiplayerGameState::HandleSpawnSelf(sf::Packet& packet)
{
    sf::Int8 ship_id, role;
    sf::Vector2f ship_position;
    packet >> ship_id >> role >> ship_position.x >> ship_position.y;
    Ship* ship = m_world.AddShip(ship_id);
    ship->setPosition(ship_position);
    if (role == 0)
    {
        m_players[ship_id].reset(new Player(m_multiplayerManager->GetSocket(), ship_id, GetContext().keys1));
    }
    else if (role == 1)
    {
        m_players[ship_id].reset(new Player(m_multiplayerManager->GetSocket(), ship_id, GetContext().keys2));
    }
    else
    {
        m_players[ship_id].reset(new Player(m_multiplayerManager->GetSocket(), ship_id, nullptr));
    }
    m_local_player_id = ship_id;
    m_playerShip[ship_id] = ship_id;
    m_game_started = true;
}

void NewMultiplayerGameState::HandlePlayerConnect(sf::Packet& packet)
{
    sf::Int8 ship_id, role;
    sf::Vector2f ship_position;
    packet >> ship_id >> role >> ship_position.x >> ship_position.y;
    Ship* ship = m_world.AddShip(ship_id);
    ship->setPosition(ship_position);
    if (role == 0)
    {
        m_players[ship_id].reset(new Player(m_multiplayerManager->GetSocket(), ship_id, GetContext().keys1));
    }
    else if (role == 1)
    {
        m_players[ship_id].reset(new Player(m_multiplayerManager->GetSocket(), ship_id, GetContext().keys2));
    }
    else
    {
        m_players[ship_id].reset(new Player(m_multiplayerManager->GetSocket(), ship_id, nullptr));
    }
}

void NewMultiplayerGameState::HandlePlayerDisconnect(sf::Packet& packet)
{
    sf::Int8 player_id;
    packet >> player_id;
    std::cerr << "Player " << static_cast<int>(player_id) << " disconnected" << std::endl;
    m_players.erase(player_id);
}

void NewMultiplayerGameState::HandleInitialState(sf::Packet& packet)
{
    float world_height, current_scroll;
    packet >> world_height >> current_scroll;
    m_world.SetWorldHeight(world_height);
    m_world.SetCurrentBattleFieldPosition(current_scroll);

    sf::Int32 ship_count;
    packet >> ship_count;
    for (sf::Int32 i = 0; i < ship_count; ++i)
    {
        sf::Int8 ship_id;
        sf::Vector2f ship_position;
        sf::Int8 hitpoints, missile_ammo;
        sf::Int8 pilot_id, gunner_id;
        packet >> ship_id >> ship_position.x >> ship_position.y >> hitpoints >> missile_ammo >> pilot_id >> gunner_id;
        Ship* ship = m_world.AddShip(ship_id);
        ship->setPosition(ship_position);
        ship->SetHitpoints(hitpoints);
        ship->SetMissileAmmo(missile_ammo);
        ship->SetPilot(pilot_id);
        ship->SetGunner(gunner_id);
        if (pilot_id != -1)
        {
            m_players[pilot_id] = std::make_unique<Player>(m_multiplayerManager->GetSocket(), pilot_id, GetContext().keys1);
            if (pilot_id == m_local_player_id)
                m_local_player_id = pilot_id;
        }
        if (gunner_id != -1)
        {
            m_players[gunner_id] = std::make_unique<Player>(m_multiplayerManager->GetSocket(), gunner_id, GetContext().keys2);
            if (pilot_id == m_local_player_id)
                m_local_player_id = gunner_id;
        }
    }
    m_game_started = true;
}

void NewMultiplayerGameState::HandlePlayerEvent(sf::Packet& packet)
{
    sf::Int32 ship_identifier, action;
    packet >> ship_identifier >> action;
    auto itr = m_players.find(ship_identifier);
    if (itr != m_players.end())
    {
        itr->second->HandleNetworkEvent(static_cast<Action>(action), m_world.GetCommandQueue());
    }
}

void NewMultiplayerGameState::HandleRealTimeChange(sf::Packet& packet)
{
    sf::Int32 ship_identifier, action;
    bool action_enabled;
    packet >> ship_identifier >> action >> action_enabled;
    auto itr = m_players.find(ship_identifier);
    if (itr != m_players.end())
    {
        itr->second->HandleNetworkRealtimeChange(static_cast<Action>(action), action_enabled);
    }
}

void NewMultiplayerGameState::HandleSpawnEnemy(sf::Packet& packet)
{
    float height;
    sf::Int32 type;
    float relative_x;
    packet >> type >> height >> relative_x;
    m_world.AddEnemy(static_cast<ShipType>(type), relative_x, height);
    m_world.SortEnemies();
}

void NewMultiplayerGameState::HandleUpdateClient(sf::Packet& packet)
{
    float current_world_position;
    sf::Int32 ship_count;
    packet >> current_world_position >> ship_count;
    float current_view_position = m_world.GetViewBounds().top + m_world.GetViewBounds().height;
    m_world.SetWorldScrollCompensation(current_view_position / current_world_position);
    for (sf::Int32 i = 0; i < ship_count; ++i)
    {
        sf::Vector2f ship_position;
        sf::Int32 ship_identifier;
        sf::Int32 hitpoints;
        sf::Int32 ammo;
        packet >> ship_identifier >> ship_position.x >> ship_position.y >> hitpoints >> ammo;
        Ship* ship = m_world.GetShip(ship_identifier);
        bool is_local = (ship_identifier == static_cast<sf::Int32>(m_local_player_id));
        if (ship && !is_local)
        {
            sf::Vector2f interpolated_position = ship->getPosition() + (ship_position - ship->getPosition()) * 0.1f;
            ship->setPosition(interpolated_position);
            ship->SetHitpoints(hitpoints);
            ship->SetMissileAmmo(ammo);
        }
    }
}

void NewMultiplayerGameState::HandlePacket(sf::Int32 packet_type, sf::Packet& packet)
{
    switch (static_cast<Server::PacketType>(packet_type))
    {
    case Server::PacketType::kBroadcastMessage:
        HandleBroadcastMessage(packet);
        break;
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
    case Server::PacketType::kPlayerEvent:
        HandlePlayerEvent(packet);
        break;
    case Server::PacketType::kPlayerRealtimeChange:
        HandleRealTimeChange(packet);
        break;
    case Server::PacketType::kSpawnEnemy:
        HandleSpawnEnemy(packet);
        break;
    case Server::PacketType::kMissionSuccess:
        RequestStackPush(StateID::kMissionSuccess);
        break;
    case Server::PacketType::kSpawnPickup:
    {
        sf::Int32 type;
        sf::Vector2f position;
        packet >> type >> position.x >> position.y;
        m_world.CreatePickup(position, static_cast<PickupType>(type));
        break;
    }
    case Server::PacketType::kUpdateClientState:
        HandleUpdateClient(packet);
        break;
    default:
        break;
    }
}
