#include "Slider.hpp"
#include <sstream>
#include <iomanip>


namespace Interface
{
    Slider::Slider(float min, float max, float value, const sf::Font& font)
        : m_min(min)
        , m_max(max)
        , m_value(value)
        , m_callback(nullptr)

        , m_is_dragging(false)
    {
        m_background.setSize(sf::Vector2f(200.f, 10.f));
        m_background.setFillColor(sf::Color::White);

        m_foreground.setSize(sf::Vector2f((m_value - m_min) / (m_max - m_min) * 200.f, 10.f));
        m_foreground.setFillColor(sf::Color::Green);

        m_knob.setRadius(10.f);
        m_knob.setFillColor(sf::Color::Red);
        m_knob.setOrigin(10.f, 10.f);
        m_knob.setPosition(m_foreground.getSize().x, m_background.getSize().y / 2.f);

        m_value_text.setFont(font);
        m_value_text.setCharacterSize(15);
        m_value_text.setFillColor(sf::Color::White);
        m_value_text.setPosition(m_background.getSize().x + 15.f, -5.f);

        std::stringstream ss;
        ss << std::fixed << std::setprecision(0) << m_value;
        m_value_text.setString(ss.str());
    }

    void Slider::SetCallback(std::function<void(float)> callback)
    {
        m_callback = callback;
    }

    void Slider::HandleEvent(const sf::Event& event, const sf::RenderWindow& window)
    {
        if (event.type == sf::Event::MouseButtonPressed)
        {
            if (event.mouseButton.button == sf::Mouse::Left)
            {
                sf::Vector2f mouse_pos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                if (m_knob.getGlobalBounds().contains(mouse_pos))
                {
                    m_knob.setFillColor(sf::Color::Yellow);
                }
            }
        }
        else if (event.type == sf::Event::MouseButtonReleased)
        {
            if (event.mouseButton.button == sf::Mouse::Left)
            {
                m_knob.setFillColor(sf::Color::Red);
            }
        }
        else if (event.type == sf::Event::MouseMoved)
        {
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && m_knob.getFillColor() == sf::Color::Yellow)
            {
                sf::Vector2f mouse_pos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                float new_x = std::max(0.f, std::min(mouse_pos.x - getPosition().x, m_background.getSize().x));
                m_foreground.setSize(sf::Vector2f(new_x, m_foreground.getSize().y));
                m_knob.setPosition(new_x, m_background.getSize().y / 2.f);

                m_value = m_min + (new_x / m_background.getSize().x) * (m_max - m_min);

                std::stringstream ss;
                ss << std::fixed << std::setprecision(0) << m_value;
                m_value_text.setString(ss.str());

                if (m_callback)
                {
                    m_callback(m_value);
                }
            }
        }
    }

    float Slider::GetValue() const
    {
        return m_value;
    }

    void Slider::draw(sf::RenderTarget& target, sf::RenderStates states) const
    {
        states.transform *= getTransform();
        target.draw(m_background, states);
        target.draw(m_foreground, states);
        target.draw(m_knob, states);
        target.draw(m_value_text, states);
    }
}