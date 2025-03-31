#include "MenuState.hpp"
#include "ResourceHolder.hpp"
#include "Utility.hpp"
#include "Button.hpp"

MenuState::MenuState(StateStack& stack, Context context)
    :State(stack, context)
{
    sf::Texture& texture = context.textures->Get(TextureID::kTitleScreen);

    m_background_sprite.setTexture(texture);

    auto play_button = std::make_shared<gui::Button>(context);
    play_button->setPosition(850, 250);
    play_button->SetText("Play");
    play_button->SetCallback([this]()
    {
        RequestStackPop();
        RequestStackPush(StateID::kGame);
    });

    auto lobby_button = std::make_shared<gui::Button>(context);
    lobby_button->setPosition(550, 450);
    lobby_button->SetText("Lobby Host");
    lobby_button->SetCallback([this]()
        {
            RequestStackPop();
            RequestStackPush(StateID::kLobbyHost);
        });

    auto lobby_join_button = std::make_shared<gui::Button>(context);
    lobby_join_button->setPosition(550, 600);
    lobby_join_button->SetText("Lobby Join");
    lobby_join_button->SetCallback([this]()
        {
            RequestStackPop();
            RequestStackPush(StateID::kJoinSettings);
        });
    auto host_play_button = std::make_shared<gui::Button>(context);
    host_play_button->setPosition(850, 450);
    host_play_button->SetText("Host");
    host_play_button->SetCallback([this]()
        {
            RequestStackPop();
            RequestStackPush(StateID::kHostGame);
        });

    auto join_play_button = std::make_shared<gui::Button>(context);
    join_play_button->setPosition(850, 600);
    join_play_button->SetText("Join");
    join_play_button->SetCallback([this]()
        {
            RequestStackPop();
            RequestStackPush(StateID::kJoinGame);
        });

    auto settings_button = std::make_shared<gui::Button>(context);
    settings_button->setPosition(1150, 450);
    settings_button->SetText("Settings");
    settings_button->SetCallback([this]()
    {
        RequestStackPush(StateID::kSettings);
    });

    auto exit_button = std::make_shared<gui::Button>(context);
    exit_button->setPosition(1150, 600);
    exit_button->SetText("Exit");
    exit_button->SetCallback([this]()
    {
            RequestStackPop();
    });

    m_gui_container.Pack(play_button);
    m_gui_container.Pack(lobby_button);
	m_gui_container.Pack(lobby_join_button);
    m_gui_container.Pack(host_play_button);
    m_gui_container.Pack(join_play_button);
    m_gui_container.Pack(settings_button);
    m_gui_container.Pack(exit_button);

    //Play the music
    context.music->Play(MusicThemes::kMenuTheme);
}

void MenuState::Draw()
{
    sf::RenderWindow& window = *GetContext().window;
    window.setView(window.getDefaultView());
    window.draw(m_background_sprite);
    window.draw(m_gui_container);
}

bool MenuState::Update(sf::Time dt)
{
    return true;
}

bool MenuState::HandleEvent(const sf::Event& event)
{
    m_gui_container.HandleEvent(event);
    return true;
}

