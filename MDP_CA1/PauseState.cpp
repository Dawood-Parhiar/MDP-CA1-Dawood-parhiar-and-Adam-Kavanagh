#include "PauseState.hpp"
#include "ResourceHolder.hpp"
#include <SFML/Graphics/RenderWindow.hpp>

#include "Button.hpp"
#include "Utility.hpp"

PauseState::PauseState(StateStack& stack, Context context, bool lets_update_through)
    :State(stack, context)
	, m_lets_updates_through(lets_update_through)
{
    sf::Font& font = context.fonts->Get(Font::kMain);
    sf::Vector2f view_size = context.window->getView().getSize();

    m_paused_text.setFont(font);
    m_paused_text.setString("Game Paused");
    m_paused_text.setCharacterSize(70);
    Utility::CentreOrigin(m_paused_text);
    m_paused_text.setPosition(0.5f * view_size.x, 0.4f * view_size.y);

    auto returnButton = std::make_shared<gui::Button>(context);
    returnButton->setPosition(0.5f * view_size.x - 100, 0.4f * view_size.y + 75);
    returnButton->SetText("Return");
    returnButton->SetCallback([this]()
        {
            RequestStackPop();
        });

    auto volumeUpButton = std::make_shared<gui::Button>(context);
    volumeUpButton->setPosition(0.5f * view_size.x - 100, 0.4f * view_size.y + 175);
    volumeUpButton->SetText("Volume Up");
    volumeUpButton->SetCallback([this]()
        {
            float volume = GetContext().music->GetVolume();
            GetContext().music->SetVolume(std::min(volume + 10.f, 100.f));
        });

    auto volumeDownButton = std::make_shared<gui::Button>(context);
    volumeDownButton->setPosition(0.5f * view_size.x - 100, 0.4f * view_size.y + 275);
    volumeDownButton->SetText("Volume Down");
    volumeDownButton->SetCallback([this]()
        {
            float volume = GetContext().music->GetVolume();
            GetContext().music->SetVolume(std::max(volume - 10.f, 0.f));
        });

    auto backToMenuButton = std::make_shared<gui::Button>(context);
    backToMenuButton->setPosition(0.5f * view_size.x - 100, 0.4f * view_size.y + 375);
    backToMenuButton->SetText("Back to menu");
    backToMenuButton->SetCallback([this]()
        {
            RequestStackClear();
            RequestStackPush(StateID::kMenu);
        });


    m_gui_container.Pack(returnButton);
    m_gui_container.Pack(volumeUpButton);
    m_gui_container.Pack(volumeDownButton);
    m_gui_container.Pack(backToMenuButton);


    //Pause the music
    GetContext().music->SetPaused(true);
}

void PauseState::Draw()
{
    sf::RenderWindow& window = *GetContext().window;
    window.setView(window.getDefaultView());

    sf::RectangleShape backgroundShape;
    backgroundShape.setFillColor(sf::Color(0, 0, 0, 150));
    backgroundShape.setSize(window.getView().getSize());

    window.draw(backgroundShape);
    window.draw(m_paused_text);
    window.draw(m_gui_container);
}

bool PauseState::Update(sf::Time dt)
{
    return m_lets_updates_through;
}

bool PauseState::HandleEvent(const sf::Event& event)
{
    
    m_gui_container.HandleEvent(event);
    return false;
}

PauseState::~PauseState()
{
    GetContext().music->SetPaused(false);
}