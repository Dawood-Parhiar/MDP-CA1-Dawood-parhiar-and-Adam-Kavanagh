#include "Cannon.hpp"
#include "Utility.hpp"

//Dawood Parhiar D00248313
Cannon::Cannon(const TextureHolder& textures)
    : m_sprite(textures.Get(TextureID::kCannon)),
    m_rotationSpeed(100.f),
    m_rotationInput(0.f),
	Entity(1)
{
   Utility::CentreOrigin(m_sprite);
    
}

void Cannon::SetRotationInput(float rotation)
{
    m_rotationInput = rotation;
}

sf::Vector2f Cannon::GetMouthPosition() const
{
    sf::FloatRect bounds = m_sprite.getLocalBounds();
    sf::Vector2f mouthLocal{ 0.f, 10.f};
    sf::Transform full = GetWorldTransform() * m_sprite.getTransform();
    return full.transformPoint(mouthLocal);
}


void Cannon::UpdateCurrent(sf::Time dt, CommandQueue& commands)
{
    
    rotate(m_rotationInput * m_rotationSpeed * dt.asSeconds());
    m_rotationInput = 0;
    GetMouthPosition();
}

void Cannon::SetPlayerName(std::string& name, const FontHolder& fonts)
{
    auto nameNode = std::make_unique<TextNode>(fonts, name);
    m_name_display = nameNode.get();
    sf::FloatRect bounds = m_sprite.getLocalBounds();
    float xOffset = getPosition().x - 40.f;
    float yOffset = getPosition().y - 13.f;
    
    nameNode->setPosition(xOffset,yOffset);

    // Add it as a child so it inherits all transforms
    AttachChild(std::move(nameNode));
}


void Cannon::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(m_sprite, states);
    // inside Cannon::DrawCurrent after drawing m_sprite
    sf::CircleShape dot(4.f);
    dot.setOrigin(4.f, 4.f);
    dot.setPosition(GetMouthPosition());
    dot.setFillColor(sf::Color::Red);
    target.draw(dot);

}

float Cannon::GetRotation() const
{
    return m_sprite.getRotation();
}

void Cannon::SetRotation(float angle)
{
    m_sprite.setRotation(angle);
}
