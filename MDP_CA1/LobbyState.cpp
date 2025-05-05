#include "LobbyState.hpp"
#include "Utility.hpp"
#include "NetworkProtocol.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Network/TcpListener.hpp>
#include <SFML/Network/IpAddress.hpp>
#include <fstream>

namespace
{
    // Reads the server IP from ip.txt or defaults to localhost.
    sf::IpAddress GetAddressFromFile()
    {
        std::ifstream in("ip.txt");
        if (in)
        {
            std::string ip;
            if (in >> ip)
                return ip;
        }
        std::ofstream out("ip.txt");
        out << "127.0.0.1";
        return "127.0.0.1";
    }
}

LobbyState::LobbyState(StateStack& stack, Context& context, bool isHost)
    : State(stack, context)
    , m_is_host(isHost)
{
    // Build UI first
    CreateUI(context);

    // Networking setup
    if (m_is_host)
    {
        context.multiplayer_manager->HostServer();
        m_server_ip = "127.0.0.1";
        sf::sleep(sf::milliseconds(200));
    }
    else
    {
        m_server_ip = GetAddressFromFile();
    }

    m_socket = context.multiplayer_manager->ConnectToServer(m_server_ip);
    if (m_socket)
    {
        m_connected = true;
        m_socket->setBlocking(false);
    }
    else
    {
        Utility::Debug("LobbyState: Failed to connect to server");
        m_connected = false;
    }

    m_is_connecting = true;
    m_lobby_time = sf::Time::Zero;
    m_send_time = sf::seconds(0.5f);
    m_client_timeout = sf::seconds(2.f);
    m_time_since_last_packet = sf::Time::Zero;
}

void LobbyState::CreateUI(Context& context)
{
    auto& window = *context.window;
    const float winW = static_cast<float>(window.getSize().x);
    const float winH = static_cast<float>(window.getSize().y);

    // "Connecting..." text
    Utility::CreateLabel(context, m_failed_connection_text,
        winW / 2.f, winH / 2.f,
        "Connecting to server...", 30);
    Utility::CentreOrigin(m_failed_connection_text->GetText());
    m_gui_fail_container.Pack(m_failed_connection_text);

    // Lobby title
    std::shared_ptr<gui::Label> title;
    Utility::CreateLabel(context, title, 100.f, 30.f, "Lobby", 60);
    m_gui_container.Pack(title);

    // Change Name button + label
    Utility::CreateButton(
        context,
        m_change_name_button,         // out-param
        winW - 220.f, 30.f,           // position
        "Name",                       // label
        [this] {                      // on-click
            m_change_name_button->Activate();
        },
        [this] {                      // enable predicate
            // Only allow renaming once we’re connected and not already typing
            return m_connected && !m_change_name_button->IsActive();
        }
    );
    m_gui_container.Pack(m_change_name_button);

    Utility::CreateLabel(
        context,
        m_current_name_label,
        winW - 100.f, 30.f,
        m_player_input_name, 20
    );
    Utility::CentreOrigin(m_current_name_label->GetText());
    m_gui_container.Pack(m_current_name_label);


    // Ready toggle button
    Utility::CreateButton(
        context,
        m_ready_button,
        100.f, winH - 80.f,
        "Ready",
        [this] {                      // on-click
            SendReadyToggle(!m_players[m_player_id].ready);
        },
        [this] {                      // enable predicate
            // Only allow toggling ready once connected and lobby isn’t locked
            return m_connected && !m_game_started;
        }
    );
    m_gui_container.Pack(m_ready_button);


    // Start Game button (host-only)
   // ✅ Use the Utility::CreateButton overload that takes a predicate
    gui::Button::Ptr startBtn;
    Utility::CreateButton(
        context,
        startBtn,            // out parameter
        300.f, winH - 80.f,  // position
        "Start",             // label
        [this] {             // on-click
            SendStartGame();
        },
        [this] {             // enable predicate
            if (!m_is_host) return false;
            if (m_players.size() < 2) return false;
            for (auto& kv : m_players)
                if (!kv.second.ready) return false;
            return true;
        }
    );
    m_gui_container.Pack(startBtn);


    // Countdown label (hidden until countdown starts)
    Utility::CreateLabel(context, m_start_countdown_label,
        100.f, winH - 120.f,
        "", 20);
    m_start_countdown_label->SetDrawPredicate([this] { return m_start_countdown; });
    m_gui_container.Pack(m_start_countdown_label);
}

