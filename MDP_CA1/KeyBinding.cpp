#include "KeyBinding.h"
#include <string>
#include <algorithm>
#include <iostream>
#include <SFML/Window/Keyboard.hpp>
#include "Action.hpp"

KeyBinding::KeyBinding(int control_preconfiguration)
	: m_key_map()
{
	 //Set initial key bindings for player 1
	if (control_preconfiguration == 1)
	{
		m_key_map[sf::Keyboard::W] = Action::kMoveUp;
		m_key_map[sf::Keyboard::S] = Action::kMoveDown;
		m_key_map[sf::Keyboard::A] = Action::kRotateLeft;
		m_key_map[sf::Keyboard::D] = Action::kRotateRight;
	}
	else if (control_preconfiguration == 2)
	{
		// Player 2
		
		
		m_key_map[sf::Keyboard::Return] = Action::kMissileFire;
		m_key_map[sf::Keyboard::Left] = Action::kRotateCannonLeft;
		m_key_map[sf::Keyboard::Right] = Action::kRotateCannonRight;
	}
}

void KeyBinding::AssignKey(Action action, sf::Keyboard::Key key)
{
	// Remove all keys that already map to action
	for (auto itr = m_key_map.begin(); itr != m_key_map.end(); )
	{
		if (itr->second == action)
			m_key_map.erase(itr++);
		else
			++itr;
	}

	// Insert new binding
	m_key_map[key] = action;
}

sf::Keyboard::Key KeyBinding::GetAssignedKey(Action action) const
{
	for (auto pair : m_key_map)
	{
		if (pair.second == action)
			return pair.first;
	}

	return sf::Keyboard::Unknown;
}

bool KeyBinding::CheckAction(sf::Keyboard::Key key, Action& out) const
{
	auto found = m_key_map.find(key);
	if (found == m_key_map.end())
	{
		return false;
	}
	else
	{
		out = found->second;
		return true;
	}
}

std::vector<Action> KeyBinding::GetRealtimeActions() const
{
	// Return all realtime actions that are currently active.
	std::vector<Action> actions;

	
	for (auto pair : m_key_map)
	{
		// If key is pressed and an action is a realtime action, store it
		if (sf::Keyboard::isKeyPressed(pair.first) && IsRealtimeAction(pair.second))
			actions.push_back(pair.second);
	}
	return actions;
}


bool IsRealtimeAction(Action action)
{
	switch (action)
	{
	case Action::kMoveUp:
	case Action::kMoveDown:
	case Action::kRotateLeft:
	case Action::kRotateRight:
	case Action::kRotateCannonLeft:
	case Action::kRotateCannonRight:
		return true;

	default:
		return false;
	}
}