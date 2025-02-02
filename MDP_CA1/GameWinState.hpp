#pragma once
#include <SFML/Graphics/Sprite.hpp>

#include "State.hpp"
#include <SFML/Graphics/Text.hpp>

class GameWinState : public State
{
public:
	GameWinState(StateStack& stack, Context context, const std::string& text);
	virtual void Draw() override;
	virtual bool Update(sf::Time dt) override;
	virtual bool HandleEvent(const sf::Event& event);

private:
	sf::Sprite m_bg_sprite;
	sf::Text m_game_won_text;
	sf::Time m_elapsed_time;
};