void LobbyState::Draw()
{
    auto& window = *GetContext().window;
    window.clear(sf::Color(45, 37, 97));
    if (m_connected)
        window.draw(m_gui_container);
    else
        window.draw(m_gui_fail_container);
}

bool LobbyState::Update(sf::Time dt)
{
    if (m_is_connecting)
    {
        sf::Packet ping;
        if (m_socket && m_socket->send(ping) == sf::Socket::Done)
        {
            m_is_connecting = false;
            m_connected = true;
        }
        else if (m_failed_connection_clock.getElapsedTime() > sf::seconds(5.f))
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
        // Periodic keep-alive
        m_lobby_time += dt;
        if (m_lobby_time > m_send_time)
        {
            m_lobby_time = sf::Time::Zero;
            sf::Packet ping;
            m_socket->send(ping);
        }

        // Receive and dispatch
        sf::Packet packet;
        if (m_socket->receive(packet) == sf::Socket::Done)
        {
            m_time_since_last_packet = sf::Time::Zero;
            sf::Int8 type;
            packet >> type;
            HandlePacket(type, packet);
        }
        else
        {
            m_time_since_last_packet += dt;
            if (m_time_since_last_packet > m_client_timeout)
            {
                m_connected = false;
                m_failed_connection_text->SetText("Lost connection to server");
                Utility::CentreOrigin(m_failed_connection_text->GetText());
                m_failed_connection_clock.restart();
            }
        }

        // Countdown to start
        if (m_start_countdown)
        {
            m_start_countdown_timer -= dt;
            m_start_countdown_label->SetText(
                std::to_string(static_cast<int>(m_start_countdown_timer.asSeconds())));
            if (m_start_countdown_timer <= sf::Time::Zero && m_is_host)
            {
                SendStartGame();
                m_start_countdown = false;
            }
        }
    }
    else if (m_failed_connection_clock.getElapsedTime() >= sf::seconds(5.f))
    {
        RequestStackClear();
        RequestStackPush(StateID::kMenu);
    }

    return true;
}

bool LobbyState::HandleEvent(const sf::Event& event)
{
    if (m_game_started)
        return false;

    // Name-change mode
    if (m_change_name_button->IsActive())
    {
        if (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::Return)
        {
            m_change_name_button->Deactivate();
            if (m_player_id != -1 && m_players.count(m_player_id))
            {
                m_players[m_player_id].name = m_player_input_name;
                SendPlayerName(m_player_id, m_player_input_name);
                UpdatePlayerListUI();
            }
        }
        else if (event.type == sf::Event::TextEntered)
        {
            if (event.text.unicode == '')
            {
                if (!m_player_input_name.empty())
                    m_player_input_name.pop_back();
            }
            else if (event.text.unicode >= 32 && event.text.unicode < 128)
            {
                m_player_input_name.push_back(static_cast<char>(event.text.unicode));
                if (m_player_input_name.size() > 15)
                    m_player_input_name.resize(15);
            }
            m_current_name_label->SetText(m_player_input_name);
            Utility::CentreOrigin(m_current_name_label->GetText());
        }
    }
    else
    {
        m_gui_container.HandleEvent(event);
    }

    if (event.type == sf::Event::GainedFocus)
        GetContext().multiplayer_manager->SetPassFocus(true);
    else if (event.type == sf::Event::LostFocus)
        GetContext().multiplayer_manager->SetPassFocus(false);

    return false;
}

void LobbyState::OnStackPopped()
{
    if (!m_game_started)
        SendClientDisconnect(m_player_id);
}

void LobbyState::HandlePacket(sf::Int8 packetType, sf::Packet& packet)
{
    switch (static_cast<Server::PacketType>(packetType))
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
    case Server::PacketType::kLobbyPlayers:
        HandleLobbyPlayers(packet);
        break;
    case Server::PacketType::kPlayerUpdate:
        HandleUpdatePlayer(packet);
        break;
    case Server::PacketType::kStartGameCountdown:
        HandleStartGameCountdown();
        break;
    case Server::PacketType::kGameStart:
        HandleGameStart();
        break;
    default:
        break;
    }
}

void LobbyState::HandlePlayerConnect(sf::Packet& packet)
{
    sf::Int8 id;
    std::string name;
    packet >> id >> name;
    AddPlayer(id, name);
}

