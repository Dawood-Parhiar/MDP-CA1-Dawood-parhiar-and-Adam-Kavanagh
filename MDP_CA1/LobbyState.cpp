#include "LobbyState.hpp"

#include <iostream>
#include <SFML/Network/Packet.hpp>

#include "FontID.hpp"
#include "NetworkProtocol.hpp"
#include "TextNode.hpp"


LobbyState::LobbyState(StateStack& stack, Context context)
    : State(stack, context)
    , m_thread(&LobbyState::ExecutionThread, this)
    , m_listening_state(false),
    m_client_timeout(sf::seconds(10.f)),
    m_waiting_thread_end(false),
    m_connected_players(0),
    m_ships_assigned(0),
    m_max_connected_players(30),
    m_ship_id_counter(0)

{
    m_background_sprite.setTexture(context.textures->Get(TextureID::kLobbyBg));


    // Title label
    auto title_label = std::make_shared<gui::Label>("Lobby - Waiting for Players", *context.fonts, *context.textures);
    title_label->setPosition(200.f, 50.f);
    m_gui_container.Pack(title_label);

    // Start button (disabled until 2 players are assigned)
    m_start_button = std::make_shared<gui::Button>(context);
    m_start_button->setPosition(1600.f, 900.f);
    m_start_button->SetText("Start Game");
    m_start_button->SetCallback([this]() { RequestStackPop(); RequestStackPush(StateID::kGame); });
   // m_start_button->SetEnabled(true); // Initially disabled
    

    auto back_button = std::make_shared<gui::Button>(context);
    back_button->setPosition(1600.f, 750.f);
    back_button->SetText("Return");
    back_button->SetCallback([this] { RequestStackPop(); RequestStackPush(StateID::kMenu); });

    m_gui_container.Pack(m_start_button);
    m_gui_container.Pack(back_button);

    SetListening(true);
    m_thread.launch();
}

LobbyState::~LobbyState()
{
    m_waiting_thread_end = true;
    m_thread.wait();
}

void LobbyState::Draw()
{
    sf::RenderWindow& window = *GetContext().window;
    window.draw(m_background_sprite);
    window.draw(m_gui_container);

    for (const auto& label : m_player_labels)
    {
        window.draw(*label);
    }
}

bool LobbyState::Update(sf::Time dt)
{
    return true;
}

void LobbyState::UpdatePlayerList()
{
    m_player_labels.clear();
    for (size_t i = 0; i < m_players.size(); ++i)
    {
        auto label = std::make_shared<gui::Label>("Player " + std::to_string(m_players[i]), *GetContext().fonts, *GetContext().textures);
        label->setPosition(100.f, 50.f + i * 30.f);
        m_player_labels.push_back(label);
    }
}

void LobbyState::HandlePlayerJoin(int player_id)
{
    m_players.push_back(player_id);
    UpdatePlayerList();
    TryAssignShips();
}

void LobbyState::TryAssignShips()
{
    while (m_players.size() >= 2)
    {
        int p1 = m_players.back(); m_players.pop_back();
        int p2 = m_players.back(); m_players.pop_back();
        m_assigned_ships.emplace_back(std::to_string(p1), std::to_string(p2));
        std::cout << "Assigned Players " << p1 << " and " << p2 << " to a ship." << std::endl;
    }
}

bool LobbyState::HandleEvent(const sf::Event& event)
{
    m_gui_container.HandleEvent(event);
    return true;
}

void LobbyState::AddPlayer(const std::string& playerName)
{
    m_players.push_back(m_connected_players);
    m_connected_players++;

    auto label = std::make_shared<gui::Label>(playerName, *GetContext().fonts, *GetContext().textures);
    label->setPosition(100.f, 200.f + m_players.size() * 50);
    m_player_labels.push_back(label);
}

void LobbyState::AssignShips()
{
    for (size_t i = 0; i < m_players.size(); i += 2)
    {
        if (i + 1 < m_players.size())
        {
            std::string ship_id = "Ship_" + std::to_string(m_ship_id_counter++);
            m_assigned_ships.push_back({ std::to_string(m_players[i]), std::to_string(m_players[i + 1]) });
            std::cout << "Assigned Ship " << ship_id << " to Players " << m_players[i] << " and " << m_players[i + 1] << "\n";
        }
    }
}

