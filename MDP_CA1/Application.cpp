#include "Application.hpp"


const sf::Time Application::kTimePerFrame = sf::seconds(1.f/60.f);
Application::Application() : m_window(sf::VideoMode(1024, 768), "States", sf::Style::Close)
    , m_stack(State::Context(m_window, m_textures, m_fonts, m_player))
{
    m_window.setKeyRepeatEnabled(false);
    m_fonts.Load(FontID::kMain, "Media/Fonts/Sansation.ttf");
    m_textures.Load(TextureID::kTitleScreen, "Media/Textures/TitleScreen.png");
    m_textures.Load(TextureID::kButtonNormal, "Media/Textures/ButtonNormal.png");
    m_textures.Load(TextureID::kButtonSelected, "Media/Textures/ButtonSelected.png");
    m_textures.Load(TextureID::kButtonActivated, "Media/Textures/ButtonPressed.png");
    m_textures.Load(TextureID::kBullet, "Media/Textures/Bullet.png");
    m_textures.Load(TextureID::kMissile, "Media/Textures/Missile.png");

    RegisterStates();
    m_stack.PushState(StateID::kTitle);
}

void Application::Run()
{
    sf::Clock clock;
    sf::Time time_since_last_update = sf::Time::Zero;
    while (m_window.isOpen())
    {
        time_since_last_update += clock.restart();
        while(time_since_last_update > kTimePerFrame)
        {
            time_since_last_update -= kTimePerFrame;
            ProcessInput();
            Update(kTimePerFrame);

            if (m_stack.IsEmpty())
            {
                m_window.close();
            }
        }
        Render();
    }
}

void Application::ProcessInput()
{
        sf::Event event;
        while (m_window.pollEvent(event))
        {
            m_stack.HandleEvent(event);

            if (event.type == sf::Event::Closed)
            {
                m_window.close();
            }
        }
}

void Application::Update(sf::Time dt)
{
    m_stack.Update(dt);
}

void Application::Render()
{
    m_window.clear();
    m_stack.Draw();
    m_window.display();
}

void Application::RegisterStates()
{
    m_stack.RegisterState<TitleState>(StateID::kTitle);
    m_stack.RegisterState<MenuState>(StateID::kMenu);
}
