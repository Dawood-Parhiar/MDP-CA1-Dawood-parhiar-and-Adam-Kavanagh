#include "GameWinState.hpp"
#include "ResourceHolder.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include "Player.hpp"
#include "Utility.hpp"


GameWinState::GameWinState(StateStack& stack, Context context, const std::string& text)
    : State(stack, context)
    , m_game_won_text()
    , m_elapsed_time(sf::Time::Zero)
{
    sf::Texture& texture = context.textures->Get(TextureID::kGameWonSprite);
    m_bg_sprite.setTexture(texture);

    sf::Font& font = context.fonts->Get(Font::kMain);
    sf::Vector2f window_size(context.window->getSize());

    m_game_won_text.setFont(font);
    m_game_won_text.setString(text);

    m_game_won_text.setCharacterSize(70);
    Utility::CentreOrigin(m_game_won_text);
    m_game_won_text.setPosition(0.5f * window_size.x, 0.4 * window_size.y);

}

void GameWinState::Draw()
{
    sf::RenderWindow& window = *GetContext().window;
    window.setView(window.getDefaultView());

    //Create a dark semi-transparent background
    sf::RectangleShape background_shape;
    background_shape.setFillColor(sf::Color(0, 0, 0, 200));
    background_shape.setSize(window.getView().getSize());

    window.draw(m_bg_sprite);
    window.draw(background_shape);
    window.draw(m_game_won_text);
}

bool GameWinState::Update(sf::Time dt)
{
    //Show gameover for 3 seconds and then return to the main menu
    m_elapsed_time += dt;
    if (m_elapsed_time > sf::seconds(3))
    {
        RequestStackClear();
        RequestStackPush(StateID::kMenu);
    }
    return false;
}

bool GameWinState::HandleEvent(const sf::Event& event)
{
    return false;
}
