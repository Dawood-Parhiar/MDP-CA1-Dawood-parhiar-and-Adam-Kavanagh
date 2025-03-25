#include "PauseState.hpp"
#include "ResourceHolder.hpp"
#include <SFML/Graphics/RenderWindow.hpp>

#include "Button.hpp"
#include "Utility.hpp"

#include "Slider.hpp"

PauseState::PauseState(StateStack& stack, Context context, bool lets_update_through)
    :State(stack, context)
	, m_lets_updates_through(lets_update_through)
    , m_volume_slider(0.f, 100.f, context.music->GetVolume(), context.fonts->Get(Font::kMain))
    , m_sound_slider(0.f, 100.f, context.sounds->GetVolume(), context.fonts->Get(Font::kMain))
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

    auto backToMenuButton = std::make_shared<gui::Button>(context);
    backToMenuButton->setPosition(0.5f * view_size.x - 100, 0.4f * view_size.y + 375);
    backToMenuButton->SetText("Back to menu");
    backToMenuButton->SetCallback([this]()
        {
            RequestStackClear();
            RequestStackPush(StateID::kMenu);
        });

    m_volume_slider.setPosition(0.5f * view_size.x - 100, 0.4f * view_size.y + 200);
    m_volume_slider.SetCallback([this](float value)
        {
            GetContext().music->SetVolume(value);
        });

    m_sound_slider.setPosition(0.5f * view_size.x - 100, 0.4f * view_size.y + 300);
    m_sound_slider.SetCallback([this](float value)
        {
            GetContext().sounds->SetVolume(value);
        });

    m_gui_container.Pack(returnButton);
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


	window.draw(m_volume_slider);
    window.draw(m_sound_slider);
}

bool PauseState::Update(sf::Time dt)
{
    return m_lets_updates_through;
}

bool PauseState::HandleEvent(const sf::Event& event)
{
    
    m_gui_container.HandleEvent(event);
    m_volume_slider.HandleEvent(event, *GetContext().window);
    m_sound_slider.HandleEvent(event, *GetContext().window);
    return false;
}

PauseState::~PauseState()
{
    GetContext().music->SetPaused(false);
}