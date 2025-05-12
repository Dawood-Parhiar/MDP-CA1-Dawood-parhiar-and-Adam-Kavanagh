#pragma once
#include "State.hpp"
#include "World.hpp"
#include "Player.hpp"
#include "ScoreManager.hpp" // Include the ScoreManager 

class GameState : public State
{
public:
    GameState(StateStack& stack, Context context);

    virtual void Draw();
    virtual bool Update(sf::Time dt);
    virtual bool HandleEvent(const sf::Event& event);

private:
    World m_world;
    Player m_player;
    ScoreManager m_score_manager; // Add ScoreManager instance
};


