#include "SettingsState.hpp"

#include "KeyBinding.h"
#include "ResourceHolder.hpp"
#include "Utility.hpp"

SettingsState::SettingsState(StateStack& stack, Context context)
	: State(stack, context)
	, m_gui_container()
{
	m_background_sprite.setTexture(context.textures->Get(TextureID::kMenuBoard));

	for (std::size_t x = 0; x < 2 ; ++x)
	{
		if (x == 0)
		{
			AddButtonLabel(static_cast<int>(Action::kMoveUp), x, 0, "Move Up", context);
			AddButtonLabel(static_cast<int>(Action::kMoveDown), x, 1, "Move Down", context);
			AddButtonLabel(static_cast<int>(Action::kRotateLeft), x, 2, "Rotate Left", context);
			AddButtonLabel(static_cast<int>(Action::kRotateRight), x, 3, "Rotate Right", context);
		}
		else if (x == 1) {

			AddButtonLabel(static_cast<int>(Action::kMissileFire), x, 4, "Missile", context);
			AddButtonLabel(static_cast<int>(Action::kRotateCannonLeft), x, 5, "Rotate Cannon Left", context);
			AddButtonLabel(static_cast<int>(Action::kRotateCannonRight), x, 6, "Rotate Cannon Right", context);
		}
	}
	UpdateLabels();

	auto back_button = std::make_shared<gui::Button>(context);
	back_button->setPosition(350, 100.f);
	back_button->SetText("Return");
	back_button->SetCallback([this] { RequestStackPop(); });
	m_gui_container.Pack(back_button);
}

void SettingsState::Draw()
{
	sf::RenderWindow& window = *GetContext().window;
	window.draw(m_background_sprite);
	window.draw(m_gui_container);
}

bool SettingsState::Update(sf::Time dt)
{
	return true;
}

bool SettingsState::HandleEvent(const sf::Event& event)
{
	bool is_key_binding = false;

	//Iterate through all of the key binding buttons to see if they are being presssed, waiting for the user to enter a key
	for (std::size_t action = 0; action < (static_cast<int>(Action::kActionCount)*2); ++action)
	{
		if (m_binding_buttons[action] && m_binding_buttons[action]->IsActive())
		{
			is_key_binding = true;
			if (event.type == sf::Event::KeyReleased)
			{
				// Player 1
				if (action < static_cast<int>(Action::kActionCount))
				GetContext().keys1->AssignKey(static_cast<Action>(action), event.key.code);
				// Player 2
				else
				GetContext().keys2->AssignKey(static_cast<Action>(action - static_cast<int>(Action::kActionCount)), event.key.code);
				m_binding_buttons[action]->Deactivate();
			}
			break;
		}
	}

	//If pressed button changed key bindings, then update the labels
	if (is_key_binding)
	{
		UpdateLabels();
	}
	else
	{
		m_gui_container.HandleEvent(event);
	}
	return false;
}

void SettingsState::UpdateLabels()
{
	// Update labels for Player 1's key bindings
	for (std::size_t i = 0; i < static_cast<int>(Action::kActionCount); ++i)
	{
		auto action = static_cast<Action>(i);

		// Update Player 1 key label
		if (m_binding_labels[i])
		{
			sf::Keyboard::Key key1 = GetContext().keys1->GetAssignedKey(action);
			m_binding_labels[i]->SetText(Utility::toString(key1));
		}

		// Update Player 2 key label (offset by Action count)
		if (m_binding_labels[i + static_cast<int>(Action::kActionCount)])
		{
			sf::Keyboard::Key key2 = GetContext().keys2->GetAssignedKey(action);
			m_binding_labels[i + static_cast<int>(Action::kActionCount)]->SetText(Utility::toString(key2));
		}
	}


}

void SettingsState::AddButtonLabel(std::size_t index, std::size_t x, std::size_t y, const std::string& text, Context context)
{
	index += static_cast<int>(Action::kActionCount) * x;

	if (index >= m_binding_buttons.size())
		return;

	m_binding_buttons[index] = std::make_shared<gui::Button>(context);
	m_binding_buttons[index]->setPosition(400.f * x + 100.f, 80.f * y + 200.f);
	m_binding_buttons[index]->SetText(text);
	m_binding_buttons[index]->SetToggle(true);

	m_binding_labels[index] = std::make_shared<gui::Label>("", *context.fonts,*context.textures);
	m_binding_labels[index]->setPosition(400.f * x + 350.f, 80.f * y + 200.f);

	m_gui_container.Pack(m_binding_buttons[index]);
	m_gui_container.Pack(m_binding_labels[index]);
}
