#include "Player.hpp"
#include "ReceiverCategories.hpp"
#include "Ship.hpp"

struct ShipMover
{
    ShipMover(float vx, float vy) :velocity(vx, vy)
    {}
    void operator()(Ship& ship, sf::Time) const
    {
        ship.Accelerate(velocity);
    }

    sf::Vector2f velocity;
};

Player::Player()
: m_current_mission_status(MissionStatus::kMissionRunning)

{
    
    //Set initial key bindings
    m_key_binding[sf::Keyboard::A] = Action::kMoveLeft;
    m_key_binding[sf::Keyboard::D] = Action::kMoveRight;
    m_key_binding[sf::Keyboard::W] = Action::kMoveUp;
    m_key_binding[sf::Keyboard::S] = Action::kMoveDown;
    m_key_binding[sf::Keyboard::M] = Action::kMissileFire;
    m_key_binding[sf::Keyboard::Left] = Action::kRotateLeft;
    m_key_binding[sf::Keyboard::Right] = Action::kRotateRight;
    m_key_binding[sf::Keyboard::RShift] = Action::kAim;
    //m_key_binding[sf::Keyboard::Space] = Action::kBulletFire;

    //Set initial action bindings
    InitialiseActions();

    //Assign all categories to a player's aircraft
    for (auto& pair : m_action_binding)
    {
        pair.second.category = static_cast<unsigned int>(ReceiverCategories::kPlayerAircraft);
    }
}

void Player::HandleEvent(const sf::Event& event, CommandQueue& command_queue)
{
    if (event.type == sf::Event::KeyPressed)
    {
        auto found = m_key_binding.find(event.key.code);
        if (found != m_key_binding.end() && !IsRealTimeAction(found->second))
        {
            command_queue.Push(m_action_binding[found->second]);
        }
    }
}

void Player::HandleRealTimeInput(CommandQueue& command_queue)
{
    //Check if any of the key bindings are pressed
    for (auto pair : m_key_binding)
    {
        if (sf::Keyboard::isKeyPressed(pair.first) && IsRealTimeAction(pair.second))
        {
            command_queue.Push(m_action_binding[pair.second]);
        }
    }
}

void Player::AssignKey(Action action, sf::Keyboard::Key key)
{
    //Remove keys that are currently bound to the action
    for (auto itr = m_key_binding.begin(); itr != m_key_binding.end();)
    {
        if (itr->second == action)
        {
            m_key_binding.erase(itr++);
        }
        else
        {
            ++itr;
        }
    }
    m_key_binding[key] = action;
}

sf::Keyboard::Key Player::GetAssignedKey(Action action) const
{
    for (auto pair : m_key_binding)
    {
        if (pair.second == action)
        {
            return pair.first;
        }
    }
    return sf::Keyboard::Unknown;
}

void Player::SetMissionStatus(MissionStatus status)
{
    m_current_mission_status = status;
}

MissionStatus Player::GetMissionStatus() const
{
    return m_current_mission_status;
}

void Player::InitialiseActions()
{
    const float kPlayerSpeed = 200.f;
    m_action_binding[Action::kMoveLeft].action = DerivedAction<Ship>(ShipMover(-kPlayerSpeed, 0.f));
    m_action_binding[Action::kMoveRight].action = DerivedAction<Ship>(ShipMover(kPlayerSpeed, 0.f));
    m_action_binding[Action::kMoveUp].action = DerivedAction<Ship>(ShipMover(0.f, -kPlayerSpeed));
    m_action_binding[Action::kMoveDown].action = DerivedAction<Ship>(ShipMover(0.f, kPlayerSpeed));
    // m_action_binding[Action::kBulletFire].action = DerivedAction<Ship>([](Ship& a, sf::Time dt)
    //     {
    //         a.Fire();
    //     }
    // );

    m_action_binding[Action::kMissileFire].action = DerivedAction<Ship>([](Ship& a, sf::Time dt)
        {
            a.LaunchMissile();
        }
    );
    m_action_binding[Action::kRotateLeft].action = DerivedAction<Ship>([](Ship& a, sf::Time dt)
        {
            a.RotateShip();
        }
    );
    m_action_binding[Action::kRotateRight].action = DerivedAction<Ship>([](Ship& a, sf::Time dt)
        {
            a.RotateShip();
        }
    );
    m_action_binding[Action::kAim].action = DerivedAction<Ship>([this](Ship& a, sf::Time dt)
    {
       
        a.Aim();
        
    });

}

bool Player::IsRealTimeAction(Action action)
{
    switch (action)
    {
    case Action::kMoveLeft:
    case Action::kMoveRight:
    case Action::kMoveDown:
    case Action::kMoveUp:
    case Action::kRotateLeft:
    case Action::kRotateRight:
    case Action::kAim:
    //case Action::kBulletFire:
        return true;
    default:
        return false;
    }
}