void LobbyState::HandlePlayerDisconnect(sf::Packet& packet)
{
    sf::Int8 id;
    packet >> id;
    RemovePlayer(id);
}

void LobbyState::HandleUpdatePlayer(sf::Packet& packet)
{
    sf::Int8 id;
    std::string name;
    packet >> id >> name;
    if (m_players.count(id))
    {
        m_players[id].name = name;
        UpdatePlayerListUI();
    }
}

void LobbyState::HandleInitialState(sf::Packet& packet)
{
    HandleLobbyPlayers(packet);
}

void LobbyState::HandleLobbyPlayers(sf::Packet& packet)
{
    sf::Int8 count;
    packet >> count;

    // Clear existing UI
    for (auto& kv : m_players)
        m_gui_container.Pull(kv.second.label);
    m_players.clear();

    // Rebuild list
    const float startX = 100.f;
    const float startY = 100.f;
    for (int i = 0; i < count; ++i)
    {
        sf::Int8 id;
        std::string name;
        sf::Int8 ready;
        packet >> id >> name >> ready;

        PlayerInfo info;
        info.name = name;
        info.ready = (ready != 0);
        Utility::CreateLabel(GetContext(), info.label,
            startX, startY + i * 30.f,
            name + (info.ready ? " [Ready]" : " [ ]"), 20);
        m_players[id] = std::move(info);
        m_gui_container.Pack(m_players[id].label);
    }
    UpdatePlayerListUI();
}

void LobbyState::HandleSpawnSelf(sf::Packet& packet)
{
    sf::Int8 id;
    packet >> id;
    m_player_id = id;
    AddPlayer(id, m_player_input_name);
    SendPlayerName(id, m_player_input_name);
}

void LobbyState::HandleGameStart()
{
    m_game_started = true;
    RequestStackClear();
    RequestStackPush(StateID::kNetworkGame);
}

void LobbyState::HandleStartGameCountdown()
{
    m_start_countdown = true;
    m_start_countdown_timer = sf::seconds(5.f);
    m_start_countdown_label->SetText(
        std::to_string(static_cast<int>(m_start_countdown_timer.asSeconds())));
}

void LobbyState::SendClientDisconnect(sf::Int8 id) const
{
    sf::Packet packet;
    packet << static_cast<sf::Int8>(Client::PacketType::kQuit) << id;
    m_socket->send(packet);
}

void LobbyState::SendPlayerName(sf::Int8 id, const std::string& name) const
{
    if (!m_socket) return;
    sf::Packet packet;
    packet << static_cast<sf::Int8>(Client::PacketType::kPlayerUpdate)
        << id << name;
    m_socket->send(packet);
}

void LobbyState::SendReadyToggle(bool isReady)
{
    m_players.at(m_player_id).ready = isReady;
    sf::Packet packet;
    packet << static_cast<sf::Int8>(Client::PacketType::kLobbyReady)
        << m_player_id
        << static_cast<sf::Int8>(isReady);
    m_socket->send(packet);
    UpdatePlayerListUI();
}

void LobbyState::SendStartGameCountdown() const
{
    sf::Packet packet;
    packet << static_cast<sf::Int8>(Client::PacketType::kStartGameCountdown);
    m_socket->send(packet);
}

void LobbyState::SendStartGame() const
{
    sf::Packet packet;
    packet << static_cast<sf::Int8>(Client::PacketType::kStartNetworkGame);
    m_socket->send(packet);
}

void LobbyState::AddPlayer(sf::Int8 id, const std::string& name)
{
    if (m_players.count(id)) return;
    PlayerInfo info;
    info.name = name;
    Utility::CreateLabel(GetContext(), info.label,
        100.f, 100.f + m_players.size() * 30.f,
        name + " [ ]", 20);
    m_players[id] = std::move(info);
    m_gui_container.Pack(m_players[id].label);
    UpdatePlayerListUI();
}

void LobbyState::RemovePlayer(sf::Int8 id)
{
    auto it = m_players.find(id);
    if (it == m_players.end()) return;
    m_gui_container.Pull(it->second.label);
    m_players.erase(it);
    UpdatePlayerListUI();
}

void LobbyState::UpdatePlayerListUI()
{
    float y = 100.f;
    for (auto& kv : m_players)
    {
        auto& info = kv.second;
        info.label->SetText(
            info.name + (info.ready ? " [Ready]" : " [ ]"));
        info.label->setPosition(100.f, y);
        y += 30.f;
    }
}
