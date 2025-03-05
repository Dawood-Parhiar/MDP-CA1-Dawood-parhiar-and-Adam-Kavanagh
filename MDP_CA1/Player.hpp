#pragma once
#include <SFML/Window/Event.hpp>
#include "Action.hpp"
#include "CommandQueue.hpp"
#include "MissionStatus.hpp"
#include <map>
#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Network/TcpSocket.hpp"
#include "SFML/Network/Packet.hpp"
#include "KeyBinding.h"

class Command;


class Player
{
public:
	Player(sf::TcpSocket* socket,sf::Int32 id, const KeyBinding* binding);
	void HandleEvent(const sf::Event& event, CommandQueue& command_queue);
	void HandleRealtimeInput(CommandQueue& command_queue);
	void HandleRealtimeNetworkInput(CommandQueue& commands);

	void HandleNetworkEvent(Action action, CommandQueue& commands);
	void HandleNetworkRealtimeChange(Action action, bool action_enabled);

	void SetMissionStatus(MissionStatus status);
	MissionStatus GetMissionStatus() const;

	void DisableAllRealtimeActions();
	bool IsLocal() const;

private:
	void InitialiseActions();
	//static bool IsRealTimeAction(Action action);

private:
	const KeyBinding* m_key_binding;
	std::map<Action, Command> m_action_binding;
	std::map<Action, bool> m_action_proxies;
	MissionStatus m_current_mission_status;
	int m_identifier;
	sf::TcpSocket* m_socket;

};

