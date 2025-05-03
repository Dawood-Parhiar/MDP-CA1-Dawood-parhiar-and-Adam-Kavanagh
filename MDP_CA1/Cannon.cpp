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
    // 1. local bounds of the CANNON TEXTURE
    sf::FloatRect bounds = m_sprite.getLocalBounds();

    // 2. “mouth” in sprite-local coordinates (unrotated, unscaled):
    //    bounds.width is the right edge, .height/2 is vertically centered.
    sf::Vector2f mouthLocal{ 0.f, 15.f};

    // 3. build the full transform: ship→cannon (getWorldTransform())
    //    then cannon→sprite (m_sprite.getTransform())
    sf::Transform full = GetWorldTransform() * m_sprite.getTransform();

    // 4. push the mouth point all the way into world space:
    return full.transformPoint(mouthLocal);
}


void Cannon::UpdateCurrent(sf::Time dt, CommandQueue& commands)
{
    
    rotate(m_rotationInput * m_rotationSpeed * dt.asSeconds());
    m_rotationInput = 0;
    GetMouthPosition();
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
