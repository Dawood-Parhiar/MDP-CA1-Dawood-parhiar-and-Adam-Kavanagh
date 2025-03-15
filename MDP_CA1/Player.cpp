#include "Player.hpp"

#include "NetworkProtocol.hpp"
#include "ReceiverCategories.hpp"
#include "Ship.hpp"

struct ShipMissileTrigger
{
    ShipMissileTrigger(int identifier)
        : ship_id(identifier)
    {
    }

    void operator() (Ship& ship, sf::Time) const
    {
        if (ship.GetIdentifier() == ship_id)
            ship.LaunchPlayerCannon();
    }

    int ship_id;
};

Player::Player(sf::TcpSocket* socket,sf::Int32 id, const KeyBinding* binding) //Code changes from Dawood Parhiar D00248313
	: m_current_mission_status(MissionStatus::kMissionRunning)
	,m_key_binding(binding)
    ,m_identifier(id)
	,m_socket(socket)
{
    InitialiseActions();

    //Assign all categories to a player's aircraft
    for (auto& pair : m_action_binding)
    {
        pair.second.category = static_cast<unsigned int>(ReceiverCategories::kPlayerShip);
            
    }
}

void Player::HandleEvent(const sf::Event& event, CommandQueue& command_queue)
{
    if (event.type == sf::Event::KeyPressed)
    {
        Action action;
       
        if (m_key_binding && m_key_binding->CheckAction(event.key.code, action) && !IsRealtimeAction(action))
        {
            // Network connected -> send event over network
            if (m_socket)
            {
                sf::Packet packet;
                packet << static_cast<sf::Int32>(Client::PacketType::kPlayerEvent);
                packet << m_identifier;
                packet << static_cast<sf::Int32>(action);
                m_socket->send(packet);
            }
            else
            {
                command_queue.Push(m_action_binding[action]);
            }
        }
    }
    // Realtime change (network connected)
    if ((event.type == sf::Event::KeyPressed || event.type == sf::Event::KeyReleased) && m_socket)
    {
        Action action;
        if (m_key_binding && m_key_binding->CheckAction(event.key.code, action) && IsRealtimeAction(action))
        {
            // Send realtime change over network
            sf::Packet packet;
            packet << static_cast<sf::Int32>(Client::PacketType::kPlayerRealtimeChange);
            packet << m_identifier;
            packet << static_cast<sf::Int32>(action);
            packet << (event.type == sf::Event::KeyPressed);
            m_socket->send(packet);
        }
    }
}

bool Player::IsLocal() const
{
    // No key binding means this player is remote
    return m_key_binding != nullptr;
}

void Player::HandleRealtimeInput(CommandQueue& command_queue)
{
    if (m_socket && IsLocal() || !m_socket) 
    {
        std::vector<Action> activeActions = m_key_binding->GetRealtimeActions();
        for (Action action : activeActions)
            command_queue.Push(m_action_binding[action]);
    }
}

void Player::SetMissionStatus(MissionStatus status)
{
    m_current_mission_status = status;
}

MissionStatus Player::GetMissionStatus() const
{
    return m_current_mission_status;
}

void Player::HandleRealtimeNetworkInput(CommandQueue& commands)
{
    if (m_socket && !IsLocal())
    {
	    for (auto pair: m_action_proxies)
	    {
            if (pair.second && IsRealtimeAction(pair.first))
                commands.Push(m_action_binding[pair.first]);
	    }
    }
}

void Player::HandleNetworkEvent(Action action, CommandQueue& commands)
{
    commands.Push(m_action_binding[action]);
}

void Player::HandleNetworkRealtimeChange(Action action, bool action_enabled)
{
    m_action_proxies[action] = action_enabled;
}

void Player::DisableAllRealtimeActions()
{
    for (auto& action : m_action_proxies)
    {
        sf::Packet packet;
        packet << static_cast<sf::Int32>(Client::PacketType::kPlayerRealtimeChange);
        packet << m_identifier;
        packet << static_cast<sf::Int32>(action.first);
        packet << false;
        m_socket->send(packet);
    }
}

void Player::InitialiseActions()
{
    const float kPlayerSpeed = 200.f;

    m_action_binding[Action::kMoveUp].action = DerivedAction<Ship>([kPlayerSpeed](Ship& s, sf::Time dt){
        //Move the ship up
        s.MoveShip(dt, -kPlayerSpeed);
    });
    m_action_binding[Action::kMoveDown].action = DerivedAction<Ship>([kPlayerSpeed](Ship& s, sf::Time dt) {

        //Move the ship down
        s.MoveShip(dt, kPlayerSpeed);
        });
    m_action_binding[Action::kRotateLeft].action = DerivedAction<Ship>([](Ship& a, sf::Time dt)
	    {
        //rotate the ship left
    	a.rotate(-0.5f);
            
	    });

    m_action_binding[Action::kRotateRight].action = DerivedAction<Ship>([](Ship& a, sf::Time dt)
	    {
	    //rotate the ship right
    	a.rotate(0.5f);
            
	    });

    m_action_binding[Action::kMissileFire].action = DerivedAction<Ship>([](Ship& a, sf::Time dt)
        {
            a.LaunchPlayerCannon();
        }
    );
    m_action_binding[Action::kRotateCannonLeft].action = DerivedAction<Ship>([this](Ship& s, sf::Time dt)
    {
            if (s.GetCannon())
            {
                s.GetCannon()->SetRotationInput(-0.5);
            }
        
    });
    m_action_binding[Action::kRotateCannonRight].action = DerivedAction<Ship>([this](Ship& s, sf::Time dt)
        {
            if (s.GetCannon())
            {
                s.GetCannon()->SetRotationInput(0.5);
            }

        });
}