LobbyState::RemotePeer::RemotePeer(): m_ready(false), m_timed_out(false)
{
    m_socket.setBlocking(false);
}

void LobbyState::SetListening(bool enable)
{
    if (enable)
    {
        if (!m_listening_state)
        {
            m_listening_state = (m_listener_socket.listen(SERVER_PORT) == sf::Socket::Done);
        }
    }
    else
    {
        m_listener_socket.close();
        m_listening_state = false;
    }
}

void LobbyState::ExecutionThread()
{
    while (!m_waiting_thread_end)
    {
        HandleIncomingConnections();
        HandleIncomingPackets();
        sf::sleep(sf::milliseconds(10)); // Prevents tight infinite loop
    }
}


void LobbyState::Tick()
{
}

sf::Time LobbyState::Now() const
{
    return m_clock.getElapsedTime();
}

void LobbyState::HandleIncomingPackets()
{
    for (auto& peer : m_peers)
    {
        sf::Packet packet;
        if (peer->m_socket.receive(packet) == sf::Socket::Done)
        {
            int packet_type;
            packet >> packet_type;

            switch (packet_type)
            {
            case static_cast<int>(Server::PacketType::kPlayerConnect):
            {
                int player_id;
                packet >> player_id;
                HandlePlayerJoin(player_id);
                UpdateClientState(); // Notify all players
                break;
            }
            case static_cast<int>(Server::PacketType::kBroadcastMessage):
            {
                std::string message;
                packet >> message;
                BroadcastMessage(message);
                break;
            }
            case static_cast<int>(Server::PacketType::kPlayerReady):
            {
                peer->m_ready = true;
                std::cout <<"Player " << peer->m_socket.getRemotePort() <<" is ready!" << std::endl;
                break;
            }
            default:
                std::cout << "Received unknown packet type." << std::endl;
                break;
            }
        }
    }
}


void LobbyState::HandleIncomingPackets(sf::Packet& packet, RemotePeer& receiving_peer, bool& detected_timeout)
{
    for (auto& peer : m_peers)
    {
        sf::Packet packet;
        if (peer->m_socket.receive(packet) == sf::Socket::Done)
        {
            int player_id;
            packet >> player_id;
            HandlePlayerJoin(player_id);
        }
    }
}

void LobbyState::HandleIncomingConnections()
{
    m_peers.emplace_back(std::make_unique<RemotePeer>());
    if (m_listener_socket.accept(m_peers.back()->m_socket) == sf::Socket::Done)
    {
        std::cout << "New player joined!" << std::endl;
        int new_player_id = static_cast<int>(m_peers.size()); // Assign a player ID
        HandlePlayerJoin(new_player_id);
        UpdateClientState(); // Notify all clients
    }
    else
    {
        m_peers.pop_back(); // Remove the failed connection
    }
}



void LobbyState::HandleDisconnections()
{
    for (auto it = m_peers.begin(); it != m_peers.end();)
    {
        if ((*it)->m_socket.getRemoteAddress() == sf::IpAddress::None)
        {
            std::cout << "Player disconnected!" << std::endl;
            it = m_peers.erase(it);
        }
        else
        {
            ++it;
        }
    }
}


void LobbyState::InformWorldState(sf::TcpSocket& socket)
{
}

void LobbyState::BroadcastMessage(const std::string& message)
{
    sf::Packet packet;
    packet << message;
    SendToAll(packet);
}


void LobbyState::SendToAll(sf::Packet& packet)
{
    for (auto& peer : m_peers)
    {
        if (peer->m_socket.send(packet) != sf::Socket::Done)
        {
            std::cout << "Failed to send packet to a peer." << std::endl;
        }
    }
}


void LobbyState::UpdateClientState()
{
    sf::Packet packet;
    packet << static_cast<int>(m_players.size()); // Send number of players
    for (int player_id : m_players)
    {
        packet << player_id; // Send each player ID
    }

    SendToAll(packet);
}

