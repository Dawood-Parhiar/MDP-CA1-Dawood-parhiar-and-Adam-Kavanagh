#include "LobbyState.hpp"



LobbyState::LobbyState(StateStack& stack, Context context)
    : State(stack, context)
    , m_gui_container()
{
    m_background_sprite.setTexture(context.textures->Get(TextureID::kLobbyBg));

    // Title label
    auto title_label = std::make_shared<gui::Label>("Lobby - Waiting for Players", *context.fonts, *context.textures);
    title_label->setPosition(300.f, 50.f);
    m_gui_container.Pack(title_label);

    // Start button (disabled until 2 players are assigned)
    m_start_button = std::make_shared<gui::Button>(context);
    m_start_button->setPosition(350.f, 500.f);
    m_start_button->SetText("Start Game");
    m_start_button->SetCallback([this]() { RequestStackPop(); RequestStackPush(StateID::kGame); });
   // m_start_button->SetEnabled(true); // Initially disabled
    

    auto back_button = std::make_shared<gui::Button>(context);
    back_button->setPosition(600, 500.f);
    back_button->SetText("Return");
    back_button->SetCallback([this] { RequestStackPop(); RequestStackPush(StateID::kMenu); });

    m_gui_container.Pack(m_start_button);
    m_gui_container.Pack(back_button);
}

void LobbyState::Draw()
{
    sf::RenderWindow& window = *GetContext().window;
    window.draw(m_background_sprite);
    window.draw(m_gui_container);
}

bool LobbyState::Update(sf::Time dt)
{
    UpdatePlayerList();
    return true;
}

void LobbyState::UpdatePlayerList()
{
   // m_gui_container.Clear();

    // Re-add title
    auto title_label = std::make_shared<gui::Label>("Lobby - Waiting for Players", *GetContext().fonts, *GetContext().textures);
    title_label->setPosition(300.f, 50.f);
    m_gui_container.Pack(title_label);

    int yOffset = 150;
    for (std::size_t i = 0; i < m_players.size(); ++i)
    {
        std::string player_text = "Player " + std::to_string(m_players[i]);
        if (m_players[i] != -1)
        {
            player_text += " - Ship " + std::to_string(m_players[i]);
        }

        auto player_label = std::make_shared<gui::Label>(player_text, *GetContext().fonts, *GetContext().textures);
        player_label->setPosition(300.f, yOffset);
        m_gui_container.Pack(player_label);

        yOffset += 50;
    }

    // Re-add start button
    m_gui_container.Pack(m_start_button);
}

void LobbyState::HandlePlayerJoin(int player_id)
{
    m_players.push_back(1);
    TryAssignShips();
}

void LobbyState::TryAssignShips()
{
    if (m_players.size() >= 2)
    {
       /* int ship_id = m_ships_assigned++;
        m_players[m_players.size() - 2].ship_id = ship_id;
        m_players[m_players.size() - 1].ship_id = ship_id;*/
        m_start_button->SetEnabled(true);
    }
}

bool LobbyState::HandleEvent(const sf::Event& event)
{
    m_gui_container.HandleEvent(event);
    return true;
}

void LobbyState::AddPlayer(const std::string& playerName)
{
}

void LobbyState::AssignShips()
{
}
