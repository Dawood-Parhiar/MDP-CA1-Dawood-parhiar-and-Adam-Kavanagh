#include "ScoreManager.hpp"

ScoreManager::ScoreManager()
    : m_score(0), m_enemiesKilled(0)
{
    // Load the font
    if (!m_font.loadFromFile("Media/Fonts/BlackRose-2Onld.ttf"))
    {
        throw std::runtime_error("Failed to load font");
    }

    // Configure the score text
    m_scoreText.setFont(m_font);
    m_scoreText.setCharacterSize(24);
    m_scoreText.setFillColor(sf::Color::White);
    UpdateScoreText();

	// Background for score text
    m_background.setFillColor(sf::Color(0, 0, 0, 150)); 
    m_background.setOutlineColor(sf::Color::White);    
    m_background.setOutlineThickness(2.f);             
    UpdateBackground();
}

void ScoreManager::AddEnemyKill()
{
    m_enemiesKilled++;
    m_score += 10; // Add 10 points per enemy killed
    UpdateScoreText();
}

void ScoreManager::UpdateTimeSurvived()
{
    // Add 5 points for every minute survived
    if (m_clock.getElapsedTime().asSeconds() >= 60.f)
    {
        m_score += 5;
        m_clock.restart(); // Reset the clock for the next minute
        UpdateScoreText();
    }
}

void ScoreManager::ResetScore()
{
    m_score = 0;
    m_enemiesKilled = 0;
    m_clock.restart();
    UpdateScoreText();
}

int ScoreManager::GetScore() const
{
    return m_score;
}

void ScoreManager::SetCenteredPosition(const sf::RenderWindow& window)
{
    // Top position
    float x = window.getSize().x / 2.f - m_scoreText.getLocalBounds().width / 2.f;
    float y = -30.f; 
    m_scoreText.setPosition(x, y);

    // Update the background position
    UpdateBackground();
}

void ScoreManager::Draw(sf::RenderWindow& window)
{
    window.draw(m_background); 
    window.draw(m_scoreText);  
}

void ScoreManager::UpdateScoreText()
{
    m_scoreText.setString("Score: " + std::to_string(m_score));
    UpdateBackground(); // 
}

void ScoreManager::UpdateBackground()
{
    
    sf::FloatRect textBounds = m_scoreText.getLocalBounds();

    
    m_background.setSize(sf::Vector2f(textBounds.width + 10.f, textBounds.height + 10.f));

    
    m_background.setPosition(m_scoreText.getPosition().x - 5.f, m_scoreText.getPosition().y - 5.f);
}
