#pragma once

#include <SFML/Graphics.hpp>
#include <functional>

namespace Interface
{
    class Slider : public sf::Drawable, public sf::Transformable
    {
    public:
        Slider(float min, float max, float value, const sf::Font& font);

        void SetCallback(std::function<void(float)> callback);
        void HandleEvent(const sf::Event& event, const sf::RenderWindow& window);
        float GetValue() const;

    private:
        virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

        float m_min;
        float m_max;
        float m_value;
        std::function<void(float)> m_callback;

        sf::RectangleShape m_background;
        sf::RectangleShape m_foreground;
        sf::CircleShape m_knob;
        sf::Text m_value_text;

        bool m_is_dragging;
    };
}