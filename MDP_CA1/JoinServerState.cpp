#include "JoinServerState.h"

#include <SFML/Graphics/RenderWindow.hpp>

#include "Button.hpp"
#include "Label.hpp"

//Written by Paul Bichler (D00242563)
JoinServerState::JoinServerState(StateStack& stack, Context& context)
	: State(stack, context),
	m_player_input_ip("127.0.0.1"),
	m_player_input_name("Default")
{
	const sf::Texture& texture = context.textures->Get(TextureID::kLobbyBg);
	m_background_sprite.setTexture(texture);

	//Ip Input
	m_change_ip_button = std::make_shared<gui::Button>(context);
	m_change_ip_button->SetText("IP Address");
	m_change_ip_button->SetToggle(true);
	m_change_ip_button->setPosition(80.f, 300.f);

	m_current_ip_label = std::make_shared<gui::Label>(m_player_input_ip, *context.fonts, 20);
	m_current_ip_label->setPosition(310.f, 315.f);

	m_gui_container.Pack(m_change_ip_button);
	m_gui_container.Pack(m_current_ip_label);

	//Name Input
	m_change_name_button = std::make_shared<gui::Button>(context);
	m_change_name_button->SetText("Name");
	m_change_name_button->SetToggle(true);
	m_change_name_button->setPosition(80.f, 400.f);

	m_current_name_label = std::make_shared<gui::Label>(m_player_input_name, *context.fonts, 20);
	m_current_name_label->setPosition(310.f, 415.f);

	m_gui_container.Pack(m_change_name_button);
	m_gui_container.Pack(m_current_name_label);

	//Connect Button
	const auto connect_button = std::make_shared<gui::Button>(context);
	connect_button->setPosition(80.f, 500.f);
	connect_button->SetText("Connect");
	connect_button->SetCallback([this]
		{
			GetContext().player_name = m_player_input_name;
			GetContext().server_ip = m_player_input_ip;

			RequestStackPop(); //Pop Menu State
			RequestStackPush(StateID::kJoinGame);
		});

	m_gui_container.Pack(connect_button);

	//Back Button
	const auto back_button = std::make_shared<gui::Button>(context);
	back_button->setPosition(80.f, 600.f);
	back_button->SetText("Back");
	back_button->SetCallback([this]
		{
			RequestStackPop();
			RequestStackPush(StateID::kMenu);
		});

	m_gui_container.Pack(back_button);
}

//Written by Paul Bichler (D00242563)
void JoinServerState::Draw()
{
	sf::RenderWindow& window = *GetContext().window;
	window.draw(m_background_sprite);
	window.draw(m_gui_container);
}

//Written by Paul Bichler (D00242563)
bool JoinServerState::Update(sf::Time dt)
{
	return true;
}

//Written by Paul Bichler (D00242563)
bool JoinServerState::HandleEvent(const sf::Event& event)
{
	if (m_change_ip_button->IsActive())
	{
		//Ip Input
		if (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::Return)
		{
			//Make Ip Address persistent by saving it once input in finished
			m_change_ip_button->Deactivate();
			//GetContext().m_player_data_manager->GetData().m_ip_address = m_player_input_ip;
			//GetContext().m_player_data_manager->Save();
			m_change_name_button->Deactivate();
		}
		else if (event.type == sf::Event::TextEntered)
		{
			if (event.text.unicode == '\b')
			{
				//Handle backspace
				if (!m_player_input_ip.empty())
					m_player_input_ip.erase(m_player_input_ip.size() - 1, 1);
				if (!m_player_input_name.empty())
					m_player_input_name.erase(m_player_input_name.size() - 1, 1);
			}
			else if (event.text.unicode != '\n' && event.text.unicode != '\r')
			{
				//Handle player text input (new lines and carriage returns are not allowed!)
				m_player_input_ip += event.text.unicode;
				m_player_input_ip = m_player_input_ip.substr(0, 25);

				m_player_input_name += event.text.unicode;
				m_player_input_name = m_player_input_name.substr(0, 25);
			}

			m_current_ip_label->SetText(m_player_input_ip);
			m_current_name_label->SetText(m_player_input_name);
		}
	}
	else if (m_change_name_button->IsActive())
	{
		//Ip Input
		if (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::Return)
		{
			m_change_name_button->Deactivate();
		}
		else if (event.type == sf::Event::TextEntered)
		{
			if (event.text.unicode == '\b')
			{
				if (!m_player_input_name.empty())
					m_player_input_name.erase(m_player_input_name.size() - 1, 1);
			}
			else if (event.text.unicode != '\n' && event.text.unicode != '\r')
			{
				m_player_input_name += event.text.unicode;
				m_player_input_name = m_player_input_name.substr(0, 25);
			}

			m_current_name_label->SetText(m_player_input_name);
		}
	}
	else
	{
		m_gui_container.HandleEvent(event);
	}

	return false;
}
