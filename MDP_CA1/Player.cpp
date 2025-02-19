#include "Player.hpp"
#include "ReceiverCategories.hpp"
#include "Ship.hpp"

//struct ShipMover
//{
//    ShipMover(float vx, float vy) :velocity(vx,vy)
//    {}
//    void operator()(Ship& ship, sf::Time dt) const
//    {
//        ship.Accelerate(velocity);
//    }
//    sf::Vector2f velocity;
//};

Player::Player(sf::Int32 id, const KeyBinding* binding)
: m_current_mission_status(MissionStatus::kMissionRunning)
,m_key_binding(binding)
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
            command_queue.Push(m_action_binding[action]);
        }


    }
}

void Player::HandleRealTimeInput(CommandQueue& command_queue)
{
    std::vector<Action> activeActions = m_key_binding->GetRealtimeActions();
    for (Action action : activeActions)
        command_queue.Push(m_action_binding[action]);

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
            a.LaunchMissile();
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
    case Action::kMoveDown:
    case Action::kMoveUp:
    case Action::kRotateLeft:
    case Action::kRotateRight:
    case Action::kAim:
    //case Action::kMissileFire:
        return true;
    default:
        return false;
    }
}
