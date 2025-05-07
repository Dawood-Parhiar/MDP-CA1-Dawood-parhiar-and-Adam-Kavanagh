#pragma once
#include <SFML/Graphics/Sprite.hpp>

#include "Button.hpp"
#include "Container.hpp"
#include "Label.hpp"
#include "State.hpp"


/*
 * Code from Dylan and Paul
 */
class JoinServerState : public State
{
public:
	JoinServerState(StateStack& stack, Context& context);
	void Draw() override;
	bool Update(sf::Time dt) override;
	bool HandleEvent(const sf::Event& event) override;

private:
	sf::Sprite m_background_sprite;
	gui::Container m_gui_container;

	gui::Button::Ptr m_change_ip_button;
	gui::Label::Ptr m_current_ip_label;
	std::string m_player_input_ip;

	gui::Button::Ptr m_change_name_button;
	gui::Label::Ptr m_current_name_label;
	std::string m_player_input_name;
};

