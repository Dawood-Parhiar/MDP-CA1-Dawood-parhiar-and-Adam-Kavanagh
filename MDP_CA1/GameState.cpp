#include "GameState.hpp"
#include "Player.hpp"
#include "MissionStatus.hpp"

GameState::GameState(StateStack& stack, Context context)
	: State(stack, context)
	, m_world(*context.window, *context.fonts, *context.sounds)
	, m_player(nullptr, 1, context.keys1)
	, m_score_manager() // Initialize ScoreManager
{
	m_world.AddShip(1);
	m_player.SetMissionStatus(MissionStatus::kMissionRunning);

	// Set the position of the score at top of the screen
	m_score_manager.SetCenteredPosition(*context.window);

	// Play the music
	context.music->Play(MusicThemes::kMissionTheme);
}


void GameState::Draw()
{
    m_world.Draw();

    // Draw the score
    m_score_manager.Draw(*GetContext().window);
}

bool GameState::Update(sf::Time dt)
{
    m_world.Update(dt);

    // Check if the player is alive or has reached the end
    if (!m_world.HasAlivePlayer())
    {
        m_player.SetMissionStatus(MissionStatus::kMissionFailure);
        RequestStackPush(StateID::kGameOver);
    }
    else if (m_world.HasPlayerReachedEnd())
    {
        m_player.SetMissionStatus(MissionStatus::kMissionSuccess);
        RequestStackPush(StateID::kMissionSuccess);
    }

    // Update the score for killing enemies
    if (/* condition for enemy killed */ false)
    {
        m_score_manager.AddEnemyKill();
    }

    // Update the score based on time survived
    m_score_manager.UpdateTimeSurvived();

    // Update the command queue
    CommandQueue& commands = m_world.GetCommandQueue();
    m_player.HandleRealtimeInput(commands);

    return true;
}


bool GameState::HandleEvent(const sf::Event& event)
{
    CommandQueue& commands = m_world.GetCommandQueue();
    m_player.HandleEvent(event, commands);

    // Escape should bring up the pause menu
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
    {
        RequestStackPush(StateID::kPause);
    }

    // : Reset score when a specific key is pressed
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R)
    {
        m_score_manager.ResetScore();
    }

    return true;
}

