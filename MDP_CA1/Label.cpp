#include "Label.hpp"
#include "ResourceHolder.hpp"
#include "Utility.hpp"

gui::Label::Label(const std::string& text, const FontHolder& fonts, const TextureHolder& textures)
    :m_text(text, fonts.Get(Font::kMain), 16)
	
{
    m_text.setFillColor(sf::Color::Black);
    m_sprite.setTexture(textures.Get(TextureID::kLabel));
    sf::FloatRect bounds = m_sprite.getLocalBounds();
    Utility::CentreOrigin(m_text);
    m_text.setPosition(bounds.left + bounds.width / 2, bounds.top + bounds.height / 2);
}

bool gui::Label::IsSelectable() const
{
    return false;
}

void gui::Label::SetText(const std::string& text)
{
    m_text.setString(text);
    m_text.setFillColor(sf::Color::Black);
    
}

void gui::Label::HandleEvent(const sf::Event& event)
{
}

void gui::Label::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    states.transform *= getTransform();
    target.draw(m_sprite, states);
    target.draw(m_text, states);
}

