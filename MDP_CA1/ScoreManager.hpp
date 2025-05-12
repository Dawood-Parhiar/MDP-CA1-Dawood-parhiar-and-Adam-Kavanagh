#pragma once
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Clock.hpp>

class ScoreManager
{
public:
    ScoreManager();

    void AddEnemyKill(); // Increment score for killing an enemy
    void AddCoinPickup(); // Increment score for picking up a coin
    void UpdateTimeSurvived(); // Update score based on time survived
    void ResetScore();
    int GetScore() const;

    void UpdateAmmo(int ammo); 
    void UpdateHealth(int health); 
    void UpdateCoins(int coins); 

    void SetCenteredPosition(const sf::RenderWindow& window); // Set the score at the center top
    void Draw(sf::RenderWindow& window);

private:
    int m_score;
    int m_enemiesKilled;
    int m_ammo;
    int m_health;
    int m_coins;
    sf::Clock m_clock; // Tracks elapsed time
    sf::Font m_font;
    sf::Text m_scoreText;
    sf::Text m_ammoText;
    sf::Text m_healthText;
    sf::Text m_coinsText;
    sf::RectangleShape m_background; 

    void UpdateScoreText();
    void UpdateAmmoText();
    void UpdateHealthText();
    void UpdateCoinsText();
    void UpdateBackground(); 
};
