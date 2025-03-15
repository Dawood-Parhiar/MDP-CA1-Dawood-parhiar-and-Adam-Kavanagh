#pragma once
#include "Container.hpp"
#include "State.hpp"
#include "Label.hpp"
#include "ResourceHolder.hpp"
#include "Utility.hpp"
#include "Button.hpp"
class LobbyState : public State
{
public:
    explicit LobbyState(StateStack& stack, Context context);

    void Draw() override;
    bool Update(sf::Time dt) override;
    void UpdatePlayerList();
    void HandlePlayerJoin(int player_id);
    void TryAssignShips();
    bool HandleEvent(const sf::Event& event) override;

    void AddPlayer(const std::string& playerName);
    void AssignShips();

private:
    sf::Sprite m_background_sprite;
    gui::Container m_gui_container;
    std::vector<int> m_players;
    std::vector<std::shared_ptr<gui::Label>> m_player_labels;
    std::vector<std::pair<std::string, std::string>> m_assigned_ships;
    std::shared_ptr<gui::Button> m_start_button;
};

